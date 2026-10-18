#version 330 core
in vec2 vUV;

uniform sampler2D uScene;
uniform vec2 uCanvasSize;  // canvas pixels
uniform float uCellPx;     // average pane size in canvas pixels
uniform float uLead;       // half-width of the lead lines, in panes
uniform bool uPanePass;    // draw one texel per pane (its color) instead of the glass
uniform bool uGrouped;     // uLabels holds pane groups; otherwise every pane is its own
uniform ivec2 uPaneGrid;   // panes across and down
uniform sampler2D uLabels; // per pane: r is the group id, gba is the group's mean color
uniform int uColorMode;    // MODE_*

out vec4 FragColor;

const int MODE_TRUECOLOR = 0;
const int MODE_ANSI256 = 1;
// Anything else is ANSI 16.

#include "palette.glsl"

const vec3 kLeadColor = vec3(0.03, 0.03, 0.04);

vec2 hash22(vec2 p)
{
  p = vec2(dot(p, vec2(127.1, 311.7)), dot(p, vec2(269.5, 183.3)));
  return fract(sin(p) * 43758.5453);
}

// Fixed pane site for a grid cell, kept away from the cell border so a 3x3
// neighborhood always contains the nearest site.
vec2 siteOf(vec2 cell)
{
  return cell + 0.15 + 0.7 * hash22(cell);
}

vec3 sceneAt(vec2 panePos)
{
  vec2 uv = clamp(panePos * uCellPx / uCanvasSize, 0.0, 1.0);
  return texture(uScene, uv).rgb;
}

// One piece of glass takes the mean of a few samples around its site.
vec3 paneColor(vec2 site)
{
  vec3 c = sceneAt(site);
  c += sceneAt(site + vec2(0.25, 0.0));
  c += sceneAt(site - vec2(0.25, 0.0));
  c += sceneAt(site + vec2(0.0, 0.25));
  c += sceneAt(site - vec2(0.0, 0.25));
  return c / 5.0;
}

// Group the pane in the given grid cell belongs to. Cells outside the canvas
// take their nearest edge pane's group, so no lead is drawn along the canvas edge.
float groupOf(vec2 cell)
{
  ivec2 c = clamp(ivec2(cell), ivec2(0), uPaneGrid - 1);
  return uGrouped ? texelFetch(uLabels, c, 0).r : float(c.y * uPaneGrid.x + c.x);
}

// Mean color of the group the pane in the given grid cell belongs to.
vec3 groupMeanOf(vec2 cell)
{
  ivec2 c = clamp(ivec2(cell), ivec2(0), uPaneGrid - 1);
  return texelFetch(uLabels, c, 0).gba;
}

void main()
{
  if (uPanePass)
  {
    FragColor = vec4(paneColor(siteOf(floor(gl_FragCoord.xy))), 1.0);
    return;
  }

  // Work in pane units: one grid cell is one unit.
  vec2 p = vUV * uCanvasSize / uCellPx;
  vec2 base = floor(p);

  // Nearest site
  vec2 site = vec2(0.0);
  vec2 siteCell = vec2(0.0);
  float best = 1e9;
  for (int j = -1; j <= 1; ++j)
  {
    for (int i = -1; i <= 1; ++i)
    {
      vec2 cell = base + vec2(i, j);
      vec2 s = siteOf(cell);
      float d = dot(p - s, p - s);
      if (d < best)
      {
        best = d;
        site = s;
        siteCell = cell;
      }
    }
  }

  vec3 glass = paneColor(site);
  float group = groupOf(siteCell);
  float aa = 1.0 / uCellPx;  // one canvas pixel

  // Each border is the perpendicular bisector to a neighbor. Only borders between
  // different groups get lead, so it outlines each group of panes.
  float edge = 1e9;  // nearest border that keeps its lead, for the highlight
  float lead = 0.0;
  for (int j = -2; j <= 2; ++j)
  {
    for (int i = -2; i <= 2; ++i)
    {
      vec2 cell = base + vec2(i, j);
      vec2 s = siteOf(cell);
      if (s != site)
      {
        float d = dot(0.5 * (site + s) - p, normalize(s - site));
        float other = groupOf(cell) != group ? 1.0 : 0.0;
        edge = min(edge, d + (1.0 - other) * 10.0);
        lead = max(lead, other * (1.0 - smoothstep(uLead - aa, uLead + aa, d)));
      }
    }
  }

  // In a terminal palette a whole group is one flat color, taken from the group
  // mean; truecolor keeps each pane's own color.
  vec3 source = glass;
  if (uColorMode != MODE_TRUECOLOR && uGrouped)
  {
    source = groupMeanOf(siteCell);
  }

  // Richer color, a little brightness variation per pane, brighter toward the center.
  float luma = dot(source, vec3(0.299, 0.587, 0.114));
  glass = mix(vec3(luma), source, 1.3);
  if (uColorMode == MODE_ANSI256)
  {
    glass = quantizeAnsi256(clamp(glass, 0.0, 1.0));
  }
  else if (uColorMode != MODE_TRUECOLOR)
  {
    glass = quantizeAnsi16(clamp(glass, 0.0, 1.0));
  }
  glass *= 0.9 + 0.2 * hash22(siteCell + 17.0).x;
  glass *= 1.1 - 0.35 * sqrt(best);
  glass += 0.10 * (1.0 - smoothstep(0.0, 0.3, edge)) * glass;

  FragColor = vec4(mix(clamp(glass, 0.0, 1.0), kLeadColor, lead), 1.0);
}
