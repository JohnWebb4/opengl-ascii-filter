#include "stained_glass_filter.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <GLFW/glfw3.h>

// Canvas length of the longer side, whatever the source aspect.
static const int kCanvasLongSide = 1024;

static const int kDefaultPanes = 40;
static const int kMinPanes = 8;
static const int kMaxPanes = 160;

static const float kDefaultLead = 0.05f;

StainedGlassFilter::StainedGlassFilter()
    : shader_("fullscreen.vert", "stained.frag"), panesAcross_(kDefaultPanes), lead_(kDefaultLead)
{
}

glm::ivec2 StainedGlassFilter::canvasSize(float aspect, int maxTexture)
{
  int longSide = std::min(kCanvasLongSide, maxTexture);
  int shortSide = std::max(1, static_cast<int>(std::lround(longSide / std::max(aspect, 1.0f / aspect))));
  size_ = aspect >= 1.0f ? glm::ivec2(longSide, shortSide) : glm::ivec2(shortSide, longSide);
  return size_;
}

void StainedGlassFilter::prepare(GLuint scene, const glm::ivec2 &canvasSize)
{
}

void StainedGlassFilter::render(const Framebuffer &canvas, GLuint scene)
{
  canvas.bind();
  shader_.use();
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, scene);
  shader_.setInt("uScene", 0);
  shader_.setVec2("uCanvasSize", glm::vec2(size_));
  shader_.setFloat("uCellPx", static_cast<float>(size_.x) / panesAcross_);
  shader_.setFloat("uLead", lead_);
  glDrawArrays(GL_TRIANGLES, 0, 3);
}

bool StainedGlassFilter::onKey(int key, int action)
{
  // Both adjust on key repeat too, so holding the key sweeps them.
  if (key == GLFW_KEY_LEFT_BRACKET || key == GLFW_KEY_RIGHT_BRACKET)
  {
    int delta = key == GLFW_KEY_RIGHT_BRACKET ? 4 : -4;
    panesAcross_ = glm::clamp(panesAcross_ + delta, kMinPanes, kMaxPanes);
    return true;
  }
  if (key == GLFW_KEY_UP || key == GLFW_KEY_DOWN)
  {
    float delta = key == GLFW_KEY_UP ? 0.005f : -0.005f;
    lead_ = glm::clamp(lead_ + delta, 0.0f, 0.2f);
    return true;
  }
  return false;
}

std::string StainedGlassFilter::status() const
{
  char text[128];
  snprintf(text, sizeof(text), "%d panes across, lead %.3f, export %dx%d px", panesAcross_, lead_, size_.x,
           size_.y);
  return text;
}
