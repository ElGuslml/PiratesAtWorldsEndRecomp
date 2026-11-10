"""Makes the button pictures the game shows for controllers whose art is not on its disc (src/button_prompts.cpp).

They come from Nicolae "Xelu" Berbece's Free Controller & Key Prompts (CC0, https://thoseawesomeguys.com/prompts/),
retouched to the game's two sizes:
- a font glyph: 32 pixels high, as wide as the picture needs (the game's are 32 or 64 wide);
- a HUD sprite: 128x128 (the game's button sprites).
The retouch: the picture cropped to what it draws, scaled, given a dark outline so it reads on any scene, and, for
the PS4's buttons (a dark pill with its word under it), made light and laid out with the word beside the pill in a
glyph.

    python res/prompts/make_prompts.py <Xelu's pack folder> [--preview preview.png]

Writes the PNG files next to this script; res/game.rc embeds them. Pillow and NumPy.
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
    split = 72  # the PS4 pictures' empty rows between the pill and the word
    ps5_split = 24  # the PS5 pictures' empty rows between the symbol and the pill
    return {
        "ps5_options_glyph": glyph_symbol_beside(ps5_options, ps5_split),
        "ps5_create_glyph": glyph_symbol_beside(ps5_create, ps5_split),
        "ps4_options_glyph": glyph_pill_and_word(ps4_options, split),
        "ps4_share_glyph": glyph_pill_and_word(ps4_share, split),
        "ps5_options_sprite": sprite(ps5_options),
        "ps5_create_sprite": sprite(ps5_create),
        "ps4_options_sprite": sprite(ps4_options),
        "ps4_share_sprite": sprite(ps4_share),
    }


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
    if args.preview:
        preview(pictures, args.preview)
    print(f"{len(pictures)} pictures in {HERE}")


if __name__ == "__main__":
    main()
