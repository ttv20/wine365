#!/usr/bin/env python3
"""Add the Hebrew ranges from Liberation Sans to Wine Tahoma.

This maintenance helper requires fontTools.  It is not run by the Wine build.
The generated tahoma.ttf and tahomabd.ttf remain checked-in build inputs.
"""

import argparse
import copy
import subprocess
import sys
import tempfile
from pathlib import Path

from fontTools.ttLib import TTFont

HEBREW_RANGES = "U+0590-05FF,U+FB1D-FB4F"
PRESERVED_TABLES = ("BDF ", "EBDT", "EBLC", "FFTM", "VDMX")
SMOOTH_GASP_RANGES = {8: 2, 16: 3, 65535: 3}


def run_module(module, *args):
    subprocess.run([sys.executable, "-m", module, *map(str, args)], check=True)


def build(base_path, donor_path, output_path, workdir):
    subset_path = workdir / (donor_path.stem + "-hebrew-subset.ttf")
    merged_path = workdir / (output_path.stem + "-merged.ttf")

    run_module(
        "fontTools.subset",
        donor_path,
        f"--unicodes={HEBREW_RANGES}",
        "--layout-features=*",
        "--glyph-names",
        "--name-IDs=*",
        f"--output-file={subset_path}",
    )
    run_module(
        "fontTools.merge",
        f"--output-file={merged_path}",
        base_path,
        subset_path,
    )

    base = TTFont(base_path, recalcTimestamp=False)
    merged = TTFont(merged_path, recalcTimestamp=False)
    base_order = base.getGlyphOrder()
    if merged.getGlyphOrder()[: len(base_order)] != base_order:
        raise RuntimeError("fontTools changed the base glyph order")

    # fontTools.merge intentionally drops these first-font-only tables.  The
    # appended glyphs do not participate in Tahoma's embedded bitmap strikes,
    # so preserving the original tables is valid and keeps legacy UI quality.
    for tag in PRESERVED_TABLES:
        if tag in base:
            merged[tag] = copy.deepcopy(base[tag])

    # Wine enables font smoothing by default. Keep hinting above 8 ppem, but
    # request grayscale outlines at every size instead of selecting the
    # monochrome embedded strikes used by legacy dialogs.
    merged["gasp"].gaspRange = SMOOTH_GASP_RANGES
    merged["head"].created = base["head"].created
    merged["head"].modified = base["head"].modified
    merged.recalcTimestamp = False
    merged.save(output_path, reorderTables=False)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("base_regular", type=Path)
    parser.add_argument("base_bold", type=Path)
    parser.add_argument("donor_regular", type=Path)
    parser.add_argument("donor_bold", type=Path)
    parser.add_argument("output_regular", type=Path)
    parser.add_argument("output_bold", type=Path)
    args = parser.parse_args()

    with tempfile.TemporaryDirectory(prefix="tahoma-hebrew-") as directory:
        workdir = Path(directory)
        build(args.base_regular, args.donor_regular, args.output_regular, workdir)
        build(args.base_bold, args.donor_bold, args.output_bold, workdir)


if __name__ == "__main__":
    main()
