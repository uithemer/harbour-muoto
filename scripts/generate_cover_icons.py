#!/usr/bin/env python3
"""Generate wireframe cover icon options (theme + font)."""

from __future__ import annotations

import math
from dataclasses import dataclass
from pathlib import Path

from PIL import Image, ImageDraw

SIZE = 128
STROKE = 2
WHITE = (255, 255, 255, 255)
BLACK = (0, 0, 0, 255)
TRANSPARENT = (0, 0, 0, 0)
MARGIN = 22
ROOT = Path(__file__).resolve().parents[1]
OUT_DIR = ROOT / "images" / "icon-options"
IMAGES_DIR = ROOT / "images"


@dataclass(frozen=True)
class DrawStyle:
    stroke: int = STROKE
    color: tuple[int, int, int, int] = WHITE


STYLE = DrawStyle()


def canvas() -> tuple[Image.Image, ImageDraw.ImageDraw]:
    img = Image.new("RGBA", (SIZE, SIZE), TRANSPARENT)
    return img, ImageDraw.Draw(img)


def rounded_rect(
    draw: ImageDraw.ImageDraw,
    box: tuple[float, float, float, float],
    radius: float,
    *,
    outline: tuple[int, int, int, int] | None = None,
    width: int | None = None,
) -> None:
    x0, y0, x1, y1 = box
    r = min(radius, (x1 - x0) / 2, (y1 - y0) / 2)
    draw.rounded_rectangle(
        box,
        radius=r,
        outline=outline if outline is not None else STYLE.color,
        width=width if width is not None else STYLE.stroke,
    )


def circle(
    draw: ImageDraw.ImageDraw,
    cx: float,
    cy: float,
    r: float,
    *,
    outline: tuple[int, int, int, int] | None = None,
    width: int | None = None,
) -> None:
    draw.ellipse(
        (cx - r, cy - r, cx + r, cy + r),
        outline=outline if outline is not None else STYLE.color,
        width=width if width is not None else STYLE.stroke,
    )


def line(
    draw: ImageDraw.ImageDraw,
    x0: float,
    y0: float,
    x1: float,
    y1: float,
    *,
    outline: tuple[int, int, int, int] | None = None,
    width: int | None = None,
) -> None:
    draw.line(
        (x0, y0, x1, y1),
        fill=outline if outline is not None else STYLE.color,
        width=width if width is not None else STYLE.stroke,
    )


def arc(
    draw: ImageDraw.ImageDraw,
    box: tuple[float, float, float, float],
    start: float,
    end: float,
    *,
    outline: tuple[int, int, int, int] | None = None,
    width: int | None = None,
) -> None:
    draw.arc(
        box,
        start,
        end,
        fill=outline if outline is not None else STYLE.color,
        width=width if width is not None else STYLE.stroke,
    )


def render(style: DrawStyle, fn) -> Image.Image:
    global STYLE
    prev = STYLE
    STYLE = style
    try:
        return fn()
    finally:
        STYLE = prev


# --- Theme icons -------------------------------------------------------------

def theme_palette() -> Image.Image:
    """Three overlapping circles — color / palette."""
    img, draw = canvas()
    r = 22
    circle(draw, 52, 58, r)
    circle(draw, 76, 58, r)
    circle(draw, 64, 74, r)
    return img


def theme_grid() -> Image.Image:
    """2×2 rounded squares — UI / icon grid."""
    img, draw = canvas()
    gap = 8
    side = (SIZE - 2 * MARGIN - gap) / 2
    x0 = MARGIN
    y0 = MARGIN + 4
    for row in range(2):
        for col in range(2):
            x = x0 + col * (side + gap)
            y = y0 + row * (side + gap)
            rounded_rect(draw, (x, y, x + side, y + side), radius=10)
    return img


def theme_brush() -> Image.Image:
    """Diagonal paint brush."""
    img, draw = canvas()
    line(draw, 34, 90, 72, 52, width=STYLE.stroke + 1)
    line(draw, 72, 52, 82, 42, width=STYLE.stroke + 2)
    for angle in (-28, -14, 0, 14, 28):
        rad = math.radians(angle - 45)
        x0, y0 = 82, 42
        x1 = x0 + 26 * math.cos(rad)
        y1 = y0 + 26 * math.sin(rad)
        line(draw, x0, y0, x1, y1)
    return img


