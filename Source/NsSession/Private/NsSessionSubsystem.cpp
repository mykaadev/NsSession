// Copyright (C) 2025 nulled.softworks. All rights reserved.

#include "NsSessionSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/PackageName.h"
#include "NsSessionModule.h"
#include "NsSteamLobbySearch.h"
#include "Online/OnlineSessionNames.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "UObject/UObjectGlobals.h"

void UNsSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UNsSessionSubsystem::HandlePostLoadMap);
    TravelFailureHandle = GEngine->OnTravelFailure().AddUObject(this, &UNsSessionSubsystem::HandleTravelFailure);
    NetworkFailureHandle = GEngine->OnNetworkFailure().AddUObject(this, &UNsSessionSubsystem::HandleNetworkFailure);
}

void UNsSessionSubsystem::Deinitialize()
{
    StopSearches();
    FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
    if (GEngine != nullptr)
    {
        GEngine->OnTravelFailure().Remove(TravelFailureHandle);
        GEngine->OnNetworkFailure().Remove(NetworkFailureHandle);
    }
    Super::Deinitialize();
}

UNsSessionSubsystem* UNsSessionSubsystem::Get(const UObject* WorldContext)
{
    const UGameInstance* const GameInstance = UGameplayStatics::GetGameInstance(WorldContext);
    return GameInstance != nullptr ? GameInstance->GetSubsystem<UNsSessionSubsystem>() : nullptr;
}

bool UNsSessionSubsystem::HostRoom(const FSoftObjectPath& Map)
{
    if (IsSearching() || bPendingHost)
    {
        return false;
    }
    const FString MapPackage = Map.GetLongPackageName();
    if (MapPackage.IsEmpty())
    {
        ReportStatus(ENsRoomStatus::NoMap, true, FString(), RoomNetwork);
        return false;
    }
    if (!FPackageName::DoesPackageExist(MapPackage))
    {
        ReportStatus(ENsRoomStatus::MapMissing, true, FString(), RoomNetwork, MapPackage);
        return false;
    }

    // The session needs the listen port, so the map opens first and the session follows after the load
    LeaveRoom();
    RoomCode = MakeRoomCode();
    RoomNetwork = CanUseSteam() ? ENsRoomNetwork::Steam : ENsRoomNetwork::Lan;
    bPendingHost = true;
    ReportStatus(ENsRoomStatus::Opening, false, RoomCode, RoomNetwork);

    // A LAN room listens on an IP socket, also when the Steam net driver is active
    const TCHAR* const Options = RoomNetwork == ENsRoomNetwork::Lan ? TEXT("listen?bIsLanMatch") : TEXT("listen");
    UGameplayStatics::OpenLevel(this, FName(*MapPackage), true, Options);
    return true;
}

bool UNsSessionSubsystem::JoinRoom(const FString& Code)
{
    const FString Normalized = NormalizeRoomCode(Code);
    if (Normalized.IsEmpty())
    {
        ReportStatus(ENsRoomStatus::BadCode, true, Code, ENsRoomNetwork::Lan);
        return false;
    }
    if (IsSearching() || bPendingHost)
    {
        return false;
    }

    LeaveRoom();
    PendingJoinCode = Normalized;
    bool bStarted = StartSearch(ENsRoomNetwork::Lan);
    if (CanUseSteam())
    {
        bStarted |= StartSearch(ENsRoomNetwork::Steam);
    }
    if (!bStarted)
    {
        ReportStatus(ENsRoomStatus::SearchFailed, true, PendingJoinCode, ENsRoomNetwork::Lan);
        PendingJoinCode.Reset();
        return false;
    }

    // The LAN search fills its results one reply at a time, so each frame checks for the code
    SearchTicker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UNsSessionSubsystem::TickSearch), 0.1f);
    ReportStatus(ENsRoomStatus::Searching, false, PendingJoinCode, ENsRoomNetwork::Lan);
    return true;
}

