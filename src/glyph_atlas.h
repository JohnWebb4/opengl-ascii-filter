#pragma once

#include <glad/glad.h>

// Bitmap font atlas for the luminance ramp: glyphs laid out left to right in
// order of increasing ink density. Single-channel texture, 255 where inked.
class GlyphAtlas
{
public:
  static constexpr int kGlyphWidth = 8;
  static constexpr int kGlyphHeight = 16;

  GlyphAtlas();
  ~GlyphAtlas();

  GlyphAtlas(const GlyphAtlas &) = delete;
  GlyphAtlas &operator=(const GlyphAtlas &) = delete;

  GLuint texture() const { return texture_; }
  int glyphCount() const;

private:
  GLuint texture_ = 0;
};
