// potcawe - the button prompts' pictures: embedded in the executable (res/game.rc), and put together
// (src/prompt_pictures.cpp).

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace potcawe {

// R8G8B8A8, straight alpha, rows top down.
struct RgbaPicture {
  std::vector<uint8_t> rgba;
  uint32_t width = 0, height = 0;

  bool empty() const { return !width || !height; }
};

// An RCDATA resource of the executable, decoded (PNG); empty if missing or not a picture.
RgbaPicture LoadEmbeddedPicture(const wchar_t* name);
// An RCDATA resource of the executable as it is; empty if missing.
std::string LoadEmbeddedText(const wchar_t* name);

// A transparent picture.
RgbaPicture BlankPicture(uint32_t width, uint32_t height);
// The part of `picture` at (x, y), `width` x `height` (clipped to it).
RgbaPicture CropPicture(const RgbaPicture& picture, uint32_t x, uint32_t y, uint32_t width, uint32_t height);
// `source` over `target` at (x, y) (clipped).
void BlendPicture(RgbaPicture& target, const RgbaPicture& source, int32_t x, int32_t y);
// `picture` made smaller, each pixel the average of its footprint (alpha weighted). A larger size is not made.
RgbaPicture ShrinkPicture(const RgbaPicture& picture, uint32_t width, uint32_t height);

}  // namespace potcawe
