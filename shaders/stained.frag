#version 330 core
in vec2 vUV;

uniform sampler2D uScene;
uniform vec2 uCanvasSize;  // canvas pixels
uniform float uCellPx;     // average pane size in canvas pixels
uniform float uLead;       // half-width of the lead lines, in panes
uniform float uMerge;      // panes closer than this in color lose the lead between them; 0 is off

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

// Luma-weighted RGB distance, 0 for equal colors and 1 for black against white.
float colorDistance(vec3 a, vec3 b)
{
  vec3 d = a - b;
  return sqrt(dot(d * d, vec3(0.299, 0.587, 0.114)));
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

  vec3 glass = paneColor(site);
  float aa = 1.0 / uCellPx;  // one canvas pixel

  // Each border is the perpendicular bisector to a neighbor. Where the neighbor
  // is close in color the lead there is dropped, so similar panes read as one.
  float edge = 1e9;  // nearest border that keeps its lead, for the highlight
  float lead = 0.0;
  for (int j = -2; j <= 2; ++j)
  {
    for (int i = -2; i <= 2; ++i)
    {
      vec2 s = siteOf(base + vec2(i, j));
      if (s != site)
      {
        float d = dot(0.5 * (site + s) - p, normalize(s - site));
        float merged = 0.0;
        // Only borders near enough to show lead or highlight need the neighbor's color.
        if (uMerge > 0.0 && d < 0.3)
        {
          merged = 1.0 - smoothstep(0.6 * uMerge, uMerge, colorDistance(glass, paneColor(s)));
        }
        edge = min(edge, d + merged * 10.0);
        lead = max(lead, (1.0 - merged) * (1.0 - smoothstep(uLead - aa, uLead + aa, d)));
      }
    }
  }

  // Richer color, a little brightness variation per pane, brighter toward the center.
  float luma = dot(glass, vec3(0.299, 0.587, 0.114));
  glass = mix(vec3(luma), glass, 1.3);
  glass *= 0.9 + 0.2 * hash22(siteCell + 17.0).x;
  glass *= 1.1 - 0.35 * sqrt(best);
  glass += 0.10 * (1.0 - smoothstep(0.0, 0.3, edge)) * glass;

  FragColor = vec4(mix(clamp(glass, 0.0, 1.0), kLeadColor, lead), 1.0);
}
