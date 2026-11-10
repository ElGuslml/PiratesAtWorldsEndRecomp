// potcawe - drawing into the game's DXT5 textures where they sit in memory (src/dxt5_texture.cpp).

#pragma once

#include <climits>
#include <cstddef>
#include <cstdint>

namespace potcawe {

// A 2D tiled DXT5 texture with packed mips, as the game's textures are once loaded: its base level at `base`, its
// mips from `mips`, in host memory (guest memory translated, or a copy).
struct Dxt5Texture {
  uint8_t* base = nullptr;
  uint8_t* mips = nullptr;
  uint32_t width = 0, height = 0;
  uint32_t levels = 1;  // the base included
};

// The game's texture blob (level_packages.cpp): a 4 KB header (0xAF000008; the width and height at +0x24, the level
// count at +0x2D, the format at +0x2F, 1 for DXT5) and the surface after it, the mips from the base level's end. False
// if it is not a 2D DXT5 texture, or `size` (when not 0) does not hold its surface.
bool Dxt5TextureFromBlob(uint8_t* blob, size_t size, Dxt5Texture& out);

// Puts `rgba` (`width` x `height`, R8G8B8A8, straight alpha) at (x, y) of the base level, in place of what is there,
// and the same area of the first `level_count` levels (all by default), box-filtered from it. For the mips the pixels
// around the picture count as transparent: draw a picture over all of the texture, or where it is empty for as many
// levels as a mip pixel's footprint stays in the empty part.
void DrawDxt5(const Dxt5Texture& texture, uint32_t x, uint32_t y, const uint8_t* rgba, uint32_t width,
              uint32_t height, uint32_t level_count = UINT32_MAX);

}  // namespace potcawe
