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
uniform int uColorMode;

out vec4 FragColor;

// xterm's default 16 colors.
const vec3 kAnsi16[16] = vec3[16](
    vec3(0, 0, 0) / 255.0, vec3(205, 0, 0) / 255.0, vec3(0, 205, 0) / 255.0, vec3(205, 205, 0) / 255.0,
    vec3(0, 0, 238) / 255.0, vec3(205, 0, 205) / 255.0, vec3(0, 205, 205) / 255.0, vec3(229, 229, 229) / 255.0,
    vec3(127, 127, 127) / 255.0, vec3(255, 0, 0) / 255.0, vec3(0, 255, 0) / 255.0, vec3(255, 255, 0) / 255.0,
    vec3(92, 92, 255) / 255.0, vec3(255, 0, 255) / 255.0, vec3(0, 255, 255) / 255.0, vec3(255, 255, 255) / 255.0);

// Component levels of the 6x6x6 color cube (codes 16-231).
const float kCubeLevels[6] = float[6](0.0, 95.0 / 255.0, 135.0 / 255.0, 175.0 / 255.0, 215.0 / 255.0, 1.0);

float nearestCubeLevel(float v)
{
  float best = kCubeLevels[0];
  for (int i = 1; i < 6; ++i)
  {
    if (abs(kCubeLevels[i] - v) < abs(best - v))
    {
      best = kCubeLevels[i];
    }
  }
  return best;
}

vec3 quantizeAnsi16(vec3 c)
{
  vec3 best = kAnsi16[0];
  float bestDist = distance(c, best);
  for (int i = 1; i < 16; ++i)
  {
    float d = distance(c, kAnsi16[i]);
    if (d < bestDist)
    {
      bestDist = d;
      best = kAnsi16[i];
    }
  }
  return best;
}

// Nearest of the cube (16-231) and grayscale ramp (232-255); the 16 system
// colors are skipped because terminals let users redefine them.
vec3 quantizeAnsi256(vec3 c)
{
  vec3 cube = vec3(nearestCubeLevel(c.r), nearestCubeLevel(c.g), nearestCubeLevel(c.b));

  float gray = (c.r + c.g + c.b) / 3.0;
  float step = clamp(round((gray * 255.0 - 8.0) / 10.0), 0.0, 23.0);
  vec3 grayColor = vec3((8.0 + 10.0 * step) / 255.0);

  return distance(c, cube) <= distance(c, grayColor) ? cube : grayColor;
}

void main()
{
  ivec2 px = ivec2(gl_FragCoord.xy);
  ivec2 cell = px / uCellSize;
  ivec2 local = px - cell * uCellSize;

  vec4 cellData = texelFetch(uCells, cell, 0);
  float luma = clamp(cellData.a * uGain, 0.0, 0.999);
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
