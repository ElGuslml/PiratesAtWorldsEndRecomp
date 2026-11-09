// This game's profile: everything the launcher knows about the game it belongs to. The launcher's code is the same
// for every game (the RecompLauncher submodule, recomp_launcher\, with game_types.h for the types below); game.h,
// game.cpp (the game's own texts) and palette.h (its colors) are each game's, with the art (launcher\art), the icon
// (res\icon.ico), the resource script (res\launcher.rc) and the version (res\version.h).

#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "game_types.h"
#include "version.h"

namespace launcher::game {

// --- Names ---

// The game, as the texts name it ("the .iso file of %s").
inline constexpr const char* kName = "Pirates of the Caribbean: At World's End";
// The recompiled executable and the launcher, side by side; the launcher's name is its window's title.
inline constexpr const char* kRecompExecutable = "PiratesAtWorldsEndRecomp.exe";
inline constexpr const char* kLauncherExecutable = "PiratesAtWorldsEnd Launcher.exe";
inline constexpr const char* kLauncherName = "PiratesAtWorldsEnd Launcher";
// The game's settings next to it: its base file (src/startup.cpp writes the defaults) and what the launcher saves.
inline constexpr const char* kConfigFile = "potcawe.toml";
inline constexpr const char* kOptionsFile = "potcawe_options.toml";
// The game's user data when its folder cannot be written: Documents\<this> (the SDK's default, the app's name).
inline constexpr const char* kDocumentsFolder = "potcawe";
// res\version.h.
inline constexpr const char* kVersion = POTCAWERECOMP_VERSION_STR;

// --- The look ---

// The title the Play page shows over the art: a small line above, the name below.
inline constexpr const char* kArtKicker = "PIRATES OF THE CARIBBEAN";
inline constexpr const char* kArtTitle = "At World's End";
// The mark in the header's logo while the game's icon is unknown: an icon's SVG paths in a 24-unit box, stroked as the
// launcher's icons are (ui.cpp).
inline constexpr const char* kBrandIcon =
    "<circle cx='12' cy='5' r='2'/><path d='M12 7v14'/><path d='M8 11h8'/><path d='M5 14a7 7 0 0 0 14 0'/>";
// The header's name beside the logo, on two lines (in capitals); an empty title: the art's (kArtTitle) alone.
inline constexpr const char* kBrandTitle = "";
inline constexpr const char* kBrandSubtitle = "";
// Where the art under the pages (launcher\art\background.svg, 1280 x 760: the ship at sunset of Gustavo's mockup) is
// cropped in a window of another shape: the point kept in view, as fractions of its width and height. In a wide window
// the ship and the sun on the horizon stay between the header and the footer.
inline constexpr float kArtFocusX = 0.5f;
inline constexpr float kArtFocusY = 0.55f;
// The logo's mark on a disc instead of a rounded square.
inline constexpr bool kBrandRound = false;
// The titles in capitals (the game's own texts that the launcher shows as titles).
inline constexpr bool kUppercaseTitles = false;

// --- The disc ---

// The disc's executable, at its root and in assets\ once extracted; extracted last, so that its presence marks a
// complete installation.
inline constexpr const char* kDiscExecutable = "default.xex";
// The disc this build was recompiled from (src/startup.cpp checks the same CRC at the game's start).
inline constexpr uint32_t kTitleId = 0x425607D1;
inline constexpr uint32_t kExecutableCrc32 = 0xA31B1069;
inline constexpr const char* kRegion = "USA";
// The game partition's files on that disc (25 files): the space the first run asks for before an ISO is chosen.
inline constexpr uint64_t kInstalledBytes = 6835718080;
// The game's frame: the Xenos path's scales and NativeXRenderer's internal resolutions are multiples of it.
inline constexpr int kFrameWidth = 1280;
inline constexpr int kFrameHeight = 720;

// --- Updates ---

// This project's releases (updater.cpp): GitHub's API for them, newest first, and their page. Empty: the launcher
// does not look for updates (a private repository), and its Updates page only shows the version.
inline constexpr const char* kReleasesApi =
    "https://api.github.com/repos/ElGuslml/PiratesAtWorldsEndRecomp/releases?per_page=20";
inline constexpr const char* kReleasesPage = "https://github.com/ElGuslml/PiratesAtWorldsEndRecomp/releases";

// --- The game's own options (a cvar each; empty: the game has no such option, and the launcher no row) ---

// NativeXRenderer draws this game (src/native_x_renderer_*.cpp): the Graphics page's renderer cards, its internal
// resolution, antialiasing and image format. False: the game is drawn by Xenos only, and the launcher says nothing
// of NativeXRenderer.
inline constexpr bool kHasNativeXRenderer = true;
// The renderer chosen until the player picks one (an options file without native_x_renderer): NativeXRenderer.
inline constexpr bool kNativeXRendererDefault = true;
// The upscaling row's AMD FSR 3.1 and NVIDIA DLSS (NativeXRenderer's D47-D49, D116: the main 3D view rebuilt from
// several frames; RecompLauncher 3.6, 3.9): only for a game whose hooks mark its views (native_x_renderer::BeginView
// and EndViewGeometry, src/views.cpp). Without them DLSS never runs; false: FSR 1 and
// NIS only.
inline constexpr bool kHasDlaa = true;

// The Graphics page's shadow quality row: NativeXRenderer draws the game's shadow maps at a scale of their own
// (native_x_renderer_shadow_scale, its D92): at least twice or four times the game's resolution. Only where it
// finds them: the depth passes with no color target that are not of the game's output size.
// Its character shadows: two cascades in a 1024x1024 depth map, drawn with no color target.
inline constexpr bool kShadowQuality = kHasNativeXRenderer && true;

// The Graphics page's frame rate row: kFrameRateCvar's choices (game_types.h's FrameRate) and the value without the
// key. A boolean cvar (kFrameRateIsBool: the values are 0 and 1, as
// src/framerate.cpp's 60 FPS) or a number (the emulated vblank's rate, video_mode_refresh_rate, which some games
// follow). A rate above 60 is offered only where the monitor reaches it, and marked experimental.
inline constexpr const char* kFrameRateCvar = "potcawe_60fps";
inline constexpr bool kFrameRateIsBool = true;
inline constexpr std::array<FrameRate, 2> kFrameRates{{{0, 30}, {1, 60}}};
inline constexpr int kFrameRateDefault = 0;

// The Xenos path's MSAA turned off (src/msaa.cpp): the Graphics page's antialiasing row with Xenos, the game's own
// antialiasing (its label) or none.
inline constexpr const char* kNoMsaaCvar = "potcawe_no_msaa";
inline constexpr const char* kMsaaLabel = "2× MSAA";

// The game's cameras widened for a screen wider than 16:9 (src/widescreen.cpp), set with the Video page's image
// format; the Xenos path then stretches its image over the screen (present_letterbox = false). Empty: the format
// applies to NativeXRenderer only (this game widens its cameras by itself when NativeXRenderer fills the screen).
inline constexpr const char* kUltrawideCvar = "";

// The languages the game has: the Game page's language row, in this order. Empty: the game asks in its own menu,
// and the launcher has no row. Each is an XLanguage id (user_language: 1 English, 2 Japanese, 3 German, 4 French,
// 5 Spanish, 6 Italian, 7 Korean, 8 Traditional Chinese, 9 Portuguese, 10 Simplified Chinese, 11 Polish,
// 12 Russian) with its name in its own words (UTF-8; the launcher draws other scripts with Windows' fonts). A game
// with two versions of a language lists it twice, the second with a boolean cvar set when it is chosen (Kameo's
// Latin American Spanish). Until one is chosen, the Windows display language's: the entry whose `locales` (a
// locale name prefix, "es-ES") matches the user's locale best, or the first with that id.
inline constexpr std::array<Language, 0> kLanguages{};

// Switches of the game's own (src/), each a boolean cvar with a row in the Settings tab (game_types.h's Toggle); their
// titles, hints and help are game.cpp's (GameTexts::toggles and toggle_help, in this order).
// The screen effects (src/screen_fx.cpp, docs/screen-effects.md), all on as on the console.
inline constexpr std::array<Toggle, 6> kToggles{{
    {"potcawe_fx_bloom", true, ToggleGroup::kScreenEffects},
    {"potcawe_fx_depth_of_field", true, ToggleGroup::kScreenEffects},
    {"potcawe_fx_color", true, ToggleGroup::kScreenEffects},
    {"potcawe_fx_heat_haze", true, ToggleGroup::kScreenEffects},
    {"potcawe_fx_radial_blur", true, ToggleGroup::kScreenEffects},
    {"potcawe_fx_motion_blur", true, ToggleGroup::kScreenEffects},
}};

// Whether some switch is in `group`.
constexpr bool HasToggles(ToggleGroup group) {
  for (const Toggle& toggle : kToggles) {
    if (toggle.group == group) return true;
  }
  return false;
}

// The Settings tab has a Game category (page_settings.cpp) when the game has a language row or switches of its own.
// Read by RecompLauncher before 3.6 only (from 3.6 the Game category is always there, with the launcher's language).
inline constexpr bool kHasGamePage = !kLanguages.empty() || HasToggles(ToggleGroup::kGame);

// --- Downloadable content ---

// The DLC page (page_dlc.cpp, dlc.h): Xbox 360 content packages, each installed by the game's --install_content tool
// mode (SDK change 44) into its user data's content folder. Off until checked with this game's content.
inline constexpr bool kHasDlcPage = false;

// NativeXRenderer's pipeline list (its D129, pipelines.h): "Compiling shaders" before Play. The game's shaders are read
// from its compressed level packages (src/level_packages.cpp, NativeXRenderer's D136; docs/compiling-shaders.md).
inline constexpr bool kHasPipelineList = true;

// --- The keyboard ---

// The Keyboard page's defaults where they are not the SDK's (game_types.h's KeyDefault); empty: the SDK's.
inline constexpr std::array<KeyDefault, 0> kKeyboardDefaults{};
// The Keyboard page's switch that makes some keys also tap a control (the SDK's mnk_tap_target); "": no switch.
inline constexpr const char* kKeyboardTapTarget = "";
// The whammy bar's row on each controller playing guitar (page_controls.cpp, RecompLauncher 3.4): how far the game
// reads the bar, and its calibration (the SDK's gamepad_whammy_calibration, recomp change 60). False: no such row (a
// game without a guitar).
inline constexpr bool kHasWhammy = false;

// --- Singing ---

// The Controls tab's microphone and each controller's part (page_controls.cpp, RecompLauncher 3.1): a recording device
// of the PC as the game's USB microphone (the SDK's usb_microphone_1, recomp change 55) and a controller playing the
// game's instrument or singing (the SDK's gamepad_device_subtypes, change 56). False: the game has no microphone.
inline constexpr bool kHasMicrophones = false;
// The microphone's gain in dB (usb_microphone_1_gain) until the player changes it, and the Microphone's Reset.
inline constexpr int kMicrophoneGainDefault = 0;

// --- The controls in the game's words ---

// The game's functions (game_types.h's GameControl) in groups, in the launcher's language (game.cpp); empty: the pages
// show the Xbox 360 controls (texts.cpp).
const std::vector<GameControlGroup>& GameControlsFor(uint32_t language);

// --- Texts of the game's own (game.cpp) ---

struct GameTexts {
  // The first run's welcome, in the game's spirit; null: the launcher's own (texts.cpp).
  const char* welcome_title;
  const char* welcome_text;
  // kToggles' rows: a title and a hint each.
  std::array<const char*, kToggles.size() * 2> toggles;
  // kToggles' help, the Settings tab's panel for the row in focus; null: the hint.
  std::array<const char*, kToggles.size()> toggle_help{};
  // The first run's last screen (the game installed), in the game's spirit; null: the launcher's own.
  const char* done_title = nullptr;
  const char* done_text = nullptr;
  // The DLC page: where the game shows what was installed; null: nothing said.
  const char* dlc_where = nullptr;
  // The first run's extraction screen (while it runs), in the game's spirit; null: the launcher's own.
  const char* extracting_title = nullptr;
  const char* extracting_text = nullptr;
  // The Controls tab's keyboard switch for kKeyboardTapTarget, and its tip in the help.
  const char* keyboard_tap_title = nullptr;
  const char* keyboard_tap_hint = nullptr;
  // kHasMicrophones: a controller's two parts, the game's instrument and the singer's; null: the launcher's words.
  const char* part_instrument = nullptr;
  const char* part_vocals = nullptr;
};
// In the language of the launcher's texts (an XLanguage); missing ones fall back to English.
const GameTexts& GameTextsFor(uint32_t language);

}  // namespace launcher::game
