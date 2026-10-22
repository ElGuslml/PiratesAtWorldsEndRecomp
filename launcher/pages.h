// Layout pieces the pages share (page_*.cpp).

#pragma once

#include <algorithm>

#include "ui/ui.h"

namespace launcher {

// A page's content: the area right of the sidebar less the mockup's padding (32 40 24), at most 1240 dp wide.
inline ImRect PageContent(const ImRect& rect) {
  float width = std::min(rect.GetWidth() - ui::Px(80.0f), ui::Px(1240.0f));
  return ImRect(ImVec2(rect.Min.x + ui::Px(40.0f), rect.Min.y + ui::Px(32.0f)),
                ImVec2(rect.Min.x + ui::Px(40.0f) + width, rect.Max.y - ui::Px(24.0f)));
}

// A card whose height is known after its rows: its background goes under them through a draw list channel.
class Card {
 public:
  Card() {
    ImGui::GetWindowDrawList()->ChannelsSplit(2);
    ImGui::GetWindowDrawList()->ChannelsSetCurrent(1);
  }
  void End(const ImRect& rect, ImU32 color = ui::col::kCard, float rounding = 12.0f) {
    ImGui::GetWindowDrawList()->ChannelsSetCurrent(0);
    ui::FillRect(rect, color, rounding);
    ImGui::GetWindowDrawList()->ChannelsMerge();
  }
};

}  // namespace launcher
