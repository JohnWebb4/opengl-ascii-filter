// Terminal palettes. Included by shaders with #include "palette.glsl".

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
