// This game's palette, from Gustavo's mockup of 2026-11-06 ("PotC AWE Launcher 2"): every color the launcher draws, by
// name (ui.h, the pages). Each game has its own, like game.h; the names are the same in every game.

#pragma once

#include "ui/color.h"

namespace ui::col {

inline constexpr ImU32 kBackground = Hex(0x0A1424);
// The mockup does not show these eight (the first run's fields, the dialogs, the Controls page): the old greys moved to
// the night blue of its background.
inline constexpr ImU32 kCard = Hex(0x0F1A2A);
inline constexpr ImU32 kCardRaised = Hex(0x142033);
inline constexpr ImU32 kChip = Hex(0x1A2638);
inline constexpr ImU32 kField = Hex(0x1A2638);
inline constexpr ImU32 kFieldBorder = Hex(0x27354A);
inline constexpr ImU32 kTrack = Hex(0x223044);
inline constexpr ImU32 kOutline = Hex(0x2B3A50);
inline constexpr ImU32 kDashed = Hex(0x3B4C66);
inline constexpr ImU32 kGlyph = Hex(0x1E2B3E);
inline constexpr ImU32 kText = Hex(0xEEF2F7);
inline constexpr ImU32 kTextSoft = Hex(0xC8D1DE);
inline constexpr ImU32 kTextMuted = Hex(0xA9B4C4);
inline constexpr ImU32 kLabel = Hex(0xA9B4C4);
inline constexpr ImU32 kTextDim = Hex(0x7E8A9C);
inline constexpr ImU32 kAccent = Hex(0xE8B04B);
inline constexpr ImU32 kAccentLight = Hex(0xF4CC7A);
inline constexpr ImU32 kOnAccent = Hex(0x1A1203);
inline constexpr ImU32 kGlyphOnAccent = kOnAccent;  // a button's disc inside an accent button (RecompLauncher 3.2)
inline constexpr ImU32 kAccentTint = Hex(0x2A2618);
inline constexpr ImU32 kDonate = Hex(0xEF7458);
inline constexpr ImU32 kOnDonate = Hex(0x1A0D09);
inline constexpr ImU32 kDonateIcon = kOnDonate;      // Donate's heart (RecompLauncher 3.2)
inline constexpr ImU32 kSuccess = Hex(0x5BC489);
inline constexpr ImU32 kSuccessTint = Hex(0x1B3A2A);
inline constexpr ImU32 kError = Hex(0xF07B6B);
inline constexpr ImU32 kErrorTint = Hex(0x3A1F1C);
inline constexpr ImU32 kGlyphA = Hex(0x5BC489);
inline constexpr ImU32 kGlyphB = Hex(0xF07B6B);
inline constexpr ImU32 kGlyphX = Hex(0x6AA6F2);
inline constexpr ImU32 kGlyphY = Hex(0xE8C547);

// The colors of a single place each.
inline constexpr ImU32 kNoticeTint = Hex(0x17281F);        // a success notice's background (the Play page)
inline constexpr ImU32 kScrollbarActive = Hex(0x465A78);   // a scrollbar's thumb while dragged


// The shell of launcher 3.0: the header and the footer over the art, the panels and their chips over it, the
// controller's keys in the hints.
inline constexpr ImU32 kHeader = Hex(0x060B13, 219);         // rgba(6, 11, 19, 0.86)
inline constexpr ImU32 kFooter = Hex(0x060B13, 219);         // the same
inline constexpr ImU32 kHairline = Hex(0xFFFFFF, 20);        // their borders, the panels' (0.08)
inline constexpr ImU32 kPanel = Hex(0x060B13, 189);          // a card over the art (0.74)
inline constexpr ImU32 kPanelChip = Hex(0xFFFFFF, 26);       // a chip in it (0.10)
inline constexpr ImU32 kPanelTrack = Hex(0xFFFFFF, 31);      // a bar's track in it (0.12)
inline constexpr ImU32 kBumper = Hex(0xFFFFFF, 31);          // the header's LB and RB (0.12)
inline constexpr ImU32 kOnBumper = Hex(0xC8D1DE);
inline constexpr ImU32 kTabText = Hex(0xA9B4C4);
inline constexpr ImU32 kTabActive = Hex(0xFFFFFF);
inline constexpr ImU32 kFocus = Hex(0xFFFFFF);               // the focus outline, 3 dp, 4 dp out
// Over the art under a page. The launcher fades this by 0.9 under the tabs and 0.55 on the first run, and Fade
// multiplies the alpha: 0.867 gives the mockup's 0.78 under the tabs (and 0.48 on the first run, which it does not show).
inline constexpr ImU32 kDim = Hex(0x050A12, 221);
inline constexpr ImU32 kHintBumper = Hex(0xFFFFFF, 36);      // the hints' LB RB and arrows (0.14)
inline constexpr ImU32 kOnHintBumper = Hex(0xE6EBF2);
inline constexpr ImU32 kOnGlyphA = Hex(0x06210F);
inline constexpr ImU32 kOnGlyphB = Hex(0x2A0B06);
inline constexpr ImU32 kOnGlyphX = Hex(0x061A33);
inline constexpr ImU32 kOnGlyphY = Hex(0x2A2205);
inline constexpr ImU32 kGhostFill = Hex(0x060B13, 153);      // a dark button over the art (0.6)
inline constexpr ImU32 kGhostBorder = Hex(0xFFFFFF, 46);     // its border (0.18)

// Every corner's radius is the mockup's times this (ui.cpp): 1, the launcher's own corners.
inline constexpr float kRoundness = 1.0f;

}  // namespace ui::col