void UNsSessionSubsystem::LeaveRoom()
{
    StopSearches();
    bPendingHost = false;
    RoomCode.Reset();
    for (const ENsRoomNetwork Network : { ENsRoomNetwork::Lan, ENsRoomNetwork::Steam })
    {
        const TSharedPtr<IOnlineSession, ESPMode::ThreadSafe> Sessions = GetSessionInterface(Network);
        if (Sessions.IsValid() && Sessions->GetNamedSession(NAME_GameSession) != nullptr)
        {
            Sessions->DestroySession(NAME_GameSession);
        }
    }
}

bool UNsSessionSubsystem::JoinDirectAddress(const FString& Address)
{
    if (Address.IsEmpty())
    {
        return false;
    }
    LeaveRoom();
    UGameplayStatics::OpenLevel(this, FName(*Address), true);
    return true;
}

bool UNsSessionSubsystem::IsSearching() const
{
    return LanSearch.IsValid() || SteamSearch.IsValid();
}

bool UNsSessionSubsystem::ConsumeLastFailure(FNsRoomStatus& OutStatus)
{
    if (!LastFailure.IsSet())
    {
        return false;
    }
    OutStatus = LastFailure.GetValue();
    LastFailure.Reset();
    return true;
}

bool UNsSessionSubsystem::GetLastStatus(FNsRoomStatus& OutStatus) const
{
    if (!LastStatus.IsSet())
    {
        return false;
    }
    OutStatus = LastStatus.GetValue();
    return true;
}

FText UNsSessionSubsystem::GetLastStatusText() const
{
    return LastStatus.IsSet() ? LastStatus->GetDefaultText() : FText::GetEmpty();
}

FString UNsSessionSubsystem::NormalizeRoomCode(const FString& Code)
{
    const FString Upper = Code.TrimStartAndEnd().ToUpper();
    if (Upper.Len() != RoomCodeLength)
    {
        return FString();
    }
    for (const TCHAR Letter : Upper)
    {
        if (FCString::Strchr(RoomCodeAlphabet, Letter) == nullptr)
        {
            return FString();
        }
    }
    return Upper;
}

FString UNsSessionSubsystem::MakeRoomCode()
{
    const int32 LetterCount = FCString::Strlen(RoomCodeAlphabet);
    FString Code;
    for (int32 Index = 0; Index < RoomCodeLength; ++Index)
    {
        Code.AppendChar(RoomCodeAlphabet[FMath::RandRange(0, LetterCount - 1)]);
    }
    return Code;
}

TSharedPtr<IOnlineSession, ESPMode::ThreadSafe> UNsSessionSubsystem::GetSessionInterface(const ENsRoomNetwork Network) const
{
    // Each Play In Editor window gets its own subsystem instance from its world
    const FName SubsystemName = Network == ENsRoomNetwork::Steam ? STEAM_SUBSYSTEM : NULL_SUBSYSTEM;
    IOnlineSubsystem* const Subsystem = Online::GetSubsystem(GetWorld(), SubsystemName);
    return Subsystem != nullptr ? Subsystem->GetSessionInterface() : nullptr;
}

FString UNsSessionSubsystem::GetGameTag() const
{
    return GameTag.IsEmpty() ? FString(FApp::GetProjectName()) : GameTag;
}

bool UNsSessionSubsystem::CanUseSteam() const
{
    // Play In Editor windows share one Steam account, so they always use LAN
    const UWorld* const World = GetWorld();
    if ((World != nullptr && World->IsPlayInEditor()) || FParse::Param(FCommandLine::Get(), TEXT("lanroom")))
    {
        return false;
    }
    IOnlineSubsystem* const Steam = Online::GetSubsystem(GetWorld(), STEAM_SUBSYSTEM);
    const IOnlineIdentityPtr Identity = Steam != nullptr ? Steam->GetIdentityInterface() : nullptr;
    return Identity.IsValid() && Identity->GetLoginStatus(0) == ELoginStatus::LoggedIn;
}

