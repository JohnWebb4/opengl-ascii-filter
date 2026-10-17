#include "texture.h"

#include <iostream>
#include <utility>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

Texture::Texture(const std::string &path)
{
  // GL expects the first row to be the bottom of the image.
  stbi_set_flip_vertically_on_load(true);

  int channels;
  unsigned char *pixels = stbi_load(path.c_str(), &width_, &height_, &channels, 4);
  if (!pixels)
  {
    std::cout << "Failed to load image " << path << ": " << stbi_failure_reason() << std::endl;
    width_ = height_ = 0;
    return;
  }

  glGenTextures(1, &id_);
  glBindTexture(GL_TEXTURE_2D, id_);
  // Row size isn't necessarily a multiple of the default 4 bytes.
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width_, height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  stbi_image_free(pixels);
}

Texture::~Texture()
{
  glDeleteTextures(1, &id_);
}

Texture::Texture(Texture &&other) noexcept
    : id_(other.id_), width_(other.width_), height_(other.height_)
{
  other.id_ = 0;
  other.width_ = other.height_ = 0;
}

Texture &Texture::operator=(Texture &&other) noexcept
{
  if (this != &other)
  {
    glDeleteTextures(1, &id_);
    id_ = std::exchange(other.id_, 0);
    width_ = std::exchange(other.width_, 0);
    height_ = std::exchange(other.height_, 0);
  }
  return *this;
}
