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
// With a PlayStation controller in hand (SDL's PS3, PS4 or PS5 type, rex::input::GetActiveDevice), the prompts show
// the PS3's art, and the original comes back with any other controller or the keyboard:
// - Text: in each loaded font's glyph table, the 28 Xbox glyphs' records get the PS3 glyphs' records (their UVs in the
//   same atlas, their size and advance). The text code reads the records as it lays out a line (sub_822C8618), so
//   nothing in the text changes, only the pictures of its characters.
// - Sprites: when the game has decompressed a section of a package (sub_82324F80), a texture holding an Xbox sprite's
//   pixels (the player's hud_main.edb, read at start) is noted and, while a PlayStation controller is in hand, gets its
//   PS3 counterpart's pixels; on a change of controller every noted texture that still holds either is rewritten. Both
//   renderers' texture caches watch the pages, and see these writes as they see the game's.
// Both are checked every fourth frame (the Direct3D library's Swap, sub_822B58B0). potcawe_button_prompts chooses: auto
// (the default), xbox or playstation.
//
// Not covered: the words START and A in a few texts ("Press START", "A - Select"), the controls page's diagram (the
// PS3's has the left stick and the d-pad's labels the other way round), four 64x64 A B X Y sprites with no PS3
// counterpart.

#include "button_prompts.h"

#include "level_packages.h"

#include <rex/cvar.h>
#include <rex/hash.h>
#include <rex/input/input_system.h>
#include <rex/logging.h>
#include <rex/ppc/context.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xmemory.h>

#include <array>
#include <cstring>
#include <map>
#include <mutex>
#include <string>
#include <vector>

REXCVAR_DEFINE_STRING(potcawe_button_prompts, "auto", "Potcawe",
                      "The buttons the game shows: auto (the PlayStation 3's art with a PlayStation controller in "
                      "hand, the original Xbox 360 art otherwise), xbox or playstation");

namespace {

enum class Style { kXbox, kPlayStation };

// The fonts: sub_82081AB8 registers Font, Font2 and Font3 with the font manager (sub_822C9818), which keeps them as
// 68-byte entries, their count at +140 and the first at +144. An entry's +16 is its loaded .FNT (sub_822E4178): +12
// the glyph table (u16 offsets from the table, by character code, made big-endian on load), +18 the table's length. A
// glyph's record is 24 bytes: u0 v0 u1 v1, then its size, offsets and advance.
constexpr uint32_t kFontManager = 0x8257E418;
constexpr uint32_t kFontCount = 140, kFontEntries = 144, kFontEntrySize = 68, kFontData = 16;
constexpr uint32_t kMaxFonts = 8;
constexpr uint32_t kGlyphTable = 12, kGlyphTableLength = 18;
constexpr uint32_t kGlyphRecordSize = 24;
// The button macros' characters, in the order of the PS3 glyphs from 0x100.
constexpr std::array<uint16_t, 28> kXboxGlyphs = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0B,
                                                  0x0C, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16,
                                                  0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E};
constexpr uint16_t kPlayStationGlyphs = 0x100;

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
// A texture as the game keeps it (level_packages.cpp): a 4 KB header, then the surface. The sprites' are 128x128 DXT5.
constexpr uint32_t kTextureMagic = 0xAF000008;
constexpr uint32_t kTextureSize = 0x24, kTextureFormat = 0x2F;
constexpr uint32_t kSpriteSize = 128 << 16 | 128, kSpriteFormat = 1;
constexpr uint32_t kTextureHeader = 0x1000, kSpritePixels = 0x10000;

struct SpriteArt {
  std::vector<uint8_t> xbox, playstation;  // the surfaces, kSpritePixels each
  uint64_t xbox_hash = 0;
};
std::vector<SpriteArt> g_sprites;  // as kSpritePairs; read before the guest starts, then only read

// The sprites' state, shared by the loading thread (noting) and the render thread (a change of controller).
std::mutex g_sprite_mutex;
std::map<uint32_t, size_t> g_noted;  // the textures holding a sprite: guest address -> kSpritePairs index
Style g_sprite_style = Style::kXbox;
uint32_t g_loaded = 0;  // sprites noted since the frame hook last logged them

