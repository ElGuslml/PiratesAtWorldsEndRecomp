// potcawe - button prompts for the controller in the player's hands.
//
// The game shows its buttons in two ways. Both have art for the Xbox 360 controller and, unused on that console, art
// for the PlayStation 3's, since the engine's data is shared with the PS3 version (local_docs/12-button-glyphs.md):
// - In text: a button macro (##ABUTT, ##LB...) expands to one character of the game's fonts, codes 1-30 (the macro
//   expander sub_822C6BC8, the macro table at 0x82030F00). The three fonts (Font, Font2, Font3: a 512x512 atlas in
//   menuresource.edb and a glyph table, font*.fnt) have the PS3's buttons at codes 0x100-0x11B, in the same order.
// - On the HUD: 128x128 sprites, one texture each, in hud_main.edb: A B X Y, the shoulders, the triggers, START, BACK,
//   the d-pad, the guide button and the sticks with their arrows, with the PS3's counterparts beside them.
//
// The style follows the controller in hand (SDL's type, rex::input::GetActiveDevice): the PS3's art for a PS3
// controller; for a PS4's or a PS5's the same, but OPTIONS and SHARE / CREATE in place of START and SELECT; for an Xbox
// One's or a Series' the 360's, but MENU and VIEW in place of START and BACK; the original for anything else. The
// buttons the disc has no picture of are ours (res/prompts/, retouched from Xelu's CC0 prompts by make_prompts.py).
// - Text: in each loaded font's glyph table, the 28 Xbox glyphs' records get the style's records (their UVs in the
//   atlas, their size and advance). The text code reads the records as it lays out a line (sub_822C8618), so nothing
//   in the text changes, only the pictures of its characters. Our glyphs are drawn into the atlases' empty rows when
//   the game decompresses them, and their records are made from the records they replace.
// - Sprites: when the game has decompressed a section of a package (sub_82324F80), a texture holding an Xbox sprite's
//   pixels (the player's hud_main.edb, read at start) is noted and gets the style's pixels; on a change of controller
//   every noted texture that still holds one of its versions is rewritten. Both renderers' texture caches watch the
//   pages, and see these writes as they see the game's.
// - The word START: two texts name the button with it, in every language (text.edb): the title's "Press START"
//   (DCC00198, the frontend's PressStart widget) and the co-op's "Press START to join" (DCC00699, sub_820A0EB0). The
//   game finds a text's record by its GUID (sub_82348D28 -> sub_82426AE8: 20 bytes, the string at +12 + the offset
//   stored there). For these two, a copy of the record whose string has the controller's own word is returned instead:
//   OPTIONS (PS4, PS5), MENU (Xbox One, Series), + (Switch), the key bound to Start with the keyboard; START stays for
//   the PS3's and the 360's. Seen by a text set up after the change (the title's appears each time it is shown).
//   Three legends written with letters ("A - Select     B - Backup", "A - Continue", "Start - Continue     A - Select")
//   are used nowhere: no package (their compressed sections included), script or code names them.
// All is checked every fourth frame (the Direct3D library's Swap, sub_822B58B0). potcawe_button_prompts chooses: auto
// (the default), xbox (or xbox360), xboxone (or xboxseries), ps3 (or playstation), ps4, ps5.
//
// Not covered: the controls page's diagram (the PS3's has the left stick and the d-pad's labels the other way round),
// four 64x64 A B X Y sprites with no PS3 counterpart (used nowhere either).

#include "button_prompts.h"

#include "dxt5_texture.h"
#include "level_packages.h"

#include <rex/cvar.h>
#include <rex/hash.h>
#include <rex/input/input_system.h>
#include <rex/logging.h>
#include <rex/ppc/context.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xmemory.h>
#include <rex/ui/image_decode.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <cstring>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

REXCVAR_DEFINE_STRING(potcawe_button_prompts, "auto", "Potcawe",
                      "The buttons the game shows: auto (for the controller in hand: a PlayStation's or an Xbox One's "
                      "and Series' with one of those, the original Xbox 360's otherwise), xbox (or xbox360), xboxone "
                      "(or xboxseries), ps3 (or playstation), ps4 or ps5");

