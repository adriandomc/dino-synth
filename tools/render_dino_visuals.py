#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Package the actual 240x240 LCD frames emitted by tests/dinoperro_vis_test.c."""
from pathlib import Path
import argparse
from PIL import Image, ImageDraw


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("directory", nargs="?", type=Path, default=Path("build/dino-visuals"))
    args = parser.parse_args()
    names = ("COSMIC DOG", "HALO", "NEBULA", "CONWAY DREAM")
    width = 240 * len(names)
    scenarios = ((7, "IDLE"), (15, "TRACK 1 / LOW NOTE"), (23, "TRACK 3 / HIGH NOTE"),
                 (31, "TRACKS 1 + 3"), (39, "REDUCED DETAIL"), (47, "RELEASE"))
    frames = [[Image.open(args.directory / f"cosmic-{style}-{frame:02}.ppm").convert("RGB")
               for frame in range(48)] for style in range(len(names))]
    assert all(frame.size == (240, 240) for sequence in frames for frame in sequence)
    sheet = Image.new("RGB", (width, 262 * len(scenarios)), "#11111b")
    draw = ImageDraw.Draw(sheet)
    for row, (frame, label) in enumerate(scenarios):
        for style, name in enumerate(names):
            sheet.paste(frames[style][frame], (240 * style, 262 * row + 22))
            draw.text((240 * style + 8, 262 * row + 5), f"{name} / {label}", fill="#dddded")
    sheet.save(args.directory / "cosmic-contact-sheet.png")
    sheet.crop((0, 262 * 3, width, 262 * 4)).save(args.directory / "cosmic-preview.png")
    frames[3][31].save(args.directory / "conway-preview.png")
    frames[3][0].save(args.directory / "conway-animation.gif", save_all=True, append_images=frames[3][1:],
                      duration=100, loop=0, optimize=False)
    animation = []
    for frame in range(48):
        picture = Image.new("RGB", (width, 262), "#11111b")
        draw = ImageDraw.Draw(picture)
        for style, name in enumerate(names):
            picture.paste(frames[style][frame], (240 * style, 22))
            draw.text((240 * style + 8, 5), name, fill="#dddded")
        animation.append(picture)
    animation[0].save(args.directory / "cosmic-animation.gif", save_all=True, append_images=animation[1:],
                      duration=100, loop=0, optimize=False)
    print(f"Actual LCD previews: {args.directory / 'cosmic-preview.png'}")
    print(f"Contact sheet: {args.directory / 'cosmic-contact-sheet.png'}")
    print(f"Animation: {args.directory / 'cosmic-animation.gif'}")


if __name__ == "__main__":
    main()
