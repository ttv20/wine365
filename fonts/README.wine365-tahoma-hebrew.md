# Wine 365 Hebrew additions to Tahoma

Wine's metric-compatible Tahoma files did not contain Hebrew glyphs.  Some
Office UI paths ask a Tahoma `IDWriteFontFace` for glyph indices directly, so
GDI `FontSubstitutes` and normal DirectWrite fallback cannot repair those
paths.  The checked-in `tahoma.ttf` and `tahomabd.ttf` therefore retain the
original Wine Tahoma faces and append only these Hebrew ranges:

- U+0590-U+05FF
- U+FB1D-U+FB4F

The appended outlines and Hebrew OpenType layout data come from Liberation
Sans.  See `LICENSE.Liberation`.  The primary family remains Tahoma, not the
reserved Liberation name.

## Reproduction

The current files were generated with:

- Wine base fonts from commit `43c31c5a2ba73810f87cbb669f8d7c9a39b30680`
- Debian `fonts-liberation2` `1:2.1.5-3`
- fontTools commit `386243ed95d6a42114f7695f21de3ef45524108a`
- Liberation donor SHA-256:
  - regular: `bade59d822652f76e6941aa87b40a87c13d1cc70db98ededb5011127efafd1d3`
  - bold: `1b5f2da6f4cadce4c05b9ecebe3a6fcd374eb95ae443605e799f4c3287978939`

Example, from the Wine source root:

```sh
git show 43c31c5a2ba73810f87cbb669f8d7c9a39b30680:fonts/tahoma.ttf > /tmp/tahoma-base.ttf
git show 43c31c5a2ba73810f87cbb669f8d7c9a39b30680:fonts/tahomabd.ttf > /tmp/tahomabd-base.ttf
python3 fonts/generate_tahoma_hebrew.py \
  /tmp/tahoma-base.ttf /tmp/tahomabd-base.ttf \
  /path/to/LiberationSans-Regular.ttf \
  /path/to/LiberationSans-Bold.ttf \
  fonts/tahoma.ttf fonts/tahomabd.ttf
```

Expected output SHA-256:

- `tahoma.ttf`: `09d29233e0e39f8c185e0f68b6e5973b43f69703624419177639dc35af480985`
- `tahomabd.ttf`: `de6d07a8c299a137b7b3d293f82e900d234727d0126a0e0fecda6d3df870604c`

The generator restores Tahoma's original embedded-bitmap and VDMX tables after
fontTools merges the outline and layout tables.  This preserves legacy UI
rendering while the appended Hebrew glyphs remain outline-only.
