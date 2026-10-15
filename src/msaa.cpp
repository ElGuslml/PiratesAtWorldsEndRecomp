// potcawe - render without multisampling, and so in one EDRAM tile (launcher: Antialiasing).
//
// The game's renderer creates its EDRAM surfaces through its own wrapper of XGSetSurfaceHeader
// (sub_824D5750), with the multisample type taken from a table indexed by the engine's
// antialiasing level (0 = none, 1 = 2 samples, 2 = 4 samples; table at 0x8255E254). The scene is
// rendered with 2x MSAA into surfaces that do not fit the 10 MB EDRAM at 1280x720, so the engine
// renders it in two tiles of 416 and 368 rows (the same 962 draws twice, with a resolve of each).
// On the PC the EDRAM is virtual and multisampling costs about 3 ms per frame at a 2x2 render
// scale, the second tile another 3-4 ms. With potcawe_no_msaa the hooks below pass
// D3DMULTISAMPLE_NONE at the six surface creations (three functions create the surfaces) and
// force the number of tiles, which the engine computes from the antialiasing level, to one.
// Dock of Port Royal at 2x2 and 60 FPS on an RTX 4070 Ti: GPU 89 % -> 59 %. The image loses only
// the 2x edge antialiasing. The sun flare also reads the level (see PotcaweFlareAaLevelHook).
// NativeXRenderer draws each pass once at its surface's size and does not join the two tiles yet, so
// with it the hooks always apply, whatever potcawe_no_msaa says.

#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/ppc/context.h>
#include <native_x_renderer/native_x_renderer.h>

#include <atomic>
#include <cstdint>

REXCVAR_DEFINE_BOOL(potcawe_no_msaa, false, "Potcawe",
                    "Create the game's render surfaces without multisampling (no 2x MSAA edge "
                    "antialiasing; less GPU work at high render scales). Always on with NativeXRenderer");

namespace {

// Why the surfaces are created without multisampling, or nullptr if they are not.
const char* NoMsaaReason() {
  if (native_x_renderer::Renders()) return "NativeXRenderer";
  return REXCVAR_GET(potcawe_no_msaa) ? "potcawe_no_msaa" : nullptr;
}

}  // namespace

// [[midasm_hook]] at the bl sub_824D5750 of sub_823FBC90, sub_823F98D8 and sub_823F9058: r6 = the
// D3DMULTISAMPLE_TYPE of the surface being created.
void PotcaweMsaaHook(PPCRegister& r6) {
  const char* reason = NoMsaaReason();
  if (!reason || r6.u32 == 0) {
    return;
  }
  static std::atomic<int> logged{0};
  if (logged.fetch_add(1) < 8) {
    REXLOG_INFO("[msaa] surface multisample type {} -> 0 ({})", r6.u32, reason);
  }
  r6.u32 = 0;
}

// [[midasm_hook]] 0x823FBE20 (sub_823FBC90, after "add r3,r10,r11"): r3 = the number of EDRAM
// tiles the scene is rendered in, ceil(surface bytes * (1 << antialiasing level) / 10 MB) - 2 at
// 1280x720 with 2x MSAA. Without multisampling the color and depth surfaces fit in one, but the
// engine still sizes them from the level, so the count is forced to 1 here.
void PotcaweTileCountHook(PPCRegister& r3) {
  const char* reason = NoMsaaReason();
  if (!reason || r3.u32 <= 1) {
    return;
  }
  static std::atomic<bool> logged{false};
  if (!logged.exchange(true)) {
    REXLOG_INFO("[msaa] scene tiles {} -> 1 ({})", r3.u32, reason);
  }
  r3.u32 = 1;
}

// [[midasm_hook]] 0x824522E4 (sub_824521F8, after bl sub_823B1E58): r3 = the antialiasing level.
// The sun flare's strength is the fraction of a quad at the sun that its occlusion query counts,
// and the engine multiplies the samples it expects by 1, 2 or 4 for the level. Without
// multisampling the query counts one sample per pixel, so the flare sees level 0; otherwise it
// would reach only half its strength.
void PotcaweFlareAaLevelHook(PPCRegister& r3) {
  if (NoMsaaReason()) {
    r3.u32 = 0;
  }
}
