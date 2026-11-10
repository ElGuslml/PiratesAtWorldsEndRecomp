// potcawe - see dxt5_texture.h.
//
// The game's textures are Xenos 2D textures: tiled in 32x32-block tiles (texture_util::GetTiledOffset2D), mips after the
// base level with the levels of 16 texels or less packed into one tail (GetGuestTextureLayout, GetPackedMipOffset), the
// 16-bit words of each DXT5 block byte swapped (their 8-in-16 endianness). Checked against the game's own: the font
// atlases' and the HUD sprites' levels decode where the layout puts them (local_docs/12-button-glyphs.md).
//
// A block is encoded the plain way: the alpha's range in 8 steps, the colors on the line of their principal axis
// between the two extreme pixels, both inset by a sixteenth. Good enough for flat icons.

#include "dxt5_texture.h"

#include <rex/graphics/pipeline/texture/util.h>
#include <rex/graphics/xenos.h>

#include <algorithm>
#include <cmath>
#include <cstring>

namespace potcawe {

namespace {

namespace texture_util = rex::graphics::texture_util;
namespace xenos = rex::graphics::xenos;

constexpr uint32_t kTextureMagic = 0xAF000008;
constexpr uint32_t kHeaderSize = 0x1000;
constexpr uint32_t kBlockBytes = 16, kBlockBytesLog2 = 4;

struct Pixel {
  uint8_t r, g, b, a;
};

uint32_t Be32(const uint8_t* p) {
  return uint32_t(p[0]) << 24 | uint32_t(p[1]) << 16 | uint32_t(p[2]) << 8 | uint32_t(p[3]);
}

void SwapWords(uint8_t* block) {
  for (uint32_t i = 0; i < kBlockBytes; i += 2) std::swap(block[i], block[i + 1]);
}

void Expand565(uint32_t c, int out[3]) {
  out[0] = int((c >> 11) & 31) * 255 / 31;
  out[1] = int((c >> 5) & 63) * 255 / 63;
  out[2] = int(c & 31) * 255 / 31;
}

uint32_t Pack565(float r, float g, float b) {
  auto q = [](float v, int levels) { return uint32_t(std::clamp(std::lround(v * levels / 255.0f), 0L, long(levels))); };
  return q(r, 31) << 11 | q(g, 63) << 5 | q(b, 31);
}

void DecodeBlock(const uint8_t* stored, Pixel out[16]) {
  uint8_t b[kBlockBytes];
  std::memcpy(b, stored, kBlockBytes);
  SwapWords(b);
  int alpha[8] = {b[0], b[1]};
  if (alpha[0] > alpha[1]) {
    for (int i = 1; i < 7; ++i) alpha[i + 1] = ((7 - i) * alpha[0] + i * alpha[1]) / 7;
  } else {
    for (int i = 1; i < 5; ++i) alpha[i + 1] = ((5 - i) * alpha[0] + i * alpha[1]) / 5;
    alpha[6] = 0;
    alpha[7] = 255;
  }
  uint64_t alpha_bits = 0;
  for (int i = 0; i < 6; ++i) alpha_bits |= uint64_t(b[2 + i]) << (8 * i);
  int colors[4][3];
  Expand565(uint32_t(b[8] | b[9] << 8), colors[0]);
  Expand565(uint32_t(b[10] | b[11] << 8), colors[1]);
  for (int c = 0; c < 3; ++c) {
    colors[2][c] = (2 * colors[0][c] + colors[1][c]) / 3;
    colors[3][c] = (colors[0][c] + 2 * colors[1][c]) / 3;
  }
  const uint32_t color_bits = uint32_t(b[12] | b[13] << 8 | b[14] << 16 | uint32_t(b[15]) << 24);
  for (int i = 0; i < 16; ++i) {
    const int* color = colors[(color_bits >> (2 * i)) & 3];
    out[i] = {uint8_t(color[0]), uint8_t(color[1]), uint8_t(color[2]), uint8_t(alpha[(alpha_bits >> (3 * i)) & 7])};
  }
}

void EncodeBlock(const Pixel in[16], uint8_t* stored) {
  uint8_t b[kBlockBytes] = {};

  // Alpha: its range in 8 steps (alpha0 > alpha1), or one value.
  int low = 255, high = 0;
  for (int i = 0; i < 16; ++i) {
    low = std::min<int>(low, in[i].a);
    high = std::max<int>(high, in[i].a);
  }
  b[0] = uint8_t(high);
  b[1] = uint8_t(low);
  if (high > low) {
    int alpha[8] = {high, low};
    for (int i = 1; i < 7; ++i) alpha[i + 1] = ((7 - i) * high + i * low) / 7;
    uint64_t bits = 0;
    for (int i = 0; i < 16; ++i) {
      int best = 0;
      for (int k = 1; k < 8; ++k) {
        if (std::abs(alpha[k] - in[i].a) < std::abs(alpha[best] - in[i].a)) best = k;
      }
      bits |= uint64_t(best) << (3 * i);
    }
    for (int i = 0; i < 6; ++i) b[2 + i] = uint8_t(bits >> (8 * i));
  }

  // Colors: the pixels that show (all of them if none does), along their principal axis.
  int count = 0;
  float mean[3] = {};
  for (int i = 0; i < 16; ++i) {
    if (in[i].a < 16) continue;
    mean[0] += in[i].r;
    mean[1] += in[i].g;
    mean[2] += in[i].b;
    ++count;
  }
  const bool all = count == 0;
  if (all) {
    for (int i = 0; i < 16; ++i) {
      mean[0] += in[i].r;
      mean[1] += in[i].g;
      mean[2] += in[i].b;
    }
    count = 16;
  }
  for (float& m : mean) m /= float(count);
  float cov[6] = {};
  for (int i = 0; i < 16; ++i) {
    if (!all && in[i].a < 16) continue;
    const float d[3] = {in[i].r - mean[0], in[i].g - mean[1], in[i].b - mean[2]};
    cov[0] += d[0] * d[0];
    cov[1] += d[0] * d[1];
    cov[2] += d[0] * d[2];
    cov[3] += d[1] * d[1];
    cov[4] += d[1] * d[2];
    cov[5] += d[2] * d[2];
  }
  float axis[3] = {1.0f, 1.0f, 1.0f};
  for (int iteration = 0; iteration < 8; ++iteration) {
    const float next[3] = {cov[0] * axis[0] + cov[1] * axis[1] + cov[2] * axis[2],
                           cov[1] * axis[0] + cov[3] * axis[1] + cov[4] * axis[2],
                           cov[2] * axis[0] + cov[4] * axis[1] + cov[5] * axis[2]};
    const float length = std::sqrt(next[0] * next[0] + next[1] * next[1] + next[2] * next[2]);
    if (length < 1e-6f) break;
    for (int c = 0; c < 3; ++c) axis[c] = next[c] / length;
  }
  float lowest = 1e9f, highest = -1e9f;
  for (int i = 0; i < 16; ++i) {
    if (!all && in[i].a < 16) continue;
    const float t = (in[i].r - mean[0]) * axis[0] + (in[i].g - mean[1]) * axis[1] + (in[i].b - mean[2]) * axis[2];
    lowest = std::min(lowest, t);
    highest = std::max(highest, t);
  }
  const float inset = (highest - lowest) / 16.0f;
  lowest += inset;
  highest -= inset;
  uint32_t c0 = Pack565(mean[0] + axis[0] * highest, mean[1] + axis[1] * highest, mean[2] + axis[2] * highest);
  uint32_t c1 = Pack565(mean[0] + axis[0] * lowest, mean[1] + axis[1] * lowest, mean[2] + axis[2] * lowest);
  if (c0 < c1) std::swap(c0, c1);
  b[8] = uint8_t(c0);
  b[9] = uint8_t(c0 >> 8);
  b[10] = uint8_t(c1);
  b[11] = uint8_t(c1 >> 8);
  int colors[4][3];
  Expand565(c0, colors[0]);
  Expand565(c1, colors[1]);
  for (int c = 0; c < 3; ++c) {
    colors[2][c] = (2 * colors[0][c] + colors[1][c]) / 3;
    colors[3][c] = (colors[0][c] + 2 * colors[1][c]) / 3;
  }
  uint32_t bits = 0;
  for (int i = 0; i < 16; ++i) {
    int best = 0, best_distance = 1 << 30;
    for (int k = 0; k < 4; ++k) {
      const int dr = colors[k][0] - in[i].r, dg = colors[k][1] - in[i].g, db = colors[k][2] - in[i].b;
      const int distance = dr * dr + dg * dg + db * db;
      if (distance < best_distance) {
        best_distance = distance;
        best = k;
      }
    }
    bits |= uint32_t(best) << (2 * i);
  }
  b[12] = uint8_t(bits);
  b[13] = uint8_t(bits >> 8);
  b[14] = uint8_t(bits >> 16);
  b[15] = uint8_t(bits >> 24);
  SwapWords(b);
  std::memcpy(stored, b, kBlockBytes);
}

texture_util::TextureGuestLayout Layout(uint32_t width, uint32_t height, uint32_t levels) {
  return texture_util::GetGuestTextureLayout(xenos::DataDimension::k2DOrStacked, (width + 31) / 32, width, height, 1,
                                             true, xenos::TextureFormat::k_DXT4_5, true, true, levels - 1);
}

}  // namespace

bool Dxt5TextureFromBlob(uint8_t* blob, size_t size, Dxt5Texture& out) {
  if (!blob || (size && size < kHeaderSize) || Be32(blob) != kTextureMagic || blob[0x2F] != 1) return false;
  const uint32_t extent = Be32(blob + 0x24);
  const uint32_t width = extent >> 16, height = extent & 0xFFFF, levels = blob[0x2D];
  if (!width || !height || width > 4096 || height > 4096 || !levels || levels > 13) return false;
  const texture_util::TextureGuestLayout layout = Layout(width, height, levels);
  const size_t base_size = layout.base.array_slice_stride_bytes;
  if (size && size < kHeaderSize + base_size + (levels > 1 ? layout.mips_total_extent_bytes : 0)) return false;
  out.base = blob + kHeaderSize;
  out.mips = out.base + base_size;
  out.width = width;
  out.height = height;
  out.levels = levels;
  return true;
}

void DrawDxt5(const Dxt5Texture& texture, uint32_t x, uint32_t y, const uint8_t* rgba, uint32_t width,
              uint32_t height, uint32_t level_count) {
  const texture_util::TextureGuestLayout layout = Layout(texture.width, texture.height, texture.levels);
  for (uint32_t level = 0; level < std::min(texture.levels, level_count); ++level) {
    const uint32_t level_width = std::max(texture.width >> level, 1u), level_height = std::max(texture.height >> level, 1u);
    const uint32_t step = 1u << level;
    const uint32_t x0 = x >> level, y0 = y >> level;
    const uint32_t x1 = std::min(level_width, (x + width + step - 1) >> level);
    const uint32_t y1 = std::min(level_height, (y + height + step - 1) >> level);
    if (x1 <= x0 || y1 <= y0) continue;

    // Where the level is: the base, a mip of its own, or the packed tail at an offset.
    uint8_t* storage = texture.base;
    uint32_t pitch_blocks = layout.base.row_pitch_bytes / kBlockBytes;
    uint32_t offset_x = 0, offset_y = 0;
    if (level) {
      const uint32_t stored = std::min(level, layout.packed_level);
      storage = texture.mips + layout.mip_offsets_bytes[stored];
      pitch_blocks = layout.mips[stored].row_pitch_bytes / kBlockBytes;
    }
    if (level >= layout.packed_level) {
      uint32_t offset_z = 0;
      texture_util::GetPackedMipOffset(texture.width, texture.height, 1, xenos::TextureFormat::k_DXT4_5, level,
                                       offset_x, offset_y, offset_z);
    }

    // The level's pixel from the picture: the average of its footprint at the base, outside the picture transparent.
    auto sample = [&](uint32_t px, uint32_t py) {
      const uint32_t fx0 = px << level, fy0 = py << level;
      const uint32_t fx1 = std::min(fx0 + step, texture.width), fy1 = std::min(fy0 + step, texture.height);
      float sum[4] = {};
      for (uint32_t sy = std::max(fy0, y); sy < std::min(fy1, y + height); ++sy) {
        for (uint32_t sx = std::max(fx0, x); sx < std::min(fx1, x + width); ++sx) {
          const uint8_t* p = rgba + (size_t(sy - y) * width + (sx - x)) * 4;
          const float a = p[3];
          sum[0] += p[0] * a;
          sum[1] += p[1] * a;
          sum[2] += p[2] * a;
          sum[3] += a;
        }
      }
      const float area = float((fx1 - fx0) * (fy1 - fy0));
      if (sum[3] <= 0.0f) return Pixel{0, 0, 0, 0};
      return Pixel{uint8_t(std::lround(sum[0] / sum[3])), uint8_t(std::lround(sum[1] / sum[3])),
                   uint8_t(std::lround(sum[2] / sum[3])), uint8_t(std::lround(sum[3] / area))};
    };

    for (uint32_t by = y0 / 4; by < (y1 + 3) / 4; ++by) {
      for (uint32_t bx = x0 / 4; bx < (x1 + 3) / 4; ++bx) {
        uint8_t* block = storage + texture_util::GetTiledOffset2D(int32_t(bx + offset_x), int32_t(by + offset_y),
                                                                  pitch_blocks, kBlockBytesLog2);
        Pixel pixels[16];
        DecodeBlock(block, pixels);
        for (uint32_t j = 0; j < 4; ++j) {
          for (uint32_t i = 0; i < 4; ++i) {
            const uint32_t px = bx * 4 + i, py = by * 4 + j;
            if (px >= x0 && px < x1 && py >= y0 && py < y1) pixels[j * 4 + i] = sample(px, py);
          }
        }
        EncodeBlock(pixels, block);
      }
    }
  }
}

}  // namespace potcawe
