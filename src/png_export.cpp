#include "png_export.h"

#include <ctime>
#include <iostream>
#include <vector>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

std::string timestampedPngName()
{
  std::time_t now = std::time(nullptr);
  char buffer[64];
  std::strftime(buffer, sizeof(buffer), "ascii_%Y%m%d_%H%M%S.png", std::localtime(&now));
  return buffer;
}

bool writePng(const Framebuffer &fbo, const std::string &path)
{
  const int width = fbo.width();
  const int height = fbo.height();
  std::vector<unsigned char> pixels(static_cast<size_t>(width) * height * 3);

  fbo.bind();
  // RGB rows aren't necessarily a multiple of the default 4 bytes.
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
  glPixelStorei(GL_PACK_ALIGNMENT, 4);

  // GL rows run bottom to top; PNG rows run top to bottom.
  stbi_flip_vertically_on_write(1);
  if (!stbi_write_png(path.c_str(), width, height, 3, pixels.data(), width * 3))
  {
    std::cout << "Failed to write " << path << std::endl;
    return false;
  }
  return true;
}
