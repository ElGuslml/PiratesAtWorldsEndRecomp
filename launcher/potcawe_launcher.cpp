// POTCAWE Launcher: options for potcawe (recompiled Pirates of the Caribbean: At World's End) in a
// separate executable, so that potcawerecomp.exe starts right away (for example from Steam Big
// Picture).
//
// Plain Win32, no SDK. Reads potcawe.toml and potcawe_options.toml (next to this exe and
// potcawerecomp.exe), shows monitor, display, resolution, renderer, render quality, frame rate,
// antialiasing, anisotropic filtering and graphics API, and saves the choice to potcawe_options.toml,
// which potcawerecomp.exe loads at startup (src/options.cpp).
// Renderer: the SDK's Xenos emulation, or XenosNative (xenos_native, experimental), which renders at the
// resolution of the screen or the window with no scale to choose (xenos_native/docs/decisions.md, D37):
// the render quality row shows that resolution, disabled. XenosNative has its own antialiasing instead of
// the game's MSAA (SMAA, FXAA or off, D41; NVIDIA DLAA, D47, or the DLSS modes, which draw the 3D view at a
// lower resolution and upscale it, D49), fills screens of any aspect ratio (D43), and always uses Vulkan.
// Frame rate: 30 FPS (the original present interval) or 60 FPS (src/framerate.cpp). Anisotropic
// filtering: the SDK's anisotropic_override, 16x by default (src/startup.cpp); XenosNative applies it too
// (D44).
// "Play" also starts potcawerecomp.exe (with the arguments the launcher received) and waits for it
// to exit, so Steam keeps seeing the game as running. Works with mouse, keyboard or controller
// (XInput). The launcher texts follow the Windows display language (English, German, French,
// Spanish, Italian); the game asks for its own language in its first menu.
//
// Ported from GRAWRecomp's launcher (launcher/graw_launcher.cpp), without its language and aspect
// ratio options.

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <commctrl.h>
#include <shellscalingapi.h>
#include <xinput.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <map>
#include <string>
#include <utility>
#include <vector>

// Controls with the current Windows look (comctl32 v6) instead of the classic one.
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

namespace {

// The game always renders at 1280x720 (its Direct3D presentation parameters are hard-coded; the
// console video mode only drives the output scaler): resolution is raised with the SDK render scale
// (draw_resolution_scale_x/y, integers, per axis) and the presenter fits the image to the window or
// the screen.
constexpr int kGuestWidth = 1280;
constexpr int kGuestHeight = 720;
constexpr int kMaxScale = 3;

using Size = std::pair<int, int>;  // width, height

enum Display : int { kFullscreen, kBorderless, kWindow };

struct Options {
  int monitor = 1;  // cvar monitor: 1 = primary, in SDL order
  int display = kFullscreen;
  Size window{kGuestWidth, kGuestHeight};  // window size in physical pixels
  int scale = 0;                           // 0 = automatic; 1..kMaxScale = chosen
  bool fps60 = false;                      // potcawe_60fps (src/framerate.cpp)
  bool no_msaa = false;                    // potcawe_no_msaa (src/msaa.cpp)
  int anisotropic = 5;                     // anisotropic_override (SDK): 5 = 16x, 4 = 8x, 3 = 4x, 2 = 2x, 0 = off
  bool vulkan = false;                     // gpu_backend = "vulkan" (otherwise "d3d12")
  bool native = false;                     // xenos_native: XenosNative draws instead of Xenos
  int native_aa = 0;                       // xenos_native_antialiasing, an index of kNativeAntialiasing
};

// --- potcawe.toml / potcawe_options.toml ---

// Minimal TOML reader ("key = value"; [x] tables are flattened as x_key, like the SDK does).
using Config = std::map<std::string, std::string>;

std::string Trim(const std::string& s) {
  size_t begin = s.find_first_not_of(" \t\r\n");
  size_t end = s.find_last_not_of(" \t\r\n");
  return begin == std::string::npos ? std::string() : s.substr(begin, end - begin + 1);
}

void ReadToml(const std::filesystem::path& path, Config& config) {
  std::ifstream f(path);
  std::string line;
  std::string table;
  bool first_line = true;
  while (std::getline(f, line)) {
    if (first_line && line.starts_with("\xEF\xBB\xBF")) {  // UTF-8 BOM
      line.erase(0, 3);
    }
    first_line = false;
    bool quoted = false;
    for (size_t i = 0; i < line.size(); ++i) {
      if (line[i] == '"') {
        quoted = !quoted;
      } else if (line[i] == '#' && !quoted) {
        line.resize(i);
        break;
      }
    }
    line = Trim(line);
    if (line.empty()) {
      continue;
    }
    if (line.front() == '[' && line.back() == ']') {
      table = Trim(line.substr(1, line.size() - 2));
      continue;
    }
    size_t eq = line.find('=');
    if (eq == std::string::npos) {
      continue;
    }
    std::string key = Trim(line.substr(0, eq));
    std::string value = Trim(line.substr(eq + 1));
    if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
      value = value.substr(1, value.size() - 2);
    }
    config[table.empty() ? key : table + "_" + key] = value;
  }
}

std::string GetString(const Config& c, const std::string& key) {
  auto it = c.find(key);
  return it != c.end() ? it->second : std::string();
}

int GetInt(const Config& c, const std::string& key, int fallback) {
  std::string s = GetString(c, key);
  char* end = nullptr;
  long value = std::strtol(s.c_str(), &end, 10);
  return end != s.c_str() ? int(value) : fallback;
}

bool GetBool(const Config& c, const std::string& key, bool fallback) {
  std::string s = GetString(c, key);
  return s == "true" ? true : s == "false" ? false : fallback;
}

// --- Texts ---

// Launcher texts in the Windows display language (XLanguage ids, like the SDK's user_language:
// 1 English, 3 German, 4 French, 5 Spanish, 6 Italian).
uint32_t SystemLanguage() {
  switch (PRIMARYLANGID(GetUserDefaultUILanguage())) {
    case LANG_GERMAN:
      return 3;
    case LANG_FRENCH:
      return 4;
    case LANG_SPANISH:
      return 5;
    case LANG_ITALIAN:
      return 6;
    default:
      return 1;
  }
}

struct Texts {
  const wchar_t* subtitle;
  const wchar_t* monitor;
  const wchar_t* primary;
  const wchar_t* display;
  const wchar_t* fullscreen;
  const wchar_t* borderless;
  const wchar_t* window;
  const wchar_t* resolution;
  const wchar_t* monitor_resolution;
  const wchar_t* quality;
  const wchar_t* automatic;
  const wchar_t* original;
  const wchar_t* framerate;
  const wchar_t* fps30;
  const wchar_t* fps60;
  const wchar_t* antialiasing;
  const wchar_t* aa_msaa;
  const wchar_t* aa_off;
  const wchar_t* anisotropic;
  const wchar_t* aniso_off;
  const wchar_t* graphics;
  const wchar_t* renderer;
  const wchar_t* renderer_xenos;
  const wchar_t* renderer_native;
  const wchar_t* quality_hint;
  const wchar_t* vulkan_hint;
  const wchar_t* native_hint;
  const wchar_t* from_screen;  // XenosNative's render resolution: computed, not chosen
  const wchar_t* pad_hint;
  const wchar_t* play;
  const wchar_t* save;
  const wchar_t* quit;
  const wchar_t* game_missing;
  const wchar_t* save_failed;
};

