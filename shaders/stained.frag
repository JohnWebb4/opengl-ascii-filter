#version 330 core
in vec2 vUV;

uniform sampler2D uScene;
uniform vec2 uCanvasSize;  // canvas pixels
uniform float uCellPx;     // average pane size in canvas pixels
uniform float uLead;       // half-width of the lead lines, in panes

out vec4 FragColor;

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

void main()
{
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

  // Distance to the nearest pane border: the perpendicular bisector to each neighbor.
  float edge = 1e9;
  for (int j = -2; j <= 2; ++j)
  {
    for (int i = -2; i <= 2; ++i)
    {
      vec2 s = siteOf(base + vec2(i, j));
      if (s != site)
      {
        edge = min(edge, dot(0.5 * (site + s) - p, normalize(s - site)));
      }
    }
  }

  // One piece of glass takes the mean of a few samples around its site.
  vec3 glass = sceneAt(site);
  glass += sceneAt(site + vec2(0.25, 0.0));
  glass += sceneAt(site - vec2(0.25, 0.0));
  glass += sceneAt(site + vec2(0.0, 0.25));
  glass += sceneAt(site - vec2(0.0, 0.25));
  glass /= 5.0;

  // Richer color, a little brightness variation per pane, brighter toward the center.
  float luma = dot(glass, vec3(0.299, 0.587, 0.114));
  glass = mix(vec3(luma), glass, 1.3);
  glass *= 0.9 + 0.2 * hash22(siteCell + 17.0).x;
  glass *= 1.1 - 0.35 * sqrt(best);
  glass += 0.10 * (1.0 - smoothstep(0.0, 0.3, edge)) * glass;

  float aa = 1.0 / uCellPx;  // one canvas pixel
  float lead = 1.0 - smoothstep(uLead - aa, uLead + aa, edge);
  FragColor = vec4(mix(clamp(glass, 0.0, 1.0), kLeadColor, lead), 1.0);
}
