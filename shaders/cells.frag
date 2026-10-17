#version 330 core

uniform sampler2D uScene;
uniform ivec2 uCellSize;  // cell footprint in scene pixels

// One output texel per character cell: rgb = mean color, a = mean luminance.
out vec4 FragColor;

void main()
{
  ivec2 sceneSize = textureSize(uScene, 0);
  ivec2 origin = ivec2(gl_FragCoord.xy) * uCellSize;

  vec3 sum = vec3(0.0);
  float count = 0.0;
  for (int y = 0; y < uCellSize.y; ++y)
  {
    for (int x = 0; x < uCellSize.x; ++x)
    {
      ivec2 p = origin + ivec2(x, y);
      // Edge cells may extend past the scene when the size isn't a multiple.
      if (p.x < sceneSize.x && p.y < sceneSize.y)
      {
        sum += texelFetch(uScene, p, 0).rgb;
        count += 1.0;
      }
    }
  }

  vec3 mean = sum / max(count, 1.0);
  float luma = dot(mean, vec3(0.2126, 0.7152, 0.0722));
  FragColor = vec4(mean, luma);
}
