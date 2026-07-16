#!/usr/bin/env python3
"""Normalize generated artwork into deterministic EasyX sprite atlases."""

from __future__ import annotations

import argparse
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw


def visible_bbox(image: Image.Image, threshold: int = 12) -> tuple[int, int, int, int]:
    """Return the non-transparent bounds while ignoring isolated edge noise."""
    alpha = image.getchannel("A")
    bounds = alpha.point(lambda value: 255 if value >= threshold else 0).getbbox()
    if bounds is None:
        raise ValueError("source image does not contain visible pixels")
    return bounds


def remove_connected_chroma(image: Image.Image) -> Image.Image:
    """Remove only magenta screen pixels connected to the outer canvas.

    Generated enemies contain lavender and violet paint inside their bodies.
    A global color-key test mistakes those colors for the magenta screen and
    punches holes through the artwork.  Flood-filling the key mask from the
    corner keeps enclosed subject colors while removing the background and its
    blended edge pixels.
    """
    pixels = np.asarray(image.convert("RGBA")).copy()
    red = pixels[..., 0].astype(np.int16)
    green = pixels[..., 1].astype(np.int16)
    blue = pixels[..., 2].astype(np.int16)
    chroma = (
        (red > 155)
        & (blue > 155)
        & (red + blue - green * 2 > 115)
        & (np.abs(red - blue) < 105)
    )

    # Zero marks a possible screen pixel.  Flood fill changes only the region
    # connected to the upper-left corner, so similarly colored enclosed paint
    # remains part of the subject.
    # ``copy`` makes the NumPy-backed Pillow image writable; floodfill otherwise
    # cannot update some Pillow builds even though no exception is raised.
    mask = Image.fromarray(np.where(chroma, 0, 255).astype(np.uint8), "L").copy()
    ImageDraw.floodfill(mask, (0, 0), 128, thresh=0)
    outside = np.asarray(mask) == 128
    # Tiny screen-colored islands may be enclosed by claws or accessories and
    # therefore cannot be reached by the flood fill.  Their saturation is much
    # stronger than any painted violet, so this stricter global test is safe.
    pure_chroma = (
        (red > 180)
        & (blue > 180)
        & (green < 85)
        & (np.abs(red - blue) < 60)
    )
    pixels[outside | pure_chroma] = 0
    return Image.fromarray(pixels, "RGBA")


def harden_alpha(image: Image.Image, threshold: int = 64) -> Image.Image:
    """Remove chroma-key haze before the final high-quality downscale.

    The resize step recreates a narrow antialiased edge, so the atlas remains
    smooth without making the lavender and violet subjects translucent.
    """
    red, green, blue, alpha = image.split()
    alpha = alpha.point(lambda value: 255 if value >= threshold else 0)
    return Image.merge("RGBA", (red, green, blue, alpha))


def resize_for_easyx(image: Image.Image, size: tuple[int, int]) -> Image.Image:
    """Resize premultiplied colors and finish with a binary EasyX mask.

    EasyX samples the atlas with nearest-neighbour coordinates.  Leaving soft
    alpha in the PNG therefore exposes matte colors as a gray or magenta halo.
    Premultiplied resizing keeps edge colors correct, and the binary mask makes
    every final pixel either fully visible or fully transparent.
    """
    source = np.asarray(image.convert("RGBA"), dtype=np.uint8)
    alpha = source[..., 3]
    premultiplied = (
        source[..., :3].astype(np.uint16) * alpha[..., None].astype(np.uint16) // 255
    ).astype(np.uint8)

    resized_color = np.asarray(
        Image.fromarray(premultiplied, "RGB").resize(size, Image.Resampling.LANCZOS),
        dtype=np.uint8,
    )
    resized_alpha = np.asarray(
        Image.fromarray(alpha, "L").resize(size, Image.Resampling.LANCZOS),
        dtype=np.uint8,
    )

    output = np.zeros((*resized_alpha.shape, 4), dtype=np.uint8)
    visible = resized_alpha >= 96
    safe_alpha = np.maximum(resized_alpha.astype(np.uint16), 1)
    restored = np.clip(
        resized_color.astype(np.uint16) * 255 // safe_alpha[..., None],
        0,
        255,
    ).astype(np.uint8)
    output[visible, :3] = restored[visible]
    output[visible, 3] = 255
    return Image.fromarray(output, "RGBA")