// The fonts' state and the frame hook's count, for one thread at a time (Swap and Synchronize may come from two).
std::mutex g_frame_mutex;
uint32_t g_frames = 0;
struct FontPatch {
  uint32_t table = 0;
  std::array<std::array<uint8_t, kGlyphRecordSize>, kXboxGlyphs.size()> original = {};
  std::array<bool, kXboxGlyphs.size()> present = {};
  Style style = Style::kXbox;
};
std::map<uint32_t, FontPatch> g_fonts;  // by the font's loaded .FNT

uint32_t Be16(const uint8_t* p) { return uint32_t(p[0]) << 8 | p[1]; }
uint32_t Be32(const uint8_t* p) {
  return uint32_t(p[0]) << 24 | uint32_t(p[1]) << 16 | uint32_t(p[2]) << 8 | uint32_t(p[3]);
}

uint8_t* Guest(uint32_t address) {
  auto* kernel = rex::system::kernel_state();
  if (!address || !kernel || !kernel->memory()) return nullptr;
  return kernel->memory()->TranslateVirtual<uint8_t*>(address);
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

// The style potcawe_button_prompts asks for; for auto, from player 1's controller in hand. A change is logged.
Style WantedStyle() {
  static std::string last_reason;
  const std::string setting = REXCVAR_GET(potcawe_button_prompts);
  Style style = Style::kXbox;
  std::string reason;
  if (setting == "playstation" || setting == "xbox") {
    style = setting == "playstation" ? Style::kPlayStation : Style::kXbox;
    reason = "potcawe_button_prompts = " + setting;
  } else {
    const rex::input::DeviceInfo device = rex::input::GetActiveDevice(0);
    using rex::input::GamepadType;
    const GamepadType type = device.gamepad_type;
    if (type == GamepadType::kPS3 || type == GamepadType::kPS4 || type == GamepadType::kPS5) {
      style = Style::kPlayStation;
    }
    if (device.id == rex::input::DeviceId::kInvalid) {
      reason = "no controller";
    } else if (device.synthetic) {
      reason = device.name;
    } else {
      reason = device.name + ", " + TypeName(type);
    }
  }
  reason = (style == Style::kPlayStation ? "PlayStation" : "Xbox") + std::string(" (") + reason + ")";
  if (reason != last_reason) {
    REXLOG_INFO("[prompts] {} button prompts", reason);
    last_reason = reason;
  }
  return style;
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
    size_t changed = 0;
    for (size_t k = 0; k < kXboxGlyphs.size(); ++k) {
      if (!patch.present[k]) continue;
      uint8_t* record = glyphs + Be16(glyphs + kXboxGlyphs[k] * 2);
      const uint8_t* from = style == Style::kPlayStation ? glyphs + Be16(glyphs + (kPlayStationGlyphs + k) * 2)
                                                         : patch.original[k].data();
      std::memcpy(record, from, kGlyphRecordSize);
      ++changed;
    }
    patch.style = style;
    REXLOG_INFO("[prompts] font {} (0x{:08X}): {} button glyphs as {}", i, font, changed,
                style == Style::kPlayStation ? "PlayStation's" : "Xbox's");
  }
}

void ApplyToSprites(Style style) {
  std::lock_guard lock(g_sprite_mutex);
  if (g_loaded) {
    REXLOG_INFO("[prompts] {} HUD sprites loaded ({} known), as {}", g_loaded, g_noted.size(),
                g_sprite_style == Style::kPlayStation ? "PlayStation's" : "Xbox's");
    g_loaded = 0;
  }
  if (g_sprite_style == style) return;
  g_sprite_style = style;
  size_t written = 0, gone = 0;
  for (auto it = g_noted.begin(); it != g_noted.end();) {
    uint8_t* blob = Guest(it->first);
    const SpriteArt& art = g_sprites[it->second];
    uint8_t* pixels = blob ? blob + kTextureHeader : nullptr;
    const bool holds = pixels && (!std::memcmp(pixels, art.xbox.data(), kSpritePixels) ||
                                  !std::memcmp(pixels, art.playstation.data(), kSpritePixels));
    if (!holds) {  // freed, and the memory holds something else now
      it = g_noted.erase(it);
      ++gone;
      continue;
    }
    std::memcpy(pixels, style == Style::kPlayStation ? art.playstation.data() : art.xbox.data(), kSpritePixels);
    ++written;
    ++it;
  }
  REXLOG_INFO("[prompts] {} HUD sprites as {} ({} no longer loaded)", written,
              style == Style::kPlayStation ? "PlayStation's" : "Xbox's", gone);
}

}  // namespace

