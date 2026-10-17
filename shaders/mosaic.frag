#version 330 core

uniform sampler2D uCells;
uniform ivec2 uCellSize;

out vec4 FragColor;

// Debug view: each cell drawn as a flat block of its mean color.
void main()
{
  ivec2 cell = ivec2(gl_FragCoord.xy) / uCellSize;
  FragColor = vec4(texelFetch(uCells, cell, 0).rgb, 1.0);
}
