#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Convert the actual LCD frames emitted by dino_ui_test into review images."""
from pathlib import Path
import argparse
from PIL import Image, ImageDraw

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("directory", nargs="?", type=Path, default=Path("build/dino-ui"))
args = parser.parse_args()
names = ("live", "presets", "performance", "save", "drums", "bank-full")
sheet = Image.new("RGB", (512, 816), "#11111b")
draw = ImageDraw.Draw(sheet)
for index, name in enumerate(names):
    picture = Image.open(args.directory / f"{name}.ppm").convert("RGB")
    assert picture.size == (240, 240)
    picture.save(args.directory / f"{name}.png")
    x, y = 8 + index % 2 * 256, 8 + index // 2 * 272
    draw.text((x + 4, y), name.upper(), fill="#dddded")
    sheet.paste(picture, (x, y + 20))
sheet.save(args.directory / "ui-contact-sheet.png")
print(f"Actual LCD previews: {args.directory / 'ui-contact-sheet.png'}")
