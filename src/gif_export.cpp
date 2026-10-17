#include "gif_export.h"

#include <ctime>
#include <vector>

// OpenGL reads pixels bottom row first; gif.h flips them while writing.
#define GIF_FLIP_VERT
#include <gif.h>

struct GifExporter::Impl
{
  GifWriter writer;
  bool open = false;
  std::vector<unsigned char> pixels;
};

std::string timestampedGifName()
{
  std::time_t now = std::time(nullptr);
  char buffer[64];
  std::strftime(buffer, sizeof(buffer), "ascii_%Y%m%d_%H%M%S.gif", std::localtime(&now));
  return buffer;
}

GifExporter::GifExporter(const std::string &path, int width, int height)
    : impl_(new Impl), width_(width), height_(height)
{
  // The initial delay only needs to be nonzero: that writes the infinite-loop header.
  impl_->open = GifBegin(&impl_->writer, path.c_str(), width, height, 1);
  impl_->pixels.resize(static_cast<size_t>(width) * height * 4);
}

GifExporter::~GifExporter()
{
  finish();
}

bool GifExporter::ok() const
{
  return impl_->open;
}

bool GifExporter::addFrame(const Framebuffer &fbo, int delayCentiseconds)
{
  if (!impl_->open || fbo.width() != width_ || fbo.height() != height_)
  {
    return false;
  }

  fbo.bind();
  glReadPixels(0, 0, width_, height_, GL_RGBA, GL_UNSIGNED_BYTE, impl_->pixels.data());
  return GifWriteFrame(&impl_->writer, impl_->pixels.data(), width_, height_, delayCentiseconds);
}

void GifExporter::finish()
{
  if (impl_->open)
  {
    GifEnd(&impl_->writer);
    impl_->open = false;
  }
}