def theme_swatch() -> Image.Image:
    """Rounded rectangle with diagonal gradient lines."""
    img, draw = canvas()
    box = (MARGIN, MARGIN + 6, SIZE - MARGIN, SIZE - MARGIN + 2)
    rounded_rect(draw, box, radius=14)
    x0, y0, x1, y1 = box
    step = 12
    t = 0
    while True:
        sx = x0 + 8 + t
        sy = y1 - 8
        ex = sx + (y1 - y0) - 16
        ey = y0 + 8
        if sx > x1 - 8:
            break
        if ex > x0 + 8 and sy > y0 + 8:
            line(draw, max(sx, x0 + 8), sy, min(ex, x1 - 8), max(ey, y0 + 8))
        t += step
    return img


def theme_layers() -> Image.Image:
    """Stacked sheets — theme layers."""
    img, draw = canvas()
    offsets = [(0, 0), (10, 8), (20, 16)]
    w, h = 62, 46
    x_base = MARGIN + 4
    y_base = MARGIN + 10
    for dx, dy in offsets:
        rounded_rect(draw, (x_base + dx, y_base + dy, x_base + dx + w, y_base + dy + h), radius=8)
    return img


# --- Font icons --------------------------------------------------------------

def font_stylized_a() -> Image.Image:
    """Decorative wireframe A (similar to existing font.png)."""
    img, draw = canvas()
    line(draw, 38, 96, 52, 38)
    line(draw, 52, 38, 88, 96)
    arc(draw, (72, 78, 98, 104), 200, 340)
    line(draw, 46, 68, 78, 68)
    return img


def font_aa_pair() -> Image.Image:
    """Uppercase A + lowercase a."""
    img, draw = canvas()
    line(draw, 28, 92, 44, 40)
    line(draw, 44, 40, 60, 92)
    line(draw, 34, 72, 54, 72)
    circle(draw, 88, 68, 16)
    line(draw, 104, 56, 104, 92)
    return img


def font_serif_t() -> Image.Image:
    """Serif T — classic typography mark."""
    img, draw = canvas()
    cx = 64
    line(draw, 30, 38, 98, 38)
    line(draw, 30, 38, 30, 46)
    line(draw, 98, 38, 98, 46)
    line(draw, cx, 38, cx, 96)
    line(draw, cx - 10, 96, cx + 10, 96)
    return img


def font_text_lines() -> Image.Image:
    """Three lines of varying length — body text."""
    img, draw = canvas()
    y = 40
    lengths = (74, 58, 66)
    for length in lengths:
        line(draw, 64 - length / 2, y, 64 + length / 2, y)
        y += 18
    return img


def font_size_contrast() -> Image.Image:
    """Large A and small a with size bracket."""
    img, draw = canvas()
    line(draw, 30, 88, 46, 36)
    line(draw, 46, 36, 62, 88)
    line(draw, 36, 66, 56, 66)
    circle(draw, 88, 72, 10)
    line(draw, 98, 64, 98, 88)
    line(draw, 108, 34, 108, 90)
    line(draw, 108, 34, 114, 34)
    line(draw, 108, 90, 114, 90)
    return img


THEME_ICONS = [
    ("theme-01-palette", theme_palette),
    ("theme-02-grid", theme_grid),
    ("theme-03-brush", theme_brush),
    ("theme-04-swatch", theme_swatch),
    ("theme-05-layers", theme_layers),
]

FONT_ICONS = [
    ("font-01-stylized-a", font_stylized_a),
    ("font-02-aa-pair", font_aa_pair),
    ("font-03-serif-t", font_serif_t),
    ("font-04-text-lines", font_text_lines),
    ("font-05-size-contrast", font_size_contrast),
]

FINAL_STROKE = 6

FINAL_ICONS = [
    ("icon-dark.png", theme_grid, DrawStyle(stroke=FINAL_STROKE, color=WHITE)),
    ("icon-light.png", theme_grid, DrawStyle(stroke=FINAL_STROKE, color=BLACK)),
    ("font-dark.png", font_aa_pair, DrawStyle(stroke=FINAL_STROKE, color=WHITE)),
    ("font-light.png", font_aa_pair, DrawStyle(stroke=FINAL_STROKE, color=BLACK)),
]


def generate_options() -> None:
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    style = DrawStyle()
    for name, fn in THEME_ICONS + FONT_ICONS:
        path = OUT_DIR / f"{name}.png"
        render(style, fn).save(path, "PNG")
        print(f"wrote {path}")


def generate_final() -> None:
    IMAGES_DIR.mkdir(parents=True, exist_ok=True)
    for name, fn, style in FINAL_ICONS:
        path = IMAGES_DIR / name
        render(style, fn).save(path, "PNG")
        print(f"wrote {path}")


def main() -> None:
    generate_options()
    generate_final()


if __name__ == "__main__":
    main()