def fit_subject(source: Path, cell_size: int, occupancy: float,
                clean_chroma: bool = False) -> Image.Image:
    image = Image.open(source).convert("RGBA")
    if clean_chroma:
        image = remove_connected_chroma(image)
    image = harden_alpha(image)
    image = image.crop(visible_bbox(image))
    maximum = max(1, round(cell_size * occupancy))
    scale = min(maximum / image.width, maximum / image.height)
    width = max(1, round(image.width * scale))
    height = max(1, round(image.height * scale))
    image = resize_for_easyx(image, (width, height))

    cell = Image.new("RGBA", (cell_size, cell_size), (0, 0, 0, 0))
    cell.alpha_composite(image, ((cell_size - width) // 2, (cell_size - height) // 2))
    return cell


def build_horizontal_atlas(sources: list[Path], output: Path,
                           occupancies: list[float],
                           clean_chroma: bool = False) -> None:
    cell_size = 256
    atlas = Image.new("RGBA", (cell_size * len(sources), cell_size), (0, 0, 0, 0))
    for index, (source, occupancy) in enumerate(zip(sources, occupancies, strict=True)):
        atlas.alpha_composite(
            fit_subject(source, cell_size, occupancy, clean_chroma),
            (index * cell_size, 0),
        )
    output.parent.mkdir(parents=True, exist_ok=True)
    atlas.save(output, optimize=True)


def build_player_sheet(source: Path, output: Path) -> None:
    """Repack the original collage into a strict 3 x 4 grid.

    Every row uses one common scale and a shared foot baseline.  This removes
    per-frame size jumps and keeps all visible pixels inside the walkable tile.
    """
    image = Image.open(source).convert("RGBA")
    columns, rows, cell_size = 3, 4, 96
    x_edges = [round(index * image.width / columns) for index in range(columns + 1)]
    y_edges = [round(index * image.height / rows) for index in range(rows + 1)]
    atlas = Image.new("RGBA", (columns * cell_size, rows * cell_size), (0, 0, 0, 0))

    for row in range(rows):
        frames: list[Image.Image] = []
        for column in range(columns):
            frame = image.crop((
                x_edges[column], y_edges[row],
                x_edges[column + 1], y_edges[row + 1],
            ))
            frame = harden_alpha(frame)
            frames.append(frame.crop(visible_bbox(frame)))

        # The widest walking pose and tallest idle pose define one row scale,
        # preventing left/right frames from making the character visibly pulse.
        scale = min(
            84 / max(frame.width for frame in frames),
            88 / max(frame.height for frame in frames),
        )
        for column, frame in enumerate(frames):
            width = max(1, round(frame.width * scale))
            height = max(1, round(frame.height * scale))
            resized = resize_for_easyx(frame, (width, height))
            x = column * cell_size + (cell_size - width) // 2
            y = row * cell_size + 93 - height
            atlas.alpha_composite(resized, (x, y))

    output.parent.mkdir(parents=True, exist_ok=True)
    atlas.save(output, optimize=True)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--item-sources", required=True, type=Path)
    parser.add_argument("--enemy-sources", required=True, type=Path)
    parser.add_argument("--player-source", required=True, type=Path)
    parser.add_argument("--assets-root", required=True, type=Path)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    build_horizontal_atlas(
        [
            args.item_sources / "blast_range.png",
            args.item_sources / "bubble_capacity.png",
            args.item_sources / "speed.png",
            args.item_sources / "shield.png",
        ],
        args.assets_root / "ui" / "item_icons_v1.png",
        [0.78, 0.78, 0.78, 0.78],
        clean_chroma=True,
    )
    build_horizontal_atlas(
        [
            args.enemy_sources / "patrol.png",
            args.enemy_sources / "hunter.png",
            args.enemy_sources / "boss.png",
        ],
        args.assets_root / "sprites" / "enemy_sprites_v1.png",
        [0.76, 0.78, 0.88],
        clean_chroma=True,
    )
    build_player_sheet(
        args.player_source,
        args.assets_root / "sprites" / "player_walk_sheet.png",
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