const Texts& TextsFor(uint32_t language) {
  static const Texts kEnglish = {
      L"Options are saved; potcawerecomp.exe starts straight away with them.",
      L"Monitor",
      L"primary",
      L"Display",
      L"Fullscreen",
      L"Borderless window",
      L"Window",
      L"Resolution",
      L"(monitor resolution)",
      L"Render quality",
      L"Automatic",
      L"(original)",
      L"Frame rate",
      L"30 FPS (original)",
      L"60 FPS",
      L"Antialiasing",
      L"2× MSAA (original)",
      L"Off (faster)",
      L"Anisotropic filtering",
      L"Off",
      L"Graphics API",
      L"Renderer",
      L"Xenos (emulated)",
      L"XenosNative (experimental)",
      L"The game renders at 1280 × 720 times a whole number (2×, 3×) and the image is fitted "
      L"to the screen. Automatic picks the smallest scale that covers the resolution. 60 FPS "
      L"presents every vblank instead of every second one; the game speed does not change.",
      L"Vulkan has been tested less than DirectX 12 in this game: use it if DirectX 12 gives you "
      L"problems.",
      L"XenosNative draws the game natively with Vulkan instead of emulating the Xbox 360 GPU, at the "
      L"resolution of the screen or the window, filling it without bars (ultrawide, 16:10), with its own "
      L"antialiasing. It is experimental: some effects may look different than with Xenos.",
      L"(from the screen)",
      L"Controller: ↑↓ choose · ←→ change · A play · B quit",
      L"Play",
      L"Save",
      L"Quit",
      L"potcawerecomp.exe was not found next to POTCAWE Launcher.",
      L"Could not save the options to potcawe_options.toml.",
  };
  static const Texts kGerman = {
      L"Die Optionen werden gespeichert; potcawerecomp.exe startet direkt mit ihnen.",
      L"Monitor",
      L"Hauptmonitor",
      L"Anzeige",
      L"Vollbild",
      L"Rahmenloses Fenster",
      L"Fenster",
      L"Auflösung",
      L"(Monitorauflösung)",
      L"Renderqualität",
      L"Automatisch",
      L"(Original)",
      L"Bildrate",
      L"30 FPS (Original)",
      L"60 FPS",
      L"Kantenglättung",
      L"2× MSAA (Original)",
      L"Aus (schneller)",
      L"Anisotrope Filterung",
      L"Aus",
      L"Grafik-API",
      L"Renderer",
      L"Xenos (emuliert)",
      L"XenosNative (experimentell)",
      L"Das Spiel rendert in 1280 × 720 mal einer ganzen Zahl (2×, 3×), das Bild wird an den "
      L"Bildschirm angepasst. „Automatisch“ wählt die kleinste Stufe, die die Auflösung abdeckt. "
      L"60 FPS zeigt jedes Vblank ein Bild statt jedes zweite; die Spielgeschwindigkeit bleibt gleich.",
      L"Vulkan ist in diesem Spiel weniger getestet als DirectX 12: nutze es, wenn DirectX 12 "
      L"Probleme macht.",
      L"XenosNative zeichnet das Spiel nativ mit Vulkan, statt die GPU der Xbox 360 zu emulieren, in "
      L"der Auflösung des Bildschirms oder Fensters, ohne schwarze Balken (Ultrawide, 16:10), mit eigener "
      L"Kantenglättung. Es ist experimentell: manche Effekte können anders aussehen als mit Xenos.",
      L"(nach Bildschirm)",
      L"Controller: ↑↓ wählen · ←→ ändern · A spielen · B beenden",
      L"Spielen",
      L"Speichern",
      L"Beenden",
      L"potcawerecomp.exe wurde neben POTCAWE Launcher nicht gefunden.",
      L"Die Optionen konnten nicht in potcawe_options.toml gespeichert werden.",
  };
  static const Texts kFrench = {
      L"Les options sont enregistrées ; potcawerecomp.exe démarre directement avec elles.",
      L"Moniteur",
      L"principal",
      L"Affichage",
      L"Plein écran",
      L"Fenêtre sans bordure",
      L"Fenêtre",
      L"Résolution",
      L"(résolution du moniteur)",
      L"Qualité de rendu",
      L"Automatique",
      L"(d'origine)",
      L"Fréquence d'images",
      L"30 FPS (d'origine)",
      L"60 FPS",
      L"Anticrénelage",
      L"2× MSAA (d'origine)",
      L"Désactivé (plus rapide)",
      L"Filtrage anisotrope",
      L"Désactivé",
      L"API graphique",
      L"Moteur de rendu",
      L"Xenos (émulé)",
      L"XenosNative (expérimental)",
      L"Le jeu est rendu en 1280 × 720 multiplié par un nombre entier (2×, 3×) et l'image "
      L"s'adapte à l'écran. « Automatique » choisit la plus petite échelle qui couvre la "
      L"résolution. 60 FPS affiche une image à chaque vblank au lieu d'une sur deux ; la vitesse du "
      L"jeu ne change pas.",
      L"Vulkan a été moins testé que DirectX 12 dans ce jeu : utilisez-le si DirectX 12 pose "
      L"problème.",
      L"XenosNative dessine le jeu nativement avec Vulkan au lieu d'émuler le GPU de la Xbox 360, à la "
      L"résolution de l'écran ou de la fenêtre, sans bandes noires (ultra-large, 16:10), avec son propre "
      L"anticrénelage. Il est expérimental : certains effets peuvent différer de Xenos.",
      L"(selon l'écran)",
      L"Manette : ↑↓ choisir · ←→ modifier · A jouer · B quitter",
      L"Jouer",
      L"Enregistrer",
      L"Quitter",
      L"potcawerecomp.exe est introuvable à côté de POTCAWE Launcher.",
      L"Impossible d'enregistrer les options dans potcawe_options.toml.",
  };
  static const Texts kSpanish = {
      L"Las opciones se guardan y potcawerecomp.exe arranca directo con ellas.",
      L"Monitor",
      L"principal",
      L"Pantalla",
      L"Pantalla completa",
      L"Ventana sin bordes",
      L"Ventana",
      L"Resolución",
      L"(resolución del monitor)",
      L"Calidad de render",
      L"Automática",
      L"(original)",
      L"Frecuencia de imagen",
      L"30 FPS (original)",
      L"60 FPS",
      L"Antialiasing",
      L"2× MSAA (original)",
      L"Desactivado (más rápido)",
      L"Filtrado anisotrópico",
      L"Desactivado",
      L"API gráfica",
      L"Renderizador",
      L"Xenos (emulado)",
      L"XenosNative (experimental)",
      L"El juego dibuja a 1280 × 720 por un número entero (2×, 3×) y la imagen se ajusta a la "
      L"pantalla. «Automática» elige la menor escala que cubre la resolución. 60 FPS presenta una "
      L"imagen por vblank en vez de una de cada dos; la velocidad del juego no cambia.",
      L"Vulkan está menos probado que DirectX 12 en este juego: úsalo si DirectX 12 te da "
      L"problemas.",
      L"XenosNative dibuja el juego de forma nativa con Vulkan en vez de emular la GPU de Xbox 360, a "
      L"la resolución de la pantalla o la ventana, llenándola sin bandas (ultrawide, 16:10), con su propio "
      L"antialiasing. Es experimental: algunos efectos pueden verse distintos que con Xenos.",
      L"(según la pantalla)",
      L"Mando: ↑↓ elegir · ←→ cambiar · A jugar · B salir",
      L"Jugar",
      L"Guardar",
      L"Salir",
      L"No se encontró potcawerecomp.exe junto a POTCAWE Launcher.",
      L"No se pudieron guardar las opciones en potcawe_options.toml.",
  };
  static const Texts kItalian = {
      L"Le opzioni vengono salvate; potcawerecomp.exe si avvia direttamente con esse.",
      L"Monitor",
      L"principale",
      L"Schermo",
      L"Schermo intero",
      L"Finestra senza bordi",
      L"Finestra",
      L"Risoluzione",
      L"(risoluzione del monitor)",
      L"Qualità di rendering",
      L"Automatica",
      L"(originale)",
      L"Frequenza fotogrammi",
      L"30 FPS (originale)",
      L"60 FPS",
      L"Antialiasing",
      L"2× MSAA (originale)",
      L"Disattivato (più veloce)",
      L"Filtro anisotropico",
      L"Disattivato",
      L"API grafica",
      L"Renderer",
      L"Xenos (emulato)",
      L"XenosNative (sperimentale)",
      L"Il gioco disegna a 1280 × 720 per un numero intero (2×, 3×) e l'immagine si adatta "
      L"allo schermo. «Automatica» sceglie la scala più piccola che copre la risoluzione. 60 FPS "
      L"presenta un'immagine a ogni vblank invece di una su due; la velocità del gioco non cambia.",
      L"Vulkan è meno collaudato di DirectX 12 in questo gioco: usalo se DirectX 12 dà "
      L"problemi.",
      L"XenosNative disegna il gioco in modo nativo con Vulkan invece di emulare la GPU della Xbox "
      L"360, alla risoluzione dello schermo o della finestra, riempiendolo senza bande (ultrawide, 16:10), "
      L"con un proprio antialiasing. È sperimentale: alcuni effetti possono apparire diversi rispetto a "
      L"Xenos.",
      L"(in base allo schermo)",
      L"Controller: ↑↓ scegli · ←→ cambia · A gioca · B esci",
      L"Gioca",
      L"Salva",
      L"Esci",
      L"potcawerecomp.exe non trovato accanto a POTCAWE Launcher.",
      L"Impossibile salvare le opzioni in potcawe_options.toml.",
  };
  switch (language) {
    case 3:
      return kGerman;
    case 4:
      return kFrench;
    case 5:
      return kSpanish;
    case 6:
      return kItalian;
    default:
      return kEnglish;
  }
}

