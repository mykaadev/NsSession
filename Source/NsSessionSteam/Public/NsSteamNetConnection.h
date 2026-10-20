// Copyright (C) 2025 nulled.softworks. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "SteamSocketsNetConnection.h"
#include "NsSteamNetConnection.generated.h"

/**
 * A Steam connection that keeps the host address map correct
 * One Steam ID can open a second connection before the first one closes. Both connections use the same address key
 * The engine removes that key when the first connection closes, and the host crashes when the second connection closes
 * Set it as NetConnectionClassName of the Steam net driver
 */
UCLASS(Transient, Config = Engine)
class NSSESSIONSTEAM_API UNsSteamNetConnection : public USteamSocketsNetConnection
{
    GENERATED_BODY()

// Functions
public:
    //~ Begin UNetConnection Interface
    virtual void CleanUp() override;
    //~ End UNetConnection Interface
};
