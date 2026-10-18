#pragma once

#include <string>
#include <glad/glad.h>
#include <glm/glm.hpp>

#include "framebuffer.h"

// One way of turning the source picture into the canvas. Exactly one filter is
// active at a time. The caller owns the source pass and the present pass, binds
// an empty VAO before calling prepare() and render(), and has depth testing off.
class Filter
{
public:
  virtual ~Filter() = default;

  virtual const char *name() const = 0;

  // Pixel size of the canvas for content of the given aspect ratio (width / height).
  // The source is drawn at this size too.
  virtual glm::ivec2 canvasSize(float aspect, int maxTexture) = 0;

  // Once per frame, after the source has been drawn into scene: any passes that
  // measure or reduce it before render().
  virtual void prepare(GLuint scene, const glm::ivec2 &canvasSize) = 0;

  // Draws the filter's current view into the canvas.
  virtual void render(const Framebuffer &canvas, GLuint scene) = 0;

  // Draws what an exported PNG or GIF frame shows, which may differ from the
  // on-screen view.
  virtual void renderExport(const Framebuffer &canvas, GLuint scene) { render(canvas, scene); }

  // Called around a GIF export so the filter can drop any on-screen smoothing
  // while prepare() runs once per exported frame.
  virtual void beginExport() {}
  virtual void endExport() {}

  // Logs a warning if an export at this size is likely to look bad when shared.
  virtual void warnExport(const glm::ivec2 &canvasSize) const {}

  // Key press, release excluded. Returns true if the window title should refresh.
  virtual bool onKey(int key, int action) = 0;

  // The filter's part of the window title: its size and current settings.
  virtual std::string status() const = 0;
};
