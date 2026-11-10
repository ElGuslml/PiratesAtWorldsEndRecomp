// This game's own texts (game.h): the first run's welcome and its last screen, in the game's spirit.

#include "game.h"

namespace launcher::game {

const GameTexts& GameTextsFor(uint32_t language) {
  static const GameTexts kEnglish{
      "Welcome aboard",
      "Before setting sail the game has to be installed. The launcher extracts its files from your Xbox 360 ISO; you "
      "only need to do it once.",
      {
          "Bloom", "The glow around lights and bright surfaces.",
          "Depth of field", "What is far from the focus, blurred.",
          "Color correction", "The colors and contrast of each place.",
          "Heat haze", "The image shimmering as through hot air.",
          "Radial blur", "The blur outward from the center.",
          "Motion blur", "The blur of the camera's fast moves.",
          "Skip intro videos", "Straight to the language menu at startup.",
      },
      {
          "The halo of light around the sun, fire and the brightest surfaces. Off, the image is sharper and flatter.",
          "Blurs what is far from the focus, as a camera's lens does. Off, everything stays sharp and the graphics card has a little less to do.",
          "The saturation, contrast, hue and temperature each place sets for its mood. Off, you see the scene's colors without that adjustment.",
          "Makes the image shimmer as through hot air, in the places that use it.",
          "Blurs the image outward from its center, in the moments that use it.",
          "Blurs the image when the camera moves fast, in the moments that use it.",
          "Skips the logo videos that play when the game starts: it goes straight to the language menu and then the title screen. The title screen's video and the levels' videos still play.",
      },
      "All set, captain!",
      "The game was installed. Set sail now, or first review the video, graphics and control settings; you can always "
      "change them in the launcher."};
  static const GameTexts kGerman{
      "Willkommen an Bord",
      "Bevor es losgeht, muss das Spiel installiert werden. Der Launcher entpackt die Dateien aus deiner Xbox-360-ISO; "
      "das musst du nur einmal tun.",
      {
          "Bloom", "Das Leuchten um Lichter und helle Flächen.",
          "Tiefenschärfe", "Was weit vom Fokus entfernt ist, wird unscharf.",
          "Farbkorrektur", "Die Farben und der Kontrast jedes Ortes.",
          "Hitzeflimmern", "Das Bild flimmert wie durch heiße Luft.",
          "Radiale Unschärfe", "Die Unschärfe von der Mitte nach außen.",
          "Bewegungsunschärfe", "Die Unschärfe schneller Kamerabewegungen.",
          "Intro-Videos überspringen", "Beim Start direkt zum Sprachmenü.",
      },
      {
          "Der Lichtschein um die Sonne, Feuer und die hellsten Flächen. Aus wirkt das Bild schärfer und flacher.",
          "Macht unscharf, was weit vom Fokus entfernt ist, wie ein Kameraobjektiv. Aus bleibt alles scharf, und die Grafikkarte hat etwas weniger zu tun.",
          "Sättigung, Kontrast, Farbton und Farbtemperatur, die jeder Ort für seine Stimmung setzt. Aus siehst du die Farben der Szene ohne diese Anpassung.",
          "Lässt das Bild wie durch heiße Luft flimmern, an den Orten, die es verwenden.",
          "Macht das Bild von der Mitte nach außen unscharf, in den Momenten, die es verwenden.",
          "Macht das Bild bei schnellen Kamerabewegungen unscharf, in den Momenten, die es verwenden.",
          "Überspringt die Logo-Videos beim Spielstart: Es geht direkt zum Sprachmenü und dann zum Titelbildschirm. Das Video des Titelbildschirms und die Videos der Level laufen weiterhin.",
      },
      "Alles bereit, Käpt'n!",
      "Das Spiel wurde installiert. Stich jetzt in See oder prüfe zuerst die Video-, Grafik- und "
      "Steuerungseinstellungen; du kannst sie jederzeit im Launcher ändern."};
  static const GameTexts kFrench{
      "Bienvenue à bord",
      "Avant de prendre la mer, il faut installer le jeu. Le launcher extrait les fichiers de votre ISO Xbox 360 ; vous "
      "n'aurez à le faire qu'une fois.",
      {
          "Bloom", "La lueur autour des lumières et des surfaces claires.",
          "Profondeur de champ", "Ce qui est loin du point net devient flou.",
          "Correction des couleurs", "Les couleurs et le contraste de chaque lieu.",
          "Brume de chaleur", "L'image ondule comme à travers l'air chaud.",
          "Flou radial", "Le flou du centre vers l'extérieur.",
          "Flou de mouvement", "Le flou des mouvements rapides de la caméra.",
          "Passer les vidéos d'intro", "Directement au menu des langues au démarrage.",
      },
      {
          "Le halo autour du soleil, du feu et des surfaces les plus claires. Désactivé, l'image est plus nette et plus plate.",
          "Floute ce qui est loin du point net, comme un objectif. Désactivée, tout reste net et la carte graphique a un peu moins de travail.",
          "La saturation, le contraste, la teinte et la température que chaque lieu fixe pour son ambiance. Désactivée, vous voyez les couleurs de la scène sans cet ajustement.",
          "Fait onduler l'image comme à travers l'air chaud, dans les lieux qui l'utilisent.",
          "Floute l'image du centre vers l'extérieur, dans les moments qui l'utilisent.",
          "Floute l'image quand la caméra bouge vite, dans les moments qui l'utilisent.",
          "Passe les vidéos des logos au lancement du jeu : il va directement au menu des langues, puis à l'écran titre. La vidéo de l'écran titre et celles des niveaux sont toujours lues.",
      },
      "Tout est prêt, capitaine !",
      "Le jeu est installé. Prenez la mer maintenant, ou vérifiez d'abord les paramètres vidéo, graphiques et de "
      "commandes ; vous pourrez toujours les modifier dans le launcher."};
  static const GameTexts kSpanish{
      "Bienvenido a bordo",
      "Antes de zarpar hay que instalar el juego. El launcher extrae los archivos desde tu ISO de Xbox 360; solo "
      "tendrás que hacerlo una vez.",
      {
          "Bloom", "El resplandor de luces y superficies brillantes.",
          "Profundidad de campo", "Lo que queda lejos del foco, desenfocado.",
          "Corrección de color", "Los colores y el contraste de cada lugar.",
          "Distorsión por calor", "La imagen ondula como con aire caliente.",
          "Desenfoque radial", "Desde el centro hacia afuera.",
          "Desenfoque de movimiento", "Al mover rápido la cámara.",
          "Saltar videos de inicio", "Directo al menú de idioma al arrancar.",
      },
      {
          "El halo de luz alrededor del sol, el fuego y las superficies más brillantes. Desactivado, la imagen se ve más nítida y plana.",
          "Desenfoca lo que queda lejos del foco, como el objetivo de una cámara. Desactivada, todo se ve nítido y la tarjeta gráfica trabaja un poco menos.",
          "La saturación, el contraste, el tono y la temperatura que cada lugar aplica a su ambiente. Desactivada, ves los colores de la escena sin ese ajuste.",
          "Hace ondular la imagen como el aire caliente, en los lugares que la usan.",
          "Desenfoca la imagen desde el centro hacia afuera, en los momentos que lo usan.",
          "Desenfoca la imagen cuando la cámara se mueve rápido, en los momentos que lo usan.",
          "Salta los videos de logotipos que se reproducen al iniciar el juego: pasa directo al menú de idioma y luego a la pantalla de título. El video de la pantalla de título y los de los niveles se siguen reproduciendo.",
      },
      "¡Todo listo, capitán!",
      "El juego se instaló correctamente. Puedes zarpar ya o revisar primero los ajustes de video, gráficos y "
      "controles; siempre podrás cambiarlos desde el launcher."};
  static const GameTexts kItalian{
      "Benvenuto a bordo",
      "Prima di salpare bisogna installare il gioco. Il launcher estrae i file dalla tua ISO Xbox 360; dovrai farlo "
      "una sola volta.",
      {
          "Bloom", "Il bagliore intorno a luci e superfici chiare.",
          "Profondità di campo", "Ciò che è lontano dal fuoco diventa sfocato.",
          "Correzione colore", "I colori e il contrasto di ogni luogo.",
          "Foschia di calore", "L'immagine ondula come attraverso l'aria calda.",
          "Sfocatura radiale", "Dal centro verso l'esterno.",
          "Sfocatura di movimento", "Nei movimenti rapidi della telecamera.",
          "Salta i video iniziali", "Subito al menu della lingua all'avvio.",
      },
      {
          "L'alone di luce intorno al sole, al fuoco e alle superfici più chiare. Disattivato, l'immagine è più nitida e più piatta.",
          "Sfoca ciò che è lontano dal fuoco, come l'obiettivo di una fotocamera. Disattivata, tutto resta nitido e la scheda grafica lavora un po' meno.",
          "La saturazione, il contrasto, la tonalità e la temperatura che ogni luogo imposta per la sua atmosfera. Disattivata, vedi i colori della scena senza questa regolazione.",
          "Fa ondulare l'immagine come attraverso l'aria calda, nei luoghi che la usano.",
          "Sfoca l'immagine dal centro verso l'esterno, nei momenti che la usano.",
          "Sfoca l'immagine quando la telecamera si muove velocemente, nei momenti che la usano.",
          "Salta i video dei loghi all'avvio del gioco: si passa subito al menu della lingua e poi alla schermata del titolo. Il video della schermata del titolo e quelli dei livelli vengono comunque riprodotti.",
      },
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

const std::vector<GameControlGroup>& GameControlsFor(uint32_t) {
  static const std::vector<GameControlGroup> none;
  return none;
}

}  // namespace launcher::game