namespace {

enum class Style { kXbox360, kXboxOne, kPS3, kPS4, kPS5 };
constexpr size_t kStyleCount = 5;

const char* StyleName(Style style) {
  switch (style) {
    case Style::kXboxOne: return "Xbox One";
    case Style::kPS3: return "PS3";
    case Style::kPS4: return "PS4";
    case Style::kPS5: return "PS5";
    default: return "Xbox 360";
  }
}

bool IsPlayStation(Style style) {
  return style == Style::kPS3 || style == Style::kPS4 || style == Style::kPS5;
}

// The fonts: sub_82081AB8 registers Font, Font2 and Font3 with the font manager (sub_822C9818), which keeps them as
// 68-byte entries, their count at +140 and the first at +144. An entry's +16 is its loaded .FNT (sub_822E4178): +12
// the glyph table (u16 offsets from the table, by character code, made big-endian on load), +18 the table's length,
// +20 and +22 the atlas's width and height. A glyph's record is 24 bytes: u0 v0 u1 v1 (big-endian floats), its width,
// height, x and y offsets (bytes), then a 32-bit word: the advance (7 bits) and a flag, as the loader turns them
// (rotated left by one), and its place in the atlas, x | y << 9 (24 bits).
constexpr uint32_t kFontManager = 0x8257E418;
constexpr uint32_t kFontCount = 140, kFontEntries = 144, kFontEntrySize = 68, kFontData = 16;
constexpr uint32_t kMaxFonts = 8;
constexpr uint32_t kGlyphTable = 12, kGlyphTableLength = 18, kAtlasWidth = 20, kAtlasHeight = 22;
constexpr uint32_t kGlyphRecordSize = 24;
using GlyphRecord = std::array<uint8_t, kGlyphRecordSize>;
// The button macros' characters, in the order of the PS3 glyphs from 0x100.
constexpr std::array<uint16_t, 28> kXboxGlyphs = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0B,
                                                  0x0C, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16,
                                                  0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E};
constexpr uint16_t kPlayStationGlyphs = 0x100;
constexpr uint16_t kStartGlyph = 0x1C, kBackGlyph = 0x11;

// The font atlases (menuresource.edb), 512x512 DXT5, empty from row 314 down (local_docs/12): our glyphs go from row
// 320, at multiples of 64 so that the mips' pixels up to level 6 (a 64-pixel footprint) cover only empty rows and ours.
constexpr uint32_t kMenuResource = 0xC10003DF;
constexpr uint32_t kAtlases[] = {0xC2006254, 0xC2006255, 0xC2006256};
constexpr uint32_t kAtlasSize = 512 << 16 | 512, kAtlasMips = 10, kAtlasSurface = 0x60000, kAtlasLevels = 7;
struct NewGlyph {
  Style style;
  uint16_t xbox_glyph;  // the button it shows
  const wchar_t* picture;
  uint32_t x, y;
};
constexpr NewGlyph kNewGlyphs[] = {
    {Style::kPS5, kStartGlyph, L"PROMPT_PS5_OPTIONS_GLYPH", 0, 320},
    {Style::kPS5, kBackGlyph, L"PROMPT_PS5_CREATE_GLYPH", 64, 320},
    {Style::kPS4, kStartGlyph, L"PROMPT_PS4_OPTIONS_GLYPH", 128, 320},  // up to 124 wide
    {Style::kPS4, kBackGlyph, L"PROMPT_PS4_SHARE_GLYPH", 256, 320},
    {Style::kXboxOne, kStartGlyph, L"PROMPT_XBOX_MENU_GLYPH", 384, 320},
    {Style::kXboxOne, kBackGlyph, L"PROMPT_XBOX_VIEW_GLYPH", 448, 320},
};