namespace potcawe {

void InitButtonPrompts(const std::filesystem::path& game_data_root) {
  std::vector<uint32_t> guids;
  for (const SpritePair& pair : kSpritePairs) {
    guids.push_back(pair.xbox);
    guids.push_back(pair.playstation);
  }
  auto textures = ReadPackageTextures(game_data_root, kHudMain, guids);
  auto surface = [&textures](uint32_t guid, std::vector<uint8_t>& out) {
    auto it = textures.find(guid);
    if (it == textures.end() || it->second.size() != kTextureHeader + kSpritePixels) return false;
    const uint8_t* blob = it->second.data();
    if (Be32(blob) != kTextureMagic || Be32(blob + kTextureSize) != kSpriteSize || blob[kTextureFormat] != kSpriteFormat) {
      return false;
    }
    out.assign(blob + kTextureHeader, blob + kTextureHeader + kSpritePixels);
    return true;
  };
  std::vector<SpriteArt> sprites(kSpriteCount);
  size_t read = 0;
  for (size_t i = 0; i < kSpriteCount; ++i) {
    SpriteArt& art = sprites[i];
    if (!surface(kSpritePairs[i].xbox, art.xbox) || !surface(kSpritePairs[i].playstation, art.playstation)) {
      art = SpriteArt{};  // left out: no texture will match an empty one
      continue;
    }
    art.xbox_hash = XXH3_64bits(art.xbox.data(), art.xbox.size());
    ++read;
  }
  g_sprites = std::move(sprites);
  REXLOG_INFO("[prompts] {} of {} HUD sprite pairs read from hud_main.edb", read, kSpriteCount);
}

}  // namespace potcawe

// [[midasm_hook]] at 0x822B58C0 in sub_822B58B0, the Direct3D library's Swap and Synchronize (once a frame or more).
void PotcaweButtonPromptsFrameHook() {
  std::unique_lock lock(g_frame_mutex, std::try_to_lock);
  if (!lock.owns_lock() || g_frames++ % 4) return;
  const Style style = WantedStyle();
  ApplyToFonts(style);
  ApplyToSprites(style);
}

// [[midasm_hook]] at 0x82325010 in sub_82324F80, which decompresses a section of a package as it is read: after the
// EDL decoder (r3 nonzero: decoded), r31 = the section's memory. A texture there that is one of the Xbox sprites is
// noted, and gets its PS3 counterpart while those are shown.
void PotcaweButtonPromptsSectionHook(PPCRegister& r3, PPCRegister& r31) {
  if (!r3.u32 || g_sprites.empty()) return;
  uint8_t* blob = Guest(r31.u32);
  if (!blob || Be32(blob) != kTextureMagic || Be32(blob + kTextureSize) != kSpriteSize ||
      blob[kTextureFormat] != kSpriteFormat) {
    return;
  }
  uint8_t* pixels = blob + kTextureHeader;
  const uint64_t hash = XXH3_64bits(pixels, kSpritePixels);
  for (size_t i = 0; i < g_sprites.size(); ++i) {
    const SpriteArt& art = g_sprites[i];
    if (art.xbox.empty() || art.xbox_hash != hash || std::memcmp(pixels, art.xbox.data(), kSpritePixels)) continue;
    std::lock_guard lock(g_sprite_mutex);
    g_noted[r31.u32] = i;
    ++g_loaded;
    if (g_sprite_style == Style::kPlayStation) std::memcpy(pixels, art.playstation.data(), kSpritePixels);
    return;
  }
}
