// potcawe - button prompts for the keyboard: the keys bound to the controller's buttons, as pictures
// (src/keyboard_prompts.cpp).

#pragma once

#include "prompt_pictures.h"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace potcawe::keyboard {

// A control of the controller the keyboard plays (rex::input::mnk's keybind_* cvars). Each group of four directions is
// up, down, left, right.
enum class Control : uint8_t {
  kA,
  kB,
  kX,
  kY,
  kLeftShoulder,
  kRightShoulder,
  kLeftTrigger,
  kRightTrigger,
  kStart,
  kBack,
  kGuide,
  kDpadUp,
  kDpadDown,
  kDpadLeft,
  kDpadRight,
  kLeftStickUp,
  kLeftStickDown,
  kLeftStickLeft,
  kLeftStickRight,
  kRightStickUp,
  kRightStickDown,
  kRightStickLeft,
  kRightStickRight,
};

// What a prompt names: one control; a stick's or the d-pad's up and down; or its four directions (the stick, the
// d-pad). For the last two `control` is the group's up.
struct Prompt {
  enum class Kind : uint8_t { kOne, kVertical, kWhole };
  Kind kind = Kind::kOne;
  Control control = Control::kA;
};

// Xelu's light keys (res/prompts/keys_*.png, keys.txt), and the prompts made of them from the bindings now.
class KeyArt {
 public:
  // From the executable's resources; false if they are missing.
  bool Load();

  // The prompt for a line of text: the keys side by side, `height` high (the keys centered), with a pixel of margin
  // each side, shrunk to `max_width` if wider. Empty if a control has nothing bound or a key bound has no picture.
  RgbaPicture Glyph(const Prompt& prompt, uint32_t height, uint32_t max_width) const;
  // The prompt as a HUD sprite, `size` x `size`, the keys within `inner` x `inner`: a chord in a row, four directions
  // as a cross (up over left, down, right) under their shared modifier.
  RgbaPicture Sprite(const Prompt& prompt, uint32_t size, uint32_t inner) const;
  // The keys, for the log ("Shift+Up", "W A S D", "mouse"); empty as Glyph.
  std::string Describe(const Prompt& prompt) const;

 private:
  struct Place {
    uint32_t x, y, width, height;
  };
  struct Key {
    Place glyph, sprite;
  };
  using Rows = std::vector<std::vector<std::string>>;  // pictures by name, in rows

  Rows Layout(const Prompt& prompt) const;
  RgbaPicture Row(const std::vector<std::string>& names, bool sprite, uint32_t gap) const;

  RgbaPicture glyph_atlas_, sprite_atlas_;
  std::map<std::string, Key> keys_;
};

}  // namespace potcawe::keyboard
