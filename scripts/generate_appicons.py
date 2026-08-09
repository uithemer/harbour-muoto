#!/usr/bin/env python3
"""Generate Sailfish-style app icon concept mockups into appicons/icon-concepts/.

Does not overwrite production appicons/*/apps/harbour-muoto.png.
Uses the live 256px icon alpha as the Sailfish silhouette mask.
"""

from __future__ import annotations

import math
from collections.abc import Callable
from pathlib import Path

from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
MASK_SRC = ROOT / "appicons" / "256x256" / "apps" / "harbour-muoto.png"
OUT_ROOT = ROOT / "appicons" / "icon-concepts"
REVIEW_SIZES = (172, 86)

# Match harbour-muoto.png palette
GRAD_A = (0, 188, 212)
GRAD_B = (171, 71, 188)
GRAD_C = (236, 64, 122)

WHITE = (255, 255, 255, 255)
SHADE = (236, 234, 244, 255)
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


def load_mask(size: int) -> Image.Image:
    """Sailfish petal alpha from the shipping icon, scaled to size."""
    src = Image.open(MASK_SRC).convert("RGBA").split()[3]
    if src.size != (size, size):
        src = src.resize((size, size), Image.Resampling.LANCZOS)
    return src


def gradient_plate(size: int) -> Image.Image:
    grad = Image.new("RGBA", (size, size))
    px = grad.load()
    for y in range(size):
        for x in range(size):
            t = (x * 0.65 + y * 0.85) / size
            px[x, y] = (*tri_grad_color(t), 255)
    mask = load_mask(size)
    out = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    out.paste(grad, (0, 0), mask)
    return out


def canvas(size: int) -> tuple[Image.Image, ImageDraw.ImageDraw, float]:
    img = gradient_plate(size)
    return img, ImageDraw.Draw(img), size / 256.0


def apply_silhouette(img: Image.Image) -> Image.Image:
    """Re-apply SFOS mask so glyph drawing cannot spill outside the plate."""
    mask = load_mask(img.size[0])
    out = Image.new("RGBA", img.size, (0, 0, 0, 0))
    out.paste(img, (0, 0), mask)
    return out


def pt(s: float, x: float, y: float) -> tuple[int, int]:
    return int(round(x * s)), int(round(y * s))


def rad(s: float, r: float) -> int:
    return max(2, int(round(r * s)))


def capsule_pts(
    a: tuple[float, float], b: tuple[float, float], r: float, steps: int = 12
) -> list[tuple[float, float]]:
    ax, ay = a
    bx, by = b
    dx, dy = bx - ax, by - ay
    length = math.hypot(dx, dy)
    if length < 1e-6:
        return [
            (
                ax + r * math.cos(2 * math.pi * i / (steps * 2)),
                ay + r * math.sin(2 * math.pi * i / (steps * 2)),
            )
            for i in range(steps * 2)
        ]
    ux, uy = dx / length, dy / length
    px, py = -uy * r, ux * r
    pts: list[tuple[float, float]] = [(ax + px, ay + py), (bx + px, by + py)]
    for i in range(steps + 1):
        ang = -math.pi / 2 + math.pi * i / steps
        pts.append(
            (
                bx + r * math.cos(ang) * ux + r * math.sin(ang) * px / r,
                by + r * math.cos(ang) * uy + r * math.sin(ang) * py / r,
            )
        )
    pts.extend([(bx - px, by - py), (ax - px, ay - py)])
    for i in range(steps + 1):
        ang = math.pi / 2 + math.pi * i / steps
        pts.append(
            (
                ax + r * math.cos(ang) * ux + r * math.sin(ang) * (-px / r),
                ay + r * math.cos(ang) * uy + r * math.sin(ang) * (-py / r),
            )
        )
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


# M geometry (256-space)
M = {
    "lb": (66.0, 196.0),
    "lt": (66.0, 56.0),
    "v": (128.0, 118.0),
    "rt": (190.0, 56.0),
}
R = 22.0


