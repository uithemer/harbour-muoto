#!/usr/bin/env python3
"""Generate square app icon alternatives — solid M + brush (current appicon style)."""

from __future__ import annotations

import math
from pathlib import Path

from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
OUT_ROOT = ROOT / "appicons" / "icon-options"
SIZES = (86, 108, 128, 172, 256)
CORNER_RATIO = 0.18

# Match harbour-muoto.png palette
GRAD_A = (0, 188, 212)
GRAD_B = (171, 71, 188)
GRAD_C = (236, 64, 122)

WHITE = (255, 255, 255, 255)
SHADE = (236, 234, 244, 255)       # fold / back ribbon
FERRULE = (228, 228, 235, 255)


def lerp(a: float, b: float, t: float) -> float:
    return a + (b - a) * t


def lerp_color(c0: tuple[int, int, int], c1: tuple[int, int, int], t: float) -> tuple[int, int, int]:
    return (
        int(lerp(c0[0], c1[0], t)),
        int(lerp(c0[1], c1[1], t)),
        int(lerp(c0[2], c1[2], t)),
    )


def tri_grad_color(t: float) -> tuple[int, int, int]:
    t = max(0.0, min(1.0, t))
    if t < 0.5:
        return lerp_color(GRAD_A, GRAD_B, t * 2)
    return lerp_color(GRAD_B, GRAD_C, (t - 0.5) * 2)


def rounded_gradient_square(size: int) -> Image.Image:
    radius = int(size * CORNER_RATIO)
    grad = Image.new("RGBA", (size, size))
    px = grad.load()
    for y in range(size):
        for x in range(size):
            t = (x * 0.65 + y * 0.85) / size
            px[x, y] = (*tri_grad_color(t), 255)

    mask = Image.new("L", (size, size), 0)
    ImageDraw.Draw(mask).rounded_rectangle((0, 0, size - 1, size - 1), radius=radius, fill=255)
    out = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    out.paste(grad, (0, 0), mask)
    return out


def canvas(size: int) -> tuple[Image.Image, ImageDraw.ImageDraw, float]:
    img = rounded_gradient_square(size)
    return img, ImageDraw.Draw(img), size / 256.0


def pt(s: float, x: float, y: float) -> tuple[int, int]:
    return int(x * s), int(y * s)


def rad(s: float, r: float) -> int:
    return max(2, int(r * s))


def circle_pts(cx: float, cy: float, r: float, steps: int = 20) -> list[tuple[float, float]]:
    return [
        (cx + r * math.cos(2 * math.pi * i / steps), cy + r * math.sin(2 * math.pi * i / steps))
        for i in range(steps)
    ]


def capsule_pts(a: tuple[float, float], b: tuple[float, float], r: float, steps: int = 10) -> list[tuple[float, float]]:
    ax, ay = a
    bx, by = b
    dx, dy = bx - ax, by - ay
    length = math.hypot(dx, dy)
    if length < 1e-6:
        return circle_pts(ax, ay, r, steps * 2)

    ux, uy = dx / length, dy / length
    px, py = -uy * r, ux * r
    pts: list[tuple[float, float]] = []

    for i in range(steps + 1):
        t = math.pi * i / steps
        cx, cy = ax - ux * r, ay - uy * r
        pts.append((cx + r * math.cos(t) * (-px / r) + r * math.sin(t) * (-ux), cy + r * math.cos(t) * (-py / r) + r * math.sin(t) * (-uy)))

    # Simpler capsule: body + round caps
    pts = [
        (ax + px, ay + py),
        (bx + px, by + py),
    ]
    for i in range(steps + 1):
        ang = -math.pi / 2 + math.pi * i / steps
        pts.append((bx + r * math.cos(ang) * ux + r * math.sin(ang) * px / r, by + r * math.cos(ang) * uy + r * math.sin(ang) * py / r))
    pts.extend([(bx - px, by - py), (ax - px, ay - py)])
    for i in range(steps + 1):
        ang = math.pi / 2 + math.pi * i / steps
        pts.append((ax + r * math.cos(ang) * ux + r * math.sin(ang) * (-px / r), ay + r * math.cos(ang) * uy + r * math.sin(ang) * (-py / r)))
    return pts


