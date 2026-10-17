# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

A GPU ASCII-art filter in an OpenGL 3.3 core window (GLFW + GLAD + GLM, C++17). It filters a built-in rotating sphere, a still image, or an animated GIF. The README covers the controls and export behavior. The terminal backend was deliberately declined; everything stays in the OpenGL window.

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

`src/main.cpp` owns the whole frame loop; the other files are small helpers. The pipeline runs in passes through framebuffers:

1. **Source pass** draws into `sceneFbo`: the lit sphere (`scene.*`), or the current image or GIF layer (`source.frag`, which samples a `sampler2DArray`).
2. **Cell pass** (`cells.frag`) reduces the source to one texel per character cell: rgb is the mean color, alpha is the mean luminance.
3. **Canvas pass** draws into `canvasFbo`, one pixel per output pixel, as `glyph.frag` (ASCII), `mosaic.frag`, or `blit.frag` (raw).
4. **Present pass** (`image.frag`) letterboxes the canvas into the window. PNG and GIF export read `canvasFbo`, not the window.

Non-obvious details that span files:

- All fullscreen passes use a single triangle generated from `gl_VertexID` in `fullscreen.vert`. Core profile still needs an empty VAO bound.
- The canvas size is derived, not fixed. `canvasGrid(columns, aspect, maxTexture)` in `main.cpp` gives `rows = round(cols / aspect * cellW / cellH)` and clamps to `GL_MAX_TEXTURE_SIZE`. The canvas is `cols*8` by `rows*16` px, so the column count determines the export size. `[` and `]` change the columns.
- `glyph.frag` applies luminance in a fixed order: the auto-levels remap, then gain, then contrast around 0.5, then a clamp, then the glyph index. The 10 glyphs are hand-drawn in `glyph_atlas.cpp`, ordered by ink density, in an R8 texture. The atlas stores the top row first while window y points up, so the shader flips.
- Auto-levels is computed on the CPU. `levels.cpp` reads back the small cell texture and takes the 2nd to 98th percentile of alpha with a minimum span of 0.05. `main.cpp` eases the result with a 0.15 mix per frame so the sphere does not flicker.
- Foreground color is the cell's mean color divided by its max component. ANSI16 uses xterm default colors, and ANSI256 uses the 6x6x6 cube plus the gray ramp.
- `Texture` (`texture.cpp`) holds stills and GIFs as one `GL_TEXTURE_2D_ARRAY`, one layer per frame, loaded through stb_image (`stbi_load_gif_from_memory` for GIFs). It enforces a 512 MB frame cap and raises delays of 10 ms or less to 100 ms. `AppState` tracks the playback time and layer.
- `gif.h` defines non-inline functions, so it is included in exactly one translation unit, `gif_export.cpp`, with `GIF_FLIP_VERT`. Do not include it elsewhere or the link fails with duplicate symbols.
- GIF export runs inside the render loop in `main.cpp` and blocks the window. It does not poll events. It re-renders every frame and runs auto-levels per frame without easing.
- Wide exports are a known sharing problem. Chat sites shrink anything wider than about 400 px and blur the thin glyph strokes. `kShareWidthLimit` (400) drives the export-time warning and the `(wide)` flag in the window title.
