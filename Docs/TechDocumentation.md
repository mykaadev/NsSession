# NsSession Technical Documentation

## Architecture

| Module | Depends on | Content |
|---|---|---|
| `NsSession` | Core, CoreUObject, Engine, CoreOnline, OnlineSubsystem, OnlineSubsystemUtils, Steamworks on Windows | `UNsSessionSubsystem`, `FNsRoomStatus`, the worldwide search |
| `NsSessionSteam` | Core, CoreUObject, Engine, SteamSockets | `UNsSteamNetConnection` |

`NsSessionSteam` builds on Windows only. It does not use the core module.

Host flow:

1. `HostRoom` checks the map, makes a code, and picks Steam or LAN.
2. The map opens as a listen server. A LAN room adds `bIsLanMatch`.
3. After the map load, `CreateRoomSession` advertises the code, the game tag, and the Steam ID of the host.

Join flow:

1. `JoinRoom` checks the code and starts the LAN search and the Steam search.
2. Steam filters the code on its servers. A ticker checks each LAN reply.
3. The first match joins. The other search stops.
4. When both searches find nothing, `FNsSteamLobbySearch` asks Steamworks for a lobby in every region. A match travels to `steam.<host id>`.

## Lifetime

- `Initialize` binds the map load, the travel failure, and the network failure.
- `Deinitialize` stops the searches and removes the bindings.
- `LeaveRoom` stops the searches and destroys the session on both networks.

## Extension Points

| Hook | Kind | Use |
|---|---|---|
| `OnRoomStatus` | event | Status lines in the menu |
| `OnRoomEntered` | event | Close menus, start music |
| `ConsumeLastFailure` | function | The title menu shows why the room closed |
| `FNsRoomStatus::GetDefaultText` | function | English text for any status |
| `GameTag`, `MaxPlayers` | config | Room rules of one game |

## Dependencies

Engine modules, the engine plugins `OnlineSubsystem`, `OnlineSubsystemUtils`, and `SteamSockets`, and the engine Steamworks library on Windows. No studio plugin.

## Performance

- The LAN ticker runs every 0.1 seconds, only while a search runs.
- Nothing runs while the player is in a room.

## Networking

- The session keys are `NSCODE`, `NSGAME`, and `NSHOST`. Steam stores a string key with the suffix `_s` in the lobby data.
- `UNsSteamNetConnection` points the address key at the right connection when one Steam ID has two connections. Without it, the host crashes when the second connection closes.

## Testing

- `NsSession.RoomCode`: new codes pass the code check. Short codes, the letter O, and digits fail.
- `NsSession.StatusText`: every status has a default line.
- Host and join need two machines. Use the playtest checklist of the game.

## Change Rules

- Keep the session keys the same in every build that must find the same rooms.
- Add a status at the end of `ENsRoomStatus`, and give it a default line.
- Keep menus, names, and map choices out of the plugin.