def fill_capsule(
    draw: ImageDraw.ImageDraw,
    s: float,
    a: tuple[float, float],
    b: tuple[float, float],
    radius: float,
    fill: tuple[int, int, int, int] = WHITE,
) -> None:
    draw.polygon([pt(s, x, y) for x, y in capsule_pts(a, b, radius)], fill=fill)


def fill_disc(
    draw: ImageDraw.ImageDraw,
    s: float,
    x: float,
    y: float,
    radius: float,
    fill: tuple[int, int, int, int] = WHITE,
) -> None:
    r = rad(s, radius)
    cx, cy = pt(s, x, y)
    draw.ellipse((cx - r, cy - r, cx + r, cy + r), fill=fill)


def draw_ferrule_band(
    draw: ImageDraw.ImageDraw,
    s: float,
    cx: float,
    y: float,
    *,
    width: float = 26,
    height: float = 14,
) -> None:
    x0, y0 = pt(s, cx - width / 2, y)
    x1, y1 = pt(s, cx + width / 2, y + height)
    draw.rounded_rectangle((x0, y0, x1, y1), radius=rad(s, 3), fill=FERRULE)


def draw_brush_handle(
    draw: ImageDraw.ImageDraw,
    s: float,
    cx: float,
    y0: float,
    y1: float,
    radius: float,
) -> None:
    fill_capsule(draw, s, (cx, y0), (cx, y1), radius)


def draw_brush_tip(
    draw: ImageDraw.ImageDraw,
    s: float,
    cx: float,
    y: float,
    *,
    half_width: float = 13,
    length: float = 24,
) -> None:
    draw.polygon(
        [
            pt(s, cx - half_width, y),
            pt(s, cx + half_width, y),
            pt(s, cx, y + length),
        ],
        fill=WHITE,
    )


# M geometry (256-space) — tuned to current harbour-muoto proportions
M = {
    "lb": (66.0, 196.0),
    "lt": (66.0, 56.0),
    "v": (128.0, 118.0),
    "rt": (190.0, 56.0),
}
R = 22.0  # stroke radius (filled capsule half-width)


def draw_m_ribbon(
    draw: ImageDraw.ImageDraw,
    s: float,
    *,
    brush_y: float = 112.0,
    handle_end: float = 198.0,
    tip_y: float = 224.0,
    ferrule_y: float = 112.0,
    shade: bool = True,
) -> None:
    """Solid ribbon M: back/right piece under front/left, brush on right stem."""
    cx = 190.0
    if shade:
        fill_capsule(draw, s, M["v"], M["rt"], R, fill=SHADE)
        fill_capsule(draw, s, M["rt"], (cx, brush_y), R, fill=SHADE)
    fill_capsule(draw, s, M["lb"], M["lt"], R)
    fill_capsule(draw, s, M["lt"], M["v"], R)
    fill_capsule(draw, s, M["v"], M["rt"], R)
    fill_capsule(draw, s, M["rt"], (cx, brush_y), R)
    draw_brush_handle(draw, s, cx, brush_y + 4, handle_end, 14.0)
    draw_ferrule_band(draw, s, cx, ferrule_y, width=28, height=12)
    draw_brush_tip(draw, s, cx, handle_end, half_width=12, length=26)


# --- 01: Ribbon M (closest to current appicon) ----------------------------------------

def m_brush_01_ribbon(size: int) -> Image.Image:
    img, draw, s = canvas(size)
    draw_m_ribbon(draw, s)
    return img


# --- 02: Classic filled M, brush replaces lower right leg ------------------------------

def m_brush_02_classic(size: int) -> Image.Image:
    img, draw, s = canvas(size)
    cx = 190.0
    fill_capsule(draw, s, M["lb"], M["lt"], R)
    fill_capsule(draw, s, M["lt"], M["v"], R)
    fill_capsule(draw, s, M["v"], M["rt"], R)
    fill_capsule(draw, s, M["rt"], (cx, 108.0), R)
    draw_brush_handle(draw, s, cx, 112.0, 198.0, 14.0)
    draw_ferrule_band(draw, s, cx, 108.0)
    draw_brush_tip(draw, s, cx, 198.0)
    return img


