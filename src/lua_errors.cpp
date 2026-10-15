// potcawe - the game's Lua errors, written to the log.
//
// The game's scripts run on Lua 5.1 and the game prints none of their errors: it sets no panic function, and
// its protected calls drop the message. Two hooks write them to the log:
//   - an error a protected call catches (luaD_pcall, sub_8247AB70), which the game handles and goes on;
//   - an error nothing catches, just before luaD_throw (sub_8247A5E0) calls exit(EXIT_FAILURE). exit() then
//     runs the game's static destructors while its other threads still run, and those fault: the access
//     violations in the log look like the cause, and are not.
// Until setjmp_address and longjmp_address were set (potcawe_functions.toml), every error took the second
// path: the translated longjmp returned to luaD_throw instead of jumping (docs/script-errors.md).
//
// The layout, as the game's Lua uses it (luaD_seterrorobj, luaS_newlstr): L->top at +8, L->stack at +32;
// values are 16 bytes, their type at +8 (4 for a string); a string's length at +12, its characters at +16.

#include <rex/logging.h>
#include <rex/ppc/context.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xmemory.h>

#include <algorithm>
#include <atomic>
#include <cstring>
#include <string>

namespace {

uint32_t Load32(const uint8_t* base, uint32_t address) {
  uint32_t value;
  std::memcpy(&value, base + address, 4);
  return __builtin_bswap32(value);
}

const char* ErrorName(uint32_t code) {
  switch (code) {
    case 2:
      return "runtime error";
    case 3:
      return "syntax error";
    case 4:
      return "out of memory";
    case 5:
      return "error in the error handler";
    default:
      return "error";
  }
}

// The text of the Lua value at `value`: the message if it is a string.
std::string ErrorMessage(const uint8_t* base, uint32_t value) {
  uint32_t type = Load32(base, value + 8);
  if (type != 4) return "(the error is not a string: Lua type " + std::to_string(type) + ")";
  uint32_t string = Load32(base, value);
  uint32_t length = std::min<uint32_t>(Load32(base, string + 12), 4096);
  return std::string(reinterpret_cast<const char*>(base + string + 16), length);
}

const uint8_t* GuestBase() {
  auto* kernel = rex::system::kernel_state();
  return kernel && kernel->memory() ? kernel->memory()->virtual_membase() : nullptr;
}

}  // namespace

// [[midasm_hook]] at 0x8247ABD0 in luaD_pcall, after luaD_seterrorobj: r29 is the error code, r30 the error
// object (where the called function was). The first 100 are logged.
void PotcaweLuaCaughtErrorHook(PPCRegister& r29, PPCRegister& r30) {
  static std::atomic<uint32_t> logged{0};
  if (logged.fetch_add(1, std::memory_order_relaxed) >= 100) return;
  const uint8_t* base = GuestBase();
  if (!base) return;
  REXLOG_WARN("[lua] {} caught by the game's protected call (code {}): {}", ErrorName(r29.u32), r29.u32,
              ErrorMessage(base, r30.u32));
}

// [[midasm_hook]] at 0x8247A61C in luaD_throw, on its path for an error with no handler: r31 is the
// lua_State, r11 the error code. The error object is the value below the stack top.
void PotcaweLuaUnhandledErrorHook(PPCRegister& r31, PPCRegister& r11) {
  const uint8_t* base = GuestBase();
  if (!base) return;
  uint32_t state = r31.u32;
  uint32_t top = Load32(base, state + 8);
  std::string message =
      top > Load32(base, state + 32) ? ErrorMessage(base, top - 16) : std::string("(the Lua stack is empty)");
  REXLOG_ERROR("[lua] {} that no script catches (code {}), so the game exits: {}", ErrorName(r11.u32),
               r11.u32, message);
}
