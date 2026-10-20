// Copyright (C) 2025 nulled.softworks. All rights reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Worldwide Steam lobby search for one room code
 * The engine Steam search looks only in nearby regions. This search calls Steamworks directly with the worldwide filter
 * It returns the Steam ID of the host, so the client can travel to steam.<id> without the engine session
 */
class FNsSteamLobbySearch : public TSharedFromThis<FNsSteamLobbySearch>
{
public:
    /** Receives the host Steam ID, or an empty string when no room matches. It runs on the game thread */
    using FOnSearchDone = TDelegate<void(const FString& HostSteamId)>;

    /** Constructor */
    FNsSteamLobbySearch();

    /** Destructor. It is in the source file, where the Steam state type is complete */
    ~FNsSteamLobbySearch();

    /** Returns true when Steam runs and the search can start */
    static bool IsAvailable();

    /** Starts the search. Done runs one time */
    bool Start(const FString& CodeKey, const FString& Code, const FString& GameKey, const FString& GameTag, const FString& HostKey, FOnSearchDone Done);

    /** Stops the search. Done does not run */
    void Cancel();

private:
    /** Hidden Steamworks state, so this header needs no Steam include */
    struct FImpl;
    TUniquePtr<FImpl> Impl;
};
