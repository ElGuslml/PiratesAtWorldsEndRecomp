// potcawe - the game's post-processing effects, each of which can be turned off (docs/screen-effects.md).
//
// After a view's 3D the engine (EXBasePostProcessManager, sub_823D2160) goes through its list of effects, made by
// sub_823D0B50: HDR, depth of field, heat haze, the game's fullscreen antialiasing, radial blur and motion blur, in that
// order. An effect runs when its mask ([effect+8]) has a bit in common with the view's flags: the flags of the view's
// settings (XSgPostProcessSettings), and the HDR bit (2) when HDR is on. The flags are read into r26 and used for
// nothing else, so taking a bit out of r26 is the same as the view not asking for that effect. Levels, cameras and
// timeline nodes keep driving them as usual: an effect turned back on works from the next frame.
//
// The HDR effect cannot go that way: its final pass (sub_8245D4A0) is what draws the scene into the output, and the
// bloom, the eye's adaptation, the tone mapping and the color adjustment are parts of it. The other two switches work
// inside that pass:
// - The bloom. Its blurred copy is added with the setting "HDR Bloom Colour" (effect+96, copied from the settings by
//   sub_8248A800 every frame), which the pass loads into pixel constant c19. The hook gives it a black color in place
//   of that; the setting itself stays. The passes that prepare the copy still run.
// - The color adjustment ("Colour Adjust Enable", effect+176) picks the shader: HDRFinalPass_CA, with the color matrix,
//   or HDRFinalPass. sub_8245DCD0 does the same for HDREngineToSRGB. The hooks make both see it off.
//
// The game's fullscreen antialiasing (bit 4) has no switch: the launcher's Antialiasing row covers it.

#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/ppc/context.h>

#include <atomic>
#include <cstdint>

REXCVAR_DEFINE_BOOL(potcawe_fx_bloom, true, "Potcawe",
                    "Screen effects: the bloom, the glow around lights and bright surfaces; false = off");
REXCVAR_DEFINE_BOOL(potcawe_fx_depth_of_field, true, "Potcawe",
                    "Screen effects: the depth of field, what is far from the focus blurred; false = off");
REXCVAR_DEFINE_BOOL(potcawe_fx_color, true, "Potcawe",
                    "Screen effects: the color adjustment each place sets (saturation, contrast, hue, temperature); "
                    "false = off");
REXCVAR_DEFINE_BOOL(potcawe_fx_heat_haze, true, "Potcawe",
                    "Screen effects: the heat haze, the image shimmering as through hot air; false = off");
REXCVAR_DEFINE_BOOL(potcawe_fx_radial_blur, true, "Potcawe",
                    "Screen effects: the radial blur, outward from the center of the image; false = off");
REXCVAR_DEFINE_BOOL(potcawe_fx_motion_blur, true, "Potcawe",
                    "Screen effects: the motion blur of the camera's fast moves; false = off");

namespace {

// The effects' masks ([effect+8], their constructors).
constexpr uint32_t kDepthOfField = 0x01;
constexpr uint32_t kHeatHaze = 0x08;
constexpr uint32_t kRadialBlur = 0x10;
constexpr uint32_t kMotionBlur = 0x20;

// The views' effects turned off, for the log: what each hook is told once, and again if it changes.
void LogOff(uint32_t off) {
  static std::atomic<uint32_t> logged{0};
  if (logged.exchange(off) != off) {
    REXLOG_INFO("[screen fx] effects turned off: flags 0x{:02X} (potcawe_fx_*)", off);
  }
}

}  // namespace

// [[midasm_hook]] 0x823D235C (sub_823D2160, the post-processing): r26 = the view's flags, the HDR bit already added.
void PotcaweScreenFxFlagsHook(PPCRegister& r26) {
  uint32_t off = 0;
  off |= REXCVAR_GET(potcawe_fx_depth_of_field) ? 0 : kDepthOfField;
  off |= REXCVAR_GET(potcawe_fx_heat_haze) ? 0 : kHeatHaze;
  off |= REXCVAR_GET(potcawe_fx_radial_blur) ? 0 : kRadialBlur;
  off |= REXCVAR_GET(potcawe_fx_motion_blur) ? 0 : kMotionBlur;
  if (!off) {
    return;
  }
  LogOff(off);
  r26.u32 &= ~off;
}

// [[midasm_hook]] 0x8245D524 (sub_8245D4A0, the HDR final pass): before "std r9,0(r7)" and "std r8,8(r7)", which copy
// the bloom's color (effect+96), just loaded, for pixel constant c19.
void PotcaweBloomColorHook(PPCRegister& r9, PPCRegister& r8) {
  if (REXCVAR_GET(potcawe_fx_bloom)) {
    return;
  }
  static std::atomic<bool> logged{false};
  if (!logged.exchange(true)) {
    REXLOG_INFO("[screen fx] the bloom turned off (potcawe_fx_bloom)");
  }
  r9.u64 = 0;
  r8.u64 = 0;
}

// [[midasm_hook]] 0x8245DBC0 (sub_8245D4A0) and 0x8245DF84 (sub_8245DCD0), before "cmplwi cr6,r11,0": r11 = the color
// adjustment's switch (effect+176), which picks the shader.
void PotcaweColorAdjustHook(PPCRegister& r11) {
  if (!REXCVAR_GET(potcawe_fx_color)) {
    r11.u32 = 0;
  }
}
