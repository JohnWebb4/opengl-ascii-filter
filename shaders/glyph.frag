#version 330 core

const int MODE_MONO = 0;
const int MODE_ANSI16 = 1;
const int MODE_ANSI256 = 2;
// Anything else is truecolor.

uniform sampler2D uCells;  // rgb = mean color, a = mean luminance per cell
uniform sampler2D uAtlas;  // glyphs left to right by increasing ink
uniform ivec2 uCellSize;
uniform int uGlyphCount;
uniform float uGain;       // scales luminance before picking a glyph
uniform vec2 uLevels;      // luminance range stretched to 0-1 (black point, white point)
uniform float uContrast;   // scales luminance around mid-gray
uniform int uColorMode;

out vec4 FragColor;

#include "palette.glsl"

void main()
{
  ivec2 px = ivec2(gl_FragCoord.xy);
  ivec2 cell = px / uCellSize;
  ivec2 local = px - cell * uCellSize;

  vec4 cellData = texelFetch(uCells, cell, 0);
  float luma = (cellData.a - uLevels.x) / (uLevels.y - uLevels.x);
  luma = (luma * uGain - 0.5) * uContrast + 0.5;
  luma = clamp(luma, 0.0, 0.999);
  int glyph = int(luma * float(uGlyphCount));

  // Window y points up but the atlas stores the top glyph row first.
  ivec2 atlasPx = ivec2(glyph * uCellSize.x + local.x, uCellSize.y - 1 - local.y);
  float ink = texelFetch(uAtlas, atlasPx, 0).r;

  // The glyph density already carries brightness, so keep only the hue at
  // full intensity for the foreground color.
  vec3 fg = cellData.rgb / max(max(cellData.r, max(cellData.g, cellData.b)), 1e-4);

  if (uColorMode == MODE_MONO)
  {
    fg = vec3(1.0);
  }
  else if (uColorMode == MODE_ANSI16)
  {
    fg = quantizeAnsi16(fg);
  }
  else if (uColorMode == MODE_ANSI256)
  {
    fg = quantizeAnsi256(fg);
  }

  FragColor = vec4(fg * ink, 1.0);
}