// --- Monitors ---

struct Monitor {
  std::wstring name;  // monitor name (EDID); empty if Windows does not provide it
  bool primary = false;
  Size size{0, 0};  // current mode, in physical pixels
  RECT work{};      // work area (without the taskbar)
  UINT dpi = 96;
  std::vector<Size> modes;  // 32-bit modes, unique, largest first
};

// Name of each monitor by GDI device (\\.\DISPLAYn), obtained the same way as SDL.
std::map<std::wstring, std::wstring> MonitorNames() {
  std::map<std::wstring, std::wstring> names;
  UINT32 path_count = 0;
  UINT32 mode_count = 0;
  if (GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS, &path_count, &mode_count) !=
      ERROR_SUCCESS) {
    return names;
  }
  std::vector<DISPLAYCONFIG_PATH_INFO> paths(path_count);
  std::vector<DISPLAYCONFIG_MODE_INFO> modes(mode_count);
  if (QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS, &path_count, paths.data(), &mode_count,
                         modes.data(), nullptr) != ERROR_SUCCESS) {
    return names;
  }
  for (UINT32 i = 0; i < path_count; ++i) {
    DISPLAYCONFIG_SOURCE_DEVICE_NAME source = {};
    source.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
    source.header.size = sizeof(source);
    source.header.adapterId = paths[i].sourceInfo.adapterId;
    source.header.id = paths[i].sourceInfo.id;
    DISPLAYCONFIG_TARGET_DEVICE_NAME target = {};
    target.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME;
    target.header.size = sizeof(target);
    target.header.adapterId = paths[i].targetInfo.adapterId;
    target.header.id = paths[i].targetInfo.id;
    if (DisplayConfigGetDeviceInfo(&source.header) == ERROR_SUCCESS &&
        DisplayConfigGetDeviceInfo(&target.header) == ERROR_SUCCESS &&
        target.monitorFriendlyDeviceName[0]) {
      names.emplace(source.viewGdiDeviceName, target.monitorFriendlyDeviceName);
    }
  }
  return names;
}

// Monitors in SDL order (the one the monitor cvar uses): the primary first, then the rest in
// EnumDisplayMonitors order (SDL_windowsmodes.c, WIN_AddDisplays).
std::vector<Monitor> EnumerateMonitors() {
  std::vector<HMONITOR> handles;
  EnumDisplayMonitors(
      nullptr, nullptr,
      [](HMONITOR handle, HDC, LPRECT, LPARAM param) -> BOOL {
        reinterpret_cast<std::vector<HMONITOR>*>(param)->push_back(handle);
        return TRUE;
      },
      LPARAM(&handles));
  std::map<std::wstring, std::wstring> names = MonitorNames();

  std::vector<Monitor> monitors;
  for (HMONITOR handle : handles) {
    MONITORINFOEXW info = {};
    info.cbSize = sizeof(info);
    if (!GetMonitorInfoW(handle, &info)) {
      continue;
    }
    Monitor m;
    m.primary = (info.dwFlags & MONITORINFOF_PRIMARY) != 0;
    m.work = info.rcWork;
    m.size = {info.rcMonitor.right - info.rcMonitor.left,
              info.rcMonitor.bottom - info.rcMonitor.top};
    DEVMODEW current = {};
    current.dmSize = sizeof(current);
    if (EnumDisplaySettingsW(info.szDevice, ENUM_CURRENT_SETTINGS, &current)) {
      m.size = {int(current.dmPelsWidth), int(current.dmPelsHeight)};
    }
    for (DWORD i = 0;; ++i) {
      DEVMODEW mode = {};
      mode.dmSize = sizeof(mode);
      if (!EnumDisplaySettingsW(info.szDevice, i, &mode)) {
        break;
      }
      Size size{int(mode.dmPelsWidth), int(mode.dmPelsHeight)};
      if (mode.dmBitsPerPel == 32 &&
          std::find(m.modes.begin(), m.modes.end(), size) == m.modes.end()) {
        m.modes.push_back(size);
      }
    }
    std::sort(m.modes.begin(), m.modes.end(), std::greater<>());
    UINT dpi_x = 96;
    UINT dpi_y = 96;
    if (SUCCEEDED(GetDpiForMonitor(handle, MDT_EFFECTIVE_DPI, &dpi_x, &dpi_y))) {
      m.dpi = dpi_x;
    }
    if (auto it = names.find(info.szDevice); it != names.end()) {
      m.name = it->second;
    }
    monitors.push_back(std::move(m));
  }
  std::stable_partition(monitors.begin(), monitors.end(),
                        [](const Monitor& m) { return m.primary; });
  if (monitors.empty()) {
    Monitor m;
    m.primary = true;
    m.size = {GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)};
    m.work = {0, 0, m.size.first, m.size.second};
    monitors.push_back(m);
  }
  return monitors;
}

// Window sizes that fit (frame included) in the monitor work area: its modes from 1280x720 up,
// 1280x720 always, plus keep (the saved size) if it is not in the list.
std::vector<Size> WindowSizes(const Monitor& m, Size keep) {
  RECT frame = {0, 0, 0, 0};
  AdjustWindowRectExForDpi(&frame, WS_OVERLAPPEDWINDOW, FALSE, 0, m.dpi);
  int max_width = (m.work.right - m.work.left) - (frame.right - frame.left);
  int max_height = (m.work.bottom - m.work.top) - (frame.bottom - frame.top);
  std::vector<Size> sizes;
  for (Size size : m.modes) {
    if (size.first >= kGuestWidth && size.second >= kGuestHeight && size.first <= max_width &&
        size.second <= max_height) {
      sizes.push_back(size);
    }
  }
  for (Size extra : {Size{kGuestWidth, kGuestHeight}, keep}) {
    if (extra.first > 0 && extra.second > 0 &&
        std::find(sizes.begin(), sizes.end(), extra) == sizes.end()) {
      sizes.push_back(extra);
    }
  }
  std::sort(sizes.begin(), sizes.end(), std::greater<>());
  return sizes;
}

// 16:9 aspect ratio within 1 % (1366x768 and 1360x768 count).
bool Is16x9(Size size) {
  return std::abs(int64_t(size.first) * 9 - int64_t(size.second) * 16) * 100 <=
         int64_t(size.second) * 16;
}

// Render scale per axis: the smallest one that covers (with 10 % tolerance) the 16:9 image inside
// the output (the presenter letterboxes other aspect ratios).
Size AutoScale(Size output) {
  double width = output.first;
  double height = output.second;
  if (width * 9 > height * 16) {
    width = height * 16 / 9;
  } else {
    height = width * 9 / 16;
  }
  auto axis = [](double size, int guest) {
    return std::clamp(int(std::ceil(size / guest - 0.1)), 1, kMaxScale);
  };
  return {axis(width, kGuestWidth), axis(height, kGuestHeight)};
}

