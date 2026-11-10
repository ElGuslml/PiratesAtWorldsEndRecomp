"""Makes the button pictures the game shows for controllers whose art is not on its disc (src/button_prompts.cpp).

They come from Nicolae "Xelu" Berbece's Free Controller & Key Prompts (CC0, https://thoseawesomeguys.com/prompts/),
retouched to the game's two sizes:
- a font glyph: 32 pixels high, as wide as the picture needs (the game's are 32 or 64 wide);
- a HUD sprite: 128x128 (the game's button sprites).
The retouch: the picture cropped to what it draws, scaled, given a dark outline so it reads on any scene, and:
- the PS4's buttons (a dark pill with its word under it): made light, the word beside the pill in a glyph;
- the PS5's (a light pill under its symbol): the symbol beside the pill in a glyph;
- the Xbox Series' MENU and VIEW (a dark round button, a light symbol): made light with the symbol dark, as the 360's
  START and BACK in the game. An Xbox One's are the same buttons.

    python res/prompts/make_prompts.py <Xelu's pack folder> [--preview preview.png]

And the keyboard's keys (Xelu's light ones), at two scales, packed into keys_glyph.png and keys_sprite.png with their
index keys.txt, for src/keyboard_prompts.cpp to put the prompts together from the bindings.

Writes the files next to this script; res/game.rc embeds them. Pillow and NumPy.
"""
import argparse
import os

import numpy as np
from PIL import Image, ImageFilter

HERE = os.path.dirname(os.path.abspath(__file__))
OUTLINE = (16, 16, 18)


def load(pack, name):
    return Image.open(os.path.join(pack, name)).convert("RGBA")


def crop(im):
    return im.crop(im.getchannel("A").point(lambda a: 255 if a > 8 else 0).getbbox())


def lighten(im):
    """The PS4 icons are dark grey: their lightness turned over (dark pill, light rim -> light pill, dark rim)."""
    a = np.array(im).astype(np.float32)
    rgb = a[:, :, :3]
    a[:, :, :3] = 255.0 - rgb * 0.85
    return Image.fromarray(a.clip(0, 255).astype(np.uint8), "RGBA")


def light_round(im):
    """The Xbox Series' round buttons (a dark grey face, a light symbol, a dark rim) made light as the 360's START and
    BACK in the game: a light face, the symbol dark on it, the rim kept dark. Their greys are flat: rim 27, ring 49,
    face 79, a highlight 98 at the ring's inner edge, the symbol 153 in the middle (within 17 pixels of the center)."""
    a = np.array(im).astype(np.float32)
    lum = a[:, :, 0]
    base = np.interp(lum, [27, 49, 79, 98], [40, 150, 222, 240])
    y, x = np.mgrid[0:a.shape[0], 0:a.shape[1]]
    inner = np.hypot(x - (a.shape[1] - 1) / 2, y - (a.shape[0] - 1) / 2) < 20 * a.shape[0] / 100
    symbol = np.where(inner, np.clip((lum - 79) / (153 - 79), 0, 1), 0)
    base = np.where(inner, 222, base)
    a[:, :, :3] = (base * (1 - symbol) + 45 * symbol)[:, :, None]
    return Image.fromarray(a.clip(0, 255).astype(np.uint8), "RGBA")


def glyph_round(src):
    """A round button in a 32x32 glyph: the circle 31 pixels across, as the game's A B X Y; its dark rim is its
    outline."""
    return place(fit(crop(src), 31, 31), 32, 32)


def white(im):
    """A symbol or a word in white, as the game's text (its shading would read grey beside it)."""
    out = Image.new("RGBA", im.size, (255, 255, 255, 0))
    out.putalpha(im.getchannel("A"))
    return out


def fit(im, width, height):
    scale = min(width / im.width, height / im.height)
    size = (max(1, round(im.width * scale)), max(1, round(im.height * scale)))
    return im.resize(size, Image.LANCZOS)