void UNsSessionSubsystem::HandlePostLoadMap(UWorld* World)
{
    if (!IsValid(World) || World->GetGameInstance() != GetGameInstance())
    {
        return;
    }
    const ENetMode NetMode = World->GetNetMode();
    if (NetMode == NM_ListenServer || NetMode == NM_Client)
    {
        OnRoomEntered.Broadcast();
    }
    if (bPendingHost && NetMode == NM_ListenServer)
    {
        bPendingHost = false;
        CreateRoomSession();
    }
}

void UNsSessionSubsystem::HandleTravelFailure(UWorld* World, const ETravelFailure::Type FailureType, const FString& Error)
{
    if (World != nullptr && World->GetGameInstance() != GetGameInstance())
    {
        return;
    }
    const FString Code = RoomCode;
    LeaveRoom();
    ReportStatus(ENsRoomStatus::TravelFailed, true, Code, RoomNetwork, Error);
}

void UNsSessionSubsystem::HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, const ENetworkFailure::Type FailureType, const FString& Error)
{
    if (World != nullptr && World->GetGameInstance() != GetGameInstance())
    {
        return;
    }
    const FString Code = RoomCode;
    LeaveRoom();
    const bool bHostLeft = FailureType == ENetworkFailure::ConnectionLost || FailureType == ENetworkFailure::ConnectionTimeout;
    ReportStatus(bHostLeft ? ENsRoomStatus::HostLeft : ENsRoomStatus::ConnectionFailed, true, Code, RoomNetwork, Error);
}