// hud_main.edb's button sprites, each Xbox texture's GUID and its PS3 counterpart's.
constexpr uint32_t kHudMain = 0xC10001EE;
struct SpritePair {
  uint32_t xbox, playstation;
};
constexpr SpritePair kSpritePairs[] = {
    {0xC2007DDB, 0xC2007DD0},  // A, cross
    {0xC2007DDC, 0xC2007DE8},  // B, circle
    {0xC2007DE5, 0xC2007DCE},  // X, square
    {0xC2007B44, 0xC2007DCF},  // Y, triangle
    {0xC2009394, 0xC20093E5},  // A, cross (a second pair)
    {0xC2009395, 0xC20093E4},  // Y, triangle (a second pair)
    {0xC2007DE3, 0xC2007DD4},  // LB, L1
    {0xC2007DE4, 0xC2007DD6},  // RB, R1
    {0xC2007DDE, 0xC2007DD5},  // LT, L2
    {0xC2007DDF, 0xC2007DD7},  // RT, R2
    {0xC2007DCC, 0xC2007DD8},  // START
    {0xC2007DDA, 0xC2007DD3},  // BACK, SELECT
    {0xC2007DDD, 0xC2007DD1},  // the d-pad
    {0xC2007DE6, 0xC2007DD2},  // the guide button, the PS button
    {0xC2008FC0, 0xC2008FC2},  // LS, L
    {0xC2008FC1, 0xC2008FC3},  // RS, R
    {0xC2008714, 0xC2008713},  // the left stick, an arrow over it pointing down
    {0xC2008715, 0xC2008726},  // the right stick, an arrow over it pointing down
    {0xC2008716, 0xC2008724},  // the left stick, an arrow under it
    {0xC2008717, 0xC2008725},  // the right stick, an arrow under it
    {0xC2008719, 0xC2008727},  // the left stick, an arrow to its left
    {0xC2008718, 0xC2008728},  // the right stick, an arrow to its left
    {0xC200872E, 0xC2008721},  // the left stick, an arrow to its right
    {0xC200871A, 0xC200871F},  // the right stick, an arrow to its right
    {0xC200871C, 0xC200871D},  // the left stick, an arrow over it pointing up
    {0xC200871B, 0xC200871E},  // the right stick, an arrow over it pointing up
};
constexpr size_t kSpriteCount = std::size(kSpritePairs);
constexpr uint32_t kStartSprite = 0xC2007DCC, kBackSprite = 0xC2007DDA;
struct NewSprite {
  Style style;
  uint32_t xbox;  // the Xbox sprite it stands for
  const wchar_t* picture;
};
constexpr NewSprite kNewSprites[] = {
    {Style::kPS5, kStartSprite, L"PROMPT_PS5_OPTIONS_SPRITE"},
    {Style::kPS5, kBackSprite, L"PROMPT_PS5_CREATE_SPRITE"},
    {Style::kPS4, kStartSprite, L"PROMPT_PS4_OPTIONS_SPRITE"},
    {Style::kPS4, kBackSprite, L"PROMPT_PS4_SHARE_SPRITE"},
    {Style::kXboxOne, kStartSprite, L"PROMPT_XBOX_MENU_SPRITE"},
    {Style::kXboxOne, kBackSprite, L"PROMPT_XBOX_VIEW_SPRITE"},
};
// A texture as the game keeps it (level_packages.cpp): a 4 KB header, then the surface. The sprites' are 128x128 DXT5.
constexpr uint32_t kTextureMagic = 0xAF000008;
constexpr uint32_t kTextureSize = 0x24, kTextureFormat = 0x2F;
constexpr uint32_t kSpriteSize = 128 << 16 | 128, kSpriteMips = 8, kDxt5 = 1;
constexpr uint32_t kTextureHeader = 0x1000, kSpritePixels = 0x10000;

using Surface = std::shared_ptr<const std::vector<uint8_t>>;
struct SpriteArt {
  std::array<Surface, kStyleCount> surfaces;  // by Style; kSpritePixels each
  uint64_t xbox_hash = 0;
};
std::vector<SpriteArt> g_sprites;  // as kSpritePairs; made before the guest starts, then only read

struct Picture {
  std::vector<uint8_t> rgba;
  uint32_t width = 0, height = 0;
};
std::vector<Picture> g_glyph_pictures;  // as kNewGlyphs
std::vector<uint64_t> g_atlas_hashes;   // the atlases' surfaces as the game decompresses them
std::atomic<uint32_t> g_atlases_drawn{0};

// The sprites' state, shared by the loading thread (noting) and the render thread (a change of controller).
std::mutex g_sprite_mutex;
std::map<uint32_t, size_t> g_noted;  // the textures holding a sprite: guest address -> kSpritePairs index
Style g_sprite_style = Style::kXbox360;
uint32_t g_loaded = 0;  // sprites noted since the frame hook last logged them

// The fonts' state and the frame hook's count, for one thread at a time (Swap and Synchronize may come from two).
std::mutex g_frame_mutex;
uint32_t g_frames = 0;
struct FontPatch {
  uint32_t table = 0;
  std::array<GlyphRecord, kXboxGlyphs.size()> original = {};
  std::array<bool, kXboxGlyphs.size()> present = {};
  Style style = Style::kXbox360;
};
std::map<uint32_t, FontPatch> g_fonts;  // by the font's loaded .FNT

