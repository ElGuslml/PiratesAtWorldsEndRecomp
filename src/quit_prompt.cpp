// potcawe - quitting from the title screen.
//
// The game has no way to quit it (on the console that was the guide button). On the title screen ("PRESS START"),
// B now asks whether to quit the game: the SDK's quit prompt (rex/ui/overlay/quit_prompt.h, on the recomp branch), a
// dialog in the game's language where A quits (the window closes, as with its close button) and B cancels. The game
// does not see those presses. KameoRecomp has the same.
//
// The game's menus are GX menu nodes (GXMenuNode, script driven). Two events say which one is on screen, and the
// hooks in potcawe_functions.toml record them:
// - a node set up from its widget (GXMenuNode::CreateMenuItemHandlerFromWidgetMaybe, sub_822D4380): the language
//   menu (widget C430000F), the title screen (C4300007), the main menu (C430000A); the title reuses the language
//   menu's node;
// - a node linked to the next menu ([node+680] = next, in six functions of the menu code, cleared again after the
//   transition): the title to the autosave notice (which has no widget), the notice to the main menu, the main menu
//   back to the title.
// The current menu is the last node either names; the prompt is offered while it is the title's node, a test the
// SDK runs when a press of B begins (SetQuitPromptCondition). So a B pressed in the main menu still goes back to the
// title screen, and only a new press there opens the dialog. quit_prompt = false (an SDK cvar) disables it.
//
// The game asks for its language in its first menu and does not use the SDK's user_language, so the dialog takes
// the Windows display language, as the launcher's texts do (English, French, German, Italian or Spanish; the
// others, Dutch included, get English). A player whose game language differs from Windows's sees the dialog in
// Windows's.

#include <rex/logging.h>
#include <rex/ppc/context.h>
#include <rex/ui/overlay/quit_prompt.h>

#include <atomic>
#include <cstdint>
#include <mutex>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace {

// The title screen's widget (the GUID its menu node is set up from).
constexpr uint32_t kTitleWidget = 0xC4300007;

std::atomic<uint32_t> g_current_menu{0};  // the menu node on screen, as the last event named it
std::atomic<uint32_t> g_title_menu{0};    // the node set up as the title screen; 0 = none

// The Windows display language as an XLanguage (launcher/potcawe_launcher.cpp's SystemLanguage).
uint32_t SystemLanguage() {
  switch (PRIMARYLANGID(GetUserDefaultUILanguage())) {
    case LANG_GERMAN:
      return 3;
    case LANG_FRENCH:
      return 4;
    case LANG_SPANISH:
      return 5;
    case LANG_ITALIAN:
      return 6;
    default:
      return 1;
  }
}

void RegisterCondition() {
  static std::once_flag once;
  std::call_once(once, [] {
    rex::ui::SetQuitPromptLanguage(SystemLanguage());
    rex::ui::SetQuitPromptCondition([] {
      uint32_t title = g_title_menu.load(std::memory_order_relaxed);
      return title != 0 && g_current_menu.load(std::memory_order_relaxed) == title;
    });
    REXLOG_INFO("[quit_prompt] B on the title screen asks whether to quit");
  });
}

}  // namespace

// [[midasm_hook]] at 0x822D43F4 (bl 0x822d4160 in sub_822D4380): r3 = the menu node, r5 = the widget's GUID.
void PotcaweMenuFromWidgetHook(PPCRegister& r3, PPCRegister& r5) {
  RegisterCondition();
  uint32_t node = r3.u32;
  g_current_menu.store(node, std::memory_order_relaxed);
  if (r5.u32 == kTitleWidget) {
    g_title_menu.store(node, std::memory_order_relaxed);
  } else {
    uint32_t title = node;
    g_title_menu.compare_exchange_strong(title, 0, std::memory_order_relaxed);  // the node reused for another menu
  }
}

// [[midasm_hook]]s at the six stores to [node+680] (the link to the next menu): `next` is the stored value.
void PotcaweMenuLinkHook(PPCRegister& next) {
  if (next.u32 != 0) {
    g_current_menu.store(next.u32, std::memory_order_relaxed);
  }
}
