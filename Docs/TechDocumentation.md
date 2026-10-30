# NsSession Technical Documentation

## Architecture

### Modules

| Module | Type | Loading phase | Platforms | Content |
|---|---|---|---|---|
| `NsSession` | Runtime | Default | All | `UNsSessionSubsystem`, `FNsRoomStatus`, the status enums, `FNsSteamLobbySearch`, and the `LogNsSession` log category |
| `NsSessionSteam` | Runtime | Default | Win64 (`PlatformAllowList`) | `UNsSteamNetConnection` |

`NsSessionSteam` does not depend on `NsSession`. The `.uplugin` file enables `OnlineSubsystem`, `OnlineSubsystemUtils`, and `SteamSockets` (Win64 only). None of them is optional. `OnlineSubsystemSteam` is not in the list. The game enables it.

### Folders

- `Source/NsSession/Public` – `NsSessionModule.h`, `NsSessionSubsystem.h`, `NsSessionTypes.h`.
- `Source/NsSession/Private` – `NsSessionModule.cpp`, `NsSessionSubsystem.cpp`, `NsSessionTypes.cpp`, `NsSteamLobbySearch.h`, `NsSteamLobbySearch.cpp`, and `Tests/NsSessionTest.cpp`.
- `Source/NsSessionSteam/Public` – `NsSteamNetConnection.h`.
- `Source/NsSessionSteam/Private` – `NsSessionSteamModule.cpp`, `NsSteamNetConnection.cpp`.

### Main Classes

| Type | Responsibility |
|---|---|
| `UNsSessionSubsystem` | A `UGameInstanceSubsystem` with `Config = Game`. Hosts, joins, and leaves rooms, and reports each status. |
| `FNsRoomStatus` | `Status`, `bFailed`, `RoomCode`, `Network`, `Detail`. `GetDefaultText()` gives the English line. `GetNetworkName()` gives "LAN" or "Steam". |
| `ENsRoomStatus` | The 18 steps and failures of a host or a join. |
| `ENsRoomNetwork` | `Lan`, `Steam`. |
| `FNsSteamLobbySearch` | A private class. Calls Steamworks `RequestLobbyList` with the worldwide distance filter and returns the Steam ID of the host. |
| `UNsSteamNetConnection` | A `USteamSocketsNetConnection` that overrides `CleanUp` to keep `MappedClientConnections` correct when one Steam ID has two connections. |
| `FNsSessionModule` | The module class of `NsSession`. It defines `LogNsSession` and has no startup code. `NsSessionSteam` uses `FDefaultModuleImpl`. |

### Data Flow

Host:

1. `HostRoom` returns false when a join search runs or a host is pending. It reports `NoMap` for an empty path and `MapMissing` when `FPackageName::DoesPackageExist` fails.
2. It calls `LeaveRoom`, makes a code with `MakeRoomCode`, and picks Steam when `CanUseSteam()` is true, else LAN.
3. It reports `Opening` and calls `UGameplayStatics::OpenLevel` with `listen`, or `listen?bIsLanMatch` for LAN.
4. `HandlePostLoadMap` fires `OnRoomEntered` for a listen server or client world of this game instance. For a pending host on a listen server, it calls `CreateRoomSession`.
5. `CreateRoomSession` creates `NAME_GameSession` with `NumPublicConnections = MaxPlayers`, join in progress, and the keys `NSCODE` and `NSGAME`. A Steam room also uses lobbies and presence, and sets `NSHOST` to the Steam ID of the host.
6. When the creation fails, the subsystem writes an error to `LogNsSession` and clears the code. It sends no status.

Join:

1. `JoinRoom` normalizes the code. A bad code reports `BadCode`. Then it returns false when a join search runs or a host is pending.
2. It calls `LeaveRoom` and starts the LAN search (`bIsLanQuery`, 64 results). When `CanUseSteam()` is true, it also starts the Steam search (`SEARCH_LOBBIES`, one result, filters on `NSCODE` and `NSGAME`).
3. When no search starts, it reports `SearchFailed`. Else it adds a core ticker with a 0.1 s delay and reports `Searching`.
4. The ticker and each search completion call `TryJoinMatch`, which checks the results of both searches for the code and the game tag.
5. The first match calls `JoinFoundRoom`. It stops all searches, reports `Found`, and calls `JoinSession`.
6. `HandleJoinSessionComplete` gets the connect string and calls `ClientTravel` on the first local player controller. It reports `RoomFull`, `JoinRefused`, or `NoController` on a failure.
7. When both searches end with no match, `FinishSearchWithoutMatch` starts `FNsSteamLobbySearch` when Steam can run, and reports `SearchingWorldwide`. Else it reports `NotFound`.
8. The worldwide search filters the lobby data keys `NSCODE_s` and `NSGAME_s`, and reads `NSHOST_s`. On a match, the client travels to `steam.<host id>:<default port>` and the subsystem reports `FoundWorldwide`.

Failures:

- `HandleTravelFailure` calls `LeaveRoom` and reports `TravelFailed`.
- `HandleNetworkFailure` calls `LeaveRoom` and reports `HostLeft` for `ConnectionLost` and `ConnectionTimeout`, else `ConnectionFailed`.
- `ReportStatus` writes the line to `LogNsSession` (Warning for a failure, Display for a step), stores `LastStatus`, stores `LastFailure` for `HostLeft` and `ConnectionFailed`, and fires `OnRoomStatus`.

