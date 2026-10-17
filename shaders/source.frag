#version 330 core
in vec2 vUV;

uniform sampler2DArray uSource;
uniform int uLayer;
uniform vec3 uBackground;

out vec4 FragColor;

// Draws one frame of an image across the whole target. The canvas is sized
// to the image's aspect, so no fitting is needed. Transparent pixels (common
// in GIFs and PNGs) show the background color.
void main()
{
  vec4 color = texture(uSource, vec3(vUV, float(uLayer)));
  FragColor = vec4(mix(uBackground, color.rgb, color.a), 1.0);
}
