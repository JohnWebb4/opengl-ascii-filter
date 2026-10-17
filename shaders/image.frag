#version 330 core
in vec2 vUV;

uniform sampler2D uImage;
uniform float uImageAspect;  // image width / height
uniform float uViewAspect;   // render target width / height
uniform vec3 uBackground;

out vec4 FragColor;

// Fits the image inside the target without stretching, filling the
// leftover bars with the background color.
void main()
{
  // Fraction of the target covered by the image on each axis.
  vec2 size = uImageAspect > uViewAspect
      ? vec2(1.0, uViewAspect / uImageAspect)
      : vec2(uImageAspect / uViewAspect, 1.0);

  vec2 uv = (vUV - 0.5) / size + 0.5;

  // Sampled outside the branch: mip selection needs derivatives from uniform flow.
  vec3 color = texture(uImage, uv).rgb;

  bool outside = any(lessThan(uv, vec2(0.0))) || any(greaterThan(uv, vec2(1.0)));
  FragColor = vec4(outside ? uBackground : color, 1.0);
}
