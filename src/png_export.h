#pragma once

#include <string>
#include "framebuffer.h"

// "ascii_YYYYMMDD_HHMMSS.png", relative to the working directory.
std::string timestampedPngName();

// Binds the framebuffer, reads its pixels back and writes them as a PNG.
// Returns false (after logging) if the file couldn't be written.
bool writePng(const Framebuffer &fbo, const std::string &path);
