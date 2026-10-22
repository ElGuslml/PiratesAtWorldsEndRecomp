// The Gamepad page: the controllers SDL sees, and each button of the original Xbox 360 controller on the
// chosen one. Remapping them needs the SDK's support (local_docs/10, phase 1b): until then the table shows
// the default layout.

#include <SDL3/SDL_gamepad.h>
#include <SDL3/SDL_stdinc.h>

#include <cstdio>
#include <string>
#include <vector>

#include "app.h"
#include "pages.h"

namespace launcher {

using namespace ui;

namespace {

struct Pad {
  std::string name;
  SDL_GamepadType type = SDL_GAMEPAD_TYPE_UNKNOWN;
};

std::vector<Pad> ConnectedPads() {
  std::vector<Pad> pads;
  int count = 0;
  SDL_JoystickID* ids = SDL_GetGamepads(&count);
  for (int i = 0; i < count; ++i) {
    Pad pad;
    const char* name = SDL_GetGamepadNameForID(ids[i]);
    pad.name = name && *name ? name : "";
    pad.type = SDL_GetGamepadTypeForID(ids[i]);
    pads.push_back(std::move(pad));
  }
  SDL_free(ids);
  return pads;
}

enum class Family { kXbox, kPlayStation, kNintendo };

Family FamilyOf(SDL_GamepadType type) {
  switch (type) {
    case SDL_GAMEPAD_TYPE_PS3:
    case SDL_GAMEPAD_TYPE_PS4:
    case SDL_GAMEPAD_TYPE_PS5:
      return Family::kPlayStation;
    case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_PRO:
    case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_LEFT:
    case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_RIGHT:
    case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_PAIR:
      return Family::kNintendo;
    default:
      return Family::kXbox;
  }
}

const char* TypeName(SDL_GamepadType type, const Texts& t) {
  switch (type) {
    case SDL_GAMEPAD_TYPE_XBOX360:
      return "Xbox 360";
    case SDL_GAMEPAD_TYPE_XBOXONE:
      return "Xbox One / Series";
    case SDL_GAMEPAD_TYPE_PS3:
      return "PlayStation 3";
    case SDL_GAMEPAD_TYPE_PS4:
      return "PlayStation 4";
    case SDL_GAMEPAD_TYPE_PS5:
      return "PlayStation 5";
    case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_PRO:
      return "Nintendo Switch Pro";
    case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_LEFT:
    case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_RIGHT:
    case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_PAIR:
      return "Joy-Con";
    default:
      return t.controller;
  }
}

// The physical button an original one is on, by default, per family: text or a PlayStation symbol.
struct Physical {
  std::string text;
  Icon icon = Icon::kNone;
  ImU32 color = col::kText;
};

Physical PhysicalButton(Family family, SDL_GamepadType type, int button, const Texts& t) {
  // The original buttons, in Texts::pad_buttons order: A B X Y LB RB LT RT LS RS Start Back D-pad Guide.
  switch (family) {
    case Family::kPlayStation: {
      if (button < 4) {
        constexpr Icon kIcons[] = {Icon::kPsCross, Icon::kPsCircle, Icon::kPsSquare, Icon::kPsTriangle};
        constexpr ImU32 kColors[] = {Hex(0x7CB2E8), Hex(0xF07B6B), Hex(0xE6A4D7), Hex(0x5BC489)};
        return {"", kIcons[button], kColors[button]};
      }
      bool ps5 = type == SDL_GAMEPAD_TYPE_PS5;
      bool ps4 = type == SDL_GAMEPAD_TYPE_PS4;
      const char* names[] = {"L1", "R1", "L2", "R2", "L3", "R3", ps4 || ps5 ? "Options" : "Start",
                             ps5 ? "Create" : ps4 ? "Share" : "Select", t.pad_buttons[12], "PS"};
      return {names[button - 4]};
    }
    case Family::kNintendo: {
      // Positional: the Xbox A is the Nintendo B.
      const char* names[] = {"B", "A", "Y", "X", "L", "R", "ZL", "ZR", "LS", "RS", "+", "−", t.pad_buttons[12],
                             "Home"};
      return {names[button]};
    }
    case Family::kXbox:
      break;
  }
  bool one = type == SDL_GAMEPAD_TYPE_XBOXONE;
  const char* names[] = {"A",  "B",  "X",  "Y",  "LB", "RB", "LT", "RT", "LS", "RS", one ? "Menu" : "Start",
                         one ? "View" : "Back", t.pad_buttons[12], "Xbox"};
  constexpr ImU32 kFaces[] = {col::kGlyphA, col::kGlyphB, col::kGlyphX, col::kGlyphY};
  return {names[button], Icon::kNone, button < 4 ? kFaces[button] : col::kText};
}

}  // namespace

void App::GamepadPage(const ImRect& rect) {
  ImRect content = PageContent(rect);
  float y = Header(content.Min, t_.controls, t_.nav_gamepad, t_.gamepad_subtitle) + Px(20.0f);
  std::vector<Pad> pads = ConnectedPads();
  if (selected_pad_ >= int(pads.size())) {
    selected_pad_ = 0;
  }
  constexpr Type kSection{Font::kBold, 12, 0.1f};

  // The controllers.
  ImRect left(ImVec2(content.Min.x, y), ImVec2(content.Min.x + Px(300.0f), content.Max.y));
  FillRect(left, col::kCard, 12.0f);
  {
    float x = left.Min.x + Px(20.0f);
    float width = left.GetWidth() - Px(40.0f);
    float top = left.Min.y + Px(20.0f);
    DrawLabel(kSection, ImVec2(x, top), col::kLabel, t_.detected);
    top += LineHeight(kSection) + Px(4.0f) + Px(10.0f);
    if (pads.empty()) {
      DrawParagraph(Type{Font::kRegular, 14}, ImVec2(x, top), width, 1.4f, col::kTextMuted, t_.no_controllers);
    }
    constexpr Type kName{Font::kSemiBold, 14};
    constexpr Type kKind{Font::kRegular, 12};
    for (int i = 0; i < int(pads.size()); ++i) {
      float height = Px(14.0f) * 2.0f + LineHeight(kName) + Px(3.0f) + LineHeight(kKind);
      ImRect r(ImVec2(x, top), ImVec2(x + width, top + height));
      ImGui::PushID(i);
      ItemState state = Item("pad", r);
      ImGui::PopID();
      bool on = i == selected_pad_;
      if (state.pressed) {
        selected_pad_ = i;
      }
      FillRect(r, state.hovered ? Brighten(on ? col::kSegment : col::kSidebar) : on ? col::kSegment : col::kSidebar,
               10.0f);
      StrokeRect(r, on ? col::kAccent : col::kSidebarBorder, 10.0f, 2.0f);
      DrawIcon(Icon::kGamepad, ImVec2(r.Min.x + Px(14.0f), r.Min.y + (height - Px(28.0f)) * 0.5f), 28.0f,
               on ? col::kAccent : col::kTextDim, 1.6f);
      float text_x = r.Min.x + Px(14.0f) + Px(28.0f) + Px(12.0f);
      float text_width = r.Max.x - Px(14.0f) - text_x;
      std::string name = pads[size_t(i)].name.empty() ? TypeName(pads[size_t(i)].type, t_) : pads[size_t(i)].name;
      DrawLabel(kName, ImVec2(text_x, r.Min.y + Px(14.0f)), col::kText, Ellipsize(kName, name, text_width));
      std::string kind = std::string(TypeName(pads[size_t(i)].type, t_)) + " · " + t_.connected;
      DrawLabel(kKind, ImVec2(text_x, r.Min.y + Px(14.0f) + LineHeight(kName) + Px(3.0f)), col::kTextMuted,
               Ellipsize(kKind, kind, text_width));
      if (state.focused) {
        FocusRing(r, 10.0f);
      }
      top += height + Px(10.0f);
    }
    // SDL adds and removes controllers by itself; the button reads the list again.
    ButtonSpec find{ButtonKind::kGhost, Type{Font::kSemiBold, 14}, 44, 18, 8, Icon::kRefresh, 16};
    Button("find", ImVec2(x, left.Max.y - Px(20.0f) - Px(44.0f)), find, t_.find_controllers, true, width);
  }

  // The original buttons and where they are on the chosen controller.
  ImRect right(ImVec2(left.Max.x + Px(20.0f), y), content.Max);
  FillRect(right, col::kCard, 12.0f);
  {
    float x = right.Min.x + Px(24.0f);
    float width = right.GetWidth() - Px(48.0f);
    float top = right.Min.y + Px(20.0f);
    SDL_GamepadType type = pads.empty() ? SDL_GAMEPAD_TYPE_XBOX360 : pads[size_t(selected_pad_)].type;
    std::string name = pads.empty() ? std::string("Xbox 360")
                       : pads[size_t(selected_pad_)].name.empty() ? std::string(TypeName(type, t_))
                                                                  : pads[size_t(selected_pad_)].name;
    // "ORIGINAL BUTTON → CONTROLLER": Barlow has no arrow, an icon draws it.
    std::string upper;
    for (char c : name) upper += (c >= 'a' && c <= 'z') ? char(c - 32) : c;
    ImVec2 badge = BadgeSize(t_.coming_soon);
    float title_width = MeasureText(kSection, t_.mapping_title).x;
    float arrow = Px(14.0f);
    DrawLabel(kSection, ImVec2(x, top), col::kLabel, t_.mapping_title);
    DrawIcon(Icon::kArrowRight, ImVec2(x + title_width + Px(4.0f), top + (LineHeight(kSection) - arrow) * 0.5f), 14.0f,
             col::kLabel, 2.2f);
    float name_x = x + title_width + Px(4.0f) + arrow + Px(6.0f);
    DrawLabel(kSection, ImVec2(name_x, top), col::kLabel,
              Ellipsize(kSection, upper, right.Max.x - Px(24.0f) - badge.x - Px(16.0f) - name_x));
    Badge(ImVec2(right.Max.x - Px(24.0f) - badge.x, top + (LineHeight(kSection) - badge.y) * 0.5f), t_.coming_soon,
          col::kGlyph, col::kTextSoft);
    top += LineHeight(kSection) + Px(14.0f);

    Family family = FamilyOf(type);
    constexpr const char* kGlyphs[] = {"A",  "B",  "X",  "Y",     "LB",   "RB",    "LT",
                                       "RT", "LS", "RS", "START", "BACK", "D-PAD", "GUIDE"};
    constexpr ImU32 kGlyphColors[] = {col::kGlyphA, col::kGlyphB, col::kGlyphX, col::kGlyphY};
    constexpr Type kGlyph{Font::kBold, 12};
    constexpr Type kLabel{Font::kRegular, 14};
    constexpr Type kButton{Font::kSemiBold, 14};
    float column_gap = Px(16.0f);
    float column_width = (width - column_gap) * 0.5f;
    float row_height = Px(52.0f);
    for (int i = 0; i < 14; ++i) {
      float cx = x + float(i % 2) * (column_width + column_gap);
      float cy = top + float(i / 2) * (row_height + Px(8.0f));
      if (cy + row_height > right.Max.y - Px(20.0f)) {
        break;  // a short window: the rest does not fit
      }
      // The original button.
      float glyph_width = std::max(Px(44.0f), MeasureText(kGlyph, kGlyphs[i]).x + Px(16.0f));
      ImRect pill(ImVec2(cx + Px(4.0f), cy + (row_height - Px(28.0f)) * 0.5f),
                  ImVec2(cx + Px(4.0f) + glyph_width, cy + (row_height + Px(28.0f)) * 0.5f));
      FillRect(pill, col::kGlyph, 14.0f);
      DrawLabel(kGlyph, ImVec2(pill.Min.x + (glyph_width - MeasureText(kGlyph, kGlyphs[i]).x) * 0.5f, pill.Min.y),
               i < 4 ? kGlyphColors[i] : col::kTextSoft, kGlyphs[i], pill.GetHeight());
      // Its name and the physical button.
      float button_width = std::min(Px(150.0f), column_width * 0.45f);
      ImRect button(ImVec2(cx + column_width - Px(4.0f) - button_width, cy + Px(7.0f)),
                    ImVec2(cx + column_width - Px(4.0f), cy + Px(45.0f)));
      float label_x = pill.Max.x + Px(12.0f);
      DrawParagraph(kLabel, ImVec2(label_x, cy + (row_height - LineHeight(kLabel)) * 0.5f),
                      button.Min.x - Px(12.0f) - label_x, 1.2f, col::kTextSoft, t_.pad_buttons[i]);
      FillRect(button, col::kField, 8.0f);
      StrokeRect(button, col::kFieldBorder, 8.0f);
      Physical physical = PhysicalButton(family, type, i, t_);
      if (physical.icon != Icon::kNone) {
        DrawIcon(physical.icon, ImVec2(button.Min.x + Px(12.0f), button.Min.y + (button.GetHeight() - Px(18.0f)) * 0.5f),
                 18.0f, physical.color, 2.4f);
      } else {
        DrawLabel(kButton, ImVec2(button.Min.x + Px(12.0f), button.Min.y), physical.color,
                 Ellipsize(kButton, physical.text, button_width - Px(24.0f)), button.GetHeight());
      }
      FillRect(ImRect(ImVec2(cx, cy + row_height), ImVec2(cx + column_width, cy + row_height + Snap(Px(1.0f)))),
               col::kDivider);
    }
    DrawLabel(Type{Font::kRegular, 13}, ImVec2(x, right.Max.y - Px(20.0f) - LineHeight(Type{Font::kRegular, 13})),
             col::kTextMuted, t_.remap_soon);
  }
}

}  // namespace launcher
