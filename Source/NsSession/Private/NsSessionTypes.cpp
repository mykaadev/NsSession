// Copyright (C) 2025 nulled.softworks. All rights reserved.

#include "NsSessionTypes.h"

#define LOCTEXT_NAMESPACE "NsSession"

FText FNsRoomStatus::GetDefaultText() const
{
    const FText Code = FText::FromString(RoomCode);
    const FText Net = GetNetworkName(Network);
    const FText Info = FText::FromString(Detail);
    switch (Status)
    {
    case ENsRoomStatus::Opening:            return FText::Format(LOCTEXT("Opening", "Opening room {0} on {1}..."), Code, Net);
    case ENsRoomStatus::NoMap:              return LOCTEXT("NoMap", "The game has no map to host.");
    case ENsRoomStatus::MapMissing:         return FText::Format(LOCTEXT("MapMissing", "This build has no map {0}. Package the game again with that map."), Info);
    case ENsRoomStatus::BadCode:            return LOCTEXT("BadCode", "Type the 5-letter room code.");
    case ENsRoomStatus::SearchFailed:       return LOCTEXT("SearchFailed", "Cannot search for rooms. Check your Steam or LAN connection.");
    case ENsRoomStatus::Searching:          return FText::Format(LOCTEXT("Searching", "Looking for room {0}..."), Code);
    case ENsRoomStatus::SearchingWorldwide: return FText::Format(LOCTEXT("SearchingWorldwide", "Looking for room {0} in every region..."), Code);
    case ENsRoomStatus::NotFound:           return FText::Format(LOCTEXT("NotFound", "No room {0}. Check the code, and make sure the host is still in the room."), Code);
    case ENsRoomStatus::Found:              return FText::Format(LOCTEXT("Found", "Room {0} found on {1}. Joining..."), Code, Net);
    case ENsRoomStatus::FoundWorldwide:     return FText::Format(LOCTEXT("FoundWorldwide", "Room {0} found in another region. Joining..."), Code);
    case ENsRoomStatus::JoinNoInterface:    return LOCTEXT("JoinNoInterface", "The room was found, but its network is not available.");
    case ENsRoomStatus::JoinFailed:         return LOCTEXT("JoinFailed", "The room was found, but the join could not start.");
    case ENsRoomStatus::RoomFull:           return LOCTEXT("RoomFull", "That room is full.");
    case ENsRoomStatus::JoinRefused:        return LOCTEXT("JoinRefused", "The room did not accept the join. Try again.");
    case ENsRoomStatus::NoController:       return LOCTEXT("NoController", "The join needs a local player controller.");
    case ENsRoomStatus::TravelFailed:       return FText::Format(LOCTEXT("TravelFailed", "The travel failed: {0}"), Info);
    case ENsRoomStatus::HostLeft:           return LOCTEXT("HostLeft", "The host left. The room is closed.");
    case ENsRoomStatus::ConnectionFailed:   return FText::Format(LOCTEXT("ConnectionFailed", "The connection failed: {0}"), Info);
    }
    return FText::GetEmpty();
}

FText FNsRoomStatus::GetNetworkName(const ENsRoomNetwork InNetwork)
{
    return InNetwork == ENsRoomNetwork::Steam ? LOCTEXT("NetworkSteam", "Steam") : LOCTEXT("NetworkLan", "LAN");
}

#undef LOCTEXT_NAMESPACE
