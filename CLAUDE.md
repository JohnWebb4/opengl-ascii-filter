# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

A set of GPU image filters (ASCII art and stained glass) in an OpenGL 3.3 core window (GLFW + GLAD + GLM, C++17). Tab switches the active filter. They filter a built-in rotating sphere, a still image, or an animated GIF. The README covers the controls and export behavior. The terminal backend was deliberately declined; everything stays in the OpenGL window.

## Build and run

```
cmake -S . -B build
cmake --build build --config Release
build/Release/opengl_ascii [image.png|animation.gif]   # MSVC layout; single-config generators put it in build/
```

- The project is developed on Windows/MSVC. There are no tests and no linter.
- `external/` is git-ignored and must hold `glfw/` (built via `add_subdirectory`), `glad/`, `glm/`, and `stb/` with `stb_image.h`, `stb_image_write.h` and `gif.h`. A fresh clone does not build until these are fetched.
- Shaders are loaded from the source tree at runtime through the `SHADER_DIR` compile definition. Editing a `.frag` needs no rebuild, only a restart.
- Exports (`ascii_*`) and `images/` are git-ignored.

## Architecture

`src/main.cpp` owns the frame loop, the source and present passes, the exports and the keys every filter shares (Esc, S, Space, `,`, `.`, E, G, Tab). Everything else lives behind the `Filter` interface in `src/filter.h`. `AppState` holds a list of filters and the active index, and unhandled keys go to the active filter's `onKey()`. Each frame runs in passes through framebuffers:

1. **Source pass** draws into `sceneFbo`: the lit sphere (`scene.*`), or the current image or GIF layer (`source.frag`, which samples a `sampler2DArray`). It is drawn at the filter's canvas size.
2. **`Filter::prepare()`** runs any reduction passes the filter needs before drawing (see each filter below).
3. **`Filter::render()`** draws into `canvasFbo`, one pixel per output pixel. PNG and GIF export call `renderExport()` instead, which defaults to `render()`.
4. **Present pass** (`image.frag`) letterboxes the canvas into the window. PNG and GIF export read `canvasFbo`, not the window.

A filter reports its own canvas size through `canvasSize(aspect, maxTexture)`, adds its own part of the window title through `status()`, and can warn about bad export sizes through `warnExport()`. `beginExport()` and `endExport()` bracket a GIF export so the filter can drop on-screen smoothing. Add a filter by subclassing `Filter`, adding its source file to `CMakeLists.txt`, and appending it to `state.filters` in `main.cpp`.

### ASCII filter (`ascii_filter.*`)

- `prepare()` runs the cell pass (`cells.frag`), which reduces the source to one texel per character cell: rgb is the mean color, alpha is the mean luminance.
- `render()` draws `glyph.frag` (ASCII), `mosaic.frag`, or `blit.frag` (raw), cycled with M. Export always draws the ASCII view.

### Stained glass filter (`stained_glass_filter.*`)

- The canvas is fixed at 1024 px on the long side with the source's aspect ratio, unlike ASCII. `[` and `]` change the panes across, and the pane size is `canvas width / panes`.
- `stained.frag` is one shader with two modes, selected by `uPanePass`. Each pane is one grid cell with a jittered Voronoi site; the site stays inside the cell with a 0.15 margin, so a 3x3 search finds the nearest site. The border loop searches 5x5.
- With the merge threshold above 0, `prepare()` runs the pane pass into `paneFbo_` (one texel per pane, its color), reads it back, and calls `mergeRegions()` (`region_merge.cpp`) on the CPU. Edges between the 8 grid neighbors are merged most similar first, and two groups join only if their mean colors are within the threshold, so gradients can't chain into one group. The result is uploaded to an RGBA32F label texture: r is the group id and gba is the group's mean color. Every pane in a group is drawn in the group's mean color, with its brightness variation keyed on the group id. The per-pane center highlight fades out as the threshold rises (`uMerge`) so pane seams don't show inside a group. In ANSI 256 and ANSI 16 modes (C) the shader then quantizes that color with the palette in `palette.glsl`.
- The main pass draws lead only between panes in different groups, so it outlines each group. Cells outside the canvas take the nearest edge pane's group. With a threshold of 0 the readback is skipped and every pane is its own group.
- Grouping has hysteresis: `mergeRegions()` takes last frame's groups, and a pair that shared a group merges at `threshold * (1 + kHysteresis)` while one that didn't merges at `threshold * (1 - kHysteresis)`. It is reset when the grid or threshold changes and around GIF exports. `kHysteresis` (0.25) is in `stained_glass_filter.cpp`.
- Shaders can `#include "file"` another shader file. `Shader` expands it when loading, since GLSL has no include. `palette.glsl` is shared by `glyph.frag` and `stained.frag`.
- Color distance is luma-weighted RGB (`colorDistance()` in `region_merge.cpp`). The shader no longer compares colors, only group ids.

Non-obvious details that span files:

- All fullscreen passes use a single triangle generated from `gl_VertexID` in `fullscreen.vert`. Core profile still needs an empty VAO bound.
- The ASCII canvas size is derived from the content. `canvasGrid(columns, aspect, maxTexture)` in `ascii_filter.cpp` gives `rows = round(cols / aspect * cellW / cellH)` and clamps to `GL_MAX_TEXTURE_SIZE`. The canvas is `cols*8` by `rows*16` px, so the column count determines the export size. `[` and `]` change the columns.
- `glyph.frag` applies luminance in a fixed order: the auto-levels remap, then gain, then contrast around 0.5, then a clamp, then the glyph index. The 10 glyphs are hand-drawn in `glyph_atlas.cpp`, ordered by ink density, in an R8 texture. The atlas stores the top row first while window y points up, so the shader flips.
- Auto-levels is computed on the CPU. `levels.cpp` reads back the small cell texture and takes the 2nd to 98th percentile of alpha with a minimum span of 0.05. `AsciiFilter::prepare()` eases the result with a 0.15 mix per frame so the sphere does not flicker.
- Foreground color is the cell's mean color divided by its max component. ANSI16 uses xterm default colors, and ANSI256 uses the 6x6x6 cube plus the gray ramp.
- `Texture` (`texture.cpp`) holds stills and GIFs as one `GL_TEXTURE_2D_ARRAY`, one layer per frame, loaded through stb_image (`stbi_load_gif_from_memory` for GIFs). It enforces a 512 MB frame cap and raises delays of 10 ms or less to 100 ms. `AppState` tracks the playback time and layer.
- `gif.h` defines non-inline functions, so it is included in exactly one translation unit, `gif_export.cpp`, with `GIF_FLIP_VERT`. Do not include it elsewhere or the link fails with duplicate symbols.
- GIF export runs inside the render loop in `main.cpp` and blocks the window. It does not poll events. It re-renders every frame through `prepare()`, so ASCII auto-levels run per frame without easing and the stained glass grouping is recomputed per frame.
- Wide ASCII exports are a known sharing problem. Chat sites shrink anything wider than about 400 px and blur the thin glyph strokes. `kShareWidthLimit` (400) in `ascii_filter.cpp` drives the export-time warning and the `(wide)` flag in the window title.
