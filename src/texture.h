#pragma once

#include <string>
#include <glad/glad.h>

// RGBA8 2D texture loaded from an image file (PNG, JPEG, ...).
// Row 0 is the bottom of the image, matching OpenGL's convention.
class Texture
{
public:
  // Empty texture; valid() is false.
  Texture() = default;

  // On failure the error is logged and valid() returns false.
  explicit Texture(const std::string &path);
  ~Texture();

  Texture(Texture &&other) noexcept;
  Texture &operator=(Texture &&other) noexcept;
  Texture(const Texture &) = delete;
  Texture &operator=(const Texture &) = delete;

  bool valid() const { return id_ != 0; }
  GLuint id() const { return id_; }
  int width() const { return width_; }
  int height() const { return height_; }

private:
  GLuint id_ = 0;
  int width_ = 0, height_ = 0;
};
