// This game's own texts (game.h): the first run's welcome and its last screen, in the game's spirit.

#include "game.h"

namespace launcher::game {

const GameTexts& GameTextsFor(uint32_t language) {
  static const GameTexts kEnglish{
      "Welcome aboard",
      "Before setting sail the game has to be installed. The launcher extracts its files from your Xbox 360 ISO; you "
      "only need to do it once.",
      {},
      "All set, captain!",
      "The game was installed. Set sail now, or first review the video, graphics and control settings; you can always "
      "change them in the launcher."};
  static const GameTexts kGerman{
      "Willkommen an Bord",
      "Bevor es losgeht, muss das Spiel installiert werden. Der Launcher entpackt die Dateien aus deiner Xbox-360-ISO; "
      "das musst du nur einmal tun.",
      {},
      "Alles bereit, Käpt'n!",
      "Das Spiel wurde installiert. Stich jetzt in See oder prüfe zuerst die Video-, Grafik- und "
      "Steuerungseinstellungen; du kannst sie jederzeit im Launcher ändern."};
  static const GameTexts kFrench{
      "Bienvenue à bord",
      "Avant de prendre la mer, il faut installer le jeu. Le launcher extrait les fichiers de votre ISO Xbox 360 ; vous "
      "n'aurez à le faire qu'une fois.",
      {},
      "Tout est prêt, capitaine !",
      "Le jeu est installé. Prenez la mer maintenant, ou vérifiez d'abord les paramètres vidéo, graphiques et de "
      "commandes ; vous pourrez toujours les modifier dans le launcher."};
  static const GameTexts kSpanish{
      "Bienvenido a bordo",
      "Antes de zarpar hay que instalar el juego. El launcher extrae los archivos desde tu ISO de Xbox 360; solo "
      "tendrás que hacerlo una vez.",
      {},
      "¡Todo listo, capitán!",
      "El juego se instaló correctamente. Puedes zarpar ya o revisar primero los ajustes de video, gráficos y "
      "controles; siempre podrás cambiarlos desde el launcher."};
  static const GameTexts kItalian{
      "Benvenuto a bordo",
      "Prima di salpare bisogna installare il gioco. Il launcher estrae i file dalla tua ISO Xbox 360; dovrai farlo "
      "una sola volta.",
      {},
      "Tutto pronto, capitano!",
      "Il gioco è stato installato. Salpa subito o controlla prima le impostazioni video, grafica e controlli; potrai "
      "sempre cambiarle dal launcher."};
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