// XenosNative's render size (D37): the 16:9 image that fits the output, at least the game's own
// 1280x720, at a scale that need not be a whole number; an output that is not 16:9 is filled (D43): the
// height sets the scale, the width follows from the output's aspect ratio (xenos_native_fill_screen, on
// by default). The renderer computes it the same way from its swap chain (xenos_native/src/renderer.cpp).
Size NativeRenderSize(Size output) {
  double factor = output.second ? (double(output.first) / output.second) / (double(kGuestWidth) / kGuestHeight) : 1.0;
  if (std::abs(factor - 1.0) > 0.01) {
    double scale_y = std::max(double(output.second) / kGuestHeight, 1.0 / std::min(factor, 1.0));
    return {int(std::floor(kGuestWidth * scale_y * factor + 0.5)), int(std::floor(kGuestHeight * scale_y + 0.5))};
  }
  double scale = std::max(1.0, std::min(double(output.first) / kGuestWidth, double(output.second) / kGuestHeight));
  return {int(std::floor(kGuestWidth * scale + 0.5)), int(std::floor(kGuestHeight * scale + 0.5))};
}

// --- Window ---

enum ControlId : int {
  kIdTitle = 100,
  kIdSubtitle,
  kIdMonitorLabel,
  kIdMonitor,
  kIdDisplayLabel,
  kIdDisplay,
  kIdResolutionLabel,
  kIdResolution,
  kIdRendererLabel,
  kIdRenderer,
  kIdQualityLabel,
  kIdQuality,
  kIdFramerateLabel,
  kIdFramerate,
  kIdAntialiasingLabel,
  kIdAntialiasing,
  kIdAnisotropicLabel,
  kIdAnisotropic,
  kIdBackendLabel,
  kIdBackend,
  kIdHint,
  kIdPadHint,
  kIdSave,
};

// Option rows (top to bottom) and buttons (left to right), for the controller.
constexpr int kRowIds[] = {kIdMonitor,   kIdDisplay,      kIdResolution,  kIdRenderer, kIdQuality,
                           kIdFramerate, kIdAntialiasing, kIdAnisotropic, kIdBackend};
constexpr int kButtonIds[] = {IDOK, kIdSave, IDCANCEL};
constexpr int kRows[][2] = {{kIdMonitorLabel, kIdMonitor},
                            {kIdDisplayLabel, kIdDisplay},
                            {kIdResolutionLabel, kIdResolution},
                            {kIdRendererLabel, kIdRenderer},
                            {kIdQualityLabel, kIdQuality},
                            {kIdFramerateLabel, kIdFramerate},
                            {kIdAntialiasingLabel, kIdAntialiasing},
                            {kIdAnisotropicLabel, kIdAnisotropic},
                            {kIdBackendLabel, kIdBackend}};

// XenosNative's antialiasing row (xenos_native_antialiasing), top to bottom: SMAA or FXAA on the finished scene
// (D41), DLAA with NVIDIA DLSS before the post-processing (D47), and the DLSS modes, the 3D view drawn at a
// lower resolution and upscaled (D49); SMAA where DLSS is not available.
constexpr const char* kNativeAntialiasing[] = {"smaa", "dlaa", "dlss_quality", "dlss_balanced", "dlss_performance",
                                               "dlss_ultra_performance", "fxaa", "off"};

// The anisotropic filtering row's values (anisotropic_override), top to bottom.
constexpr int kAnisotropicValues[] = {5, 4, 3, 2, 0};

enum class Result { kQuit, kSave, kPlay };

enum PadDirection : int { kPadNone, kPadUp, kPadDown, kPadLeft, kPadRight };

using XInputGetStateFn = DWORD(WINAPI*)(DWORD, XINPUT_STATE*);

struct Launcher {
  Options o;
  uint32_t ui_language = 1;  // launcher texts (SystemLanguage)
  std::vector<Monitor> monitors;
  std::vector<Size> window_sizes;  // of the chosen monitor (physical pixels)
  UINT primary_dpi = 96;           // the SDK converts window_width/height with the primary's DPI
  bool has_vulkan = false;         // vulkan-1.dll present (ships with the video driver)
  HWND hwnd = nullptr;
  HFONT font = nullptr;
  HFONT title_font = nullptr;
  Result result = Result::kQuit;

  // Controller (XInput). Polling an empty slot is slow: empty slots are checked every ~1 s.
  XInputGetStateFn xinput_get_state = nullptr;
  DWORD pad_connected = 0;  // mask of slots with a controller
  int pad_scan_ticks = 0;
  bool pad_primed = false;
  WORD pad_buttons = 0;
  int pad_direction = kPadNone;
  ULONGLONG pad_repeat_at = 0;
};

const Monitor& CurrentMonitor(const Launcher& l) {
  return l.monitors[std::clamp(l.o.monitor, 1, int(l.monitors.size())) - 1];
}

Size OutputSize(const Launcher& l) {
  return l.o.display == kWindow ? l.o.window : CurrentMonitor(l).size;
}

Size EffectiveScale(const Launcher& l) {
  return l.o.scale == 0 ? AutoScale(OutputSize(l)) : Size{l.o.scale, l.o.scale};
}

HWND Item(const Launcher& l, int id) {
  return GetDlgItem(l.hwnd, id);
}

std::wstring SizeText(Size size) {
  return std::to_wstring(size.first) + L" × " + std::to_wstring(size.second);
}

std::wstring RenderText(Size scale, const Texts& t) {
  std::wstring text = SizeText({kGuestWidth * scale.first, kGuestHeight * scale.second}) + L"  ";
  if (scale.first == 1 && scale.second == 1) {
    return text + t.original;
  }
  if (scale.first == scale.second) {
    return text + L"(" + std::to_wstring(scale.first) + L"×)";
  }
  return text + L"(" + std::to_wstring(scale.first) + L"×" + std::to_wstring(scale.second) + L")";
}

void FillCombo(HWND combo, const std::vector<std::wstring>& items, int selected) {
  SendMessageW(combo, CB_RESETCONTENT, 0, 0);
  for (const auto& item : items) {
    SendMessageW(combo, CB_ADDSTRING, 0, LPARAM(item.c_str()));
  }
  if (!items.empty()) {
    SendMessageW(combo, CB_SETCURSEL, WPARAM(std::clamp(selected, 0, int(items.size()) - 1)), 0);
  }
}

