#version 330 core

uniform sampler2D uCells;  // a = mean luminance per cell
uniform sampler2D uAtlas;  // glyphs left to right by increasing ink
uniform ivec2 uCellSize;
uniform int uGlyphCount;
uniform float uGain;       // scales luminance before picking a glyph

out vec4 FragColor;

void main()
{
  ivec2 px = ivec2(gl_FragCoord.xy);
  ivec2 cell = px / uCellSize;
  ivec2 local = px - cell * uCellSize;

  float luma = clamp(texelFetch(uCells, cell, 0).a * uGain, 0.0, 0.999);
  int glyph = int(luma * float(uGlyphCount));

  // Window y points up but the atlas stores the top glyph row first.
  ivec2 atlasPx = ivec2(glyph * uCellSize.x + local.x, uCellSize.y - 1 - local.y);
  float ink = texelFetch(uAtlas, atlasPx, 0).r;

  FragColor = vec4(vec3(ink), 1.0);
}
