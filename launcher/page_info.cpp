// The Achievements and Updates pages. Their data arrive in later phases (local_docs/10): the achievements'
// list from the game's cache after the first session, the update check from the GitHub releases. Until then
// they show what is known: the installed version, the setting for checking at start, the releases page.

#include <algorithm>
#include <cstdio>

#include "app.h"
#include "pages.h"

namespace launcher {

using namespace ui;

void App::AchievementsPage(const ImRect& rect) {
  ImRect content = PageContent(rect);
  content.Max.x = rect.Max.x - Px(40.0f);  // the list uses the whole width
  float y = Header(content.Min, nullptr, t_.achievements_title, t_.achievements_subtitle) + Px(20.0f);

  // Nothing to list before the game has run once.
  constexpr Type kTitle{Font::kSemiBold, 18};
  constexpr Type kText{Font::kRegular, 14};
  float text_width = std::min(Px(520.0f), content.GetWidth() - Px(80.0f));
  float text_height = DrawParagraph(kText, ImVec2(0, 0), text_width, 1.5f, 0, t_.achievements_empty, false);
  float height = Px(48.0f) * 2.0f + Px(64.0f) + Px(20.0f) + LineHeight(kTitle) + Px(8.0f) + text_height;
  ImRect card(ImVec2(content.Min.x, y), ImVec2(content.Max.x, y + height));
  FillRect(card, col::kCard, 12.0f);
  float cx = card.GetCenter().x;
  float top = card.Min.y + Px(48.0f);
  ImRect icon(ImVec2(cx - Px(32.0f), top), ImVec2(cx + Px(32.0f), top + Px(64.0f)));
  FillRect(icon, col::kAccentTint, 12.0f);
  DrawIcon(Icon::kTrophy, ImVec2(icon.Min.x + Px(17.0f), icon.Min.y + Px(17.0f)), 30.0f, col::kAccent, 1.8f);
  top += Px(64.0f) + Px(20.0f);
  DrawLabel(kTitle, ImVec2(cx - MeasureText(kTitle, t_.achievements_empty_title).x * 0.5f, top), col::kText,
           t_.achievements_empty_title);
  top += LineHeight(kTitle) + Px(8.0f);
  DrawParagraph(kText, ImVec2(cx - text_width * 0.5f, top), text_width, 1.5f, col::kTextMuted, t_.achievements_empty,
                  true, 0.5f);
}

void App::UpdatesPage(const ImRect& rect) {
  Options& o = settings_.o;
  ImRect content = PageContent(rect);
  float y = Header(content.Min, nullptr, t_.updates_title, t_.updates_subtitle) + Px(20.0f);
  constexpr Type kSection{Font::kBold, 12, 0.1f};
  bool changed = false;

  // The installed version.
  {
    constexpr Type kVersionType{Font::kCondensedBold, 34};
    constexpr Type kStatus{Font::kRegular, 14};
    float text_height = LineHeight(kSection) + Px(4.0f) + Px(34.0f) + Px(10.0f) + LineHeight(kStatus);
    float height = Px(28.0f) * 2.0f + std::max(Px(64.0f), text_height);
    ImRect card(ImVec2(content.Min.x, y), ImVec2(content.Max.x, y + height));
    FillRect(card, col::kCard, 12.0f);
    ImRect icon(ImVec2(card.Min.x + Px(28.0f), card.Min.y + (height - Px(64.0f)) * 0.5f),
                ImVec2(card.Min.x + Px(92.0f), card.Min.y + (height + Px(64.0f)) * 0.5f));
    FillRect(icon, col::kAccentTint, 12.0f);
    DrawIcon(Icon::kRefresh, ImVec2(icon.Min.x + Px(17.0f), icon.Min.y + Px(17.0f)), 30.0f, col::kAccent, 1.8f);
    float x = icon.Max.x + Px(24.0f);
    float top = card.Min.y + (height - text_height) * 0.5f;
    DrawLabel(kSection, ImVec2(x, top), col::kLabel, t_.installed_version);
    top += LineHeight(kSection) + Px(4.0f);
    DrawLabel(kVersionType, ImVec2(x, top), col::kText, kVersion, Px(34.0f));
    top += Px(34.0f) + Px(10.0f);
    char checked[128];
    std::snprintf(checked, sizeof(checked), t_.last_check, t_.never);
    DrawLabel(kStatus, ImVec2(x, top), col::kTextMuted, checked);
    // The check arrives with the updater (phase 4).
    ButtonSpec check{ButtonKind::kPrimary, Type{Font::kBold, 16}, 52, 28, 10, Icon::kRefresh, 18, 2};
    check.gap = 10;
    float width = ButtonWidth(check, t_.check_updates);
    Button("check", ImVec2(card.Max.x - Px(28.0f) - width, card.Min.y + (height - Px(52.0f)) * 0.5f), check,
           t_.check_updates, false);
    y += height + Px(20.0f);
  }

  // Checking when the launcher opens.
  {
    constexpr Type kTitle{Font::kSemiBold, 15};
    constexpr Type kHint{Font::kRegular, 13};
    float height = Px(20.0f) * 2.0f + LineHeight(kTitle) + Px(4.0f) + LineHeight(kHint);
    ImRect card(ImVec2(content.Min.x, y), ImVec2(content.Max.x, y + height));
    FillRect(card, col::kCard, 12.0f);
    DrawLabel(kTitle, ImVec2(card.Min.x + Px(28.0f), card.Min.y + Px(20.0f)), col::kText, t_.check_on_start);
    DrawLabel(kHint, ImVec2(card.Min.x + Px(28.0f), card.Min.y + Px(20.0f) + LineHeight(kTitle) + Px(4.0f)),
             col::kTextMuted, t_.check_on_start_hint);
    if (Toggle("auto", ImVec2(card.Max.x - Px(28.0f) - Px(52.0f), card.Min.y + (height - Px(30.0f)) * 0.5f),
               &o.check_updates)) {
      changed = true;
    }
    y += height + Px(20.0f);
  }

  // The release notes: the releases page until the launcher reads them.
  {
    ImRect card(ImVec2(content.Min.x, y), content.Max);
    FillRect(card, col::kCard, 12.0f);
    float x = card.Min.x + Px(28.0f);
    float top = card.Min.y + Px(24.0f);
    char title[96];
    std::snprintf(title, sizeof(title), t_.release_notes, kVersion);
    DrawLabel(kSection, ImVec2(x, top), col::kLabel, title);
    top += LineHeight(kSection) + Px(14.0f);
    DrawParagraph(Type{Font::kRegular, 14}, ImVec2(x, top), card.GetWidth() - Px(56.0f), 1.5f, col::kTextSoft,
                    t_.release_notes_soon);
    constexpr Type kLinkType{Font::kSemiBold, 14};
    if (Link("history", ImVec2(x, card.Max.y - Px(24.0f) - LineHeight(kLinkType)), t_.full_history)) {
      OpenUrl(kReleasesUrl);
    }
  }
  if (changed) {
    Changed();
  }
}

}  // namespace launcher