// Shows l.o in the window (texts, lists and selections).
void Refresh(Launcher& l) {
  const Options& o = l.o;
  const Texts& t = TextsFor(l.ui_language);
  SetWindowTextW(Item(l, kIdSubtitle), t.subtitle);
  SetWindowTextW(Item(l, kIdMonitorLabel), t.monitor);
  SetWindowTextW(Item(l, kIdDisplayLabel), t.display);
  SetWindowTextW(Item(l, kIdResolutionLabel), t.resolution);
  SetWindowTextW(Item(l, kIdQualityLabel), t.quality);
  SetWindowTextW(Item(l, kIdFramerateLabel), t.framerate);
  SetWindowTextW(Item(l, kIdAntialiasingLabel), t.antialiasing);
  SetWindowTextW(Item(l, kIdAnisotropicLabel), t.anisotropic);
  SetWindowTextW(Item(l, kIdBackendLabel), t.graphics);
  SetWindowTextW(Item(l, kIdRendererLabel), t.renderer);
  SetWindowTextW(Item(l, kIdHint), o.native ? t.native_hint : o.vulkan ? t.vulkan_hint : t.quality_hint);
  SetWindowTextW(Item(l, kIdPadHint), t.pad_hint);
  SetWindowTextW(Item(l, IDOK), t.play);
  SetWindowTextW(Item(l, kIdSave), t.save);
  SetWindowTextW(Item(l, IDCANCEL), t.quit);

  std::vector<std::wstring> monitors;
  for (size_t i = 0; i < l.monitors.size(); ++i) {
    const Monitor& m = l.monitors[i];
    std::wstring text = std::to_wstring(i + 1) + L": " +
                        (m.name.empty() ? std::wstring(t.monitor) : m.name) + L" — " +
                        SizeText(m.size);
    if (m.primary) {
      text += L" (" + std::wstring(t.primary) + L")";
    }
    monitors.push_back(std::move(text));
  }
  FillCombo(Item(l, kIdMonitor), monitors, o.monitor - 1);

  FillCombo(Item(l, kIdDisplay), {t.fullscreen, t.borderless, t.window}, o.display);

  // Resolution: the window size, or the monitor's in fullscreen and borderless.
  HWND resolution = Item(l, kIdResolution);
  if (o.display == kWindow) {
    std::vector<std::wstring> sizes;
    int size_index = 0;
    for (size_t i = 0; i < l.window_sizes.size(); ++i) {
      sizes.push_back(SizeText(l.window_sizes[i]));
      if (l.window_sizes[i] == o.window) {
        size_index = int(i);
      }
    }
    FillCombo(resolution, sizes, size_index);
  } else {
    FillCombo(resolution, {SizeText(CurrentMonitor(l).size) + L"  " + t.monitor_resolution}, 0);
    if (GetFocus() == resolution) {
      SetFocus(Item(l, kIdDisplay));
    }
  }
  EnableWindow(resolution, o.display == kWindow);

  // XenosNative needs Vulkan (it draws through DiligentCore's Vulkan backend).
  std::vector<std::wstring> renderers = {t.renderer_xenos};
  if (l.has_vulkan) {
    renderers.emplace_back(t.renderer_native);
  }
  FillCombo(Item(l, kIdRenderer), renderers, o.native ? 1 : 0);

  // With XenosNative the render quality and graphics API rows show what it uses (the resolution computed
  // from the screen, Vulkan), disabled, and the antialiasing row offers its own methods (proper names, the
  // same in every language); the choices for Xenos are kept.
  if (o.native) {
    Size render = NativeRenderSize(OutputSize(l));
    FillCombo(Item(l, kIdQuality),
              {SizeText(render) + L"  " + (render == Size{kGuestWidth, kGuestHeight} ? t.original : t.from_screen)},
              0);
    FillCombo(Item(l, kIdAntialiasing),
              {L"SMAA", L"DLAA (NVIDIA RTX)", L"DLSS Quality", L"DLSS Balanced", L"DLSS Performance",
               L"DLSS Ultra Performance", L"FXAA", t.aa_off},
              o.native_aa);
    FillCombo(Item(l, kIdBackend), {L"Vulkan"}, 0);
  } else {
    std::vector<std::wstring> qualities = {std::wstring(t.automatic) + L" — " +
                                           RenderText(AutoScale(OutputSize(l)), t)};
    for (int scale = 1; scale <= kMaxScale; ++scale) {
      qualities.push_back(RenderText({scale, scale}, t));
    }
    FillCombo(Item(l, kIdQuality), qualities, o.scale);
    FillCombo(Item(l, kIdAntialiasing), {t.aa_msaa, t.aa_off}, o.no_msaa ? 1 : 0);
    // Proper names: the same in every language.
    std::vector<std::wstring> backends = {L"DirectX 12"};
    if (l.has_vulkan) {
      backends.emplace_back(L"Vulkan");
    }
    FillCombo(Item(l, kIdBackend), backends, o.vulkan ? 1 : 0);
  }
  for (int id : {kIdQuality, kIdBackend}) {
    if (o.native && GetFocus() == Item(l, id)) {
      SetFocus(Item(l, kIdRenderer));
    }
    EnableWindow(Item(l, id), !o.native);
  }

  FillCombo(Item(l, kIdFramerate), {t.fps30, t.fps60}, o.fps60 ? 1 : 0);
  FillCombo(Item(l, kIdAnisotropic),
            {L"16×", L"8×", std::wstring(L"4×  ") + t.original, L"2×", t.aniso_off},
            int(std::find(std::begin(kAnisotropicValues), std::end(kAnisotropicValues), o.anisotropic) -
                std::begin(kAnisotropicValues)));
}

// Copies the selection of list id into l.o and refreshes the window.
void OnSelectionChange(Launcher& l, int id) {
  Options& o = l.o;
  int index = int(SendMessageW(Item(l, id), CB_GETCURSEL, 0, 0));
  if (index < 0) {
    return;
  }
  switch (id) {
    case kIdMonitor:
      o.monitor = index + 1;
      l.window_sizes = WindowSizes(CurrentMonitor(l), {0, 0});
      if (std::find(l.window_sizes.begin(), l.window_sizes.end(), o.window) ==
          l.window_sizes.end()) {
        o.window = {kGuestWidth, kGuestHeight};
      }
      break;
    case kIdDisplay:
      o.display = std::clamp(index, int(kFullscreen), int(kWindow));
      break;
    case kIdResolution:
      if (o.display == kWindow && index < int(l.window_sizes.size())) {
        o.window = l.window_sizes[index];
      }
      break;
    case kIdRenderer:
      o.native = l.has_vulkan && index == 1;
      break;
    case kIdQuality:
      if (!o.native) {
        o.scale = std::clamp(index, 0, kMaxScale);
      }
      break;
    case kIdFramerate:
      o.fps60 = index == 1;
      break;
    case kIdAntialiasing:
      if (o.native) {
        o.native_aa = std::clamp(index, 0, int(std::size(kNativeAntialiasing)) - 1);
      } else {
        o.no_msaa = index == 1;
      }
      break;
    case kIdAnisotropic:
      o.anisotropic = kAnisotropicValues[std::clamp(index, 0, int(std::size(kAnisotropicValues)) - 1)];
      break;
    case kIdBackend:
      if (!o.native) {
        o.vulkan = l.has_vulkan && index == 1;
      }
      break;
  }
  Refresh(l);
}

// Positions in pixels at 96 DPI; scaled with the monitor DPI.
constexpr int kClientWidth = 560;
constexpr int kClientHeight = 546;  // nine rows

void Layout(Launcher& l, UINT dpi) {
  auto s = [dpi](int v) { return MulDiv(v, int(dpi), 96); };

  NONCLIENTMETRICSW metrics = {sizeof(metrics)};
  SystemParametersInfoForDpi(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0, dpi);
  LOGFONTW title_logfont = metrics.lfMessageFont;
  title_logfont.lfHeight = title_logfont.lfHeight * 16 / 10;
  title_logfont.lfWeight = FW_SEMIBOLD;
  HFONT font = CreateFontIndirectW(&metrics.lfMessageFont);
  HFONT title_font = CreateFontIndirectW(&title_logfont);

  constexpr int kMargin = 20;
  constexpr int kLabelWidth = 190;
  constexpr int kComboX = kMargin + kLabelWidth + 8;
  constexpr int kComboWidth = kClientWidth - kComboX - kMargin;
  constexpr int kTextWidth = kClientWidth - 2 * kMargin;
  constexpr int kListHeight = 320;  // dropped-down list
  constexpr int kRow = 82;
  constexpr int kRowStep = 36;
  constexpr int kButtonWidth = 100;
  constexpr int kButtonHeight = 28;
  constexpr int kButtonY = kClientHeight - kMargin - kButtonHeight;
  struct Place {
    int id, x, y, w, h;
  };
  std::vector<Place> places = {
      {kIdTitle, kMargin, 16, kTextWidth, 30},
      {kIdSubtitle, kMargin, 46, kTextWidth, 20},
  };
  int y = kRow;
  for (const auto& row : kRows) {
    places.push_back({row[0], kMargin, y + 4, kLabelWidth, 20});
    places.push_back({row[1], kComboX, y, kComboWidth, kListHeight});
    y += kRowStep;
  }
  places.push_back({kIdHint, kMargin, y + 2, kTextWidth, 52});
  places.push_back({kIdPadHint, kMargin, y + 58, kTextWidth, 20});
  for (int i = 0; i < 3; ++i) {
    places.push_back({kButtonIds[i], kClientWidth - kMargin - (3 - i) * kButtonWidth - (2 - i) * 8,
                      kButtonY, kButtonWidth, kButtonHeight});
  }
  for (const Place& p : places) {
    HWND item = Item(l, p.id);
    SetWindowPos(item, nullptr, s(p.x), s(p.y), s(p.w), s(p.h), SWP_NOZORDER | SWP_NOACTIVATE);
    SendMessageW(item, WM_SETFONT, WPARAM(p.id == kIdTitle ? title_font : font), TRUE);
  }

  if (l.font) {
    DeleteObject(l.font);
  }
  if (l.title_font) {
    DeleteObject(l.title_font);
  }
  l.font = font;
  l.title_font = title_font;
}

