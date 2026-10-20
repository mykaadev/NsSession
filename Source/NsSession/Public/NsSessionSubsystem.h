// Copyright (C) 2025 nulled.softworks. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Engine/EngineBaseTypes.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "NsSessionTypes.h"
#include "OnlineSessionSettings.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "NsSessionSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNsRoomStatus, const FNsRoomStatus&, Status);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnNsRoomEntered);

/**
 * Rooms with a 5-letter code, like KXQTP. There is no session browser and no IP address to type
 *
 * Host: opens a map as a listen server, then creates a session that carries the code
 * The room is a Steam lobby when Steam runs, otherwise a LAN session. Play In Editor and -lanroom always use LAN
 *
 * Join: searches LAN and Steam at the same time. The first room with the same code wins, and the other search stops
 * Steam filters the code on its servers. The LAN search checks each reply as it arrives
 * When Steam finds nothing nearby, a second Steam search looks in every region
 *
 * Every machine keeps its own copy of the code: the host made it, and a client typed it
 */
UCLASS(Config = Game)
class NSSESSION_API UNsSessionSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

// Functions
public:
    //~ Begin USubsystem Interface
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    //~ End USubsystem Interface

    /** Returns the session subsystem of the game instance of a world context, or null */
    static UNsSessionSubsystem* Get(const UObject* WorldContext);

    /** Returns true when this machine can host and search Steam rooms */
    UFUNCTION(BlueprintPure, Category = "Session")
    bool CanUseSteam() const;

    /** Makes a new code, opens the map as a listen server, and advertises the room */
    UFUNCTION(BlueprintCallable, Category = "Session")
    bool HostRoom(const FSoftObjectPath& Map);

    /** Finds the room with this code on LAN and Steam, then joins it */
    UFUNCTION(BlueprintCallable, Category = "Session")
    bool JoinRoom(const FString& Code);

    /** Ends the current room on this machine and stops any search */
    UFUNCTION(BlueprintCallable, Category = "Session")
    void LeaveRoom();

    /** Opens a direct-connect address on this client. For tests without a code */
    UFUNCTION(BlueprintCallable, Category = "Session")
    bool JoinDirectAddress(const FString& Address);

    /** Returns the code of the current room, or an empty string */
    UFUNCTION(BlueprintPure, Category = "Session")
    FString GetRoomCode() const { return RoomCode; }

    /** Returns how players reach the current room */
    UFUNCTION(BlueprintPure, Category = "Session")
    ENsRoomNetwork GetRoomNetwork() const { return RoomNetwork; }

    /** Returns true while a join search runs on LAN or Steam. The worldwide search does not count, so a new join can stop it */
    UFUNCTION(BlueprintPure, Category = "Session")
    bool IsSearching() const;

    /** Gives the last failure of a lost room one time, then clears it. A title menu shows it after the travel home */
    bool ConsumeLastFailure(FNsRoomStatus& OutStatus);

    /** Returns the code in upper case when it is a valid room code, otherwise an empty string */
    static FString NormalizeRoomCode(const FString& Code);

    /** Makes a random code from letters that players cannot mix up */
    static FString MakeRoomCode();