def draw_m_ribbon(
    draw: ImageDraw.ImageDraw,
    s: float,
    *,
    brush_y: float = 112.0,
    handle_end: float = 198.0,
    ferrule_y: float = 112.0,
    shade: bool = True,
) -> None:
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


# --- A: M + brush -----------------------------------------------------------------


def a01_ribbon(size: int) -> Image.Image:
    img, draw, s = canvas(size)
    draw_m_ribbon(draw, s)
    return apply_silhouette(img)


def a02_classic(size: int) -> Image.Image:
    img, draw, s = canvas(size)
    cx = 190.0
    fill_capsule(draw, s, M["lb"], M["lt"], R)
    fill_capsule(draw, s, M["lt"], M["v"], R)
    fill_capsule(draw, s, M["v"], M["rt"], R)
    fill_capsule(draw, s, M["rt"], (cx, 108.0), R)
    draw_brush_handle(draw, s, cx, 112.0, 198.0, 14.0)
    draw_ferrule_band(draw, s, cx, 108.0)
    draw_brush_tip(draw, s, cx, 198.0)
    return apply_silhouette(img)


def a03_deep_fold(size: int) -> Image.Image:
    img, draw, s = canvas(size)
    fill_capsule(draw, s, M["v"], M["rt"], R + 2, fill=(225, 222, 236, 255))
    fill_capsule(draw, s, M["rt"], (190.0, 108.0), R + 1, fill=SHADE)
    draw_m_ribbon(draw, s, shade=False)
    return apply_silhouette(img)


def a04_angled(size: int) -> Image.Image:
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
    return apply_silhouette(img)


def a05_bold(size: int) -> Image.Image:
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
    return apply_silhouette(img)


def a06_monolith(size: int) -> Image.Image:
    img, draw, s = canvas(size)
    cx = 190.0
    path = [M["lb"], M["lt"], M["v"], M["rt"], (cx, 110.0), (cx, 196.0)]
    for i in range(len(path) - 1):
        fill_capsule(draw, s, path[i], path[i + 1], R if i < 4 else 15.0)
    for p in path[:5]:
        fill_disc(draw, s, *p, 8.0)
    draw_ferrule_band(draw, s, cx, 108.0)
    draw_brush_tip(draw, s, cx, 196.0, half_width=13, length=25)
    return apply_silhouette(img)


# --- B: Form / muoto --------------------------------------------------------------


def _nested_plate_mask(size: int, scale: float, dx: float, dy: float) -> Image.Image:
    """Smaller Sailfish silhouette, offset in 256-space units."""
    base = load_mask(size)
    inner = max(8, int(round(size * scale)))
    small = base.resize((inner, inner), Image.Resampling.LANCZOS)
    canvas_m = Image.new("L", (size, size), 0)
    ox = int(round((size - inner) / 2 + dx * size / 256.0))
    oy = int(round((size - inner) / 2 + dy * size / 256.0))
    canvas_m.paste(small, (ox, oy))
    return canvas_m


def b01_nested_form(size: int) -> Image.Image:
    """Form within form — white nested Sailfish plate."""
    img, _, _ = canvas(size)
    inner = _nested_plate_mask(size, 0.52, 0, 0)
    layer = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    layer.paste(WHITE, (0, 0), inner)
    img = Image.alpha_composite(img, layer)
    return apply_silhouette(img)


def b02_stacked_sheets(size: int) -> Image.Image:
    """Three offset rounded sheets — layered themes."""
    img, draw, s = canvas(size)
    sheets = [
        (48, 58, 168, 148, SHADE),
        (62, 70, 182, 160, (245, 245, 250, 255)),
        (76, 82, 196, 172, WHITE),
    ]
    for x0, y0, x1, y1, fill in sheets:
        draw.rounded_rectangle(
            (pt(s, x0, y0), pt(s, x1, y1)),
            radius=rad(s, 22),
            fill=fill,
        )
    return apply_silhouette(img)