// The texts that name Start by the word START, and their records' layout.
constexpr uint32_t kStartTexts[] = {0xDCC00198, 0xDCC00699};  // "Press START", "Press START to join"
constexpr uint32_t kTextRecordSize = 20, kTextString = 12;
constexpr std::u16string_view kStart = u"START";
constexpr size_t kMaxText = 512;
// The word for Start now (empty: START as written), set by the frame hook, read by the game's threads.
std::mutex g_text_mutex;
std::u16string g_start_word;
std::map<std::pair<uint32_t, std::u16string>, uint32_t> g_start_texts;  // (record, word) -> its copy's record

uint32_t Be16(const uint8_t* p) { return uint32_t(p[0]) << 8 | p[1]; }
uint32_t Be32(const uint8_t* p) {
  return uint32_t(p[0]) << 24 | uint32_t(p[1]) << 16 | uint32_t(p[2]) << 8 | uint32_t(p[3]);
}
void PutBe32(uint8_t* p, uint32_t v) {
  p[0] = uint8_t(v >> 24);
  p[1] = uint8_t(v >> 16);
  p[2] = uint8_t(v >> 8);
  p[3] = uint8_t(v);
}

uint8_t* Guest(uint32_t address) {
  auto* kernel = rex::system::kernel_state();
  if (!address || !kernel || !kernel->memory()) return nullptr;
  return kernel->memory()->TranslateVirtual<uint8_t*>(address);
}

// One of res/prompts/'s pictures, embedded by res/game.rc.
Picture LoadPicture(const wchar_t* name) {
  Picture picture;
  HMODULE module = GetModuleHandleW(nullptr);
  HRSRC resource = FindResourceW(module, name, MAKEINTRESOURCEW(10));  // RT_RCDATA
  HGLOBAL loaded = resource ? LoadResource(module, resource) : nullptr;
  const void* data = loaded ? LockResource(loaded) : nullptr;
  if (!data) return picture;
  int width = 0, height = 0;
  picture.rgba = rex::ui::DecodeImageRGBA(static_cast<const uint8_t*>(data), SizeofResource(module, resource), width,
                                          height);
  if (!picture.rgba.empty()) {
    picture.width = uint32_t(width);
    picture.height = uint32_t(height);
  }
  return picture;
}

const char* TypeName(rex::input::GamepadType type) {
  using rex::input::GamepadType;
  switch (type) {
    case GamepadType::kStandard: return "standard";
    case GamepadType::kXbox360: return "Xbox 360";
    case GamepadType::kXboxOne: return "Xbox One";
    case GamepadType::kPS3: return "PS3";
    case GamepadType::kPS4: return "PS4";
    case GamepadType::kPS5: return "PS5";
    case GamepadType::kSwitchPro: return "Switch Pro";
    case GamepadType::kJoyConLeft:
    case GamepadType::kJoyConRight:
    case GamepadType::kJoyConPair: return "Joy-Con";
    case GamepadType::kGameCube: return "GameCube";
    default: return "unknown";
  }
}

std::u16string Utf16(const std::string& ascii) {
  std::u16string out;
  for (char c : ascii) out.push_back(static_cast<char16_t>(static_cast<unsigned char>(c)));
  return out;
}

// Start's word for a device: its own label, or empty for START. With the keyboard, the first key of keybind_start
// ("X,Return" by default), as the binding names it, in capitals.
std::u16string StartWord(const rex::input::DeviceInfo& device) {
  using rex::input::GamepadType;
  if (device.id == rex::input::DeviceId::kInvalid) return {};
  if (device.synthetic) {
    if (device.name.rfind("Keyboard", 0) != 0) return {};
    std::string key = rex::cvar::Query<std::string>("keybind_start");
    key = key.substr(0, key.find(','));
    if (key == "Return" || key == "Enter") key = "ENTER";
    for (char& c : key) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return key.size() <= 16 ? Utf16(key) : std::u16string();
  }
  switch (device.gamepad_type) {
    case GamepadType::kPS4:
    case GamepadType::kPS5: return u"OPTIONS";
    case GamepadType::kXboxOne: return u"MENU";
    case GamepadType::kSwitchPro:
    case GamepadType::kJoyConLeft:
    case GamepadType::kJoyConRight:
    case GamepadType::kJoyConPair: return u"+";
    default: return {};
  }
}

void SetStartWord(std::u16string word) {
  std::lock_guard lock(g_text_mutex);
  if (word == g_start_word) return;
  g_start_word = std::move(word);
  std::string text = g_start_word.empty() ? "START" : std::string(g_start_word.begin(), g_start_word.end());
  REXLOG_INFO("[prompts] START reads {} in the texts set up from now on", text);
}