Threading: Steam calls `FImpl::OnLobbyList` on its callback thread. The function moves the result to the game thread with `AsyncTask`. A cancelled search drops the result. All other code runs on the game thread.

## Lifetime

1. `Initialize` binds `FCoreUObjectDelegates::PostLoadMapWithWorld`, `GEngine->OnTravelFailure()`, and `GEngine->OnNetworkFailure()`.
2. `Deinitialize` stops the searches and removes the three bindings.
3. The subsystem lives as long as the game instance. The code, the last status, and the last failure stay across map travel.
4. `LeaveRoom` stops all searches, clears the pending host and the code, and destroys `NAME_GameSession` on the Null and the Steam session interfaces. It does not travel.
5. `GetSessionInterface` uses `Online::GetSubsystem(GetWorld(), ...)`, so each Play In Editor world gets its own online subsystem instance.

## Extension Points

- `OnRoomStatus` (`FOnNsRoomStatus`, `const FNsRoomStatus& Status`) – `BlueprintAssignable`.
- `OnRoomEntered` (`FOnNsRoomEntered`, no parameters) – `BlueprintAssignable`.
- `static UNsSessionSubsystem* Get(const UObject* WorldContext)` – C++ access.
- `static FString NormalizeRoomCode(const FString& Code)` and `static FString MakeRoomCode()` – C++ only.
- `FNsRoomStatus::GetDefaultText()` – Use it, or map `Status` to your own text.
- `GameTag` and `MaxPlayers` – `UPROPERTY(Config)` in `DefaultGame.ini`, section `[/Script/NsSession.NsSessionSubsystem]`. See the [User Guide](UserGuide.md).
- `UNsSteamNetConnection` – `Config = Engine`. Set it as `NetConnectionClassName` of `SteamSocketsNetDriver`.

The subsystem has no virtual functions of its own. Its private handlers are not for override.

## Dependencies

| Dependency | Kind | Why |
|---|---|---|
| `Core`, `CoreUObject`, `Engine` | Public module (both modules) | Base types, the subsystem, travel, and the net driver |
| `CoreOnline`, `OnlineSubsystem`, `OnlineSubsystemUtils` | Public module of `NsSession` | Session interfaces, identity, and `Online::GetSubsystem` |
| `Steamworks` | Private third-party library of `NsSession`, Win64 only | The worldwide lobby search. Sets `NSSESSION_WITH_STEAM=1` on Win64, else 0 |
| `SteamSockets` | Public module of `NsSessionSteam` | `USteamSocketsNetConnection` |
| `OnlineSubsystem`, `OnlineSubsystemUtils` | Plugin, not optional | Enabled by the `.uplugin` file |
| `SteamSockets` | Plugin, not optional, Win64 | Enabled by the `.uplugin` file |

## Performance

- The subsystem does not tick. The search ticker runs only while a LAN or Steam join search runs, with a 0.1 s delay.
- `TryJoinMatch` walks up to 64 LAN results and one Steam result on each ticker call.
- Nothing runs while the player is in a room, except the engine delegates for map load, travel failure, and network failure.

## Networking

- The plugin has no replicated properties and no RPCs. It uses the online session interfaces and engine travel.
- Each machine keeps its own room code. The host makes it, and a client keeps the code that it typed.
- Session keys: `NSCODE` (the code), `NSGAME` (the game tag), and `NSHOST` (the Steam ID of the host, Steam only). All use `ViaOnlineServiceAndPing`. Steam stores a string key with the suffix `_s` in the lobby data.
- `CanUseSteam()` is false in Play In Editor, with `-lanroom`, and when the Steam identity is not logged in for user 0.
- `UNsSteamNetConnection::CleanUp` points the address key in `MappedClientConnections` at itself before the engine removes it. Then it gives the key to another open connection from the same address. It logs that case to `LogNsSessionSteam`.

## Testing

| Test | What it checks |
|---|---|
| `NsSession.RoomCode` | 50 new codes pass `NormalizeRoomCode`. A code goes to upper case. A short code, the letter O, and a digit fail. |
| `NsSession.StatusText` | Each `ENsRoomStatus` value has a default line. |

The tests use `EditorContext` and `EngineFilter`. Run them in **Session Frontend > Automation**, filter `NsSession`. Or start the editor with `-ExecCmds="Automation RunTests NsSession"`.

The plugin has no automated test for host and join. A host and join test needs two game instances, for example Play In Editor with 2 players in Standalone net mode, which uses LAN.

## Change Rules

- Keep the session keys `NSCODE`, `NSGAME`, and `NSHOST`, and the `_s` suffix, the same in every build that must find the same rooms.
- Keep `RoomCodeAlphabet` and `RoomCodeLength` the same in every build that must join the same rooms. `NormalizeRoomCode` refuses codes outside them.
- Add a new status at the end of `ENsRoomStatus`, and give it a line in `GetDefaultText`. The test `NsSession.StatusText` fails for a status without a line.
- Keep the class path `/Script/NsSessionSteam.NsSteamNetConnection`. Projects name it in `DefaultEngine.ini`.
