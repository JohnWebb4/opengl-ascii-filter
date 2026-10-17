#pragma once

#include <glad/glad.h>

// Offscreen render target: RGBA8 color texture + depth renderbuffer.
class Framebuffer
{
public:
  Framebuffer(int width, int height, bool withDepth = true);
  ~Framebuffer();

  Framebuffer(const Framebuffer &) = delete;
  Framebuffer &operator=(const Framebuffer &) = delete;

  void resize(int width, int height);
  void bind() const;
  static void unbind();

  GLuint colorTexture() const { return color_; }
  int width() const { return width_; }
  int height() const { return height_; }

private:
  void allocate();

  GLuint fbo_ = 0, color_ = 0, depth_ = 0;
  int width_, height_;
  bool withDepth_;
};