bool IsButton(int id) {
  return std::find(std::begin(kButtonIds), std::end(kButtonIds), id) != std::end(kButtonIds);
}

int FocusedId(const Launcher& l) {
  HWND focus = GetFocus();
  return focus && GetParent(focus) == l.hwnd ? GetDlgCtrlID(focus) : IDOK;
}

void Press(Launcher& l, int id) {
  SendMessageW(l.hwnd, WM_COMMAND, MAKEWPARAM(id, BN_CLICKED), LPARAM(Item(l, id)));
}

// Up/down: previous/next row (the buttons are the last one). Left/right: change the row value,
// or move between buttons.
void PadMove(Launcher& l, int direction) {
  for (int id : kRowIds) {
    SendMessageW(Item(l, id), CB_SHOWDROPDOWN, FALSE, 0);
  }
  int id = FocusedId(l);
  std::vector<int> rows;
  for (int row : kRowIds) {
    if (IsWindowEnabled(Item(l, row))) {
      rows.push_back(row);
    }
  }
  auto row = std::find(rows.begin(), rows.end(), id);
  bool on_row = row != rows.end();
  switch (direction) {
    case kPadUp:
      if (!on_row) {
        SetFocus(Item(l, rows.back()));
      } else if (row != rows.begin()) {
        SetFocus(Item(l, *(row - 1)));
      }
      break;
    case kPadDown:
      if (on_row) {
        SetFocus(Item(l, row + 1 != rows.end() ? *(row + 1) : IDOK));
      }
      break;
    case kPadLeft:
    case kPadRight: {
      int delta = direction == kPadLeft ? -1 : 1;
      if (!on_row) {
        auto button = std::find(std::begin(kButtonIds), std::end(kButtonIds), id);
        int next = button != std::end(kButtonIds)
                       ? int(button - std::begin(kButtonIds)) + delta
                       : 0;
        if (next >= 0 && next < int(std::size(kButtonIds))) {
          SetFocus(Item(l, kButtonIds[next]));
        }
        break;
      }
      HWND combo = Item(l, id);
      int count = int(SendMessageW(combo, CB_GETCOUNT, 0, 0));
      int index = int(SendMessageW(combo, CB_GETCURSEL, 0, 0));
      int next = std::clamp(index + delta, 0, count - 1);
      if (next != index) {
        SendMessageW(combo, CB_SETCURSEL, WPARAM(next), 0);
        OnSelectionChange(l, id);
      }
      break;
    }
  }
}

void PollPad(Launcher& l) {
  if (!l.xinput_get_state) {
    return;
  }
  bool scan_empty = --l.pad_scan_ticks <= 0;
  if (scan_empty) {
    l.pad_scan_ticks = 60;
  }
  DWORD was_connected = l.pad_connected;
  WORD buttons = 0;
  int stick_x = 0;
  int stick_y = 0;
  for (DWORD i = 0; i < XUSER_MAX_COUNT; ++i) {
    DWORD bit = 1u << i;
    if (!(l.pad_connected & bit) && !scan_empty) {
      continue;
    }
    XINPUT_STATE state = {};
    if (l.xinput_get_state(i, &state) != ERROR_SUCCESS) {
      l.pad_connected &= ~bit;
      continue;
    }
    l.pad_connected |= bit;
    buttons |= state.Gamepad.wButtons;
    if (std::abs(state.Gamepad.sThumbLX) > std::abs(stick_x)) {
      stick_x = state.Gamepad.sThumbLX;
    }
    if (std::abs(state.Gamepad.sThumbLY) > std::abs(stick_y)) {
      stick_y = state.Gamepad.sThumbLY;
    }
  }
  if ((was_connected != 0) != (l.pad_connected != 0)) {
    ShowWindow(Item(l, kIdPadHint), l.pad_connected ? SW_SHOWNA : SW_HIDE);
  }

  constexpr int kStick = 16000;
  int direction = kPadNone;
  if ((buttons & XINPUT_GAMEPAD_DPAD_UP) || stick_y > kStick) {
    direction = kPadUp;
  } else if ((buttons & XINPUT_GAMEPAD_DPAD_DOWN) || stick_y < -kStick) {
    direction = kPadDown;
  } else if ((buttons & XINPUT_GAMEPAD_DPAD_LEFT) || stick_x < -kStick) {
    direction = kPadLeft;
  } else if ((buttons & XINPUT_GAMEPAD_DPAD_RIGHT) || stick_x > kStick) {
    direction = kPadRight;
  }
  WORD pressed = buttons & ~l.pad_buttons;
  bool new_direction = direction != l.pad_direction;
  l.pad_buttons = buttons;
  l.pad_direction = direction;
  // Input already held when the window opened (the A used to launch from Big Picture) or while
  // the window is not active does not count.
  if (!l.pad_primed || GetForegroundWindow() != l.hwnd) {
    l.pad_primed = true;
    return;
  }

  ULONGLONG now = GetTickCount64();
  bool move = false;
  if (new_direction) {
    move = direction != kPadNone;
    l.pad_repeat_at = now + 400;
  } else if (direction != kPadNone && now >= l.pad_repeat_at) {
    move = true;
    l.pad_repeat_at = now + 120;
  }
  if (move || pressed) {
    // Show the focus (Windows hides it until the keyboard is used).
    SendMessageW(l.hwnd, WM_CHANGEUISTATE, MAKEWPARAM(UIS_CLEAR, UISF_HIDEFOCUS), 0);
  }
  if (move) {
    PadMove(l, direction);
  }
  if (pressed & XINPUT_GAMEPAD_A) {
    int id = FocusedId(l);
    Press(l, IsButton(id) ? id : IDOK);
  } else if (pressed & XINPUT_GAMEPAD_START) {
    Press(l, IDOK);
  } else if (pressed & XINPUT_GAMEPAD_B) {
    Press(l, IDCANCEL);
  }
}

LRESULT CALLBACK LauncherProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
  if (msg == WM_NCCREATE) {
    auto* create = reinterpret_cast<CREATESTRUCTW*>(lparam);
    SetWindowLongPtrW(hwnd, GWLP_USERDATA, LONG_PTR(create->lpCreateParams));
  }
  auto* l = reinterpret_cast<Launcher*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
  if (!l) {
    return DefWindowProcW(hwnd, msg, wparam, lparam);
  }
  switch (msg) {
    case WM_COMMAND: {
      int id = LOWORD(wparam);
      switch (id) {
        case IDOK:
          l->result = Result::kPlay;
          DestroyWindow(hwnd);
          return 0;
        case kIdSave:
          l->result = Result::kSave;
          DestroyWindow(hwnd);
          return 0;
        case IDCANCEL:
          l->result = Result::kQuit;
          DestroyWindow(hwnd);
          return 0;
      }
      if (std::find(std::begin(kRowIds), std::end(kRowIds), id) != std::end(kRowIds) &&
          HIWORD(wparam) == CBN_SELCHANGE) {
        OnSelectionChange(*l, id);
        return 0;
      }
      break;
    }
    case WM_TIMER:
      PollPad(*l);
      return 0;
    case DM_GETDEFID:
      return MAKELRESULT(IDOK, DC_HASDEFID);
    case WM_CTLCOLORSTATIC: {
      HDC hdc = HDC(wparam);
      int id = GetDlgCtrlID(HWND(lparam));
      SetBkMode(hdc, TRANSPARENT);
      if (id == kIdSubtitle || id == kIdHint || id == kIdPadHint) {
        SetTextColor(hdc, GetSysColor(COLOR_GRAYTEXT));
      }
      return LRESULT(GetSysColorBrush(COLOR_WINDOW));
    }
    case WM_DPICHANGED: {
      const RECT* r = reinterpret_cast<const RECT*>(lparam);
      SetWindowPos(hwnd, nullptr, r->left, r->top, r->right - r->left, r->bottom - r->top,
                   SWP_NOZORDER | SWP_NOACTIVATE);
      Layout(*l, LOWORD(wparam));
      return 0;
    }
    case WM_CLOSE:
      DestroyWindow(hwnd);
      return 0;
    case WM_DESTROY:
      KillTimer(hwnd, 1);
      PostQuitMessage(0);
      return 0;
  }
  return DefWindowProcW(hwnd, msg, wparam, lparam);
}

