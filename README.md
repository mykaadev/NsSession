<!-- GH_ONLY_START -->
<h1 align="center">
  <br>
  <a href="https://github.com/mykaadev/NsSession"><img src="https://raw.githubusercontent.com/mykaadev/NsSession/refs/heads/main/Resources/Banner.png" alt="NsSession" width="400"></a>
</h1>

<h4 align="center">Rooms with a short code over LAN and Steam, with a worldwide Steam search, for Unreal Engine.</h4>

<div align="center">
    <a href="https://github.com/mykaadev/NsSession/commits/main"><img src="https://img.shields.io/github/last-commit/mykaadev/NsSession?style=plastic&logo=github&logoColor=white" alt="GitHub Last Commit"></a>
    <a href="https://github.com/mykaadev/NsSession/issues"><img src="https://img.shields.io/github/issues-raw/mykaadev/NsSession?style=plastic&logo=github&logoColor=white" alt="GitHub Issues"></a>
    <a href="https://github.com/mykaadev/NsSession/pulls"><img src="https://img.shields.io/github/issues-pr-raw/mykaadev/NsSession?style=plastic&logo=github&logoColor=white" alt="GitHub Pull Requests"></a>
    <a href="https://github.com/mykaadev/NsSession"><img src="https://img.shields.io/github/stars/mykaadev/NsSession?style=plastic&logo=github" alt="GitHub Stars"></a>
    <a href="https://twitter.com/mykaadev/"><img src="https://img.shields.io/twitter/follow/mykaadev?style=plastic&logo=x" alt="Twitter Follow"></a>

<p style="display:none;">
  <a href="#-summary">👀 Summary</a> •
  <a href="#-features">📦 Features</a> •
  <a href="#-requirements">⚙️ Requirements</a> •
  <a href="#-installation">🛠️ Installation</a> •
  <a href="#-getting-started">🚀 Getting Started</a> •
  <a href="#-modules">🧩 Modules</a> •
  <a href="#-api">🔧 API</a> •
  <a href="#-limits">🚧 Limits</a> •
  <a href="#-documentation">📚 Documentation</a> •
  <a href="#-credits">❤️ Credits</a> •
  <a href="#-support">📞 Support</a> •
  <a href="#-license">📃 License</a>
</p>
<a href="https://buymeacoffee.com/mykaadev"><img src="https://www.svgrepo.com/show/476855/coffee-to-go.svg" alt="Coffee" width="50px"></a>
<p><b>Buy me a coffee!</b></p>
</div>
<!-- GH_ONLY_END -->

## 👀 Summary
NsSession gives multiplayer rooms with a 5-letter code, like `KXQTP`. The host opens a map and gets a code. A friend types the code and joins. A room is a Steam lobby when Steam runs, and a LAN session in other cases. When the nearby Steam search finds nothing, a second search looks in every region.

NsSession has no UI, no session browser, no player names, and no map setting. Your game gives the map, shows the status lines, and opens the title map after a leave.

## 📦 Features
- **Room codes**: Host a room and share a 5-letter code. Join with the code. The codes have no I and no O.
- **Steam and LAN**: A Steam lobby when the player is logged in to Steam, and a LAN session in other cases.
- **One join for both networks**: A join searches LAN and Steam at the same time. The first match wins.
- **Worldwide search**: When the nearby Steam search finds nothing, a second search looks in every region.
- **Status events**: Each step and each failure of a host or a join sends a status with a default English line.
- **Failure on return**: The title menu can show why the player lost the connection.
- **Steam connection fix**: A net connection class keeps the host alive when one Steam ID opens two connections.
- **Test helpers**: Start with `-lanroom` to test without Steam, or join an address directly.

## ⚙️ Requirements
- Unreal Engine 5.7+
- The plugin enables `OnlineSubsystem`, `OnlineSubsystemUtils`, and `SteamSockets` (Win64).
- For Steam rooms: enable the engine plugin `OnlineSubsystemSteam` in your project and set it up in `DefaultEngine.ini`. Steam rooms work on Win64 only.

## 🛠️ Installation
1. **Clone** or download this repository.
2. Copy the `NsSession` folder into the `Plugins` folder of your project.
3. For C++, add `"NsSession"` to the dependencies of your game module in `Build.cs`.
4. Generate the project files and enable the plugin.
5. For Steam, set the connection class of the Steam net driver in `DefaultEngine.ini`:

```ini
[/Script/SteamSockets.SteamSocketsNetDriver]
NetConnectionClassName="/Script/NsSessionSteam.NsSteamNetConnection"
```

## 🚀 Getting Started
In Blueprint, get the subsystem with `Get Ns Session Subsystem`. Call `Host Room` with a map, or `Join Room` with a code. Bind `On Room Status` and show `Get Status Text` of each status.

In C++:

```cpp
#include "NsSessionSubsystem.h"

// RoomMap is a TSoftObjectPtr<UWorld> property of the menu.
void UMyMenu::Host()
{
    UNsSessionSubsystem::Get(this)->HostRoom(RoomMap.ToSoftObjectPath());
}

void UMyMenu::Join(const FString& Code)
{
    UNsSessionSubsystem::Get(this)->JoinRoom(Code);
}

// Bound to OnRoomStatus. OnRoomStatus is a dynamic event, so this handler is a UFUNCTION.
void UMyMenu::HandleRoomStatus(const FNsRoomStatus& Status)
{
    StatusText->SetText(Status.GetDefaultText());
}
```

## 🧩 Modules
| Module | Platforms | Content |
|---|---|---|
| `NsSession` | All | The subsystem, the status types, and the worldwide Steam search (Win64) |
| `NsSessionSteam` | Win64 | `UNsSteamNetConnection` |

## 🔧 API
- `UNsSessionSubsystem` – The game instance subsystem that hosts, joins, and leaves rooms.
- `HostRoom` / `JoinRoom` / `LeaveRoom` – Open a room, join a room by code, and end the room on this machine.
- `GetRoomCode` / `GetRoomNetwork` – The code and the network of the current room.
- `GetLastStatus` / `GetLastStatusText` – The latest status, for a menu that reads it each frame.
- `ConsumeLastFailure` – The failure of a lost connection, one time.
- `OnRoomStatus` / `OnRoomEntered` – Each status step, and each load of a map as a listen server or client.
- `FNsRoomStatus` – `Status`, `bFailed`, `RoomCode`, `Network`, `Detail`, and `GetDefaultText`.
- `UNsSteamNetConnection` – The Steam net connection class.

## 🚧 Limits
- One room for each game instance.
- No session browser. A player needs the code.
- Play In Editor and `-lanroom` always use LAN.
- The worldwide search and Steam rooms work on Win64 only.

## 📚 Documentation
- [User Guide](https://mykaadev.github.io/plugins/nssession/user-guide/) – Setup, common tasks, settings, and debug tools.
- [Technical Documentation](https://mykaadev.github.io/plugins/nssession/tech-documentation/) – Architecture, lifetime, and extension points.

<!-- GH_ONLY_START -->
## ❤️ Credits
<a href="https://github.com/mykaadev/NsSession/graphs/contributors"><img src="https://contrib.rocks/image?repo=mykaadev/NsSession"/></a>

## 📞 Support
Reach out via the **[profile page](https://github.com/mykaadev)**.

## 📃 License
[![License](https://img.shields.io/badge/license-MIT-green)](https://www.tldrlegal.com/license/mit-license)
<!-- GH_ONLY_END -->
