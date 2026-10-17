#pragma once

#include "framebuffer.h"

// Reads back a cell texture (luminance in alpha) and returns the 2nd and 98th
// percentile luminance, ignoring outlier cells so a few specks don't set the range.
// The span is kept above a minimum so flat images aren't amplified into noise.
void estimateLevels(const Framebuffer &cells, float &low, float &high);
