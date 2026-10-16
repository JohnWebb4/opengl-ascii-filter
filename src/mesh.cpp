#include "mesh.h"

#include <cmath>
#include <vector>

Mesh Mesh::sphere(int stacks, int slices)
{
  const float pi = 3.14159265358979f;

  std::vector<float> vertices;
  for (int i = 0; i <= stacks; ++i)
  {
    float phi = pi * i / stacks;
    for (int j = 0; j <= slices; ++j)
    {
      float theta = 2.0f * pi * j / slices;
      float x = std::sin(phi) * std::cos(theta);
      float y = std::cos(phi);
      float z = std::sin(phi) * std::sin(theta);
      // On a unit sphere the normal equals the position.
      vertices.insert(vertices.end(), {x, y, z, x, y, z});
    }
  }

  std::vector<unsigned int> indices;
  for (int i = 0; i < stacks; ++i)
  {
    for (int j = 0; j < slices; ++j)
    {
      unsigned int a = i * (slices + 1) + j;
      unsigned int b = a + slices + 1;
      indices.insert(indices.end(), {a, b, a + 1, a + 1, b, b + 1});
    }
  }

  Mesh mesh;
  mesh.indexCount_ = static_cast<GLsizei>(indices.size());

  glGenVertexArrays(1, &mesh.vao_);
  glGenBuffers(1, &mesh.vbo_);
  glGenBuffers(1, &mesh.ebo_);

  glBindVertexArray(mesh.vao_);

  glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo_);
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo_);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  glBindVertexArray(0);
  return mesh;
}

Mesh::Mesh(Mesh &&other) noexcept
    : vao_(other.vao_), vbo_(other.vbo_), ebo_(other.ebo_), indexCount_(other.indexCount_)
{
  other.vao_ = other.vbo_ = other.ebo_ = 0;
  other.indexCount_ = 0;
}

Mesh::~Mesh()
{
  glDeleteBuffers(1, &ebo_);
  glDeleteBuffers(1, &vbo_);
  glDeleteVertexArrays(1, &vao_);
}

void Mesh::draw() const
{
  glBindVertexArray(vao_);
  glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, 0);
  glBindVertexArray(0);
}