// Shows the launcher until Play, Save or Quit is pressed (l.result).
void ShowLauncher(Launcher& l, HINSTANCE instance) {
  INITCOMMONCONTROLSEX icc = {sizeof(icc), ICC_STANDARD_CLASSES};
  InitCommonControlsEx(&icc);

  WNDCLASSEXW wc = {sizeof(wc)};
  wc.lpfnWndProc = LauncherProc;
  wc.hInstance = instance;
  wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
  // The executable's icon (res/launcher.rc, resource 1) if it has one, otherwise the generic one.
  wc.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(1));
  if (!wc.hIcon) {
    wc.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
  }
  wc.hbrBackground = HBRUSH(COLOR_WINDOW + 1);
  wc.lpszClassName = L"PotcaweLauncher";
  RegisterClassExW(&wc);

  // On the monitor of the active window (Big Picture, Explorer...), centered.
  HMONITOR monitor = MonitorFromWindow(GetForegroundWindow(), MONITOR_DEFAULTTOPRIMARY);
  MONITORINFO monitor_info = {sizeof(monitor_info)};
  GetMonitorInfoW(monitor, &monitor_info);
  UINT dpi_x = 96;
  UINT dpi_y = 96;
  GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpi_x, &dpi_y);
  const RECT& work = monitor_info.rcWork;
  DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
  DWORD ex_style = WS_EX_APPWINDOW;
  RECT rect = {0, 0, MulDiv(kClientWidth, int(dpi_x), 96), MulDiv(kClientHeight, int(dpi_y), 96)};
  AdjustWindowRectExForDpi(&rect, style, FALSE, ex_style, dpi_x);
  int window_width = rect.right - rect.left;
  int window_height = rect.bottom - rect.top;
  int x = work.left + (work.right - work.left - window_width) / 2;
  int y = work.top + (work.bottom - work.top - window_height) / 2;

  l.hwnd = CreateWindowExW(ex_style, wc.lpszClassName, L"POTCAWE Launcher", style, x, y,
                           window_width, window_height, nullptr, nullptr, instance, &l);
  if (!l.hwnd) {
    return;
  }

  auto add = [&](const wchar_t* cls, int id, DWORD item_style, const wchar_t* text = L"") {
    CreateWindowExW(0, cls, text, WS_CHILD | WS_VISIBLE | item_style, 0, 0, 0, 0, l.hwnd,
                    HMENU(INT_PTR(id)), instance, nullptr);
  };
  add(WC_STATICW, kIdTitle, SS_LEFT | SS_NOPREFIX, L"Pirates of the Caribbean: At World's End");
  add(WC_STATICW, kIdSubtitle, SS_LEFT);
  for (const auto& row : kRows) {
    add(WC_STATICW, row[0], SS_LEFT);
    add(WC_COMBOBOXW, row[1],
        CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP | (row[1] == kRowIds[0] ? WS_GROUP : 0));
  }
  add(WC_STATICW, kIdHint, SS_LEFT);
  add(WC_STATICW, kIdPadHint, SS_LEFT | SS_NOPREFIX);
  add(WC_BUTTONW, IDOK, BS_DEFPUSHBUTTON | WS_TABSTOP);
  add(WC_BUTTONW, kIdSave, BS_PUSHBUTTON | WS_TABSTOP);
  add(WC_BUTTONW, IDCANCEL, BS_PUSHBUTTON | WS_TABSTOP);
  ShowWindow(Item(l, kIdPadHint), SW_HIDE);  // until a controller shows up

  Refresh(l);
  Layout(l, GetDpiForWindow(l.hwnd));
  if (l.xinput_get_state) {
    SetTimer(l.hwnd, 1, 16, nullptr);
  }

  ShowWindow(l.hwnd, SW_SHOWNORMAL);
  SetForegroundWindow(l.hwnd);
  SetFocus(Item(l, IDOK));

  MSG msg;
  while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
    if (!IsWindow(l.hwnd) || !IsDialogMessageW(l.hwnd, &msg)) {
      TranslateMessage(&msg);
      DispatchMessageW(&msg);
    }
  }

  if (l.font) {
    DeleteObject(l.font);
  }
  if (l.title_font) {
    DeleteObject(l.title_font);
  }
  UnregisterClassW(wc.lpszClassName, instance);
}

// --- Options: read, save, launch ---

Options OptionsFromConfig(const Config& c, UINT primary_dpi) {
  Options o;
  o.monitor = std::max(1, GetInt(c, "monitor", 1));
  if (!GetBool(c, "fullscreen", true)) {
    o.display = kWindow;
  } else {
    o.display = GetBool(c, "fullscreen_borderless_window", false) ? kBorderless : kFullscreen;
  }
  // launcher_window_*: the chosen size, kept even when playing fullscreen (see SaveOptions).
  int window_width = GetInt(c, "launcher_window_width", GetInt(c, "window_width", 0));
  int window_height = GetInt(c, "launcher_window_height", GetInt(c, "window_height", 0));
  if (window_width > 0 && window_height > 0) {
    o.window = {MulDiv(window_width, int(primary_dpi), 96),
                MulDiv(window_height, int(primary_dpi), 96)};
  }
  o.vulkan = GetString(c, "gpu_backend") == "vulkan";
  o.fps60 = GetBool(c, "potcawe_60fps", false);
  o.no_msaa = GetBool(c, "potcawe_no_msaa", false);
  o.native = GetBool(c, "xenos_native", false);
  std::string native_aa = GetString(c, "xenos_native_antialiasing");
  for (int i = 0; i < int(std::size(kNativeAntialiasing)); ++i) {
    if (native_aa == kNativeAntialiasing[i]) o.native_aa = i;
  }
  // Without the key, the game's default (src/startup.cpp adds it to potcawe.toml at the next start): 16x.
  // A value the row does not offer: -1 (only what the game asks: 4x on some textures) shows as 4x, 1 (1x)
  // as off.
  int anisotropic = GetInt(c, "anisotropic_override", 5);
  o.anisotropic = anisotropic < 0 ? 3 : anisotropic == 1 ? 0 : std::min(anisotropic, 5);
  if (GetString(c, "launcher_render_scale") == "auto") {
    o.scale = 0;
  } else if (int scale = GetInt(c, "launcher_render_scale", 0); scale >= 1) {
    o.scale = std::min(scale, kMaxScale);
  } else {
    // Without that key (potcawe.toml edited by hand): the square scale that was chosen there.
    int x = GetInt(c, "draw_resolution_scale_x", 0);
    int y = GetInt(c, "draw_resolution_scale_y", 0);
    o.scale = x >= 1 && x == y ? std::min(x, kMaxScale) : 0;
  }
  return o;
}

