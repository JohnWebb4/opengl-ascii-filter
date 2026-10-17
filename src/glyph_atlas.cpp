#include "glyph_atlas.h"

#include <cstdint>
#include <vector>

// 8x16 glyphs, one byte per row, top row first, MSB is the leftmost pixel.
// Ordered by ink pixel count: 0, 4, 8, 12, 24, 24, 27, 28, 36, 40.
static const uint8_t kGlyphs[][GlyphAtlas::kGlyphHeight] = {
    // ' '
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    // '.'
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x18, 0x18, 0, 0},
    // ':'
    {0, 0, 0, 0, 0, 0x18, 0x18, 0, 0, 0, 0, 0x18, 0x18, 0, 0, 0},
    // '-'
    {0, 0, 0, 0, 0, 0, 0, 0x7E, 0x7E, 0, 0, 0, 0, 0, 0, 0},
    // '='
    {0, 0, 0, 0, 0, 0x7E, 0x7E, 0, 0x7E, 0x7E, 0, 0, 0, 0, 0, 0},
    // '+'
    {0, 0, 0, 0, 0x18, 0x18, 0x18, 0x7E, 0x7E, 0x18, 0x18, 0x18, 0, 0, 0, 0},
    // '%'
    {0, 0, 0x62, 0x92, 0x94, 0x68, 0x08, 0x10, 0x10, 0x2C, 0x52, 0x92, 0x8C, 0, 0, 0},
    // '*'
    {0, 0, 0, 0, 0x18, 0x5A, 0x3C, 0xFF, 0x3C, 0x5A, 0x18, 0, 0, 0, 0, 0},
    // '#'
    {0, 0, 0, 0x24, 0x24, 0x7E, 0x7E, 0x24, 0x24, 0x7E, 0x7E, 0x24, 0x24, 0, 0, 0},
    // '@'
    {0, 0, 0x7E, 0x42, 0x42, 0x5A, 0x5A, 0x5E, 0x5C, 0x40, 0x42, 0x7E, 0x3C, 0, 0, 0},
};

static const int kGlyphTotal = sizeof(kGlyphs) / sizeof(kGlyphs[0]);

GlyphAtlas::GlyphAtlas()
{
  const int width = kGlyphTotal * kGlyphWidth;
  std::vector<uint8_t> pixels(width * kGlyphHeight, 0);

  for (int g = 0; g < kGlyphTotal; ++g)
  {
    for (int y = 0; y < kGlyphHeight; ++y)
    {
      for (int x = 0; x < kGlyphWidth; ++x)
      {
        bool inked = (kGlyphs[g][y] >> (kGlyphWidth - 1 - x)) & 1;
        pixels[y * width + g * kGlyphWidth + x] = inked ? 255 : 0;
      }
    }
  }

  glGenTextures(1, &texture_);
  glBindTexture(GL_TEXTURE_2D, texture_);
  // Rows are width*1 bytes, which isn't a multiple of the default 4.
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width, kGlyphHeight, 0, GL_RED, GL_UNSIGNED_BYTE, pixels.data());
  glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

GlyphAtlas::~GlyphAtlas()
{
  glDeleteTextures(1, &texture_);
}

int GlyphAtlas::glyphCount() const
{
  return kGlyphTotal;
}
