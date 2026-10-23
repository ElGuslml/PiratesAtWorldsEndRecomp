// This game's own texts (game.h): the first run's welcome, in the game's spirit.

#include "game.h"

namespace launcher::game {

const GameTexts& GameTextsFor(uint32_t language) {
  static const GameTexts kEnglish{
      "Welcome aboard",
      "Before setting sail the game has to be installed. The launcher extracts its files from your Xbox 360 ISO; you "
      "only need to do it once.",
      {}};
  static const GameTexts kGerman{
      "Willkommen an Bord",
      "Bevor es losgeht, muss das Spiel installiert werden. Der Launcher entpackt die Dateien aus deiner Xbox-360-ISO; "
      "das musst du nur einmal tun.",
      {}};
  static const GameTexts kFrench{
      "Bienvenue à bord",
      "Avant de prendre la mer, il faut installer le jeu. Le launcher extrait les fichiers de votre ISO Xbox 360 ; vous "
      "n'aurez à le faire qu'une fois.",
      {}};
  static const GameTexts kSpanish{
      "Bienvenido a bordo",
      "Antes de zarpar hay que instalar el juego. El launcher extrae los archivos desde tu ISO de Xbox 360; solo "
      "tendrás que hacerlo una vez.",
      {}};
  static const GameTexts kItalian{
      "Benvenuto a bordo",
      "Prima di salpare bisogna installare il gioco. Il launcher estrae i file dalla tua ISO Xbox 360; dovrai farlo "
      "una sola volta.",
      {}};
  switch (language) {
    case 3:
      return kGerman;
    case 4:
      return kFrench;
    case 5:
      return kSpanish;
    case 6:
      return kItalian;
    default:
      return kEnglish;
  }
}

}  // namespace launcher::game
