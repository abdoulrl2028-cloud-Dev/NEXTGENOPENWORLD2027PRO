#!/usr/bin/env python3
"""Build the menu font atlas and a short original opening loop."""

import math
import struct
import wave
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

LABELS = [
    "NEXT GEN OPEN WORLD 2027 PRO",
    "NOVO JOGO",
    "CONTINUAR",
    "CONFIGURAÇÕES",
    "IDIOMA",
    "SAIR",
    "NEW GAME",
    "CONTINUE",
    "SETTINGS",
    "LANGUAGE",
    "EXIT",
    "NOUVELLE PARTIE",
    "CONTINUER",
    "PARAMÈTRES",
    "LANGUE",
    "QUITTER",
    "GRÁFICOS BAIXO",
    "GRÁFICOS MÉDIO",
    "GRÁFICOS ALTO",
    "GRAPHICS LOW",
    "GRAPHICS MEDIUM",
    "GRAPHICS HIGH",
    "GRAPHIQUES BAS",
    "GRAPHIQUES MOYEN",
    "GRAPHIQUES ÉLEVÉ",
    "VOLUME - 100%",
    "VOLUME + 100%",
    "OLHAR - 2.0",
    "OLHAR + 2.0",
    "LOOK - 2.0",
    "LOOK + 2.0",
    "REGARD - 2.0",
    "REGARD + 2.0",
    "INVERTER OLHAR: SIM",
    "INVERTER OLHAR: NÃO",
    "INVERT LOOK: ON",
    "INVERT LOOK: OFF",
    "INVERSER REGARD: OUI",
    "INVERSER REGARD: NON",
    "PORTUGUÊS",
    "ENGLISH",
    "FRANÇAIS",
    "VOLTAR",
    "BACK",
    "RETOUR",
    "ADICIONE images/menu_background.png",
    "ADD images/menu_background.png",
    "AJOUTER images/menu_background.png",
    "CENA 3D INDISPONIVEL",
]


def write_font(root: Path) -> None:
    font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 42)
    chars = []
    seen = set()
    for label in LABELS:
        for char in label:
            if char not in seen:
                seen.add(char)
                chars.append(char)
    glyphs = []
    ascent, _descent = font.getmetrics()
    for char in chars:
        bbox = font.getbbox(char)
        left, top, right, bottom = bbox
        width = max(0, right - left)
        height = max(0, bottom - top)
        advance = max(1, int(round(font.getlength(char))))
        image = None
        if width > 0 and height > 0:
            image = Image.new("L", (width, height), 0)
            ImageDraw.Draw(image).text((-left, -top), char, font=font, fill=255)
        glyphs.append((ord(char), left, ascent - top, advance, image))

    padding = 2
    atlas_size = 1024
    atlas = Image.new("RGBA", (atlas_size, atlas_size), (0, 0, 0, 0))
    x = padding
    y = padding
    row_height = 0
    metrics = ["atlas {} {}".format(atlas_size, atlas_size), "line 48"]
    for codepoint, bearing_x, bearing_y, advance, image in glyphs:
        width = 0 if image is None else image.width
        height = 0 if image is None else image.height
        if image is not None and x + width + padding > atlas_size:
            x = padding
            y += row_height + padding
            row_height = 0
        if image is not None:
            rgba = Image.merge("RGBA", (image, image, image, image))
            atlas.paste(rgba, (x, y), rgba)
        metrics.append(
            "{} {} {} {} {} {} {} {}".format(
                codepoint, x, y, width, height, bearing_x, bearing_y, advance
            )
        )
        if image is not None:
            x += width + padding
            row_height = max(row_height, height)
    if y + row_height > atlas_size:
        raise SystemExit("font atlas is too small")
    out_dir = root / "app/src/main/assets/ui"
    out_dir.mkdir(parents=True, exist_ok=True)
    atlas.save(out_dir / "ui_font.png")
    (out_dir / "ui_font.metrics").write_text("\n".join(metrics) + "\n", encoding="utf-8")
    print("font glyphs", len(glyphs), "rows_end", y + row_height)


def write_audio(root: Path) -> None:
    rate = 22050
    seconds = 4
    count = rate * seconds
    frames = bytearray()
    for index in range(count):
        t = index / rate
        sample = (
            0.22 * math.sin(2.0 * math.pi * 220.0 * t)
            + 0.12 * math.sin(2.0 * math.pi * 330.0 * t)
            + 0.06 * math.sin(2.0 * math.pi * 440.0 * t)
        )
        sample *= 0.55 + 0.45 * math.sin(2.0 * math.pi * 0.5 * t)
        frames += struct.pack("<h", int(max(-1.0, min(1.0, sample)) * 14000))
    out = root / "app/src/main/assets/audio/menu_theme.wav"
    out.parent.mkdir(parents=True, exist_ok=True)
    with wave.open(str(out), "w") as handle:
        handle.setnchannels(1)
        handle.setsampwidth(2)
        handle.setframerate(rate)
        handle.writeframes(frames)
    print("audio", out, out.stat().st_size)


if __name__ == "__main__":
    project = Path(__file__).resolve().parents[1]
    write_font(project)
    write_audio(project)
