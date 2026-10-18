#pragma once

#include <vector>
#include "filter.h"
#include "shader.h"

// Stained glass: a Voronoi diagram of panes, each filled with the source color
// at its site. Neighboring panes of similar color are grouped, and the lead
// outlines each group. The panes are fixed in the canvas, so the picture moves
// under them.
class StainedGlassFilter : public Filter
{
public:
  StainedGlassFilter();
  ~StainedGlassFilter() override;

  StainedGlassFilter(const StainedGlassFilter &) = delete;
  StainedGlassFilter &operator=(const StainedGlassFilter &) = delete;

  const char *name() const override { return "Stained Glass"; }
  glm::ivec2 canvasSize(float aspect, int maxTexture) override;
  void prepare(GLuint scene, const glm::ivec2 &canvasSize) override;
  void render(const Framebuffer &canvas, GLuint scene) override;
  void beginExport() override;
  void endExport() override;
  bool onKey(int key, int action) override;
  std::string status() const override;

private:
  void setCommonUniforms(GLuint scene) const;

  // Values match the MODE_* constants in stained.frag.
  enum class ColorMode
  {
    Truecolor,
    Ansi256,
    Ansi16,
  };

  Shader shader_;
  Framebuffer paneFbo_;  // one texel per pane: its color
  GLuint labelTexture_ = 0;  // one RGBA32F texel per pane: group id, then the group's mean color
  glm::ivec2 paneGrid_{1, 1};
  bool grouped_ = false;
  std::vector<glm::vec3> paneColors_;
  std::vector<int> group_;
  std::vector<int> previousGroup_;  // last frame's groups, for hysteresis
  float previousMerge_ = 0.0f;
  std::vector<glm::vec3> groupMean_;
  std::vector<float> labels_;
  int panesAcross_;
  float lead_;
  float merge_ = 0.0f;
  ColorMode colorMode_ = ColorMode::Truecolor;
  glm::ivec2 size_{0, 0};  // set by canvasSize()
};