# --- 03: Deep fold — stronger underlay on right diagonal -------------------------------

def m_brush_03_deep_fold(size: int) -> Image.Image:
    img, draw, s = canvas(size)
    cx = 190.0
    fill_capsule(draw, s, M["v"], M["rt"], R + 2, fill=(225, 222, 236, 255))
    fill_capsule(draw, s, M["rt"], (cx, 108.0), R + 1, fill=SHADE)
    draw_m_ribbon(draw, s, shade=False)
    return img


# --- 04: Angled brush — right stroke sweeps down-diagonal ----------------------------

def m_brush_04_angled(size: int) -> Image.Image:
    img, draw, s = canvas(size)
    fill_capsule(draw, s, M["lb"], M["lt"], R)
    fill_capsule(draw, s, M["lt"], M["v"], R)
    fill_capsule(draw, s, M["v"], M["rt"], R, fill=SHADE)
    fill_capsule(draw, s, M["rt"], (204.0, 118.0), R)
    fill_capsule(draw, s, (204.0, 118.0), (214.0, 188.0), 16.0)
    draw_ferrule_band(draw, s, 208.0, 128.0, width=24, height=11)
    draw.polygon(
        [pt(s, 200.0, 188.0), pt(s, 228.0, 188.0), pt(s, 214.0, 222.0)],
        fill=WHITE,
    )
    return img


# --- 05: Rounded-cap M — slightly wider strokes, softer valley -------------------------

def m_brush_05_bold(size: int) -> Image.Image:
    img, draw, s = canvas(size)
    r = 25.0
    cx = 188.0
    fill_capsule(draw, s, (64.0, 198.0), (64.0, 54.0), r, fill=SHADE)
    fill_capsule(draw, s, (64.0, 54.0), (128.0, 120.0), r)
    fill_capsule(draw, s, (128.0, 120.0), (192.0, 54.0), r)
    fill_capsule(draw, s, (192.0, 54.0), (cx, 110.0), r)
    draw_brush_handle(draw, s, cx, 114.0, 200.0, 15.0)
    draw_ferrule_band(draw, s, cx, 110.0, width=30, height=13)
    draw_brush_tip(draw, s, cx, 200.0, half_width=14, length=28)
    fill_disc(draw, s, 64.0, 198.0, 6.0)
    return img


# --- 06: Continuous body — one thick path, brush tip at bottom -------------------------

def m_brush_06_monolith(size: int) -> Image.Image:
    img, draw, s = canvas(size)
    cx = 190.0
    path = [
        M["lb"],
        M["lt"],
        M["v"],
        M["rt"],
        (cx, 110.0),
        (cx, 196.0),
    ]
    for i in range(len(path) - 1):
        fill_capsule(draw, s, path[i], path[i + 1], R if i < 4 else 15.0)
    for p in path[:5]:
        fill_disc(draw, s, *p, 8.0)
    draw_ferrule_band(draw, s, cx, 108.0)
    draw_brush_tip(draw, s, cx, 196.0, half_width=13, length=25)
    return img


OPTIONS = [
    ("m-brush-01-ribbon", m_brush_01_ribbon, "Ribbon M with fold — closest to current icon"),
    ("m-brush-02-classic", m_brush_02_classic, "Solid M, right leg becomes brush"),
    ("m-brush-03-deep-fold", m_brush_03_deep_fold, "Stronger fold shading on right stroke"),
    ("m-brush-04-angled", m_brush_04_angled, "Brush handle angled down-right"),
    ("m-brush-05-bold", m_brush_05_bold, "Extra-bold strokes, soft caps"),
    ("m-brush-06-monolith", m_brush_06_monolith, "Single continuous solid body"),
]


def write_all() -> None:
    for name, fn, _desc in OPTIONS:
        for px in SIZES:
            out_dir = OUT_ROOT / f"{px}x{px}" / "apps"
            out_dir.mkdir(parents=True, exist_ok=True)
            path = out_dir / f"harbour-muoto-{name}.png"
            fn(px).save(path, "PNG")
            print(f"wrote {path}")


if __name__ == "__main__":
    write_all()
