// potcawe - the game's waits for the GPU without spinning.
//
// The game's D3D (XDK library) waits for the GPU in six loops: ring space for new commands
// (sub_822AE600, by far the most common), fences, pending swaps and write-backs. Each one starts with
// sub_822AF458 (records the start of the wait) and calls sub_822AF608 (hang check: a short db16cyc
// delay, then a GPU hang is declared after 5 s without progress) every time its condition is still
// false. On the console that costs nothing; here the game's render thread spent ~95 % of its time in
// those loops (sampled from outside the process) and kept one core at 100 %, which is heat and power
// with no useful work. The hooks in potcawe_functions.toml spin briefly (short waits are unchanged)
// and then put the thread to sleep in small slices.

#include <rex/cvar.h>

#include <immintrin.h>

#include <algorithm>
#include <chrono>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

REXCVAR_DEFINE_INT32(potcawe_gpu_wait_sleep_us, 250, "Potcawe",
                     "Game waiting for the GPU: sleep this many microseconds per check once the "
                     "wait is longer than potcawe_gpu_wait_spin_us (0 = busy wait, as on the console)");
REXCVAR_DEFINE_INT32(potcawe_gpu_wait_spin_us, 100, "Potcawe",
                     "Game waiting for the GPU: busy-wait this long before sleeping");

namespace {

using Clock = std::chrono::steady_clock;

// High-resolution timer (Windows 10 1803+): sleeps for fractions of a millisecond without
// depending on the system timer resolution (timeBeginPeriod).
class WaitTimer {
 public:
  WaitTimer() {
    timer_ = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION,
                                    TIMER_ALL_ACCESS);
  }
  ~WaitTimer() {
    if (timer_) {
      CloseHandle(timer_);
    }
  }
  WaitTimer(const WaitTimer&) = delete;
  WaitTimer& operator=(const WaitTimer&) = delete;

  void Sleep(int32_t us) {
    if (timer_) {
      LARGE_INTEGER due;
      due.QuadPart = -LONGLONG(us) * 10;  // relative, in 100 ns units
      if (SetWaitableTimerEx(timer_, &due, 0, nullptr, nullptr, nullptr, 0) &&
          WaitForSingleObject(timer_, INFINITE) == WAIT_OBJECT_0) {
        return;
      }
    }
    // No high-resolution timer: at least yield the core.
    SwitchToThread();
  }

 private:
  HANDLE timer_ = nullptr;
};

thread_local Clock::time_point t_wait_start;
thread_local bool t_wait_started = false;

}  // namespace

// [[midasm_hook]] 0x822AF458: sub_822AF458, a wait loop starts (its condition was false once).
void PotcaweGpuWaitBeginHook() {
  t_wait_started = false;
}

// [[midasm_hook]] 0x822AF608: sub_822AF608, every iteration of a wait loop (the condition is still
// false).
void PotcaweGpuWaitSpinHook() {
  int32_t sleep_us = REXCVAR_GET(potcawe_gpu_wait_sleep_us);
  if (sleep_us <= 0) {
    return;
  }
  Clock::time_point now = Clock::now();
  if (!t_wait_started) {
    t_wait_started = true;
    t_wait_start = now;
  }
  int32_t spin_us = std::max(0, REXCVAR_GET(potcawe_gpu_wait_spin_us));
  if (now - t_wait_start < std::chrono::microseconds(spin_us)) {
    _mm_pause();
    return;
  }
  thread_local WaitTimer timer;
  timer.Sleep(sleep_us);
}
