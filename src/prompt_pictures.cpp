// potcawe - see prompt_pictures.h.

#include "prompt_pictures.h"

#include <rex/ui/image_decode.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <algorithm>
#include <cmath>

namespace potcawe {

namespace {

// An RCDATA resource's bytes, or null.
const uint8_t* EmbeddedBytes(const wchar_t* name, size_t& size) {
  HMODULE module = GetModuleHandleW(nullptr);
  HRSRC resource = FindResourceW(module, name, MAKEINTRESOURCEW(10));  // RT_RCDATA
  HGLOBAL loaded = resource ? LoadResource(module, resource) : nullptr;
  const void* data = loaded ? LockResource(loaded) : nullptr;
  size = data ? SizeofResource(module, resource) : 0;
  return static_cast<const uint8_t*>(data);
}

// Weights of the source pixels each target pixel covers along one axis, `from` pixels onto `to` (to <= from).
struct Span {
  uint32_t first = 0;
  std::vector<float> weights;
};
std::vector<Span> Spans(uint32_t from, uint32_t to) {
  std::vector<Span> spans(to);
  const double scale = double(from) / double(to);
  for (uint32_t i = 0; i < to; ++i) {
    const double begin = i * scale, end = (i + 1) * scale;
    Span& span = spans[i];
    span.first = uint32_t(begin);
    for (uint32_t s = span.first; s < from && s < end; ++s) {
      span.weights.push_back(float(std::min<double>(end, s + 1) - std::max<double>(begin, s)));
    }
  }
  return spans;
}

}  // namespace

RgbaPicture LoadEmbeddedPicture(const wchar_t* name) {
  RgbaPicture picture;
  size_t size = 0;
  const uint8_t* data = EmbeddedBytes(name, size);
  if (!data) return picture;
  int width = 0, height = 0;
  picture.rgba = rex::ui::DecodeImageRGBA(data, size, width, height);
  if (!picture.rgba.empty() && width > 0 && height > 0) {
    picture.width = uint32_t(width);
    picture.height = uint32_t(height);
  }
  return picture;
}

std::string LoadEmbeddedText(const wchar_t* name) {
  size_t size = 0;
  const uint8_t* data = EmbeddedBytes(name, size);
  return data ? std::string(reinterpret_cast<const char*>(data), size) : std::string();
}

RgbaPicture BlankPicture(uint32_t width, uint32_t height) {
  RgbaPicture picture;
  picture.width = width;
  picture.height = height;
  picture.rgba.assign(size_t(width) * height * 4, 0);
  return picture;
}

RgbaPicture CropPicture(const RgbaPicture& picture, uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
  width = x < picture.width ? std::min(width, picture.width - x) : 0;
  height = y < picture.height ? std::min(height, picture.height - y) : 0;
  RgbaPicture out = BlankPicture(width, height);
  for (uint32_t row = 0; row < height; ++row) {
    std::copy_n(picture.rgba.data() + (size_t(y + row) * picture.width + x) * 4, size_t(width) * 4,
                out.rgba.data() + size_t(row) * width * 4);
  }
  return out;
}

void BlendPicture(RgbaPicture& target, const RgbaPicture& source, int32_t x, int32_t y) {
  for (uint32_t sy = 0; sy < source.height; ++sy) {
    const int32_t ty = y + int32_t(sy);
    if (ty < 0 || ty >= int32_t(target.height)) continue;
    for (uint32_t sx = 0; sx < source.width; ++sx) {
      const int32_t tx = x + int32_t(sx);
      if (tx < 0 || tx >= int32_t(target.width)) continue;
      const uint8_t* s = source.rgba.data() + (size_t(sy) * source.width + sx) * 4;
      uint8_t* t = target.rgba.data() + (size_t(ty) * target.width + tx) * 4;
      const float sa = s[3] / 255.0f, ta = t[3] / 255.0f;
      const float a = sa + ta * (1.0f - sa);
      if (a <= 0.0f) continue;
      for (int c = 0; c < 3; ++c) t[c] = uint8_t(std::lround((s[c] * sa + t[c] * ta * (1.0f - sa)) / a));
      t[3] = uint8_t(std::lround(a * 255.0f));
    }
  }
}

RgbaPicture ShrinkPicture(const RgbaPicture& picture, uint32_t width, uint32_t height) {
  if (picture.empty() || !width || !height || width > picture.width || height > picture.height) return picture;
  const std::vector<Span> columns = Spans(picture.width, width), rows = Spans(picture.height, height);
  // Premultiplied, across then down.
  std::vector<float> across(size_t(width) * picture.height * 4);
  for (uint32_t y = 0; y < picture.height; ++y) {
    for (uint32_t x = 0; x < width; ++x) {
      float sum[4] = {}, total = 0;
      const Span& span = columns[x];
      for (size_t k = 0; k < span.weights.size(); ++k) {
        const uint8_t* p = picture.rgba.data() + (size_t(y) * picture.width + span.first + k) * 4;
        const float w = span.weights[k], a = p[3] * w;
        sum[0] += p[0] * a;
        sum[1] += p[1] * a;
        sum[2] += p[2] * a;
        sum[3] += a;
        total += w;
      }
      float* out = across.data() + (size_t(y) * width + x) * 4;
      for (int c = 0; c < 4; ++c) out[c] = total > 0 ? sum[c] / total : 0;
    }
  }
  RgbaPicture out = BlankPicture(width, height);
  for (uint32_t y = 0; y < height; ++y) {
    for (uint32_t x = 0; x < width; ++x) {
      float sum[4] = {}, total = 0;
      const Span& span = rows[y];
      for (size_t k = 0; k < span.weights.size(); ++k) {
        const float* p = across.data() + (size_t(span.first + k) * width + x) * 4;
        const float w = span.weights[k];
        for (int c = 0; c < 4; ++c) sum[c] += p[c] * w;
        total += w;
      }
      uint8_t* t = out.rgba.data() + (size_t(y) * width + x) * 4;
      const float alpha = total > 0 ? sum[3] / total : 0;
      if (sum[3] > 0) {
        for (int c = 0; c < 3; ++c) t[c] = uint8_t(std::clamp(std::lround(sum[c] / sum[3]), 0L, 255L));
      }
      t[3] = uint8_t(std::clamp(std::lround(alpha), 0L, 255L));
    }
  }
  return out;
}

}  // namespace potcawe
