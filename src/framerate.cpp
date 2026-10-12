// potcawe - 60 FPS: present every vblank instead of every second one.
//
// The game runs at 30 FPS only because of its Direct3D present interval. Its renderer sets
// D3DRS_PRESENTINTERVAL to D3DPRESENT_INTERVAL_TWO at start-up (sub_823F3798, from a renderer
// setting that is 2), and the XDK Direct3D library linked into the game turns that into the swap
// descriptor on every Swap and Synchronize (sub_822B58B0: interval << 8 | immediate threshold,
// handed to the vblank callback sub_822B5718, which flips the front buffer, or releases the GPU's
// WAIT_REG_MEM, only once that many vblanks have passed since the previous flip). With the SDK's
// 60 Hz vblank that is one frame every 33 ms, and the game's render thread waits for the GPU in
// between. The engine itself is frame-rate independent: its clock (sub_82343A50) measures the real
// elapsed time per frame (time base / KeQueryPerformanceFrequency), clamps it to 0.2 s and has its
// own limiter at 1/60 s, so a 60 FPS run only halves the delta time the game code sees.
//
// The hook in potcawe_functions.toml runs right after the library reads the render state, before
// it is encoded, and turns 2 into 1 when potcawe_60fps is on. Nothing else changes: the vblank
// stays at 60 Hz, D3DRS_PRESENTIMMEDIATETHRESHOLD keeps working, and GetRenderState still returns
// what the game set.

#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/ppc/context.h>

#include <atomic>
#include <cstdint>

REXCVAR_DEFINE_BOOL(potcawe_60fps, false, "Potcawe",
                    "Present every vblank (60 FPS) instead of every second one (30 FPS, the "
                    "original). The game's clock measures real time, so the game speed does not "
                    "change");

// [[midasm_hook]] 0x822B5968 (sub_822B58B0, Direct3D Swap/Synchronize): before "cmplwi r10,0",
// r10 = the D3DRS_PRESENTINTERVAL render state just loaded from the device (0 = default, 1 = one,
// 2 = two, 4 = three, 0x80000000 = immediate).
void PotcawePresentIntervalHook(PPCRegister& r10) {
  if (!REXCVAR_GET(potcawe_60fps) || r10.u32 != 2) {
    return;
  }
  static std::atomic<bool> logged{false};
  if (!logged.exchange(true)) {
    REXLOG_INFO("[framerate] present interval 2 -> 1 (potcawe_60fps)");
  }
  r10.u32 = 1;
}