// The style potcawe_button_prompts asks for; for auto, from player 1's controller in hand. Start's word is set too.
// A change is logged.
Style WantedStyle() {
  static std::string last_reason;
  const std::string setting = REXCVAR_GET(potcawe_button_prompts);
  Style style = Style::kXbox360;
  std::string reason;
  static const std::pair<std::string_view, Style> kSettings[] = {
      {"xbox", Style::kXbox360},       {"xbox360", Style::kXbox360}, {"xboxone", Style::kXboxOne},
      {"xboxseries", Style::kXboxOne}, {"ps3", Style::kPS3},         {"playstation", Style::kPS3},
      {"ps4", Style::kPS4},            {"ps5", Style::kPS5},
  };
  auto forced = std::find_if(std::begin(kSettings), std::end(kSettings),
                             [&](const auto& entry) { return entry.first == setting; });
  if (forced != std::end(kSettings)) {
    style = forced->second;
    reason = "potcawe_button_prompts = " + setting;
    SetStartWord(style == Style::kPS4 || style == Style::kPS5 ? u"OPTIONS"
                 : style == Style::kXboxOne                   ? u"MENU"
                                                              : u"");
  } else {
    const rex::input::DeviceInfo device = rex::input::GetActiveDevice(0);
    SetStartWord(StartWord(device));
    using rex::input::GamepadType;
    switch (device.gamepad_type) {
      case GamepadType::kXboxOne: style = Style::kXboxOne; break;
      case GamepadType::kPS3: style = Style::kPS3; break;
      case GamepadType::kPS4: style = Style::kPS4; break;
      case GamepadType::kPS5: style = Style::kPS5; break;
      default: break;
    }
    if (device.id == rex::input::DeviceId::kInvalid) {
      reason = "no controller";
    } else if (device.synthetic) {
      reason = device.name;
    } else {
      reason = device.name + ", " + TypeName(device.gamepad_type);
    }
  }
  reason = StyleName(style) + std::string(" (") + reason + ")";
  if (reason != last_reason) {
    REXLOG_INFO("[prompts] {} button prompts", reason);
    last_reason = reason;
  }
  return style;
}

// A record for one of our glyphs: the PS3 record of the button it replaces (for its y offset: the PS3's cells are 32
// pixels high, as our pictures), pointing at its cell in the atlas. For every style, the Xbox One's included.
GlyphRecord NewGlyphRecord(const uint8_t* ps3_record, const NewGlyph& glyph, const Picture& picture,
                           uint32_t atlas_width, uint32_t atlas_height) {
  GlyphRecord record;
  std::memcpy(record.data(), ps3_record, kGlyphRecordSize);
  const float uv[4] = {float(glyph.x) / atlas_width, float(glyph.y) / atlas_height,
                       float(glyph.x + picture.width) / atlas_width, float(glyph.y + picture.height) / atlas_height};
  for (int i = 0; i < 4; ++i) {
    uint32_t bits;
    std::memcpy(&bits, &uv[i], 4);
    PutBe32(record.data() + i * 4, bits);
  }
  record[16] = uint8_t(picture.width);
  record[17] = uint8_t(picture.height);
  record[18] = 0;
  const uint8_t advance = uint8_t(0x80 | (picture.width & 0x7F));  // a picture: the flag set, as the PS3's
  PutBe32(record.data() + 20, uint32_t(uint8_t(advance << 1 | advance >> 7)) << 24 | glyph.x | glyph.y << 9);
  return record;
}

