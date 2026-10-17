#pragma once

#include <string>
#include <vector>
#include <glad/glad.h>

// RGBA8 image loaded from a file (PNG, JPEG, GIF, ...) as a GL_TEXTURE_2D_ARRAY
// with one layer per frame; stills have a single layer. Animated GIFs keep
// their per-frame delays. Row 0 is the bottom of the image, matching OpenGL.
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

  int frameCount() const { return frameCount_; }
  bool animated() const { return frameCount_ > 1; }

  // Layer showing at the given time in seconds; the animation loops.
  int frameAt(double seconds) const;
  // Time in seconds at which the given layer starts.
  double frameStart(int frame) const;

private:
  GLuint id_ = 0;
  int width_ = 0, height_ = 0;
  int frameCount_ = 0;
  std::vector<double> frameEnds_;  // cumulative end time of each frame, in seconds
};
