#include "texture.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <utility>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// Decoded frames above this size aren't uploaded; the animation is cut short.
static const size_t kMaxFrameBytes = 512u * 1024 * 1024;

// GIFs often use 0 or 10 ms to mean "as fast as possible"; browsers play those at 100 ms.
static const int kFastDelayMs = 10;
static const int kClampedDelayMs = 100;

Texture::Texture(const std::string &path)
{
  std::ifstream file(path, std::ios::binary);
  std::vector<unsigned char> data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  if (data.empty())
  {
    std::cout << "Failed to read image " << path << std::endl;
    return;
  }

  // GL expects the first row to be the bottom of the image.
  stbi_set_flip_vertically_on_load(true);

  int channels;
  int frames = 1;
  int *delays = nullptr;
  unsigned char *pixels;
  if (data.size() >= 4 && std::memcmp(data.data(), "GIF8", 4) == 0)
  {
    pixels = stbi_load_gif_from_memory(data.data(), static_cast<int>(data.size()), &delays,
                                       &width_, &height_, &frames, &channels, 4);
  }
  else
  {
    pixels = stbi_load_from_memory(data.data(), static_cast<int>(data.size()), &width_, &height_, &channels, 4);
  }

  if (!pixels)
  {
    std::cout << "Failed to load image " << path << ": " << stbi_failure_reason() << std::endl;
    width_ = height_ = 0;
    return;
  }

  int maxLayers;
  glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &maxLayers);
  size_t frameBytes = static_cast<size_t>(width_) * height_ * 4;
  int keep = static_cast<int>(std::min<size_t>(frames, std::min<size_t>(maxLayers, kMaxFrameBytes / frameBytes)));
  keep = std::max(keep, 1);
  if (keep < frames)
  {
    std::cout << path << ": using the first " << keep << " of " << frames << " frames" << std::endl;
  }
  frameCount_ = keep;

  double end = 0.0;
  for (int i = 0; i < keep; ++i)
  {
    int ms = delays ? delays[i] : 0;
    end += (ms <= kFastDelayMs ? kClampedDelayMs : ms) / 1000.0;
    frameEnds_.push_back(end);
  }

  glGenTextures(1, &id_);
  glBindTexture(GL_TEXTURE_2D_ARRAY, id_);
  // Row size isn't necessarily a multiple of the default 4 bytes.
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, width_, height_, keep, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
  // Mipmaps so shrinking a large image into a small canvas averages instead of aliasing.
  glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  stbi_image_free(pixels);
  stbi_image_free(delays);
}

Texture::~Texture()
{
  glDeleteTextures(1, &id_);
}

Texture::Texture(Texture &&other) noexcept
    : id_(std::exchange(other.id_, 0)),
      width_(std::exchange(other.width_, 0)),
      height_(std::exchange(other.height_, 0)),
      frameCount_(std::exchange(other.frameCount_, 0)),
      frameEnds_(std::move(other.frameEnds_))
{
}

Texture &Texture::operator=(Texture &&other) noexcept
{
  if (this != &other)
  {
    glDeleteTextures(1, &id_);
    id_ = std::exchange(other.id_, 0);
    width_ = std::exchange(other.width_, 0);
    height_ = std::exchange(other.height_, 0);
    frameCount_ = std::exchange(other.frameCount_, 0);
    frameEnds_ = std::move(other.frameEnds_);
  }
  return *this;
}

int Texture::frameAt(double seconds) const
{
  if (frameCount_ <= 1)
  {
    return 0;
  }

  double total = frameEnds_.back();
  double t = std::fmod(seconds, total);
  if (t < 0.0)
  {
    t += total;
  }

  auto it = std::upper_bound(frameEnds_.begin(), frameEnds_.end(), t);
  return std::min(static_cast<int>(it - frameEnds_.begin()), frameCount_ - 1);
}

double Texture::frameStart(int frame) const
{
  return frame <= 0 ? 0.0 : frameEnds_[std::min(frame, frameCount_) - 1];
}