// Each loaded font's button glyphs to `style`, keeping the Xbox records of a font the first time it is seen.
void ApplyToFonts(Style style) {
  const uint8_t* manager = Guest(kFontManager);
  if (!manager) return;
  const uint32_t count = Be32(manager + kFontCount);
  const uint32_t entries = Be32(manager + kFontEntries);
  if (!count || count > kMaxFonts || !entries) return;
  for (uint32_t i = 0; i < count; ++i) {
    const uint8_t* entry = Guest(entries + i * kFontEntrySize);
    const uint32_t font = entry ? Be32(entry + kFontData) : 0;
    const uint8_t* font_data = Guest(font);
    if (!font_data) continue;
    const uint32_t table = Be32(font_data + kGlyphTable);
    uint8_t* glyphs = Guest(table);
    if (!glyphs || Be16(font_data + kGlyphTableLength) < kPlayStationGlyphs + kXboxGlyphs.size()) continue;

    FontPatch& patch = g_fonts[font];
    if (patch.table != table) {  // a font not seen before
      patch = FontPatch{};
      patch.table = table;
      for (size_t k = 0; k < kXboxGlyphs.size(); ++k) {
        const uint32_t xbox = Be16(glyphs + kXboxGlyphs[k] * 2);
        const uint32_t playstation = Be16(glyphs + (kPlayStationGlyphs + k) * 2);
        patch.present[k] = xbox != 0 && playstation != 0;
        if (patch.present[k]) std::memcpy(patch.original[k].data(), glyphs + xbox, kGlyphRecordSize);
      }
    }
    if (patch.style == style) continue;
    const uint32_t atlas_width = Be16(font_data + kAtlasWidth), atlas_height = Be16(font_data + kAtlasHeight);
    size_t changed = 0;
    for (size_t k = 0; k < kXboxGlyphs.size(); ++k) {
      if (!patch.present[k]) continue;
      uint8_t* record = glyphs + Be16(glyphs + kXboxGlyphs[k] * 2);
      const uint8_t* ps3 = glyphs + Be16(glyphs + (kPlayStationGlyphs + k) * 2);
      GlyphRecord to = patch.original[k];
      if (IsPlayStation(style)) std::memcpy(to.data(), ps3, kGlyphRecordSize);
      for (size_t n = 0; n < g_glyph_pictures.size(); ++n) {
        const NewGlyph& glyph = kNewGlyphs[n];
        const Picture& picture = g_glyph_pictures[n];
        if (glyph.style == style && glyph.xbox_glyph == kXboxGlyphs[k] && picture.width && atlas_width &&
            atlas_height) {
          to = NewGlyphRecord(ps3, glyph, picture, atlas_width, atlas_height);
        }
      }
      std::memcpy(record, to.data(), kGlyphRecordSize);
      ++changed;
    }
    patch.style = style;
    REXLOG_INFO("[prompts] font {} (0x{:08X}): {} button glyphs as {}'s", i, font, changed, StyleName(style));
  }
}

bool HoldsSprite(const uint8_t* pixels, const SpriteArt& art) {
  for (const Surface& surface : art.surfaces) {
    if (surface && !std::memcmp(pixels, surface->data(), kSpritePixels)) return true;
  }
  return false;
}

void ApplyToSprites(Style style) {
  std::lock_guard lock(g_sprite_mutex);
  if (g_loaded) {
    REXLOG_INFO("[prompts] {} HUD sprites loaded ({} known), as {}'s", g_loaded, g_noted.size(),
                StyleName(g_sprite_style));
    g_loaded = 0;
  }
  if (g_sprite_style == style) return;
  g_sprite_style = style;
  size_t written = 0, gone = 0;
  for (auto it = g_noted.begin(); it != g_noted.end();) {
    uint8_t* blob = Guest(it->first);
    const SpriteArt& art = g_sprites[it->second];
    uint8_t* pixels = blob ? blob + kTextureHeader : nullptr;
    if (!pixels || !HoldsSprite(pixels, art)) {  // freed, and the memory holds something else now
      it = g_noted.erase(it);
      ++gone;
      continue;
    }
    std::memcpy(pixels, art.surfaces[size_t(style)]->data(), kSpritePixels);
    ++written;
    ++it;
  }
  REXLOG_INFO("[prompts] {} HUD sprites as {}'s ({} no longer loaded)", written, StyleName(style), gone);
}

