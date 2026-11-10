// potcawe - see keyboard_prompts.h.
//
// A binding (keybind_*) is a list of alternatives, "Q,I", each a key with optional modifiers, "Shift+Up"
// (rex::input::mnk). A prompt shows the first alternative whose keys all have a picture: its modifiers, then its key.
// The pictures are Xelu's light keys (CC0), prepared by res/prompts/make_prompts.py at two scales: for a glyph (a
// square key 32 pixels high; Enter and Backspace as their symbols, their words being too small there) and for a sprite
// (a square key 96 pixels high). The bindings are read when a prompt is made.

#include "keyboard_prompts.h"

#include <rex/cvar.h>

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string_view>

namespace potcawe::keyboard {

namespace {

constexpr uint32_t kSpriteGap = 6;

const char* BindingName(Control control) {
  switch (control) {
    case Control::kA: return "keybind_a";
    case Control::kB: return "keybind_b";
    case Control::kX: return "keybind_x";
    case Control::kY: return "keybind_y";
    case Control::kLeftShoulder: return "keybind_left_shoulder";
    case Control::kRightShoulder: return "keybind_right_shoulder";
    case Control::kLeftTrigger: return "keybind_left_trigger";
    case Control::kRightTrigger: return "keybind_right_trigger";
    case Control::kStart: return "keybind_start";
    case Control::kBack: return "keybind_back";
    case Control::kGuide: return "keybind_guide";
    case Control::kDpadUp: return "keybind_dpad_up";
    case Control::kDpadDown: return "keybind_dpad_down";
    case Control::kDpadLeft: return "keybind_dpad_left";
    case Control::kDpadRight: return "keybind_dpad_right";
    case Control::kLeftStickUp: return "keybind_lstick_up";
    case Control::kLeftStickDown: return "keybind_lstick_down";
    case Control::kLeftStickLeft: return "keybind_lstick_left";
    case Control::kLeftStickRight: return "keybind_lstick_right";
    case Control::kRightStickUp: return "keybind_rstick_up";
    case Control::kRightStickDown: return "keybind_rstick_down";
    case Control::kRightStickLeft: return "keybind_rstick_left";
    case Control::kRightStickRight: return "keybind_rstick_right";
  }
  return "";
}

// The SDK's key names (rex::ui::ParseVirtualKey) whose picture has another name; letters, digits and F1-F12 have their
// own, the numpad's digits the digits'.
constexpr std::pair<std::string_view, std::string_view> kPictureNames[] = {
    {"Backtick", "Tilda"},        {"LBracket", "Bracket_Left"},   {"RBracket", "Bracket_Right"},
    {"Escape", "Esc"},            {"Return", "Enter"},            {"Delete", "Del"},
    {"PageUp", "Page_Up"},        {"PageDown", "Page_Down"},      {"Left", "Arrow_Left"},
    {"Right", "Arrow_Right"},     {"Up", "Arrow_Up"},             {"Down", "Arrow_Down"},
    {"LShift", "Shift"},          {"RShift", "Shift"},            {"Control", "Ctrl"},
    {"LCtrl", "Ctrl"},            {"RCtrl", "Ctrl"},              {"LAlt", "Alt"},
    {"RAlt", "Alt"},              {"NumpadEnter", "Enter"},       {"NumpadPlus", "Plus"},
    {"NumpadMinus", "Minus"},     {"NumpadStar", "Asterisk"},     {"NumpadSlash", "Slash"},
    {"PrintScreen", "Print_Screen"}, {"CapsLock", "Caps_Lock"},   {"NumLock", "Num_Lock"},
    {"LMB", "Mouse_Left"},        {"RMB", "Mouse_Right"},         {"MMB", "Mouse_Middle"},
};
constexpr std::string_view kMouse = "Mouse_Simple";

std::string_view Trim(std::string_view s) {
  while (!s.empty() && s.front() == ' ') s.remove_prefix(1);
  while (!s.empty() && s.back() == ' ') s.remove_suffix(1);
  return s;
}

std::string PictureName(std::string_view key) {
  for (const auto& [name, picture] : kPictureNames) {
    if (name == key) return std::string(picture);
  }
  if (key.size() == 7 && key.substr(0, 6) == "Numpad" && std::isdigit(static_cast<unsigned char>(key[6]))) {
    return std::string(key.substr(6));
  }
  return std::string(key);
}

// A binding's alternative as pictures: its modifiers', then its key's.
struct Chord {
  std::vector<std::string> pictures;
};

}  // namespace

bool KeyArt::Load() {
  glyph_atlas_ = LoadEmbeddedPicture(L"PROMPT_KEYS_GLYPH");
  sprite_atlas_ = LoadEmbeddedPicture(L"PROMPT_KEYS_SPRITE");
  std::istringstream index(LoadEmbeddedText(L"PROMPT_KEYS_INDEX"));
  keys_.clear();
  std::string name;
  Key key;
  while (index >> name >> key.glyph.x >> key.glyph.y >> key.glyph.width >> key.glyph.height >> key.sprite.x >>
         key.sprite.y >> key.sprite.width >> key.sprite.height) {
    keys_[name] = key;
  }
  return !glyph_atlas_.empty() && !sprite_atlas_.empty() && !keys_.empty();
}

KeyArt::Rows KeyArt::Layout(const Prompt& prompt) const {
  auto has = [this](const std::string& picture) { return keys_.count(picture) != 0; };
  // The first alternative of a control's binding whose keys all have pictures.
  auto chord = [&](Control control) {
    const std::string binding = rex::cvar::Query<std::string>(BindingName(control));
    std::string_view rest = binding;
    while (!rest.empty()) {
      const size_t comma = rest.find(',');
      const std::string_view token = Trim(rest.substr(0, comma));
      rest = comma == std::string_view::npos ? std::string_view() : rest.substr(comma + 1);
      if (token.empty()) continue;
      Chord out;
      std::string_view part = token;
      bool usable = true;
      for (size_t plus; usable && (plus = part.find('+')) != std::string_view::npos && plus > 0;) {
        const std::string_view modifier = part.substr(0, plus);
        if (modifier == "Shift") {
          out.pictures.push_back("Shift");
        } else if (modifier == "Ctrl" || modifier == "Control") {
          out.pictures.push_back("Ctrl");
        } else if (modifier == "Alt") {
          out.pictures.push_back("Alt");
        } else {
          usable = false;  // the SDK does not take it either
        }
        part.remove_prefix(plus + 1);
      }
      out.pictures.push_back(PictureName(part));
      if (usable && std::all_of(out.pictures.begin(), out.pictures.end(), has)) return out;
    }
    return Chord{};
  };

  if (prompt.kind == Prompt::Kind::kOne) {
    Chord one = chord(prompt.control);
    return one.pictures.empty() ? Rows{} : Rows{one.pictures};
  }
  const bool right_stick = prompt.control == Control::kRightStickUp;
  if (right_stick && rex::cvar::Query<bool>("mnk_mouse") && has(std::string(kMouse))) {
    return Rows{{std::string(kMouse)}};
  }
  const size_t count = prompt.kind == Prompt::Kind::kWhole ? 4 : 2;
  std::vector<Chord> chords;  // up, down, left, right
  for (size_t i = 0; i < count; ++i) {
    chords.push_back(chord(static_cast<Control>(uint8_t(prompt.control) + i)));
    if (chords.back().pictures.empty()) return {};
  }
  auto modifiers = [](const Chord& c) { return std::vector<std::string>(c.pictures.begin(), c.pictures.end() - 1); };
  const std::vector<std::string> shared = modifiers(chords[0]);
  const bool same = std::all_of(chords.begin(), chords.end(), [&](const Chord& c) { return modifiers(c) == shared; });
  // Each direction's key, under the modifiers they share, or each with its own.
  auto keys = [&](size_t i) {
    return same ? std::vector<std::string>{chords[i].pictures.back()} : chords[i].pictures;
  };
  Rows rows;
  if (same && !shared.empty()) rows.push_back(shared);
  rows.push_back(keys(0));
  if (count == 2) {
    rows.push_back(keys(1));
  } else {
    std::vector<std::string> bottom = keys(2);
    for (const std::string& name : keys(1)) bottom.push_back(name);
    for (const std::string& name : keys(3)) bottom.push_back(name);
    rows.push_back(bottom);
  }
  return rows;
}

RgbaPicture KeyArt::Row(const std::vector<std::string>& names, bool sprite, uint32_t gap) const {
  uint32_t width = 0, height = 0;
  for (const std::string& name : names) {
    const Place& place = sprite ? keys_.at(name).sprite : keys_.at(name).glyph;
    width += place.width + (width ? gap : 0);
    height = std::max(height, place.height);
  }
  RgbaPicture row = BlankPicture(width, height);
  uint32_t x = 0;
  for (const std::string& name : names) {
    const Place& place = sprite ? keys_.at(name).sprite : keys_.at(name).glyph;
    const RgbaPicture key =
        CropPicture(sprite ? sprite_atlas_ : glyph_atlas_, place.x, place.y, place.width, place.height);
    BlendPicture(row, key, int32_t(x), int32_t((height - place.height) / 2));
    x += place.width + gap;
  }
  return row;
}

RgbaPicture KeyArt::Glyph(const Prompt& prompt, uint32_t height, uint32_t max_width) const {
  const Rows rows = Layout(prompt);
  if (rows.empty() || max_width < 3) return {};
  std::vector<std::string> names;  // the cross read across: the modifiers, up, left, down, right
  for (const auto& row : rows) names.insert(names.end(), row.begin(), row.end());
  RgbaPicture keys = Row(names, false, 2);
  if (keys.width + 2 > max_width || keys.height > height) {
    const double scale = std::min(double(max_width - 2) / keys.width, double(height) / keys.height);
    keys = ShrinkPicture(keys, std::max(1u, uint32_t(keys.width * scale)), std::max(1u, uint32_t(keys.height * scale)));
  }
  RgbaPicture glyph = BlankPicture(keys.width + 2, height);
  BlendPicture(glyph, keys, 1, int32_t((height - keys.height) / 2));
  return glyph;
}

RgbaPicture KeyArt::Sprite(const Prompt& prompt, uint32_t size, uint32_t inner) const {
  const Rows rows = Layout(prompt);
  if (rows.empty() || !inner || inner > size) return {};
  std::vector<RgbaPicture> lines;
  uint32_t width = 0, height = 0;
  for (const auto& row : rows) {
    lines.push_back(Row(row, true, kSpriteGap));
    width = std::max(width, lines.back().width);
    height += lines.back().height + (height ? kSpriteGap : 0);
  }
  RgbaPicture keys = BlankPicture(width, height);
  uint32_t y = 0;
  for (const RgbaPicture& line : lines) {
    BlendPicture(keys, line, int32_t((width - line.width) / 2), int32_t(y));
    y += line.height + kSpriteGap;
  }
  if (keys.width > inner || keys.height > inner) {
    const double scale = std::min(double(inner) / keys.width, double(inner) / keys.height);
    keys = ShrinkPicture(keys, std::max(1u, uint32_t(keys.width * scale)), std::max(1u, uint32_t(keys.height * scale)));
  }
  RgbaPicture sprite = BlankPicture(size, size);
  BlendPicture(sprite, keys, int32_t((size - keys.width) / 2), int32_t((size - keys.height) / 2));
  return sprite;
}

std::string KeyArt::Describe(const Prompt& prompt) const {
  const Rows rows = Layout(prompt);
  std::string text;
  for (const auto& row : rows) {
    for (const std::string& name : row) text += (text.empty() ? "" : " ") + name;
  }
  return text;
}

}  // namespace potcawe::keyboard
