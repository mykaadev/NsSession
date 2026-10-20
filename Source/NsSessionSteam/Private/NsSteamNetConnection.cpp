// Copyright (C) 2025 nulled.softworks. All rights reserved.

#include "NsSteamNetConnection.h"

#include "Engine/NetDriver.h"

DEFINE_LOG_CATEGORY_STATIC(LogNsSessionSteam, Log, All);

void UNsSteamNetConnection::CleanUp()
{
    UNetDriver* const NetDriver = GetDriver();
    const TSharedPtr<const FInternetAddr> Address = GetRemoteAddr();
    if (NetDriver == nullptr || !Address.IsValid() || !NetDriver->ClientConnections.Contains(this))
    {
        Super::CleanUp();
        return;
    }

    // The engine removes the address key and checks that the key was there. Point the key at this connection first
    const TSharedRef<const FInternetAddr> Key = Address.ToSharedRef();
    NetDriver->MappedClientConnections.Add(Key, this);
    Super::CleanUp();

    // Another connection from the same Steam ID can still be open. Give the key back to it, so its close finds the key
    for (UNetConnection* const Other : NetDriver->ClientConnections)
    {
        const TSharedPtr<const FInternetAddr> OtherAddress = IsValid(Other) ? Other->GetRemoteAddr() : nullptr;
        if (Other != this && OtherAddress.IsValid() && *OtherAddress == *Address)
        {
            NetDriver->MappedClientConnections.Add(Key, Other);
            UE_LOG(LogNsSessionSteam, Display, TEXT("A connection from %s closed. Another connection from that address stays open."), *Address->ToString(true));
            break;
        }
    }
}
