#version 330 core
in vec3 vNormal;
in vec3 vWorldPos;

uniform vec3 uLightDir;  // direction the light travels
uniform vec3 uViewPos;
uniform vec3 uColor;

out vec4 FragColor;

void main()
{
  vec3 n = normalize(vNormal);
  vec3 l = normalize(-uLightDir);
  vec3 v = normalize(uViewPos - vWorldPos);

  float diffuse = max(dot(n, l), 0.0);
  float specular = pow(max(dot(n, normalize(l + v)), 0.0), 32.0);

  vec3 color = uColor * (0.1 + 0.9 * diffuse) + vec3(0.3) * specular;
  FragColor = vec4(color, 1.0);
}
