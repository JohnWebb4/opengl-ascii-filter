#pragma once

#include "filter.h"
#include "shader.h"

// Stained glass: a Voronoi diagram of panes, each filled with the source color
// at its site and outlined in lead. A single fragment shader; the panes are
// fixed in the canvas, so the picture moves under them.
class StainedGlassFilter : public Filter
{
public:
  StainedGlassFilter();

  const char *name() const override { return "Stained Glass"; }
  glm::ivec2 canvasSize(float aspect, int maxTexture) override;
  void prepare(GLuint scene, const glm::ivec2 &canvasSize) override;
  void render(const Framebuffer &canvas, GLuint scene) override;
  bool onKey(int key, int action) override;
  std::string status() const override;

private:
  Shader shader_;
  int panesAcross_;
  float lead_;
  float merge_ = 0.0f;
  glm::ivec2 size_{0, 0};  // set by canvasSize()
};