void UNsSessionSubsystem::CreateRoomSession()
{
    const TSharedPtr<IOnlineSession, ESPMode::ThreadSafe> Sessions = GetSessionInterface(RoomNetwork);
    if (!Sessions.IsValid())
    {
        UE_LOG(LogNsSession, Warning, TEXT("Room %s has no session interface. Players can join by IP address only."), *RoomCode);
        RoomCode.Reset();
        return;
    }

    const bool bSteam = RoomNetwork == ENsRoomNetwork::Steam;
    FOnlineSessionSettings Settings;
    Settings.NumPublicConnections = MaxPlayers;
    Settings.bIsLANMatch = !bSteam;
    Settings.bShouldAdvertise = true;
    Settings.bAllowJoinInProgress = true;
    Settings.bUsesPresence = bSteam;
    Settings.bAllowJoinViaPresence = bSteam;
    Settings.bUseLobbiesIfAvailable = bSteam;
    Settings.Set(RoomCodeKey, RoomCode, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
    Settings.Set(RoomGameKey, GetGameTag(), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
    if (bSteam)
    {
        // The worldwide search reads this key and travels straight to the host
        IOnlineSubsystem* const Steam = Online::GetSubsystem(GetWorld(), STEAM_SUBSYSTEM);
        const IOnlineIdentityPtr Identity = Steam != nullptr ? Steam->GetIdentityInterface() : nullptr;
        const FUniqueNetIdPtr HostId = Identity.IsValid() ? Identity->GetUniquePlayerId(0) : nullptr;
        if (HostId.IsValid())
        {
            Settings.Set(RoomHostKey, HostId->ToString(), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
        }
    }

    CreateSessionHandle = Sessions->AddOnCreateSessionCompleteDelegate_Handle(
        FOnCreateSessionCompleteDelegate::CreateUObject(this, &UNsSessionSubsystem::HandleCreateSessionComplete));
    if (!Sessions->CreateSession(0, NAME_GameSession, Settings))
    {
        Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionHandle);
        UE_LOG(LogNsSession, Error, TEXT("Room %s could not create its session. Players can join by IP address only."), *RoomCode);
        RoomCode.Reset();
    }
}

void UNsSessionSubsystem::HandleCreateSessionComplete(const FName SessionName, const bool bWasSuccessful)
{
    if (const TSharedPtr<IOnlineSession, ESPMode::ThreadSafe> Sessions = GetSessionInterface(RoomNetwork))
    {
        Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionHandle);
    }
    if (!bWasSuccessful)
    {
        UE_LOG(LogNsSession, Error, TEXT("Room %s could not create its session. Players can join by IP address only."), *RoomCode);
        RoomCode.Reset();
        return;
    }
    UE_LOG(LogNsSession, Display, TEXT("Room %s is open on %s."), *RoomCode, *FNsRoomStatus::GetNetworkName(RoomNetwork).ToString());
}

bool UNsSessionSubsystem::StartSearch(const ENsRoomNetwork Network)
{
    const TSharedPtr<IOnlineSession, ESPMode::ThreadSafe> Sessions = GetSessionInterface(Network);
    if (!Sessions.IsValid())
    {
        return false;
    }

    const bool bSteam = Network == ENsRoomNetwork::Steam;
    const TSharedRef<FOnlineSessionSearch> Search = MakeShared<FOnlineSessionSearch>();
    Search->bIsLanQuery = !bSteam;
    if (bSteam)
    {
        // Steam filters on its servers and sends back only the room with this code
        Search->MaxSearchResults = 1;
        Search->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);
        Search->QuerySettings.Set(RoomCodeKey, PendingJoinCode, EOnlineComparisonOp::Equals);
        Search->QuerySettings.Set(RoomGameKey, GetGameTag(), EOnlineComparisonOp::Equals);
        SteamSearch = Search;
    }
    else
    {
        // The LAN search gets every room on the network. TickSearch checks the code
        Search->MaxSearchResults = 64;
        LanSearch = Search;
    }
    FDelegateHandle& Handle = bSteam ? SteamSearchHandle : LanSearchHandle;
    Handle = Sessions->AddOnFindSessionsCompleteDelegate_Handle(
        FOnFindSessionsCompleteDelegate::CreateUObject(this, &UNsSessionSubsystem::HandleSearchComplete, Network));

    if (!Sessions->FindSessions(0, Search))
    {
        Sessions->ClearOnFindSessionsCompleteDelegate_Handle(Handle);
        (bSteam ? SteamSearch : LanSearch).Reset();
        UE_LOG(LogNsSession, Warning, TEXT("The %s room search could not start."), *FNsRoomStatus::GetNetworkName(Network).ToString());
        return false;
    }
    return true;
}

void UNsSessionSubsystem::HandleSearchComplete(const bool bWasSuccessful, const ENsRoomNetwork Network)
{
    const bool bSteam = Network == ENsRoomNetwork::Steam;
    if (const TSharedPtr<IOnlineSession, ESPMode::ThreadSafe> Sessions = GetSessionInterface(Network))
    {
        Sessions->ClearOnFindSessionsCompleteDelegate_Handle(bSteam ? SteamSearchHandle : LanSearchHandle);
    }
    TSharedPtr<FOnlineSessionSearch>& Search = bSteam ? SteamSearch : LanSearch;
    if (!Search.IsValid())
    {
        return;
    }

    // A match joins now. Otherwise this search ends and the other network can still find the room
    if (!TryJoinMatch())
    {
        Search.Reset();
        FinishSearchWithoutMatch();
    }
}

bool UNsSessionSubsystem::TickSearch(float DeltaTime)
{
    return !TryJoinMatch() && IsSearching();
}

bool UNsSessionSubsystem::TryJoinMatch()
{
    for (const ENsRoomNetwork Network : { ENsRoomNetwork::Lan, ENsRoomNetwork::Steam })
    {
        const TSharedPtr<FOnlineSessionSearch>& Search = Network == ENsRoomNetwork::Lan ? LanSearch : SteamSearch;
        if (!Search.IsValid())
        {
            continue;
        }
        for (const FOnlineSessionSearchResult& Result : Search->SearchResults)
        {
            if (Result.IsValid() && HasPendingCode(Result))
            {
                // Copy the result first. StopSearches frees the search that owns it
                const FOnlineSessionSearchResult Found = Result;
                JoinFoundRoom(Network, Found);
                return true;
            }
        }
    }
    return false;
}

