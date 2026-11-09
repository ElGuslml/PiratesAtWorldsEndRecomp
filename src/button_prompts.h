// potcawe - button prompts for the controller in the player's hands (src/button_prompts.cpp).

#pragma once

#include <filesystem>

namespace potcawe {

// Reads the HUD's button sprites from the player's hud_main.edb, before the guest starts (PotcaweApp::OnPostSetup).
void InitButtonPrompts(const std::filesystem::path& game_data_root);

}  // namespace potcawe
