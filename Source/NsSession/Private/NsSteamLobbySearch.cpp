// Copyright (C) 2025 nulled.softworks. All rights reserved.

#include "NsSteamLobbySearch.h"

#include "Async/Async.h"

#if NSSESSION_WITH_STEAM
THIRD_PARTY_INCLUDES_START
#include "steam/steam_api.h"
THIRD_PARTY_INCLUDES_END
#endif

struct FNsSteamLobbySearch::FImpl
{
#if NSSESSION_WITH_STEAM
    /** Steam calls this on the thread that runs the Steam callbacks */
    void OnLobbyList(LobbyMatchList_t* Result, bool bIOFailure)
    {
        FString HostId;
        if (!bIOFailure && Result != nullptr && Result->m_nLobbiesMatching > 0 && SteamMatchmaking() != nullptr)
        {
            const CSteamID Lobby = SteamMatchmaking()->GetLobbyByIndex(0);
            HostId = UTF8_TO_TCHAR(SteamMatchmaking()->GetLobbyData(Lobby, TCHAR_TO_UTF8(*HostKey)));
        }

        // Move the answer to the game thread. A cancelled search drops it
        TWeakPtr<FNsSteamLobbySearch> WeakOwner = Owner;
        AsyncTask(ENamedThreads::GameThread, [WeakOwner, HostId, DoneCopy = Done]()
        {
            if (WeakOwner.IsValid())
            {
                DoneCopy.ExecuteIfBound(HostId);
            }
        });
    }

    /** Pending Steam call */
    CCallResult<FImpl, LobbyMatchList_t> CallResult;
#endif

    /** Search that owns this state */
    TWeakPtr<FNsSteamLobbySearch> Owner;

    /** Lobby data key with the host Steam ID */
    FString HostKey;

    /** Receives the result */
    FOnSearchDone Done;
};

FNsSteamLobbySearch::FNsSteamLobbySearch() = default;

FNsSteamLobbySearch::~FNsSteamLobbySearch()
{
    Cancel();
}

bool FNsSteamLobbySearch::IsAvailable()
{
#if NSSESSION_WITH_STEAM
    return SteamMatchmaking() != nullptr;
#else
    return false;
#endif
}

bool FNsSteamLobbySearch::Start(const FString& CodeKey, const FString& Code, const FString& GameKey, const FString& GameTag,
    const FString& HostKey, FOnSearchDone Done)
{
#if NSSESSION_WITH_STEAM
    ISteamMatchmaking* const Matchmaking = SteamMatchmaking();
    if (Matchmaking == nullptr)
    {
        return false;
    }
    Cancel();
    Impl = MakeUnique<FImpl>();
    Impl->Owner = AsShared();
    Impl->HostKey = HostKey;
    Impl->Done = MoveTemp(Done);

    // The engine stores a string session setting as <key>_s in the lobby data
    Matchmaking->AddRequestLobbyListStringFilter(TCHAR_TO_UTF8(*CodeKey), TCHAR_TO_UTF8(*Code), k_ELobbyComparisonEqual);
    Matchmaking->AddRequestLobbyListStringFilter(TCHAR_TO_UTF8(*GameKey), TCHAR_TO_UTF8(*GameTag), k_ELobbyComparisonEqual);
    Matchmaking->AddRequestLobbyListDistanceFilter(k_ELobbyDistanceFilterWorldwide);
    Matchmaking->AddRequestLobbyListResultCountFilter(1);
    const SteamAPICall_t Call = Matchmaking->RequestLobbyList();
    Impl->CallResult.Set(Call, Impl.Get(), &FImpl::OnLobbyList);
    return true;
#else
    return false;
#endif
}

void FNsSteamLobbySearch::Cancel()
{
#if NSSESSION_WITH_STEAM
    if (Impl.IsValid())
    {
        Impl->CallResult.Cancel();
    }
#endif
    Impl.Reset();
}