def b03_cutout_form(size: int) -> Image.Image:
    """Solid white ring with gradient showing through a nested cut-out."""
    img = gradient_plate(size)
    ring_outer = _nested_plate_mask(size, 0.72, 0, 0)
    ring = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    ring.paste(WHITE, (0, 0), ring_outer)
    hole = _nested_plate_mask(size, 0.42, 4, -2)
    ring_px = ring.load()
    hole_px = hole.load()
    for y in range(size):
        for x in range(size):
            if hole_px[x, y] > 128:
                ring_px[x, y] = (0, 0, 0, 0)
    img = Image.alpha_composite(img, ring)
    return apply_silhouette(img)


def b04_offset_duo(size: int) -> Image.Image:
    """Two nested silhouettes, offset — dual form."""
    img, _, _ = canvas(size)
    back = _nested_plate_mask(size, 0.48, -14, 10)
    front = _nested_plate_mask(size, 0.48, 14, -10)
    layer = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    layer.paste(SHADE, (0, 0), back)
    layer.paste(WHITE, (0, 0), front)
    img = Image.alpha_composite(img, layer)
    return apply_silhouette(img)


def b05_concentric_ring(size: int) -> Image.Image:
    """Concentric Sailfish rings."""
    img, _, _ = canvas(size)
    for scale, fill in ((0.70, WHITE), (0.52, None), (0.38, WHITE)):
        m = _nested_plate_mask(size, scale, 0, 0)
        if fill is None:
            px = img.load()
            mp = m.load()
            grad = gradient_plate(size).load()
            for y in range(size):
                for x in range(size):
                    if mp[x, y] > 128:
                        px[x, y] = grad[x, y]
        else:
            layer = Image.new("RGBA", (size, size), (0, 0, 0, 0))
            layer.paste(fill, (0, 0), m)
            img = Image.alpha_composite(img, layer)
    return apply_silhouette(img)


# --- C: Grid / theming ------------------------------------------------------------


def c01_grid_2x2(size: int) -> Image.Image:
    img, draw, s = canvas(size)
    gap, side, x0, y0 = 14.0, 58.0, 56.0, 56.0
    for row in range(2):
        for col in range(2):
            x = x0 + col * (side + gap)
            y = y0 + row * (side + gap)
            draw.rounded_rectangle(
                (pt(s, x, y), pt(s, x + side, y + side)),
                radius=rad(s, 14),
                fill=WHITE,
            )
    return apply_silhouette(img)


def c02_grid_outline(size: int) -> Image.Image:
    img, draw, s = canvas(size)
    gap, side, x0, y0 = 14.0, 58.0, 56.0, 56.0
    stroke = max(3, rad(s, 7))
    for row in range(2):
        for col in range(2):
            x = x0 + col * (side + gap)
            y = y0 + row * (side + gap)
            draw.rounded_rectangle(
                (pt(s, x, y), pt(s, x + side, y + side)),
                radius=rad(s, 14),
                outline=WHITE,
                width=stroke,
            )
    return apply_silhouette(img)


def c03_palette(size: int) -> Image.Image:
    img, draw, s = canvas(size)
    r = 28.0
    for cx, cy in ((100.0, 108.0), (156.0, 108.0), (128.0, 148.0)):
        fill_disc(draw, s, cx, cy, r, fill=WHITE)
    return apply_silhouette(img)


def c04_brush_swatch(size: int) -> Image.Image:
    img, draw, s = canvas(size)
    draw.rounded_rectangle(
        (pt(s, 52, 72), pt(s, 150, 184)),
        radius=rad(s, 18),
        fill=WHITE,
    )
    fill_capsule(draw, s, (130.0, 70.0), (198.0, 168.0), 11.0)
    draw_ferrule_band(draw, s, 188.0, 148.0, width=22, height=10)
    draw.polygon(
        [pt(s, 178, 168), pt(s, 208, 178), pt(s, 196, 208)],
        fill=WHITE,
    )
    return apply_silhouette(img)


