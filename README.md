# opengl-ascii-filter

A real-time ASCII art filter that runs entirely on the GPU in an OpenGL window.

It renders a source offscreen, reduces it to a grid of character cells (8x16 px each), and draws each cell as a glyph picked by the cell's brightness. The glyphs come from a small bitmap atlas, ordered by ink density. Cell color can be shown as grayscale, ANSI 16, ANSI 256, or truecolor.

Three kinds of source feed the same filter:

- **The built-in sphere** (default): a lit, rotating orange sphere.
- **A still image**: PNG, JPG, and other formats supported by stb_image.
- **An animated GIF**: played back frame by frame, with pause and frame stepping.

The result can be exported as a PNG (the current frame) or as an animated GIF (the whole animation).

## Requirements

- OpenGL 3.3 core
- C++17 and CMake
- GLFW, GLAD, GLM, `stb_image.h`, `stb_image_write.h` and `gif.h`, placed in `external/` (git-ignored, so fetch them after cloning)

## Building and running

```
cmake -S . -B build
cmake --build build --config Release
```

Run with no arguments to get the sphere, or pass a file:

```
opengl_ascii path/to/image.png
opengl_ascii path/to/animation.gif
```

You can also drag and drop an image or GIF onto the window at any time. Press `S` to switch back to the sphere.

## Controls

| Key | Action |
| --- | --- |
| `Esc` | Quit |
| `Tab` | Switch filter: ASCII or stained glass |
| `M` | Cycle view: ASCII, mosaic (one flat color per cell), raw scene or image |
| `S` | Switch between the sphere and the loaded image or GIF |
| `C` | Cycle color mode: mono, ANSI 16, ANSI 256, truecolor |
| `A` | Toggle auto-levels (stretches the brightness range to the content) |
| `Up` / `Down` | Raise or lower gain (brightness), in steps of 0.05 |
| `Right` / `Left` | Raise or lower contrast, in steps of 0.05 |
| `]` / `[` | Add or remove 10 columns of detail (20 to 400) |
| `Space` | Play or pause an animated GIF |
| `.` / `,` | Step to the next or previous GIF frame |
| `E` | Export the current ASCII view as a PNG |
| `G` | Export the whole animated GIF as an ASCII GIF |

The keys from `M` to `]` / `[` belong to the ASCII filter. In the stained glass filter, `]` / `[` add or remove 4 panes across (8 to 160), `Up` / `Down` thicken or thin the lead lines, and `Right` / `Left` raise or lower the merge threshold (0 to 0.6): neighboring panes of similar color are grouped into one piece of glass, and the lead outlines each group. The threshold is how far apart the groups' average colors may be, and 0 turns grouping off. `C` cycles truecolor, ANSI 256 and ANSI 16: in the ANSI modes each group is one flat palette color, taken from the group's average. Its canvas is fixed at 1024 px on the long side.

The window title shows the source, the grid size in cells, the export size in pixels, the color mode, gain, contrast, and whether auto-levels is on.

## Exporting

Exports are written to the working directory with a timestamped name, such as `ascii_20261016_200745.png` or `.gif`.

- **PNG** (`E`): saves the current frame at the full canvas size.
- **GIF** (`G`): encodes every frame of the loaded GIF. The window is unresponsive while it encodes. The sphere cannot be exported as a GIF, because a uniformly colored sphere looks identical when rotated.

The number of columns sets the export size: each column is 8 px wide. The window title shows the export size, and it is flagged `(wide)` when it is over 400 px (50 columns).

### Sharing GIFs: keep the quality low

Chat sites such as Discord shrink wide uploads and re-encode GIFs as video for thumbnails and previews. The glyphs are thin, 1 to 2 px strokes, so this blurs them, and the result can look washed out, over-contrasted, or tinted red.

If the GIF will be encoded to a video afterwards, **reduce the quality before exporting**:

- Press `[` to drop to **50 columns or fewer** (400 px wide or less). This is the most reliable fix. A 960 px wide export came out broken in testing, and a 320 px wide one looked correct.
- Mono mode (`C`) can help if colors still come out tinted.
- Fewer columns gives bigger, bolder glyphs that survive video compression and downscaling.

The program prints a warning when you export a GIF that is wider than 400 px. The GIF itself is fine, and it plays correctly in a browser. Only the re-encoded preview is affected.

## Notes

- Large GIFs are capped at 512 MB of decoded frames.
- GIF frame delays of 10 ms or less are raised to 100 ms, as browsers do.
- Fractional canvas scaling uses nearest-neighbour filtering, which keeps the glyphs crisp.
