#include "levels.h"

#include <algorithm>
#include <cstdint>
#include <vector>

void estimateLevels(const Framebuffer &cells, float &low, float &high)
{
  const size_t count = static_cast<size_t>(cells.width()) * cells.height();
  std::vector<uint8_t> pixels(count * 4);

  glBindTexture(GL_TEXTURE_2D, cells.colorTexture());
  glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

  size_t histogram[256] = {};
  for (size_t i = 0; i < count; ++i)
  {
    ++histogram[pixels[i * 4 + 3]];
  }

  const size_t lowTarget = count * 2 / 100;
  const size_t highTarget = count * 98 / 100;

  size_t cumulative = 0;
  int lowBin = 0, highBin = 255;
  bool lowFound = false;
  for (int bin = 0; bin < 256; ++bin)
  {
    cumulative += histogram[bin];
    if (!lowFound && cumulative >= lowTarget)
    {
      lowBin = bin;
      lowFound = true;
    }
    if (cumulative >= highTarget)
    {
      highBin = bin;
      break;
    }
  }

  const float minSpan = 0.05f;
  low = lowBin / 255.0f;
  high = std::max(highBin / 255.0f, low + minSpan);
}