def outlined(im, radius):
    """`im` over a dark copy of its shape grown by `radius` pixels (on a canvas grown as much)."""
    pad = radius + 1
    canvas = Image.new("RGBA", (im.width + 2 * pad, im.height + 2 * pad), (0, 0, 0, 0))
    canvas.alpha_composite(im, (pad, pad))
    alpha = canvas.getchannel("A").filter(ImageFilter.MaxFilter(2 * radius + 1))
    alpha = alpha.point(lambda a: min(255, a * 2))
    under = Image.new("RGBA", canvas.size, OUTLINE + (0,))
    under.putalpha(alpha.point(lambda a: a * 220 // 255))
    under.alpha_composite(canvas)
    return under


def place(im, width, height):
    out = Image.new("RGBA", (width, height), (0, 0, 0, 0))
    out.alpha_composite(im, ((width - im.width) // 2, (height - im.height) // 2))
    return out


def glyph_symbol_beside(src, split_row):
    """The PS5's pill under its symbol: the symbol beside the pill, so the pill takes the glyph's height. As wide as
    it needs (up to 64), 32 high."""
    symbol = white(crop(src.crop((0, 0, src.width, split_row))))
    pill = crop(src.crop((0, split_row, src.width, src.height)))
    scale = 29 / pill.height
    pill = pill.resize((round(pill.width * scale), 29), Image.LANCZOS)
    symbol = symbol.resize((round(symbol.width * scale * 1.3), round(symbol.height * scale * 1.3)), Image.LANCZOS)
    row = Image.new("RGBA", (symbol.width + 2 + pill.width, pill.height), (0, 0, 0, 0))
    row.alpha_composite(symbol, (0, max(0, pill.height // 3 - symbol.height // 2)))
    row.alpha_composite(pill, (symbol.width + 2, 0))
    row = outlined(row, 1)
    return place(row, min(64, (row.width + 3) // 4 * 4), 32)


def glyph_pill_and_word(src, split_row):
    """The PS4's pill above its word: the pill on the left, the word on its right, the word as high as the game's
    capitals (15 pixels), in a glyph as wide as that needs (up to 124), 32 high."""
    pill = crop(src.crop((0, 0, src.width, split_row)))
    word = white(crop(src.crop((0, split_row, src.width, src.height))))
    pill = fit(pill, 16, 28)
    word = word.resize((round(word.width * 15 / word.height), 15), Image.LANCZOS)
    row = Image.new("RGBA", (pill.width + 3 + word.width, max(pill.height, word.height)), (0, 0, 0, 0))
    row.alpha_composite(pill, (0, (row.height - pill.height) // 2))
    row.alpha_composite(word, (pill.width + 3, (row.height - word.height) // 2))
    row = outlined(row, 1)
    return place(row, min(124, (row.width + 3) // 4 * 4), 32)


def sprite(src):
    """A 128x128 HUD sprite: the picture as large as the game's own (about 112 pixels)."""
    return place(outlined(fit(crop(src), 108, 108), 3), 128, 128)


def make(pack):
    ps5_options = load(pack, "PS5/PS5_Options.png")
    ps5_create = load(pack, "PS5/PS5_Share.png")
    ps4_options = lighten(load(pack, "Others/PS4/PS4_Options.png"))
    ps4_share = lighten(load(pack, "Others/PS4/PS4_Share.png"))
    xbox_menu = light_round(load(pack, "Xbox Series/XboxSeriesX_Menu.png"))
    xbox_view = light_round(load(pack, "Xbox Series/XboxSeriesX_View.png"))
    split = 72  # the PS4 pictures' empty rows between the pill and the word
    ps5_split = 24  # the PS5 pictures' empty rows between the symbol and the pill
    return {
        "xbox_menu_glyph": glyph_round(xbox_menu),
        "xbox_view_glyph": glyph_round(xbox_view),
        "xbox_menu_sprite": sprite(xbox_menu),
        "xbox_view_sprite": sprite(xbox_view),
        "ps5_options_glyph": glyph_symbol_beside(ps5_options, ps5_split),
        "ps5_create_glyph": glyph_symbol_beside(ps5_create, ps5_split),
        "ps4_options_glyph": glyph_pill_and_word(ps4_options, split),
        "ps4_share_glyph": glyph_pill_and_word(ps4_share, split),
        "ps5_options_sprite": sprite(ps5_options),
        "ps5_create_sprite": sprite(ps5_create),
        "ps4_options_sprite": sprite(ps4_options),
        "ps4_share_sprite": sprite(ps4_share),
    }


# The keyboard's keys, composed at run time from the bindings (src/keyboard_prompts.cpp): Xelu's light keys (a white
# cap, a dark rim and letter), cropped, at two scales, packed into two atlases with an index.
KEY_SUFFIX = "_Key_Light.png"
KEY_GLYPH_SQUARE = 32  # a square key's height in a glyph (32 high), as the game's buttons
KEY_GLYPH_MAX = 32
KEY_SPRITE_SQUARE = 96  # a square key's height at the sprites' scale (fitted into 112x112 at run time)
# In a glyph, a key whose word is too small to read at 30 pixels takes Xelu's symbol for it.
KEY_GLYPH_SOURCE = {"Enter": "Enter_Alt", "Backspace": "Backspace_Alt"}


def key_pictures(pack):
    folder = os.path.join(pack, "Keyboard & Mouse", "Light")
    return {f[: -len(KEY_SUFFIX)]: crop(Image.open(os.path.join(folder, f)).convert("RGBA"))
            for f in sorted(os.listdir(folder)) if f.endswith(KEY_SUFFIX)}


def scaled(im, scale, max_height=None):
    if max_height:
        scale = min(scale, max_height / im.height)
    return im.resize((max(1, round(im.width * scale)), max(1, round(im.height * scale))), Image.LANCZOS)


def pack_atlas(pictures, width, gutter=2):
    """Shelves, the tallest first: (atlas, {name: (x, y, w, h)})."""
    order = sorted(pictures, key=lambda n: (-pictures[n].height, n))
    places, x, y, shelf = {}, 0, 0, 0
    for n in order:
        im = pictures[n]
        if x + im.width > width:
            x, y, shelf = 0, y + shelf + gutter, 0
        places[n] = (x, y, im.width, im.height)
        x += im.width + gutter
        shelf = max(shelf, im.height)
    atlas = Image.new("RGBA", (width, y + shelf), (0, 0, 0, 0))
    for n, (x, y, w, h) in places.items():
        atlas.alpha_composite(pictures[n], (x, y))
    return atlas, places


def make_keys(pack):
    """The two atlases and their index: a line per key, `name gx gy gw gh sx sy sw sh`, the name Xelu's."""
    keys = key_pictures(pack)
    square = keys["A"].height
    glyphs = {n: scaled(keys[KEY_GLYPH_SOURCE.get(n, n)], KEY_GLYPH_SQUARE / square, KEY_GLYPH_MAX) for n in keys}
    sprites = {n: scaled(keys[n], KEY_SPRITE_SQUARE / square) for n in keys}
    glyph_atlas, glyph_places = pack_atlas(glyphs, 512)
    sprite_atlas, sprite_places = pack_atlas(sprites, 1024, 4)
    index = "".join("%s %d %d %d %d %d %d %d %d\n" % ((n,) + glyph_places[n] + sprite_places[n]) for n in sorted(keys))
    return glyph_atlas, sprite_atlas, index


def preview(pictures, path):
    """The pictures at their size and three times it, on a dark scene and on parchment."""
    names = list(pictures)
    width = sum(max(pictures[n].width * 3, 64) + 16 for n in names)
    sheet = Image.new("RGBA", (width, 2 * (128 * 3 + 160)), (0, 0, 0, 255))
    for row, background in enumerate([(34, 38, 46, 255), (196, 178, 140, 255)]):
        top = row * (128 * 3 + 160)
        sheet.paste(Image.new("RGBA", (width, 128 * 3 + 160), background), (0, top))
        x = 8
        for n in names:
            im = pictures[n]
            sheet.alpha_composite(im, (x, top + 8))
            big = im.resize((im.width * 3, im.height * 3), Image.NEAREST)
            sheet.alpha_composite(big, (x, top + 8 + 136))
            x += max(im.width * 3, 64) + 16
    sheet.save(path)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("pack")
    parser.add_argument("--preview")
    args = parser.parse_args()
    pictures = make(args.pack)
    for name, im in pictures.items():
        im.save(os.path.join(HERE, name + ".png"), optimize=True)
    glyph_atlas, sprite_atlas, index = make_keys(args.pack)
    glyph_atlas.save(os.path.join(HERE, "keys_glyph.png"), optimize=True)
    sprite_atlas.save(os.path.join(HERE, "keys_sprite.png"), optimize=True)
    with open(os.path.join(HERE, "keys.txt"), "w", newline="\n") as f:
        f.write(index)
    if args.preview:
        preview(pictures, args.preview)
    print(f"{len(pictures)} pictures and {index.count(chr(10))} keys in {HERE}")


if __name__ == "__main__":
    main()
