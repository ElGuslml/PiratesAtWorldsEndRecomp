// PiratesAtWorldsEnd Launcher's texts, UTF-8, in the Windows display language: English, German, French,
// Spanish and Italian (the Spanish ones are the mockup's). The game asks for its own language in its
// first menu.

#pragma once

#include <cstdint>

namespace launcher {

struct Texts {
  // The sidebar.
  const char* brand;  // under the game's name
  const char* nav_play;
  const char* nav_achievements;
  const char* nav_controls;
  const char* nav_keyboard;
  const char* nav_gamepad;
  const char* nav_settings;
  const char* nav_video;
  const char* nav_graphics;
  const char* nav_updates;
  const char* donate_text;
  const char* donate;
  const char* version;  // %s: the version
  // The controller's hints, under the sidebar.
  const char* hint_section;
  const char* hint_select;
  const char* hint_back;
  const char* hint_play;

  // Shared by the pages.
  const char* autosave;
  const char* reset_values;
  const char* reset_defaults;
  const char* coming_soon;  // a badge, upper case
  const char* settings;     // the kicker of the Settings pages
  const char* controls;     // the kicker of the Controls pages

  // Play.
  const char* tagline;
  const char* quit;
  const char* play;  // upper case
  const char* current_settings;
  const char* change_settings;
  const char* achievements_card;
  const char* see_achievements;
  const char* achievements_after_play;
  const char* updates_card;
  const char* find_updates;
  const char* last_check;  // %s: when
  const char* never;

  // Video.
  const char* video_subtitle;
  const char* monitor;
  const char* monitor_hint;
  const char* primary;
  const char* display;
  const char* display_hint;
  const char* fullscreen;
  const char* borderless;
  const char* window;
  const char* resolution;
  const char* resolution_hint_screen;
  const char* resolution_hint_window;
  const char* monitor_resolution;
  const char* format;
  const char* format_hint;
  const char* format_hint_16x9;
  const char* format_hint_xenos;
  const char* format_fill;
  const char* format_bars;
  const char* internal;
  const char* internal_hint;
  const char* internal_hint_xenos;
  const char* automatic;
  const char* original;

  // Graphics.
  const char* graphics_api;
  const char* recommended;  // a badge, upper case
  const char* native_text;
  const char* native_missing;
  const char* xenos_text;
  const char* framerate;
  const char* framerate_hint;
  const char* antialiasing;
  const char* antialiasing_hint;
  const char* antialiasing_hint_xenos;
  const char* none;
  const char* experimental;
  const char* anisotropic;
  const char* anisotropic_hint;
  const char* off;
  const char* shadows;
  const char* shadows_hint;
  const char* shadows_high;
  const char* shadows_ultra;
  const char* api_hint;

  // Keyboard.
  const char* keyboard_subtitle;
  const char* mnk;
  const char* mnk_hint;
  const char* column_control;
  const char* column_primary;
  const char* column_secondary;
  const char* group_movement;
  const char* group_camera;
  const char* group_buttons;
  const char* group_dpad;
  const char* group_mouse;
  const char* binds[24];  // in launcher::Bind order
  const char* unbound;
  const char* press_key;
  const char* modifier_alone;
  const char* mouse_camera;
  const char* mouse_camera_hint;
  const char* mouse_sensitivity;
  const char* mouse_sensitivity_hint;
  const char* key_space;
  const char* key_enter;
  const char* key_backspace;
  const char* key_delete;
  const char* key_insert;
  const char* key_home;
  const char* key_end;
  const char* key_page_up;
  const char* key_page_down;
  const char* key_caps_lock;
  const char* key_num_lock;
  const char* key_scroll_lock;
  const char* key_print_screen;
  const char* key_pause;
  const char* key_numpad;  // the prefix of the numeric keypad's keys
  const char* key_shift;
  const char* key_ctrl;
  const char* key_alt;
  const char* mouse_left;
  const char* mouse_right;
  const char* mouse_middle;

  // Gamepad.
  const char* gamepad_subtitle;
  const char* detected;
  const char* connected;
  const char* find_controllers;
  const char* no_controllers;
  const char* mapping_title;  // upper case, followed by an arrow and the controller
  const char* mapping_hint;
  const char* pad_buttons[14];  // A B X Y LB RB LT RT LS RS Start Back D-pad Guide
  const char* press_button;
  const char* remap_note;
  const char* controller;  // a controller of unknown type

  // Achievements.
  const char* achievements_title;
  const char* achievements_subtitle;
  const char* filter_all;       // %d
  const char* filter_unlocked;  // %d
  const char* filter_locked;    // %d
  const char* unlocked_total;   // upper case
  const char* gamerscore;       // upper case
  const char* progress;
  const char* unlocked;
  const char* locked;
  const char* achievements_empty_title;
  const char* achievements_empty;

  // Updates.
  const char* updates_title;
  const char* updates_subtitle;
  const char* installed_version;  // upper case
  const char* check_updates;
  const char* checking;
  const char* checking_status;
  const char* up_to_date;
  const char* check_on_start;
  const char* check_on_start_hint;
  const char* release_notes;  // upper case; %s: the version
  const char* release_notes_soon;
  const char* full_history;

  // Errors.
  const char* game_missing;
  const char* save_failed;
  const char* no_direct3d;
};

// The Windows display language as an XLanguage id, like the SDK's user_language: 1 English, 3 German,
// 4 French, 5 Spanish, 6 Italian.
uint32_t SystemLanguage();
const Texts& TextsFor(uint32_t language);

}  // namespace launcher
