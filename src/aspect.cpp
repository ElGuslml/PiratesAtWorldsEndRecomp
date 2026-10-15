// potcawe - any aspect ratio without bars, with XenosNative (xenos_native/docs/decisions.md, D43).
//
// XenosNative renders the game's 1280x720 image at a separate scale per axis, so that it fills the output
// (xenos_native::AspectFactor: the output's aspect ratio over 16:9). The image is then stretched by that
// factor, and the hooks below correct the game by the same factor:
//   - the 3D views (16:9 perspective cameras) see more: to the sides on a wider output (ultrawide), above
//     and below on a taller one (16:10, the Steam Deck);
//   - the 2D views (the HUD, the menus, the videos behind them) are laid out in a 1000x1000 parallel camera
//     stretched to the 16:9 screen. XenosNative draws them in a 16:9 box in the middle of the screen
//     (xenos_native::SetGameAspectBox, from src/views.cpp), with the 3D view around them. Their camera is
//     left alone: the game's text layout measures in screen pixels through it, and drops words if it is
//     narrowed;
//   - the screen rectangle some 3D effects are clipped to (the sun's haze at Port Royal) falls short of the
//     wider view, and is opened on that axis.
//
// docs/xenos-native-potcawe.md has the evidence (the views' matrices, the haze's rectangle, runs 170-184).

#include <rex/logging.h>
#include <rex/ppc/context.h>
#include <xenos_native/xenos_native.h>

#include <atomic>
#include <cmath>

#include "views.h"

namespace {

// A 16:9 view: tan(width / 2) / tan(height / 2) between these.
constexpr double kMinAspect = 1.70, kMaxAspect = 1.86;

void LogOnce(std::atomic<bool>& logged, const char* what, double factor) {
  if (!logged.exchange(true)) {
    REXLOG_INFO("[aspect] {} ({:.4f}, {} output)", what, factor, factor > 1.0 ? "wider" : "taller");
  }
}

}  // namespace

// [[midasm_hook]] at 0x82328B84 (fmr f0,f1), the perspective branch of sub_823288C8, which builds the engine
// camera's projection and culling frustum: f29 = tan(width / 2), f1 = tan(height / 2), just computed from
// the angles (+12, +8). The branch then gives them to sub_82328428 (the frustum) and stores
// P00 = 1 / tan(width / 2) at +432 and P11 = 1 / tan(height / 2) at +452, so the game's culling follows.
void PotcawePerspectiveAspectHook(PPCRegister& f1, PPCRegister& f29) {
  double factor = xenos_native::AspectFactor();
  if (factor == 1.0 || f1.f64 == 0.0) return;
  double aspect = std::abs(f29.f64 / f1.f64);
  if (aspect < kMinAspect || aspect > kMaxAspect) return;
  static std::atomic<bool> logged{false};
  LogOnce(logged, "3D views widened", factor);
  if (factor > 1.0) {
    f29.f64 = double(float(f29.f64 * factor));
  } else {
    f1.f64 = double(float(f1.f64 / factor));
  }
}

// [[midasm_hook]] at 0x823F8838 in sub_823F8750, which sets the scissor from a draw's screen rectangle
// (+4 left, +8 top, +12 width, +16 height, from 0 to 1 of the viewport): f12, f10, f11, f9. In a 3D view
// the rectangle bounds an effect for fill rate, computed for the 16:9 view; on the wider axis it is opened
// to the whole viewport. Clipping it there changes nothing at 16:9 (runs 182, 183).
void PotcaweScissorAspectHook(PPCRegister& f9, PPCRegister& f10, PPCRegister& f11, PPCRegister& f12) {
  double factor = xenos_native::AspectFactor();
  if (factor == 1.0 || potcawe::In2DView()) return;
  if (factor > 1.0) {
    f12.f64 = 0.0;
    f11.f64 = 1.0;
  } else {
    f10.f64 = 0.0;
    f9.f64 = 1.0;
  }
}