def c05_tiles_plus_accent(size: int) -> Image.Image:
    """Three tiles + one accent tile."""
    img, draw, s = canvas(size)
    positions = [(56, 56), (130, 56), (56, 130), (130, 130)]
    fills = [WHITE, WHITE, WHITE, SHADE]
    for (x, y), fill in zip(positions, fills):
        draw.rounded_rectangle(
            (pt(s, x, y), pt(s, x + 58, y + 58)),
            radius=rad(s, 14),
            fill=fill,
        )
    draw.polygon(
        [pt(s, 148, 148), pt(s, 170, 148), pt(s, 159, 172)],
        fill=WHITE,
    )
    return apply_silhouette(img)


# --- D: Hybrid form + brush -------------------------------------------------------


def d01_nested_brush(size: int) -> Image.Image:
    """Nested form with brush tip breaking the lower-right edge."""
    img, _, s = canvas(size)
    inner = _nested_plate_mask(size, 0.50, -4, -6)
    layer = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    layer.paste(WHITE, (0, 0), inner)
    img = Image.alpha_composite(img, layer)
    draw = ImageDraw.Draw(img)
    fill_capsule(draw, s, (168.0, 140.0), (188.0, 188.0), 10.0)
    draw_ferrule_band(draw, s, 182.0, 168.0, width=22, height=10)
    draw_brush_tip(draw, s, 188.0, 188.0, half_width=11, length=22)
    return apply_silhouette(img)


def d02_form_stroke(size: int) -> Image.Image:
    """White nested plate with a bold diagonal paint stroke across."""
    img, _, s = canvas(size)
    inner = _nested_plate_mask(size, 0.54, 0, 0)
    layer = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    layer.paste(WHITE, (0, 0), inner)
    img = Image.alpha_composite(img, layer)
    draw = ImageDraw.Draw(img)
    fill_capsule(draw, s, (72.0, 168.0), (184.0, 72.0), 12.0, fill=SHADE)
    fill_capsule(draw, s, (78.0, 172.0), (178.0, 78.0), 7.0, fill=(200, 80, 140, 255))
    return apply_silhouette(img)


def d03_ring_brush(size: int) -> Image.Image:
    """Ring form + vertical brush as the theming tool."""
    img = b03_cutout_form(size)
    draw = ImageDraw.Draw(img)
    s = size / 256.0
    cx = 128.0
    fill_capsule(draw, s, (cx, 88.0), (cx, 168.0), 12.0)
    draw_ferrule_band(draw, s, cx, 108.0, width=26, height=11)
    draw_brush_tip(draw, s, cx, 168.0, half_width=12, length=24)
    return apply_silhouette(img)


def d04_duo_brush(size: int) -> Image.Image:
    """Offset duo forms with small brush accent."""
    img = b04_offset_duo(size)
    draw = ImageDraw.Draw(img)
    s = size / 256.0
    fill_capsule(draw, s, (176.0, 150.0), (200.0, 196.0), 9.0)
    draw_brush_tip(draw, s, 200.0, 196.0, half_width=10, length=20)
    return apply_silhouette(img)


def d05_minimal_stroke(size: int) -> Image.Image:
    """Single bold curved stroke suggesting a brush forming a sail silhouette."""
    img, draw, s = canvas(size)
    fill_capsule(draw, s, (86.0, 70.0), (170.0, 70.0), 16.0)
    fill_capsule(draw, s, (170.0, 70.0), (186.0, 140.0), 16.0)
    fill_capsule(draw, s, (186.0, 140.0), (128.0, 196.0), 16.0)
    fill_capsule(draw, s, (128.0, 196.0), (78.0, 150.0), 14.0)
    for p in ((86, 70), (170, 70), (186, 140), (128, 196), (78, 150)):
        fill_disc(draw, s, float(p[0]), float(p[1]), 10.0)
    draw.polygon(
        [pt(s, 68, 148), pt(s, 88, 158), pt(s, 70, 186)],
        fill=WHITE,
    )
    return apply_silhouette(img)