// A just decompressed texture: one of the font atlases gets our glyphs; one of the Xbox sprites is noted.
void OnTexture(uint32_t address, uint8_t* blob) {
  potcawe::Dxt5Texture texture;
  if (!potcawe::Dxt5TextureFromBlob(blob, 0, texture)) return;
  uint8_t* surface = blob + kTextureHeader;
  // Only with all their mips, so that the surface hashed is the texture's.
  if (Be32(blob + kTextureSize) == kAtlasSize && texture.levels == kAtlasMips) {
    const uint64_t hash = XXH3_64bits(surface, kAtlasSurface);
    if (std::find(g_atlas_hashes.begin(), g_atlas_hashes.end(), hash) == g_atlas_hashes.end()) return;
    for (size_t n = 0; n < g_glyph_pictures.size(); ++n) {
      const Picture& picture = g_glyph_pictures[n];
      if (!picture.width) continue;
      potcawe::DrawDxt5(texture, kNewGlyphs[n].x, kNewGlyphs[n].y, picture.rgba.data(), picture.width, picture.height,
                        kAtlasLevels);
    }
    g_atlases_drawn.fetch_add(1, std::memory_order_relaxed);
    return;
  }
  if (Be32(blob + kTextureSize) != kSpriteSize || texture.levels != kSpriteMips) return;
  const uint64_t hash = XXH3_64bits(surface, kSpritePixels);
  for (size_t i = 0; i < g_sprites.size(); ++i) {
    const SpriteArt& art = g_sprites[i];
    const Surface& xbox = art.surfaces[size_t(Style::kXbox360)];
    if (!xbox || art.xbox_hash != hash || std::memcmp(surface, xbox->data(), kSpritePixels)) continue;
    std::lock_guard lock(g_sprite_mutex);
    g_noted[address] = i;
    ++g_loaded;
    if (g_sprite_style != Style::kXbox360) {
      std::memcpy(surface, art.surfaces[size_t(g_sprite_style)]->data(), kSpritePixels);
    }
    return;
  }
}

}  // namespace

namespace potcawe {

void InitButtonPrompts(const std::filesystem::path& game_data_root) {
  // Our pictures.
  g_glyph_pictures.clear();
  size_t pictures = 0;
  for (const NewGlyph& glyph : kNewGlyphs) {
    g_glyph_pictures.push_back(LoadPicture(glyph.picture));
    pictures += g_glyph_pictures.back().width ? 1 : 0;
  }

  // The font atlases, to know them when the game decompresses them.
  auto atlases = ReadPackageTextures(game_data_root, kMenuResource,
                                     std::vector<uint32_t>(std::begin(kAtlases), std::end(kAtlases)));
  for (const auto& [guid, blob] : atlases) {
    if (blob.size() < kTextureHeader + kAtlasSurface || Be32(blob.data()) != kTextureMagic ||
        Be32(blob.data() + kTextureSize) != kAtlasSize || blob[kTextureFormat] != kDxt5) {
      continue;
    }
    g_atlas_hashes.push_back(XXH3_64bits(blob.data() + kTextureHeader, kAtlasSurface));
  }

  // The HUD's sprites: the Xbox 360 one, the PS3 one; for the PS4 and the PS5 the PS3 one, for the Xbox One the 360
  // one, or our picture over all of it.
  std::vector<uint32_t> guids;
  for (const SpritePair& pair : kSpritePairs) {
    guids.push_back(pair.xbox);
    guids.push_back(pair.playstation);
  }
  auto textures = ReadPackageTextures(game_data_root, kHudMain, guids);
  auto is_sprite = [](const std::vector<uint8_t>& blob) {
    return blob.size() == kTextureHeader + kSpritePixels && Be32(blob.data()) == kTextureMagic &&
           Be32(blob.data() + kTextureSize) == kSpriteSize && blob[kTextureFormat] == kDxt5;
  };
  auto surface_of = [](const std::vector<uint8_t>& blob) {
    return std::make_shared<const std::vector<uint8_t>>(blob.begin() + kTextureHeader, blob.end());
  };
  std::vector<SpriteArt> sprites(kSpriteCount);
  size_t read = 0, made = 0;
  for (size_t i = 0; i < kSpriteCount; ++i) {
    auto xbox = textures.find(kSpritePairs[i].xbox), ps3 = textures.find(kSpritePairs[i].playstation);
    if (xbox == textures.end() || ps3 == textures.end() || !is_sprite(xbox->second) || !is_sprite(ps3->second)) {
      continue;  // left out: no texture will match an empty one
    }
    SpriteArt& art = sprites[i];
    art.surfaces[size_t(Style::kXbox360)] = surface_of(xbox->second);
    art.surfaces[size_t(Style::kPS3)] = surface_of(ps3->second);
    art.surfaces[size_t(Style::kPS4)] = art.surfaces[size_t(Style::kPS5)] = art.surfaces[size_t(Style::kPS3)];
    art.surfaces[size_t(Style::kXboxOne)] = art.surfaces[size_t(Style::kXbox360)];
    for (const NewSprite& sprite : kNewSprites) {
      if (sprite.xbox != kSpritePairs[i].xbox) continue;
      const Picture picture = LoadPicture(sprite.picture);
      std::vector<uint8_t> blob = IsPlayStation(sprite.style) ? ps3->second : xbox->second;
      Dxt5Texture texture;
      if (picture.width != 128 || picture.height != 128 || !Dxt5TextureFromBlob(blob.data(), blob.size(), texture)) {
        continue;
      }
      DrawDxt5(texture, 0, 0, picture.rgba.data(), picture.width, picture.height);
      art.surfaces[size_t(sprite.style)] = surface_of(blob);
      ++made;
    }
    art.xbox_hash = XXH3_64bits(art.surfaces[0]->data(), kSpritePixels);
    ++read;
  }
  g_sprites = std::move(sprites);
  REXLOG_INFO("[prompts] {} of {} HUD sprite pairs read from hud_main.edb, {} of {} sprites and {} of {} glyphs of ours, "
              "{} font atlases known",
              read, kSpriteCount, made, std::size(kNewSprites), pictures, std::size(kNewGlyphs),
              g_atlas_hashes.size());
}

}  // namespace potcawe

