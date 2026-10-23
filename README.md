<p align="center">
  <img src="res/icon.svg" width="200" alt="PiratesAtWorldsEndRecomp icon">
</p>

<h1 align="center">PiratesAtWorldsEndRecomp</h1>

<p align="center">
  <a href="https://github.com/ElGuslml/PiratesAtWorldsEndRecomp/releases/latest"><img alt="Latest release" src="https://img.shields.io/github/v/release/ElGuslml/PiratesAtWorldsEndRecomp?style=for-the-badge&label=Latest%20release&color=b22222"></a>
  <a href="https://discord.gg/XQtM5TmNs8"><img alt="Discord: join the server" src="https://img.shields.io/badge/Discord-Join%20the%20server-5865F2?style=for-the-badge&logo=discord&logoColor=white"></a>
  <a href="https://github.com/ElGuslml/ElGusRecomps"><img alt="More games: ElGus Recomps" src="https://img.shields.io/badge/More%20games-ElGus%20Recomps-2f6b45?style=for-the-badge&logo=github"></a>
</p>

🏴‍☠️ **Play Pirates of the Caribbean: At World's End on your PC, natively.** The Xbox 360 game by Eurocom (2007),
recompiled to run directly on Windows: no emulator to set up, sharper graphics, wide screens and 60 FPS.

> [!IMPORTANT]
> 💿 **You need your own original copy of the game.** This project does not include any of the game's files, and
> never will: not its executable, levels, textures, sounds, videos or text.
> - To play, you need an Xbox 360 copy of *Pirates of the Caribbean: At World's End* that you own (the USA disc).
> - You make an image (ISO) of your own disc, and the launcher installs the game from it.
> - Please do not ask for the game's files, and do not share them or links to them.
>
> The released program contains the game's code recompiled for the PC, but none of its files: without yours, it
> does not run. This project is not affiliated with or endorsed by Disney, Eurocom or Microsoft.

