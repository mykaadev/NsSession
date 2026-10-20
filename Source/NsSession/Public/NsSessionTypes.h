// Copyright (C) 2025 nulled.softworks. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NsSessionTypes.generated.h"

/** How players reach a room */
UENUM(BlueprintType)
enum class ENsRoomNetwork : uint8
{
    /** Same local network. The Null subsystem finds the room with a LAN broadcast */
    Lan,
    /** Internet through a Steam lobby and Steam relay connections */
    Steam
};

/** Step or failure of a host or a join. The game turns it into text */
UENUM(BlueprintType)
enum class ENsRoomStatus : uint8
{
    /** The host opens its map */
    Opening,
    /** The host has no map to open */
    NoMap,
    /** The map is not in this build */
    MapMissing,
    /** The code is not a valid room code */
    BadCode,
    /** No network can search */
    SearchFailed,
    /** The search runs on LAN and Steam */
    Searching,
    /** The search runs on Steam in every region */
    SearchingWorldwide,
    /** No room has the code */
    NotFound,
    /** The room is found and the join starts */
    Found,
    /** The room is found in another region and the travel starts */
    FoundWorldwide,
    /** The network of the found room is not available */
    JoinNoInterface,
    /** The join could not start */
    JoinFailed,
    /** The room is full */
    RoomFull,
    /** The room did not accept the join */
    JoinRefused,
    /** The join needs a local player controller */
    NoController,
    /** A map travel failed */
    TravelFailed,
    /** The connection to the host closed */
    HostLeft,
    /** The connection to the host failed */
    ConnectionFailed
};

/** One status line of a host or a join */
USTRUCT(BlueprintType)
struct NSSESSION_API FNsRoomStatus
{
    GENERATED_BODY()

    /** Step or failure */
    UPROPERTY(BlueprintReadOnly, Category = "Session")
    ENsRoomStatus Status = ENsRoomStatus::Opening;

    /** True when the host or the join stopped */
    UPROPERTY(BlueprintReadOnly, Category = "Session")
    bool bFailed = false;

    /** Room code, or empty */
    UPROPERTY(BlueprintReadOnly, Category = "Session")
    FString RoomCode;

    /** Network of the room */
    UPROPERTY(BlueprintReadOnly, Category = "Session")
    ENsRoomNetwork Network = ENsRoomNetwork::Lan;

    /** Map package or engine error, for the steps that have one */
    UPROPERTY(BlueprintReadOnly, Category = "Session")
    FString Detail;

    /** Returns an English line for the status. A game can show it or use its own text */
    FText GetDefaultText() const;

    /** Returns the name of a network: LAN or Steam */
    static FText GetNetworkName(ENsRoomNetwork InNetwork);
};
