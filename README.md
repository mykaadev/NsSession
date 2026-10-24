<!-- GH_ONLY_START -->
<h1 align="center">
  <br>
  <a href="https://github.com/mykaadev/NsSession"><img src="https://raw.githubusercontent.com/mykaadev/NsSession/refs/heads/main/Resources/Banner.png" alt="NsSession" width="400"></a>
</h1>

<h4 align="center">Rooms with a short code over LAN and Steam for Unreal Engine.</h4>

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
NsSession gives rooms with a 5-letter code, like `KXQTP`. The host opens a map and gets a code. A friend types the code and joins. A room is a Steam lobby when Steam runs, and a LAN session in other cases. When the nearby Steam search finds nothing, a second search looks in every region.

NsSession has no UI, no player name, and no map setting. The game gives the map and shows the status lines.

## 📦 Features
- **Room codes**: Host a room and share a 5-letter code. Join with the code.
- **Steam and LAN**: A Steam lobby when Steam runs, and a LAN session in other cases.
- **Worldwide search**: When the nearby Steam search finds nothing, a second search looks in every region.
- **Status events**: Each step of a host or a join sends a status with a default text.
- **Failure on return**: The title menu can show why the player lost the connection.
- **Test helpers**: Start with `-lanroom` to test without Steam, or join an IP address directly.

## ⚙️ Requirements
Unreal Engine 5.7+

For Steam: the engine plugin `OnlineSubsystemSteam`, set up in `DefaultEngine.ini`.

## 🛠️ Installation
1. **Clone** or download this repository.
2. Copy the `NsSession` folder into the `Plugins` folder of your project.
3. Add `"NsSession"` to the module dependencies in `Build.cs`.
4. Generate the project files and enable the plugin. It enables `OnlineSubsystem`, `OnlineSubsystemUtils`, and `SteamSockets`.
5. For Steam, set the connection class of the Steam net driver in `DefaultEngine.ini`:

```ini
[/Script/SteamSockets.SteamSocketsNetDriver]
NetConnectionClassName="/Script/NsSessionSteam.NsSteamNetConnection"
```

## 🚀 Getting Started
```cpp
#include "NsSessionSubsystem.h"

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

## 🔧 API
### Key Classes
- `UNsSessionSubsystem` – Game instance subsystem that hosts, joins, and leaves rooms.
- `FNsRoomStatus` – One status step, with `Status` and `GetDefaultText`.
- `UNsSteamNetConnection` – The Steam net connection class of the `NsSessionSteam` module.

### Main Functions
- `HostRoom` – Open a map as a room and get a code.
- `JoinRoom` – Join a room by its code.
- `LeaveRoom` – Leave the room. Call it when the player goes back to the title menu.
- `GetRoomCode` / `GetRoomNetwork` – Show the room code and the network in the HUD.
- `ConsumeLastFailure` – Get the failure of a lost connection, one time.
- `JoinDirectAddress` – Join an IP address, for tests.

### Events
- `OnRoomStatus` – Fires at each step of a host or a join.
- `OnRoomEntered` – Fires on the host and on the client when the room map loads.

### Settings
In `DefaultGame.ini`, under `[/Script/NsSession.NsSessionSubsystem]`:

| Setting | Default | Effect |
|---|---|---|
| `GameTag` | project name | Marks the rooms of this game |
| `MaxPlayers` | 6 | Players in one room, host included |

## 🚧 Limits
- One room for each game instance.
- No session browser. A player needs the code.
- The worldwide search needs Steam on Windows.

## 📚 Documentation
- [User Guide](https://mykaadev.github.io/plugins/nssession/user-guide/) – Setup, common tasks, settings, and debug tools.
- [Technical Documentation](https://mykaadev.github.io/plugins/nssession/tech-documentation/) – Architecture, lifetime, extension points, and tests.

<!-- GH_ONLY_START -->
## ❤️ Credits
<a href="https://github.com/mykaadev/NsSession/graphs/contributors"><img src="https://contrib.rocks/image?repo=mykaadev/NsSession"/></a>

## 📞 Support
Reach out via the **[profile page](https://github.com/mykaadev)**.

## 📃 License
[![License](https://img.shields.io/badge/license-MIT-green)](https://www.tldrlegal.com/license/mit-license)
<!-- GH_ONLY_END -->
