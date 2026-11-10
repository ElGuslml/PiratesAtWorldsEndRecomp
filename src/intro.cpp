// potcawe - the intro videos skipped at startup (docs/intro-videos.md).
//
// The game's boot state (GameFlow_Bootup: enter sub_82081960, update sub_82081AB8) plays two Bink videos, the logos,
// named by their file list UIDs 0xEB000002 and 0xEB000001, while it starts loading the front end in the background.
// When both have ended it shows the language menu once the front end has loaded, then the title screen. The movie
// player (sub_8229EE00) ends a movie when a button is pressed only if the movie is marked skippable ([movie+529]), and
// the boot state marks these two skippable only once its front end has loaded ([state+18]): a second or two into the
// first video on a PC, from when a button press skips each video.
//
// With potcawe_skip_intro:
// - the boot state's update sees the front end loaded as soon as its loading has started ([state+18] read as 1 when
//   [state+17] is set), so it marks both videos skippable with its own code;
// - the movie player sees a button pressed for those two videos, and ends them with its own code (Stop, which starts
//   the next queued movie, ended the same way the next frame).
// Then the boot state takes its usual path: the screen it shows after the videos, the language menu when the front end
// has loaded, the title screen. Not before its loading has started: with no video left before that, the boot state goes
// to its path without the language menu. The title screen's video (0xEB000031) and the levels' are other UIDs.

#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/ppc/context.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xmemory.h>

#include <atomic>
#include <cstdint>

REXCVAR_DEFINE_BOOL(potcawe_skip_intro, false, "Potcawe",
                    "Skip the intro videos (the logos) at startup: the language menu and the title screen come at once");

namespace {

// The boot state's two videos (file list UIDs, [movie+520]).
constexpr uint32_t kIntroVideos[] = {0xEB000002, 0xEB000001};

const uint8_t* Guest(uint32_t address) {
  auto* kernel = rex::system::kernel_state();
  if (!address || !kernel || !kernel->memory()) return nullptr;
  return kernel->memory()->TranslateVirtual<const uint8_t*>(address);
}

uint32_t Be32(const uint8_t* p) {
  return uint32_t(p[0]) << 24 | uint32_t(p[1]) << 16 | uint32_t(p[2]) << 8 | uint32_t(p[3]);
}

}  // namespace

// [[midasm_hook]] 0x82081BF4 (sub_82081AB8, the boot state's update while its videos play), before "cmplwi cr6,r11,0":
// r11 = [r25+18], the front end loaded, which makes the code after it mark both videos skippable.
void PotcaweIntroSkippableHook(PPCRegister& r11, PPCRegister& r25) {
  if (!REXCVAR_GET(potcawe_skip_intro) || r11.u32 != 0) return;
  const uint8_t* state = Guest(r25.u32);
  if (!state || state[17] == 0) return;  // the front end's loading has not started
  static std::atomic<bool> logged{false};
  if (!logged.exchange(true)) REXLOG_INFO("[intro] the intro videos skipped (potcawe_skip_intro)");
  r11.u32 = 1;
}

// [[midasm_hook]] 0x8229EF94 (sub_8229EE00, the movie player's update of a skippable movie), before "extsb r11,r25":
// r25 = a button pressed, r27 = the movie (its UID at +520).
void PotcaweIntroSkipHook(PPCRegister& r25, PPCRegister& r27) {
  if (!REXCVAR_GET(potcawe_skip_intro)) return;
  const uint8_t* movie = Guest(r27.u32);
  if (!movie) return;
  uint32_t uid = Be32(movie + 520);
  for (uint32_t video : kIntroVideos) {
    if (uid == video) {
      r25.u32 = 1;
      return;
    }
  }
}