CONCEPTS: list[tuple[str, str, list[tuple[str, Callable[[int], Image.Image]]]]] = [
    (
        "A-m-brush",
        "Refine M + brush",
        [
            ("01-ribbon", a01_ribbon),
            ("02-classic", a02_classic),
            ("03-deep-fold", a03_deep_fold),
            ("04-angled", a04_angled),
            ("05-bold", a05_bold),
            ("06-monolith", a06_monolith),
        ],
    ),
    (
        "B-form",
        "Form / muoto (shape)",
        [
            ("01-nested", b01_nested_form),
            ("02-stacked-sheets", b02_stacked_sheets),
            ("03-cutout-ring", b03_cutout_form),
            ("04-offset-duo", b04_offset_duo),
            ("05-concentric", b05_concentric_ring),
        ],
    ),
    (
        "C-grid",
        "Grid / theming",
        [
            ("01-tiles-filled", c01_grid_2x2),
            ("02-tiles-outline", c02_grid_outline),
            ("03-palette", c03_palette),
            ("04-brush-swatch", c04_brush_swatch),
            ("05-tiles-accent", c05_tiles_plus_accent),
        ],
    ),
    (
        "D-hybrid",
        "Form + brush hybrid",
        [
            ("01-nested-brush", d01_nested_brush),
            ("02-form-stroke", d02_form_stroke),
            ("03-ring-brush", d03_ring_brush),
            ("04-duo-brush", d04_duo_brush),
            ("05-minimal-stroke", d05_minimal_stroke),
        ],
    ),
]


SHIP_SIZES = (86, 108, 128, 172, 256)


def _lookup(folder: str, variant: str) -> Callable[[int], Image.Image]:
    for f, _title, variants in CONCEPTS:
        if f != folder:
            continue
        for name, fn in variants:
            if name == variant:
                return fn
    raise SystemExit(f"unknown concept: {folder}/{variant}")


def write_all() -> None:
    if not MASK_SRC.is_file():
        raise SystemExit(f"missing mask source: {MASK_SRC}")

    OUT_ROOT.mkdir(parents=True, exist_ok=True)
    index_lines = [
        "# Muoto app icon concepts",
        "",
        "Sailfish silhouette from the live 256px icon alpha.",
        "Production `appicons/*/apps/harbour-muoto.png` is not modified.",
        "",
        "Regenerate with: `python3 scripts/generate_appicons.py`",
        "",
        "Ship a winner (overwrites launcher sizes only):",
        "`python3 scripts/generate_appicons.py --ship A-m-brush 01-ribbon`",
        "",
    ]

    for folder, title, variants in CONCEPTS:
        index_lines.append(f"## {folder} — {title}")
        index_lines.append("")
        for name, fn in variants:
            out_dir = OUT_ROOT / folder
            out_dir.mkdir(parents=True, exist_ok=True)
            for px in REVIEW_SIZES:
                path = out_dir / f"harbour-muoto-{name}-{px}.png"
                fn(px).save(path, "PNG")
                path.chmod(path.stat().st_mode & ~0o111)
                print(f"wrote {path}")
            index_lines.append(f"- `{folder}/harbour-muoto-{name}-172.png` (also `-86.png`)")
        index_lines.append("")

    (OUT_ROOT / "README.md").write_text("\n".join(index_lines) + "\n", encoding="utf-8")
    print(f"index → {OUT_ROOT / 'README.md'}")


def ship(folder: str, variant: str) -> None:
    """Overwrite production launcher PNGs with the chosen concept (all sizes)."""
    fn = _lookup(folder, variant)
    for px in SHIP_SIZES:
        out = ROOT / "appicons" / f"{px}x{px}" / "apps" / "harbour-muoto.png"
        out.parent.mkdir(parents=True, exist_ok=True)
        fn(px).save(out, "PNG")
        out.chmod(out.stat().st_mode & ~0o111)
        print(f"shipped {out}")


if __name__ == "__main__":
    import sys

    if len(sys.argv) >= 4 and sys.argv[1] == "--ship":
        ship(sys.argv[2], sys.argv[3])
    else:
        write_all()
