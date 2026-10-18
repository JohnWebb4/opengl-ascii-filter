#include "stained_glass_filter.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <GLFW/glfw3.h>

#include "region_merge.h"

// Canvas length of the longer side, whatever the source aspect.
static const int kCanvasLongSide = 1024;

static const int kDefaultPanes = 40;
static const int kMinPanes = 8;
static const int kMaxPanes = 160;

static const float kDefaultLead = 0.05f;
static const float kMaxMerge = 0.6f;

StainedGlassFilter::StainedGlassFilter()
    : shader_("fullscreen.vert", "stained.frag"),
      paneFbo_(1, 1, false),
      panesAcross_(kDefaultPanes),
      lead_(kDefaultLead)
{
  glGenTextures(1, &labelTexture_);
  glBindTexture(GL_TEXTURE_2D, labelTexture_);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

StainedGlassFilter::~StainedGlassFilter()
{
  glDeleteTextures(1, &labelTexture_);
}

void StainedGlassFilter::setCommonUniforms(GLuint scene) const
{
  shader_.use();
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, scene);
  shader_.setInt("uScene", 0);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, labelTexture_);
  glActiveTexture(GL_TEXTURE0);
  shader_.setInt("uLabels", 1);
  shader_.setVec2("uCanvasSize", glm::vec2(size_));
  shader_.setFloat("uCellPx", static_cast<float>(size_.x) / panesAcross_);
  shader_.setIVec2("uPaneGrid", paneGrid_);
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
  const float cellPx = static_cast<float>(size_.x) / panesAcross_;
  paneGrid_ = glm::ivec2(panesAcross_, std::max(1, static_cast<int>(std::ceil(size_.y / cellPx - 1e-4f))));

  // Without a threshold every pane is its own group, so skip the readback.
  grouped_ = merge_ > 0.0f;
  if (!grouped_)
  {
    return;
  }

  // Pane pass -> one texel per pane
  paneFbo_.resize(paneGrid_.x, paneGrid_.y);
  paneFbo_.bind();
  setCommonUniforms(scene);
  shader_.setInt("uPanePass", 1);
  glDrawArrays(GL_TRIANGLES, 0, 3);

  const size_t count = static_cast<size_t>(paneGrid_.x) * paneGrid_.y;
  std::vector<uint8_t> pixels(count * 4);
  glBindTexture(GL_TEXTURE_2D, paneFbo_.colorTexture());
  glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

  paneColors_.resize(count);
  for (size_t i = 0; i < count; ++i)
  {
    paneColors_[i] = glm::vec3(pixels[i * 4], pixels[i * 4 + 1], pixels[i * 4 + 2]) / 255.0f;
  }
  mergeRegions(paneColors_, paneGrid_.x, paneGrid_.y, merge_, group_, groupMean_);

  labels_.resize(count * 4);
  for (size_t i = 0; i < count; ++i)
  {
    labels_[i * 4] = static_cast<float>(group_[i]);
    labels_[i * 4 + 1] = groupMean_[i].r;
    labels_[i * 4 + 2] = groupMean_[i].g;
    labels_[i * 4 + 3] = groupMean_[i].b;
  }
  glBindTexture(GL_TEXTURE_2D, labelTexture_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, paneGrid_.x, paneGrid_.y, 0, GL_RGBA, GL_FLOAT, labels_.data());
}

void StainedGlassFilter::render(const Framebuffer &canvas, GLuint scene)
{
  canvas.bind();
  setCommonUniforms(scene);
  shader_.setInt("uPanePass", 0);
  shader_.setInt("uGrouped", grouped_ ? 1 : 0);
  shader_.setFloat("uLead", lead_);
  glDrawArrays(GL_TRIANGLES, 0, 3);
}

bool StainedGlassFilter::onKey(int key, int action)
{
  // All adjust on key repeat too, so holding the key sweeps them.
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
  if (key == GLFW_KEY_RIGHT || key == GLFW_KEY_LEFT)
  {
    float delta = key == GLFW_KEY_RIGHT ? 0.02f : -0.02f;
    merge_ = glm::clamp(merge_ + delta, 0.0f, kMaxMerge);
    return true;
  }
  return false;
}

std::string StainedGlassFilter::status() const
{
  char text[160];
  snprintf(text, sizeof(text), "%d panes across, lead %.3f, merge %.2f, export %dx%d px", panesAcross_, lead_,
           merge_, size_.x, size_.y);
  return text;
}