private:
    /** Returns the session interface of one network, or null when it is not available */
    TSharedPtr<class IOnlineSession, ESPMode::ThreadSafe> GetSessionInterface(ENsRoomNetwork Network) const;

    /** Returns the value of the game key: GameTag, or the project name */
    FString GetGameTag() const;

    /** Creates the session for a pending host after its listen map loads */
    void HandlePostLoadMap(class UWorld* World);

    /** Ends the room and reports the error when a map travel fails */
    void HandleTravelFailure(class UWorld* World, ETravelFailure::Type FailureType, const FString& Error);

    /** Ends the room and reports the error when the connection to the host fails */
    void HandleNetworkFailure(class UWorld* World, class UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& Error);

    /** Creates and advertises the room session on the host */
    void CreateRoomSession();

    /** Handles the result of the room session creation */
    void HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful);

    /** Starts the search on one network. Returns true when the search started */
    bool StartSearch(ENsRoomNetwork Network);

    /** Handles the end of the search on one network */
    void HandleSearchComplete(bool bWasSuccessful, ENsRoomNetwork Network);

    /** Checks the search results for the code. Runs each frame while a search runs */
    bool TickSearch(float DeltaTime);

    /** Joins the first search result with the code. Returns true when a join started */
    bool TryJoinMatch();

    /** Reports a failed join when no search runs any more. Steam players get a worldwide search first */
    void FinishSearchWithoutMatch();

    /** Searches Steam lobbies in every region for the pending code */
    bool StartWorldwideSearch();

    /** Travels to the host that the worldwide search found */
    void HandleWorldwideResult(const FString& HostSteamId);

    /** Stops every search and removes its callbacks */
    void StopSearches();

    /** Joins one matching room on its network */
    void JoinFoundRoom(ENsRoomNetwork Network, const FOnlineSessionSearchResult& Result);

    /** Handles the join result and travels to the host */
    void HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);

    /** Returns true when a search result carries the code that the player typed */
    bool HasPendingCode(const FOnlineSessionSearchResult& Result) const;

    /** Sends a status to the game and writes it to the log */
    void ReportStatus(ENsRoomStatus Status, bool bFailed, const FString& Code, ENsRoomNetwork Network, const FString& Detail = FString());

// Variables
public:
    /** Fires with each step and each failure of a host or a join */
    UPROPERTY(BlueprintAssignable, Category = "Session")
    FOnNsRoomStatus OnRoomStatus;

    /** Fires when a host or a client loads the room map. A game closes its menus here */
    UPROPERTY(BlueprintAssignable, Category = "Session")
    FOnNsRoomEntered OnRoomEntered;

    /** Value that marks the rooms of this game. Many test games share the Spacewar app ID. Empty uses the project name */
    UPROPERTY(Config)
    FString GameTag;

    /** Players in one room, host included */
    UPROPERTY(Config)
    int32 MaxPlayers = 6;

private:
    /** Letters for room codes. I and O are missing, so players cannot mix them up with 1 and 0 */
    static constexpr const TCHAR* RoomCodeAlphabet = TEXT("ABCDEFGHJKLMNPQRSTUVWXYZ");

    /** Number of letters in a room code */
    static constexpr int32 RoomCodeLength = 5;

    /** Session key that carries the room code */
    static inline const FName RoomCodeKey = TEXT("NSCODE");

    /** Session key that marks the rooms of one game */
    static inline const FName RoomGameKey = TEXT("NSGAME");

    /** Session key with the Steam ID of the host, for the worldwide search */
    static inline const FName RoomHostKey = TEXT("NSHOST");

    /** Suffix that the engine adds to a string session setting in the Steam lobby data */
    static constexpr const TCHAR* SteamLobbyStringSuffix = TEXT("_s");

    /** Code of the current room */
    FString RoomCode;

    /** Code that the current join searches for */
    FString PendingJoinCode;

    /** Network of the current room */
    ENsRoomNetwork RoomNetwork = ENsRoomNetwork::Lan;

    /** True between the host map travel and the session creation */
    bool bPendingHost = false;

    /** Settings and results of the LAN search */
    TSharedPtr<FOnlineSessionSearch> LanSearch;

    /** Settings and results of the Steam search */
    TSharedPtr<FOnlineSessionSearch> SteamSearch;

    /** Network of the session that the join uses */
    ENsRoomNetwork JoinNetwork = ENsRoomNetwork::Lan;

    /** Callback handles on the session interfaces */
    FDelegateHandle CreateSessionHandle;
    FDelegateHandle LanSearchHandle;
    FDelegateHandle SteamSearchHandle;
    FDelegateHandle JoinSessionHandle;

    /** Frame callback that checks search results */
    FTSTicker::FDelegateHandle SearchTicker;

    /** Last failure of a lost room */
    TOptional<FNsRoomStatus> LastFailure;

    /** Worldwide Steam search, when the regional search finds nothing */
    TSharedPtr<class FNsSteamLobbySearch> WorldwideSearch;

    /** Callback for map loads */
    FDelegateHandle PostLoadMapHandle;

    /** Callbacks for failed travel and failed connections */
    FDelegateHandle TravelFailureHandle;
    FDelegateHandle NetworkFailureHandle;
};