bool SaveOptions(const std::filesystem::path& path, const Launcher& l) {
  const Options& o = l.o;
  Size scale = EffectiveScale(l);
  std::ofstream f(path, std::ios::trunc);
  if (!f) {
    return false;
  }
  auto boolean = [](bool value) { return value ? "true" : "false"; };
  f << "# POTCAWE Launcher options (\"POTCAWE Launcher.exe\"): written when \"Play\" or \"Save\" is\n"
       "# pressed. potcawerecomp.exe loads them at startup, after potcawe.toml, without showing\n"
       "# any window.\n"
       "# Command line options (for example those of a Steam shortcut) win over these.\n"
       "# monitor: 1 = primary, 2.. = the others (SDL order).\n"
       "# launcher_*: launcher only. launcher_render_scale is the choice (\"auto\" or 1-3); the\n"
       "# actual scale is draw_resolution_scale_x/y (with \"auto\", computed for the monitor or the\n"
       "# window). launcher_window_*: the chosen window size, kept even when playing fullscreen\n"
       "# (window_width/height are not written then: they are the console video mode).\n";
  f << "monitor = " << o.monitor << "\n";
  f << "fullscreen = " << boolean(o.display != kWindow) << "\n";
  f << "fullscreen_borderless_window = " << boolean(o.display == kBorderless) << "\n";
  // window_width/height are also the console video mode (VdQueryVideoMode, when there is no
  // video_mode_width/height), and the presenter takes the image aspect ratio from it.
  // In fullscreen they are not written: 1280x720 mode, as always. In a window they are (in logical
  // pixels: the SDK converts them with the primary monitor's scale), and if the window is not 16:9
  // a separate 16:9 mode is set so the image is letterboxed instead of distorted (1280x720 does not
  // work: it is the default value and the SDK would fall back to the window size).
  int window_width = MulDiv(o.window.first, 96, int(l.primary_dpi));
  int window_height = MulDiv(o.window.second, 96, int(l.primary_dpi));
  if (o.display == kWindow) {
    f << "window_width = " << window_width << "\n";
    f << "window_height = " << window_height << "\n";
    if (!Is16x9(o.window)) {
      f << "video_mode_width = 1920\n";
      f << "video_mode_height = 1080\n";
    }
  }
  f << "launcher_window_width = " << window_width << "\n";
  f << "launcher_window_height = " << window_height << "\n";
  if (o.scale == 0) {
    f << "launcher_render_scale = \"auto\"\n";
  } else {
    f << "launcher_render_scale = " << o.scale << "\n";
  }
  // resolution_scale = 1 so that draw_resolution_scale_x/y win (even if potcawe.toml says
  // otherwise). With XenosNative the SDK's backend only keeps the GPU in sync, at 1x (XenosNative
  // draws at 1280x720 for now); launcher_render_scale keeps the choice for Xenos.
  if (o.native) {
    scale = {1, 1};
  }
  f << "resolution_scale = 1\n";
  f << "draw_resolution_scale_x = " << scale.first << "\n";
  f << "draw_resolution_scale_y = " << scale.second << "\n";
  f << "gpu_backend = \"" << (o.vulkan ? "vulkan" : "d3d12") << "\"\n";
  // Present every vblank (60 FPS) instead of every second one (src/framerate.cpp).
  f << "potcawe_60fps = " << boolean(o.fps60) << "\n";
  // Render surfaces without 2x MSAA (src/msaa.cpp): less GPU work, no edge antialiasing. The game
  // applies it with XenosNative whatever this says.
  f << "potcawe_no_msaa = " << boolean(o.no_msaa) << "\n";
  // Anisotropic filtering for textures with mipmaps, sharper at oblique angles (SDK cvar; the game itself
  // asks for 4x on some textures only): 5 = 16x, 4 = 8x, 3 = 4x, 2 = 2x, 0 = off.
  f << "anisotropic_override = " << o.anisotropic << "\n";
  // XenosNative (xenos_native/): draw natively on Vulkan instead of emulating the GPU (experimental).
  f << "xenos_native = " << boolean(o.native) << "\n";
  // XenosNative's own antialiasing: smaa, dlaa, fxaa or off.
  int native_aa = std::clamp(o.native_aa, 0, int(std::size(kNativeAntialiasing)) - 1);
  f << "xenos_native_antialiasing = \"" << kNativeAntialiasing[native_aa] << "\"\n";
  return bool(f);
}

// What follows the program name on the command line, verbatim (with the leading space).
std::wstring ForwardedArguments() {
  const wchar_t* p = GetCommandLineW();
  if (*p == L'"') {
    ++p;
    while (*p && *p != L'"') {
      ++p;
    }
    if (*p) {
      ++p;
    }
  } else {
    while (*p && *p != L' ' && *p != L'\t') {
      ++p;
    }
  }
  return p;
}

// Starts potcawerecomp.exe and waits for it to exit: meanwhile Steam keeps seeing the game as
// running.
int LaunchGame(const std::filesystem::path& exe_dir, const Texts& t) {
  std::filesystem::path game = exe_dir / L"potcawerecomp.exe";
  if (!std::filesystem::exists(game)) {
    MessageBoxW(nullptr, t.game_missing, L"POTCAWE Launcher", MB_OK | MB_ICONERROR);
    return 1;
  }
  std::wstring command_line = L"\"" + game.wstring() + L"\"" + ForwardedArguments();
  STARTUPINFOW startup = {sizeof(startup)};
  PROCESS_INFORMATION process = {};
  AllowSetForegroundWindow(ASFW_ANY);
  if (!CreateProcessW(game.c_str(), command_line.data(), nullptr, nullptr, FALSE, 0, nullptr,
                      exe_dir.c_str(), &startup, &process)) {
    std::wstring text = std::wstring(L"CreateProcess: ") + std::to_wstring(GetLastError());
    MessageBoxW(nullptr, text.c_str(), L"POTCAWE Launcher", MB_OK | MB_ICONERROR);
    return 1;
  }
  CloseHandle(process.hThread);
  WaitForSingleObject(process.hProcess, INFINITE);
  DWORD exit_code = 0;
  GetExitCodeProcess(process.hProcess, &exit_code);
  CloseHandle(process.hProcess);
  return int(exit_code);
}

std::filesystem::path ExecutableFolder() {
  std::wstring path(MAX_PATH, L'\0');
  for (;;) {
    DWORD length = GetModuleFileNameW(nullptr, path.data(), DWORD(path.size()));
    if (length < path.size()) {
      path.resize(length);
      break;
    }
    path.resize(path.size() * 2);
  }
  return std::filesystem::path(path).parent_path();
}

}  // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
  // Physical pixels everywhere (monitors, modes, window) and sharp with display scaling.
  SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

  std::filesystem::path exe_dir = ExecutableFolder();
  std::filesystem::path options_path = exe_dir / L"potcawe_options.toml";
  // The same potcawerecomp.exe will see: potcawe.toml, then what the launcher saved on top.
  Config config;
  ReadToml(exe_dir / L"potcawe.toml", config);
  ReadToml(options_path, config);

  Launcher l;
  l.ui_language = SystemLanguage();
  l.monitors = EnumerateMonitors();
  l.primary_dpi = l.monitors.front().dpi;
  l.o = OptionsFromConfig(config, l.primary_dpi);
  if (l.o.monitor > int(l.monitors.size())) {
    l.o.monitor = 1;
  }
  l.window_sizes = WindowSizes(CurrentMonitor(l), l.o.window);
  if (HMODULE vulkan = LoadLibraryW(L"vulkan-1.dll")) {
    l.has_vulkan = true;
    FreeLibrary(vulkan);
  }
  l.o.vulkan = l.o.vulkan && l.has_vulkan;
  for (const wchar_t* dll : {L"xinput1_4.dll", L"xinput9_1_0.dll"}) {
    if (HMODULE xinput = LoadLibraryW(dll)) {
      l.xinput_get_state =
          reinterpret_cast<XInputGetStateFn>(GetProcAddress(xinput, "XInputGetState"));
      if (l.xinput_get_state) {
        break;
      }
    }
  }

  ShowLauncher(l, instance);
  if (l.result == Result::kQuit) {
    return 0;
  }
  const Texts& t = TextsFor(l.ui_language);
  if (!SaveOptions(options_path, l)) {
    MessageBoxW(nullptr, t.save_failed, L"POTCAWE Launcher", MB_OK | MB_ICONERROR);
  }
  return l.result == Result::kPlay ? LaunchGame(exe_dir, t) : 0;
}
