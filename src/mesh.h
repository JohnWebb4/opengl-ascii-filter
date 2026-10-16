#pragma once

#include <glad/glad.h>

// Indexed triangle mesh with interleaved position + normal (6 floats per vertex).
class Mesh
{
public:
  // Unit UV sphere centred at the origin.
  static Mesh sphere(int stacks, int slices);

  Mesh(Mesh &&other) noexcept;
  ~Mesh();

  Mesh(const Mesh &) = delete;
  Mesh &operator=(const Mesh &) = delete;

  void draw() const;

private:
  Mesh() = default;

  GLuint vao_ = 0, vbo_ = 0, ebo_ = 0;
  GLsizei indexCount_ = 0;
};
