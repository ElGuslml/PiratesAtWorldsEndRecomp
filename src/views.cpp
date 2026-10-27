// potcawe - the game's views, marked for NativeXRenderer (native_x_renderer/docs/decisions.md, D43, D47).
//
// The engine draws a frame as a series of views (sub_823B6290, once per view): the water's reflection, the
// 3D scene (its shadow maps inside it), then the HUD's 2D view; in the menus, the video's view and the menu's.
// Each view starts in sub_823B3C38, which gives the view's matrix to the vertex shaders (c0-c3), and ends
// with its post-processing (sub_823D2160), which composites its layers over the screen. NativeXRenderer is told:
//   - which views are 2D (a square parallel projection): drawn in a 16:9 box when the output is not 16:9
//     (D43, native_x_renderer::SetGameAspectBox);
//   - the 3D views and their matrices, and where their post-processing starts: DLAA jitters the main 3D view
//     and antialiases it before the post-processing (D47, native_x_renderer::BeginView / EndViewGeometry);
//   - the journal (Back): its book is a 3D view of its own with a 60 by 45 degree projection, made for the 16:9
//     screen and not widened, so it goes in the 16:9 box like the HUD drawn with it. Its two pages are composed by
//     2D views of their own camera (the parallel one with depth 0.0001 z + 0.1, which the menus and the pause menu
//     use too), each copied whole into a 1024x512 page texture: those are drawn over the whole target, or the
//     copy would take them squeezed into the box's width and leave the page's sides empty (the scene through).
//
// docs/native-x-renderer-potcawe.md has the evidence (the views' matrices and call stacks, runs 174-176).

#include "views.h"

#include <rex/logging.h>
#include <rex/ppc/context.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xmemory.h>
#include <native_x_renderer/native_x_renderer.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>

namespace {

bool g_2d_view = false;
// Views since the journal's book was last drawn: its pages are composed around it in each frame (one before
// the scene, one after the book).
int g_views_since_book = 1000;
// tan(30°) / tan(22.5°): the book's projection.
constexpr double kBookAspect = 1.3938;

float LoadFloat(const uint8_t* base, uint32_t address) {
  uint32_t value;
  std::memcpy(&value, base + address, 4);
  value = __builtin_bswap32(value);
  float f;
  std::memcpy(&f, &value, 4);
  return f;
}

}  // namespace

namespace potcawe {

bool In2DView() { return g_2d_view; }

}  // namespace potcawe

// [[midasm_hook]] at 0x823B3E48 in sub_823B3C38, which sets up a view's constants: the call that gives the
// view's matrix to the vertex shaders as c0-c3 (r5: 16 floats, rows for a row vector). A 3D view's has a
// perspective column; a 2D view's is a square parallel projection: no perspective column, w = 1, the same
// scale on x and y (the 1000x1000 camera).
void PotcaweViewHook(PPCRegister& r5) {
  if (!native_x_renderer::Renders()) return;
  auto* kernel = rex::system::kernel_state();
  if (!kernel || !kernel->memory()) return;
  const uint8_t* base = kernel->memory()->virtual_membase();
  float m[16];
  for (uint32_t i = 0; i < 16; ++i) m[i] = LoadFloat(base, r5.u32 + i * 4);
  bool parallel = m[3] == 0.0f && m[7] == 0.0f && m[11] == 0.0f && std::abs(m[15] - 1.0f) < 1e-4f;
  float scale_x = std::sqrt(m[0] * m[0] + m[4] * m[4] + m[8] * m[8]);
  float scale_y = std::sqrt(m[1] * m[1] + m[5] * m[5] + m[9] * m[9]);
  g_2d_view = parallel && scale_y > 0.0f && std::abs(scale_x / scale_y - 1.0f) < 0.01f;
  double factor = native_x_renderer::AspectFactor();
  // The journal's book: an unrotated projection of its own aspect (a widened 16:9 view's is 16:9 times the factor).
  bool book = !parallel && m[0] > 0.0f && m[5] > 0.0f && std::abs(m[1]) < 1e-4f && std::abs(m[4]) < 1e-4f &&
              std::abs(double(m[5]) / double(m[0]) - kBookAspect) < 0.01 &&
              std::abs(double(m[5]) / double(m[0]) - 16.0 / 9.0 * factor) > 0.02;
  g_views_since_book = book ? 0 : std::min(g_views_since_book + 1, 1000);
  bool page = g_2d_view && g_views_since_book <= 8 && std::abs(m[10] - 0.0001f) < 1e-5f && std::abs(m[14] - 0.1f) < 1e-4f;
  if (factor != 1.0) {
    native_x_renderer::SetGameAspectBox((g_2d_view && !page) || book);
    if (g_2d_view) {
      static std::atomic<bool> logged{false};
      if (!logged.exchange(true)) {
        REXLOG_INFO("[aspect] 2D views kept at 16:9 in the middle of the screen ({:.4f})", factor);
      }
    }
    if (book) {
      static std::atomic<bool> logged{false};
      if (!logged.exchange(true)) {
        REXLOG_INFO("[aspect] the journal's book kept at 16:9, its pages composed over the whole target");
      }
    }
  }
  native_x_renderer::BeginView(parallel ? nullptr : m);
}

// [[midasm_hook]] at 0x823B6B20 in sub_823B6290, which draws a view: the view's post-processing starts
// (sub_823D2160), which composites its layers over the whole screen.
void PotcawePostProcessingHook() {
  if (!native_x_renderer::Renders()) return;
  native_x_renderer::SetGameAspectBox(false);
  native_x_renderer::EndViewGeometry();
}