// [[midasm_hook]] at 0x822B58C0 in sub_822B58B0, the Direct3D library's Swap and Synchronize (once a frame or more).
void PotcaweButtonPromptsFrameHook() {
  std::unique_lock lock(g_frame_mutex, std::try_to_lock);
  if (!lock.owns_lock() || g_frames++ % 4) return;
  if (const uint32_t drawn = g_atlases_drawn.exchange(0, std::memory_order_relaxed)) {
    REXLOG_INFO("[prompts] our glyphs drawn into {} font atlases", drawn);
  }
  const Style style = WantedStyle();
  ApplyToFonts(style);
  ApplyToSprites(style);
}

// [[midasm_hook]] at 0x82348D88 in sub_82348D28, which finds a text by its GUID in the game's language: after the text
// store's look-up (sub_82426AE8), r3 = the text's record (0: none), r29 = the GUID. For a text that names Start by the
// word START, while the controller in hand has another word, r3 becomes a copy of the record whose string has it (made
// once per record and word, in guest memory; the original is not touched).
void PotcaweButtonPromptsTextHook(PPCRegister& r3, PPCRegister& r29) {
  if (!r3.u32 || std::find(std::begin(kStartTexts), std::end(kStartTexts), r29.u32) == std::end(kStartTexts)) return;
  std::lock_guard lock(g_text_mutex);
  if (g_start_word.empty()) return;
  const auto key = std::make_pair(r3.u32, g_start_word);
  if (auto it = g_start_texts.find(key); it != g_start_texts.end()) {
    if (it->second) r3.u32 = it->second;
    return;
  }
  uint32_t& copy = g_start_texts[key];  // 0 while the text cannot be changed
  const uint8_t* record = Guest(r3.u32);
  auto* kernel = rex::system::kernel_state();
  if (!record || !kernel || !kernel->memory()) return;
  const uint32_t string = r3.u32 + kTextString + Be32(record + kTextString);
  const uint8_t* chars = Guest(string);
  std::u16string text;
  for (size_t i = 0; chars && i < kMaxText; ++i) {
    const char16_t c = static_cast<char16_t>(Be16(chars + i * 2));
    if (!c) break;
    text.push_back(c);
  }
  size_t at = text.find(kStart);
  if (at == std::u16string::npos) return;  // a language that does not use the word
  text.replace(at, kStart.size(), g_start_word);
  const uint32_t size = kTextRecordSize + static_cast<uint32_t>(text.size() + 1) * 2;
  const uint32_t address = kernel->memory()->SystemHeapAlloc(size);
  uint8_t* out = Guest(address);
  if (!out) return;
  std::memcpy(out, record, kTextRecordSize);
  PutBe32(out + kTextString, kTextRecordSize - kTextString);  // the string right after the record
  for (size_t i = 0; i <= text.size(); ++i) {
    const char16_t c = i < text.size() ? text[i] : 0;
    out[kTextRecordSize + i * 2] = uint8_t(c >> 8);
    out[kTextRecordSize + i * 2 + 1] = uint8_t(c);
  }
  copy = address;
  r3.u32 = address;
}

// [[midasm_hook]] at 0x82325010 in sub_82324F80, which decompresses a section of a package as it is read: after the
// EDL decoder (r3 nonzero: decoded), r31 = the section's memory. A font atlas gets our glyphs; an Xbox sprite is noted
// and gets the style's pixels.
void PotcaweButtonPromptsSectionHook(PPCRegister& r3, PPCRegister& r31) {
  if (!r3.u32 || g_sprites.empty()) return;
  if (uint8_t* blob = Guest(r31.u32)) OnTexture(r31.u32, blob);
}
