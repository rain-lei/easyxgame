#!/usr/bin/env python3
"""Normalize generated artwork into deterministic EasyX sprite atlases."""

from __future__ import annotations

import argparse
from pathlib import Path

from PIL import Image


def visible_bbox(image: Image.Image, threshold: int = 12) -> tuple[int, int, int, int]:
    """Return the non-transparent bounds while ignoring isolated edge noise."""
    alpha = image.getchannel("A")
    bounds = alpha.point(lambda value: 255 if value >= threshold else 0).getbbox()
    if bounds is None:
        raise ValueError("source image does not contain visible pixels")
    return bounds


def harden_alpha(image: Image.Image, threshold: int = 24) -> Image.Image:
    """Remove chroma-key haze before the final high-quality downscale.

    The resize step recreates a narrow antialiased edge, so the atlas remains
    smooth without making the lavender and violet subjects translucent.
    """
    red, green, blue, alpha = image.split()
    alpha = alpha.point(lambda value: 255 if value >= threshold else 0)
    return Image.merge("RGBA", (red, green, blue, alpha))


def fit_subject(source: Path, cell_size: int, occupancy: float,
                hard_alpha: bool = False) -> Image.Image:
    image = Image.open(source).convert("RGBA")
    if hard_alpha:
        image = harden_alpha(image)
    image = image.crop(visible_bbox(image))
    maximum = max(1, round(cell_size * occupancy))
    scale = min(maximum / image.width, maximum / image.height)
    width = max(1, round(image.width * scale))
    height = max(1, round(image.height * scale))
    image = image.resize((width, height), Image.Resampling.LANCZOS)

    cell = Image.new("RGBA", (cell_size, cell_size), (0, 0, 0, 0))
    cell.alpha_composite(image, ((cell_size - width) // 2, (cell_size - height) // 2))
    return cell


def build_horizontal_atlas(sources: list[Path], output: Path,
                           occupancies: list[float],
                           hard_alpha: bool = False) -> None:
    cell_size = 256
    atlas = Image.new("RGBA", (cell_size * len(sources), cell_size), (0, 0, 0, 0))
    for index, (source, occupancy) in enumerate(zip(sources, occupancies, strict=True)):
        atlas.alpha_composite(
            fit_subject(source, cell_size, occupancy, hard_alpha),
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
            resized = frame.resize((width, height), Image.Resampling.LANCZOS)
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
            args.item_sources / "blast_range_alpha.png",
            args.item_sources / "bubble_capacity_alpha.png",
            args.item_sources / "speed_alpha.png",
            args.item_sources / "shield_alpha.png",
        ],
        args.assets_root / "ui" / "item_icons_v1.png",
        [0.78, 0.78, 0.78, 0.78],
    )
    build_horizontal_atlas(
        [
            args.enemy_sources / "patrol_alpha_v2.png",
            args.enemy_sources / "hunter_alpha.png",
            args.enemy_sources / "boss_alpha_v2.png",
        ],
        args.assets_root / "sprites" / "enemy_sprites_v1.png",
        [0.76, 0.78, 0.88],
        hard_alpha=True,
    )
    build_player_sheet(
        args.player_source,
        args.assets_root / "sprites" / "player_walk_sheet.png",
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