void UNsSessionSubsystem::FinishSearchWithoutMatch()
{
    if (IsSearching())
    {
        return;
    }
    StopSearches();
    if (StartWorldwideSearch())
    {
        return;
    }
    ReportStatus(ENsRoomStatus::NotFound, true, PendingJoinCode, ENsRoomNetwork::Lan);
    PendingJoinCode.Reset();
}

bool UNsSessionSubsystem::StartWorldwideSearch()
{
    if (!CanUseSteam() || !FNsSteamLobbySearch::IsAvailable())
    {
        return false;
    }
    WorldwideSearch = MakeShared<FNsSteamLobbySearch>();
    const bool bStarted = WorldwideSearch->Start(RoomCodeKey.ToString() + SteamLobbyStringSuffix, PendingJoinCode,
        RoomGameKey.ToString() + SteamLobbyStringSuffix, GetGameTag(), RoomHostKey.ToString() + SteamLobbyStringSuffix,
        FNsSteamLobbySearch::FOnSearchDone::CreateWeakLambda(this, [this](const FString& HostSteamId) { HandleWorldwideResult(HostSteamId); }));
    if (!bStarted)
    {
        WorldwideSearch.Reset();
        return false;
    }
    ReportStatus(ENsRoomStatus::SearchingWorldwide, false, PendingJoinCode, ENsRoomNetwork::Steam);
    return true;
}

void UNsSessionSubsystem::HandleWorldwideResult(const FString& HostSteamId)
{
    WorldwideSearch.Reset();
    APlayerController* const PlayerController = GetGameInstance()->GetFirstLocalPlayerController();
    if (HostSteamId.IsEmpty() || !IsValid(PlayerController))
    {
        ReportStatus(ENsRoomStatus::NotFound, true, PendingJoinCode, ENsRoomNetwork::Steam);
        PendingJoinCode.Reset();
        return;
    }

    // The host listens on Steam relay, so the Steam ID is the whole address
    RoomCode = PendingJoinCode;
    RoomNetwork = ENsRoomNetwork::Steam;
    PendingJoinCode.Reset();
    const FString Address = FString::Printf(TEXT("steam.%s:%d"), *HostSteamId, FURL::UrlConfig.DefaultPort);
    ReportStatus(ENsRoomStatus::FoundWorldwide, false, RoomCode, RoomNetwork);
    PlayerController->ClientTravel(Address, TRAVEL_Absolute);
}

void UNsSessionSubsystem::StopSearches()
{
    if (WorldwideSearch.IsValid())
    {
        WorldwideSearch->Cancel();
        WorldwideSearch.Reset();
    }
    if (SearchTicker.IsValid())
    {
        FTSTicker::GetCoreTicker().RemoveTicker(SearchTicker);
        SearchTicker.Reset();
    }
    for (const ENsRoomNetwork Network : { ENsRoomNetwork::Lan, ENsRoomNetwork::Steam })
    {
        const bool bSteam = Network == ENsRoomNetwork::Steam;
        TSharedPtr<FOnlineSessionSearch>& Search = bSteam ? SteamSearch : LanSearch;
        const TSharedPtr<IOnlineSession, ESPMode::ThreadSafe> Sessions = GetSessionInterface(Network);
        if (Sessions.IsValid())
        {
            Sessions->ClearOnFindSessionsCompleteDelegate_Handle(bSteam ? SteamSearchHandle : LanSearchHandle);
            if (Search.IsValid() && Search->SearchState == EOnlineAsyncTaskState::InProgress)
            {
                Sessions->CancelFindSessions();
            }
        }
        Search.Reset();
    }
}