> [!WARNING]
> 🚧 **Early release.** The first levels have been played through, in sessions of an hour, without problems; later
> levels have not been tested yet. Please [tell us](#-reporting-a-problem) what you find!

## 🎮 How to play

1. **Download** the latest version from
   [Releases](https://github.com/ElGuslml/PiratesAtWorldsEndRecomp/releases/latest):
   - **the installer** (`...-setup.exe`): installs for your Windows user, no administrator rights needed, with a
     Start menu shortcut (and a desktop one if you want); or
   - **the zip** (`...-windows-x64.zip`): extract it into any folder you can write to (not `Program Files`).
2. **Open PiratesAtWorldsEnd Launcher.** The first time, it asks for the ISO of your game, checks it, and installs
   the game files. You only do this once.
3. **Pick your settings and press PLAY.** 🎉

From then on, the launcher keeps itself up to date: when a new version comes out, it offers to install it for you.

### 💻 What you need

- Windows 10 or 11 (64-bit).
- A graphics card with up-to-date drivers that support Vulkan (practically any card from the last ten years).
  DLAA needs an NVIDIA RTX card.
- The [Microsoft Visual C++ Redistributable](https://aka.ms/vs/17/release/vc_redist.x64.exe) (most PCs already
  have it).
- An Xbox controller, a PlayStation controller, or any other controller. Keyboard and mouse work too
  (experimental).

## ✨ Features

- 🖥️ **Sharp, native graphics:** the game is drawn by your graphics card at your screen's resolution, with smooth
  edges (SMAA, FXAA, or NVIDIA DLAA on RTX cards), sharper textures and higher-quality shadows.
- ⚡ **60 FPS** at the original game speed.
- 📺 **Wide and ultrawide screens:** the game fills 16:10, 21:9 and wider screens and shows more of the world to the
  sides, while the HUD, menus and videos keep their shape.
- 🚀 **No shader stutter:** from the second time you play, effects you have already seen appear without a hitch.
- 🏆 **Achievements:** all 30, with a notification when you unlock one (its icon, name and gamerscore), and the full
  list in the launcher or with F7 in the game.
- 🛠️ **More stable than the original:** a game script error no longer closes the game.
- 🕹️ **Your controls:** change the buttons of your controller, or play with the keyboard and mouse.
- 🔄 **Automatic updates** from this page.

## 🧭 PiratesAtWorldsEnd Launcher

Everything is set up from the launcher, with the mouse, the keyboard or a controller:

| Page | What you find there |
|---|---|
| ▶️ **Play** | Start the game, and see your settings, achievements and updates at a glance. |
| 🏆 **Achievements** | All 30 achievements, the ones you have unlocked and when, and your gamerscore. |
| 🎮 **Controls** | Keyboard and mouse (experimental), and your controller's buttons. |
| 🖥️ **Video** | Monitor, fullscreen or window, resolution, screen shape (fill a wide screen or keep 16:9). |
| 🎨 **Graphics** | Renderer, frame rate, antialiasing, texture sharpness, shadow quality. |
| 🔄 **Updates** | Your version, and new ones when they come out. |

Your choices are saved as soon as you make them. When you quit the game, the launcher comes back.

**Tips:**
- 🚪 On the title screen ("PRESS START"), press **B** to quit the game.
- 🌍 The game's language is chosen in the game's own menu.
- 🎮 **Steam Big Picture:** add `PiratesAtWorldsEnd Launcher.exe` as a non-Steam game and play from your couch.
  Steam sees the game running until you quit the launcher.
- 🛡️ **"Windows protected your PC":** the programs are not signed by a company, so Windows may warn you the first
  time. Click **More info**, then **Run anyway**.

## 🐞 Reporting a problem

Something looks wrong or crashes? Tell us on [Discord](https://discord.gg/XQtM5TmNs8) or open an
[issue](https://github.com/ElGuslml/PiratesAtWorldsEndRecomp/issues), with:
- where in the game it happened, and a screenshot if you can;
- the latest file in the `logs` folder next to the game.

If something looks wrong, try the other renderer: in the launcher, **Graphics → Xenos** (it emulates the Xbox 360's
graphics instead), and tell us whether it happens there too.

### Known issues

- ⌨️ Keyboard and mouse controls are experimental.
- 📺 With the Xenos renderer, wide screens get black bars at the sides.

## 🛠️ For developers

PiratesAtWorldsEndRecomp is a *static recompilation*: the game's original Xbox 360 code is translated to C++ and
compiled for Windows, on top of the [ReXGlue SDK](https://github.com/rexglue/rexglue-sdk). It is drawn by
[NativeXRenderer](https://github.com/ElGuslml/NativeXRenderer), a renderer that draws the game's graphics calls
natively on Vulkan.

- [Building and developing](docs/development.md): requirements, building from source, the renderers, configuration,
  the launcher, how the SDK is modified.
- [`docs/`](docs/README.md): a short note on each problem found and how it was solved.

## 🙏 Credits

- [ReXGlue SDK](https://github.com/rexglue/rexglue-sdk), whose runtime is derived from [Xenia](https://xenia.jp).
- [DiligentCore](https://github.com/DiligentGraphics/DiligentCore), the graphics layer NativeXRenderer draws with.
- NVIDIA DLSS, for DLAA.
- [extract-xiso](https://github.com/XboxDev/extract-xiso), for extracting game files.
- NativeXRenderer's design follows [Hell's Gate](https://github.com/florinp93/hells-gate-recomp) (Dante's Inferno
  on ReXGlue with a DiligentCore renderer) and [Unleashed Recompiled](https://github.com/hedge-dev/UnleashedRecomp);
  no code was taken from them.
- Developed by Gustavo Garcia with the help of Claude (Anthropic).

## 📜 License

MIT for this project's own code (see [`LICENSE`](LICENSE)). The ReXGlue SDK, NativeXRenderer and their components
keep their own licenses (see [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md)). NVIDIA and DLSS are trademarks or
registered trademarks of NVIDIA Corporation. Pirates of the Caribbean: At World's End, its code and its assets belong
to their respective owners.
