// PiratesAtWorldsEnd Launcher's window contents: the sidebar and the pages of Gustavo's mockup
// (local_docs/launcher_design), on the settings of settings.h. main.cpp owns the window and the loop.

#pragma once

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_gamepad.h>
#include <SDL3/SDL_scancode.h>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>

#include "settings.h"
#include "texts.h"
#include "ui/ui.h"

namespace launcher {

// The launcher's version (res/version.h), shown in the sidebar and the Updates page.
extern const char* const kVersion;

// Opens a web page in the default browser.
void OpenUrl(const char* url);

// This game's releases (GitHub).
inline constexpr const char* kReleasesUrl = "https://github.com/ElGuslml/PiratesAtWorldsEndRecomp/releases";

enum class Page : int { kPlay, kAchievements, kKeyboard, kGamepad, kVideo, kGraphics, kUpdates, kCount };

class App {
 public:
  enum class Result { kNone, kPlay, kQuit };

  App(Settings settings, uint32_t language);

  // The whole window, between ImGui::NewFrame() and ImGui::Render(). `size` in pixels.
  void Frame(ImVec2 size);
  // An SDL event before ImGui sees it; true when the launcher used it (a key being assigned).
  bool OnEvent(const SDL_Event& event);

  Result result() const { return result_; }
  void Quit() { result_ = Result::kQuit; }
  // Something is changing on screen (a key being assigned, a message): keep drawing.
  bool Busy() const;
  // A key is being assigned: the controller does not navigate meanwhile (B cancels).
  bool Listening() const { return listen_bind_ >= 0 || pad_listen_ >= 0; }
  // A controller button was just assigned: the controller does nothing until its buttons are up.
  bool GamepadLocked() const { return pad_release_wait_; }
  Settings& settings() { return settings_; }
  const Texts& texts() const { return t_; }

 private:
  // A setting changed: potcawe_options.toml is written at once.
  void Changed();

  void Sidebar(const ImRect& rect);
  void ControllerHints(ImVec2 pos, float width);

  // Pages, each in its own file (page_*.cpp). `rect` is the area right of the sidebar.
  void PlayPage(const ImRect& rect);
  void AchievementsPage(const ImRect& rect);
  void KeyboardPage(const ImRect& rect);
  void GamepadPage(const ImRect& rect);
  void VideoPage(const ImRect& rect);
  void GraphicsPage(const ImRect& rect);
  void UpdatesPage(const ImRect& rect);

  // The pages' shared pieces. Each returns the y below it (pixels).
  float Header(ImVec2 pos, const char* kicker, const char* title, const char* subtitle);
  // A settings row in a card: the title and hint on the left, the control at `*control` (its height in
  // dp is `control_height`). Returns the row's height.
  float Row(ImVec2 pos, float width, float padding_y, const char* title, const char* hint, float control_height,
            bool last, ImVec2* control);
  // "Changes are saved automatically" and a reset button at the bottom of `rect`; true when reset.
  bool Footer(const ImRect& rect, const char* reset_label);

  void Navigate(Page page, bool focus_sidebar);
  void FocusSidebar() { focus_sidebar_ = true; }

  // The keyboard page's key being assigned (page_keyboard.cpp).
  bool OnKeyCapture(const SDL_Event& event);
  // The Gamepad page's control being assigned a button (page_gamepad.cpp).
  bool OnPadCapture(const SDL_Event& event);
  void AssignPad(int input);
  void StartListening(int bind, int slot);
  void StopListening();
  void Assign(std::string token);

  Settings settings_;
  const Texts& t_;
  Page page_ = Page::kPlay;
  Result result_ = Result::kNone;
  bool focus_sidebar_ = false;
  bool gamepad_used_ = false;  // the last input came from a controller: the hints show
  ImGuiID page_focus_ = 0;     // the control the controller enters the page on (App::Frame)

  // Keyboard page.
  int listen_bind_ = -1;  // the Bind whose key is being assigned, or -1
  int listen_slot_ = 0;   // 0 = primary, 1 = secondary
  uint8_t held_modifiers_ = 0;
  bool modifier_pressed_ = false;
  std::string notice_;  // a message under the keyboard page's title, for a moment
  std::chrono::steady_clock::time_point notice_until_{};

  // Gamepad page.
  int selected_pad_ = 0;
  SDL_GamepadType pad_type_ = SDL_GAMEPAD_TYPE_UNKNOWN;  // the chosen controller's type: its layout is edited
  int pad_listen_ = -1;  // the control waiting for a button, or -1
  bool pad_release_wait_ = false;

  // Achievements page.
  int achievement_filter_ = 0;
};

}  // namespace launcher