void UNsSessionSubsystem::JoinFoundRoom(const ENsRoomNetwork Network, const FOnlineSessionSearchResult& Result)
{
    StopSearches();
    const TSharedPtr<IOnlineSession, ESPMode::ThreadSafe> Sessions = GetSessionInterface(Network);
    if (!Sessions.IsValid())
    {
        ReportStatus(ENsRoomStatus::JoinNoInterface, true, PendingJoinCode, Network);
        return;
    }
    JoinNetwork = Network;
    ReportStatus(ENsRoomStatus::Found, false, PendingJoinCode, Network);
    JoinSessionHandle = Sessions->AddOnJoinSessionCompleteDelegate_Handle(
        FOnJoinSessionCompleteDelegate::CreateUObject(this, &UNsSessionSubsystem::HandleJoinSessionComplete));
    if (!Sessions->JoinSession(0, NAME_GameSession, Result))
    {
        Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionHandle);
        ReportStatus(ENsRoomStatus::JoinFailed, true, PendingJoinCode, Network);
    }
}

void UNsSessionSubsystem::HandleJoinSessionComplete(const FName SessionName, const EOnJoinSessionCompleteResult::Type Result)
{
    const TSharedPtr<IOnlineSession, ESPMode::ThreadSafe> Sessions = GetSessionInterface(JoinNetwork);
    if (Sessions.IsValid())
    {
        Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionHandle);
    }

    FString ConnectAddress;
    if (Result != EOnJoinSessionCompleteResult::Success || !Sessions.IsValid()
        || !Sessions->GetResolvedConnectString(SessionName, ConnectAddress))
    {
        ReportStatus(Result == EOnJoinSessionCompleteResult::SessionIsFull ? ENsRoomStatus::RoomFull : ENsRoomStatus::JoinRefused,
            true, PendingJoinCode, JoinNetwork);
        return;
    }

    APlayerController* const PlayerController = GetGameInstance()->GetFirstLocalPlayerController();
    if (!IsValid(PlayerController))
    {
        ReportStatus(ENsRoomStatus::NoController, true, PendingJoinCode, JoinNetwork);
        return;
    }
    RoomCode = PendingJoinCode;
    RoomNetwork = JoinNetwork;
    PendingJoinCode.Reset();
    UE_LOG(LogNsSession, Display, TEXT("Joining room %s at %s."), *RoomCode, *ConnectAddress);
    PlayerController->ClientTravel(ConnectAddress, TRAVEL_Absolute);
}

bool UNsSessionSubsystem::HasPendingCode(const FOnlineSessionSearchResult& Result) const
{
    FString Code;
    FString Game;
    return Result.Session.SessionSettings.Get(RoomCodeKey, Code) && Code == PendingJoinCode
        && Result.Session.SessionSettings.Get(RoomGameKey, Game) && Game == GetGameTag();
}

void UNsSessionSubsystem::ReportStatus(const ENsRoomStatus Status, const bool bFailed, const FString& Code, const ENsRoomNetwork Network, const FString& Detail)
{
    FNsRoomStatus Report;
    Report.Status = Status;
    Report.bFailed = bFailed;
    Report.RoomCode = Code;
    Report.Network = Network;
    Report.Detail = Detail;
    if (bFailed)
    {
        UE_LOG(LogNsSession, Warning, TEXT("%s"), *Report.GetDefaultText().ToString());
    }
    else
    {
        UE_LOG(LogNsSession, Display, TEXT("%s"), *Report.GetDefaultText().ToString());
    }

    // A lost connection sends the client home, so this failure waits for the title menu
    if (Status == ENsRoomStatus::HostLeft || Status == ENsRoomStatus::ConnectionFailed)
    {
        LastFailure = Report;
    }
    LastStatus = Report;
    OnRoomStatus.Broadcast(Report);
}
