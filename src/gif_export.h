#pragma once

#include <memory>
#include <string>
#include "framebuffer.h"

// "ascii_YYYYMMDD_HHMMSS.gif", relative to the working directory.
std::string timestampedGifName();

// Writes an animated GIF one framebuffer at a time. Each frame is quantized
// to its own 256-color palette.
class GifExporter
{
public:
  GifExporter(const std::string &path, int width, int height);
  ~GifExporter();

  GifExporter(const GifExporter &) = delete;
  GifExporter &operator=(const GifExporter &) = delete;

  // False if the file couldn't be created.
  bool ok() const;

  // Reads back the framebuffer, which must match the size given to the
  // constructor, and appends it. Delay is in hundredths of a second.
  bool addFrame(const Framebuffer &fbo, int delayCentiseconds);

  // Writes the trailer and closes the file; also done by the destructor.
  void finish();

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
  int width_, height_;
};
