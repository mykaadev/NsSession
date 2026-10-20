# NsSession User Guide

## Purpose

NsSession gives rooms with a 5-letter code, like KXQTP. The host opens a map and gets a code. A friend types the code and joins. A room is a Steam lobby when Steam runs, and a LAN session in other cases. When the nearby Steam search finds nothing, a second search looks in every region.

NsSession has no UI, no player name, and no map setting. The game gives the map, shows the status lines, and closes its menus. NsSession depends on no other studio plugin.

## Setup

1. Enable `NsSession` in the `.uproject` file. It enables the engine plugins `OnlineSubsystem`, `OnlineSubsystemUtils`, and `SteamSockets`.
2. Add `NsSession` to the module dependencies of the game.
3. For Steam, enable `OnlineSubsystemSteam` in the game and set it up in `DefaultEngine.ini`.
4. For Steam, set the connection class of the Steam net driver:

```ini
[/Script/SteamSockets.SteamSocketsNetDriver]
NetConnectionClassName="/Script/NsSessionSteam.NsSteamNetConnection"
```

## Quick Start

```cpp
void UMyMenu::Host()
{
    UNsSessionSubsystem::Get(this)->HostRoom(MyGameplayMap);
}

void UMyMenu::Join(const FString& Code)
{
    UNsSessionSubsystem::Get(this)->JoinRoom(Code);
}

void UMyMenu::HandleRoomStatus(const FNsRoomStatus& Status)
{
    StatusText->SetText(Status.GetDefaultText());
}
```

Bind `HandleRoomStatus` to `OnRoomStatus`. It is a dynamic event, so the handler is a `UFUNCTION`.

## Common Tasks

### Show the room code in the HUD

Call `GetRoomCode` and `GetRoomNetwork`. The code is empty in a game without a room.

### Use your own status text

Read `FNsRoomStatus::Status` and pick your own line. Call `GetDefaultText` for the steps that you do not change.

### Close the menus when the room opens

Bind `OnRoomEntered`. It fires on the host and on the client when the room map loads.

### Show why a player came back to the title

In the title menu, call `ConsumeLastFailure`. It gives the failure of a lost connection one time.

### Test without Steam

Start the game with `-lanroom`. Play In Editor always uses LAN.

### Join without a code

Call `JoinDirectAddress` with an IP address. This is for tests.

## Rules

- Call `LeaveRoom` when the player goes back to the title menu.
- Keep the menus, the names, and the map choice in the game.
- Give each game its own `GameTag` when two games share one Steam app ID.

## Settings

On `UNsSessionSubsystem`, in `DefaultGame.ini` under `[/Script/NsSession.NsSessionSubsystem]`:

| Setting | Default | Effect |
|---|---|---|
| `GameTag` | project name | Marks the rooms of this game |
| `MaxPlayers` | 6 | Players in one room, host included |

## Debug

- Log category `LogNsSession`: each status line, the session creation, and each search that cannot start.
- Log category `LogNsSessionSteam`: a second connection from one Steam ID.
- Run the tests `NsSession.RoomCode` and `NsSession.StatusText` in Session Frontend.

## Limits

- One room for each game instance.
- No session browser. A player needs the code.
- The worldwide search needs Steam on Windows.
