#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <string>
#include <utility>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "framebuffer.h"
#include "glyph_atlas.h"
#include "levels.h"
#include "mesh.h"
#include "png_export.h"
#include "shader.h"
#include "texture.h"

// Character cell footprint in canvas pixels.
static const glm::ivec2 kCellSize(GlyphAtlas::kGlyphWidth, GlyphAtlas::kGlyphHeight);

// Clear color of the scene target, also the bars around the canvas in the window.
static const glm::vec3 kBackground(0.05f, 0.05f, 0.08f);

// The orange test sphere tops out well below full luminance.
static const float kDefaultGain = 1.6f;

enum class View
{
  Ascii,
  Mosaic,
  Scene,
};

// Values match the MODE_* constants in glyph.frag.
enum class ColorMode
{
  Mono,
  Ansi16,
  Ansi256,
  Truecolor,
};

static const char *kColorModeNames[] = {"mono", "ANSI 16", "ANSI 256", "truecolor"};

static const int kDefaultColumns = 120;
static const int kMinColumns = 20;
static const int kMaxColumns = 400;

struct AppState
{
  Texture *image = nullptr;
  bool useImage = false;
  View view = View::Ascii;
  ColorMode colorMode = ColorMode::Truecolor;
  float gain = kDefaultGain;
  float contrast = 1.0f;
  bool autoLevels = false;
  bool exportRequested = false;
  int columns = kDefaultColumns;
  glm::ivec2 grid{0, 0};  // cells across and down, set each frame
};

// Cells across and down for content of the given aspect ratio (width / height).
// Rows follow from the column count and the cell shape; the grid is shrunk if
// the canvas would exceed the largest texture the GPU supports.
static glm::ivec2 canvasGrid(int columns, float aspect, int maxTexture)
{
  int cols = columns;
  int rows = std::max(1, static_cast<int>(std::lround(cols / aspect * kCellSize.x / kCellSize.y)));

  int maxRows = maxTexture / kCellSize.y;
  if (rows > maxRows)
  {
    cols = std::max(1, cols * maxRows / rows);
    rows = maxRows;
  }
  cols = std::min(cols, maxTexture / kCellSize.x);
  return glm::ivec2(cols, rows);
}

static void updateTitle(GLFWwindow *window, const AppState &state)
{
  char title[192];
  snprintf(title, sizeof(title), "OpenGL ASCII Filter - %s %dx%d, %s, gain %.2f, contrast %.2f, auto-levels %s",
           state.useImage ? "image" : "sphere", state.grid.x, state.grid.y,
           kColorModeNames[static_cast<int>(state.colorMode)], state.gain, state.contrast,
           state.autoLevels ? "on" : "off");
  glfwSetWindowTitle(window, title);
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
void drop_callback(GLFWwindow *window, int count, const char **paths);

int main(int argc, char **argv)
{
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow *window = glfwCreateWindow(1280, 720, "OpenGL ASCII Filter", NULL, NULL);
  if (window == NULL)
  {
    std::cout << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return -1;
  }

  glfwMakeContextCurrent(window);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
  {
    std::cout << "Failed to initialize GLAD" << std::endl;
    return -1;
  }

  glfwSetKeyCallback(window, key_callback);
  glfwSetDropCallback(window, drop_callback);

  // Scope so GL objects are destroyed before the context goes away.
  {
    int maxTexture;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTexture);

    // All three are sized to the canvas (the character grid in pixels) at the
    // top of every frame; the window only displays the result.
    Framebuffer sceneFbo(1, 1);
    Framebuffer cellFbo(1, 1, false);
    Framebuffer canvasFbo(1, 1, false);
    // Glyph pixels should stay sharp when the canvas is scaled to the window.
    canvasFbo.setFilter(GL_NEAREST);

    AppState state;
    glfwSetWindowUserPointer(window, &state);

    Shader sceneShader("scene.vert", "scene.frag");
    Shader cellShader("fullscreen.vert", "cells.frag");
    Shader glyphShader("fullscreen.vert", "glyph.frag");
    Shader mosaicShader("fullscreen.vert", "mosaic.frag");
    Shader imageShader("fullscreen.vert", "image.frag");
    Shader blitShader("fullscreen.vert", "blit.frag");
    GlyphAtlas atlas;
    Mesh sphere = Mesh::sphere(32, 64);

    // An image on the command line replaces the sphere as the scene source.
    Texture image;
    if (argc > 1)
    {
      image = Texture(argv[1]);
    }
    state.image = &image;
    state.useImage = image.valid();

    // Core profile requires a bound VAO even for vertex-less draws.
    GLuint emptyVao;
    glGenVertexArrays(1, &emptyVao);

    const glm::vec3 cameraPos(0.0f, 0.0f, 3.0f);

    // Smoothed luminance range, so auto-levels doesn't flicker on a moving scene.
    glm::vec2 levels(0.0f, 1.0f);

    // Binds the glyph shader and its inputs; the caller issues the draw.
    auto setupAscii = [&]()
    {
      glyphShader.use();
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, cellFbo.colorTexture());
      glyphShader.setInt("uCells", 0);
      glActiveTexture(GL_TEXTURE1);
      glBindTexture(GL_TEXTURE_2D, atlas.texture());
      glActiveTexture(GL_TEXTURE0);
      glyphShader.setInt("uAtlas", 1);
      glyphShader.setIVec2("uCellSize", kCellSize);
      glyphShader.setInt("uGlyphCount", atlas.glyphCount());
      glyphShader.setFloat("uGain", state.gain);
      glyphShader.setVec2("uLevels", levels);
      glyphShader.setFloat("uContrast", state.contrast);
      glyphShader.setInt("uColorMode", static_cast<int>(state.colorMode));
    };

    // Draws one of the views into the canvas; the caller has the VAO bound.
    auto renderCanvas = [&](View view)
    {
      canvasFbo.bind();
      glActiveTexture(GL_TEXTURE0);
      if (view == View::Ascii)
      {
        setupAscii();
      }
      else if (view == View::Mosaic)
      {
        mosaicShader.use();
        glBindTexture(GL_TEXTURE_2D, cellFbo.colorTexture());
        mosaicShader.setInt("uCells", 0);
        mosaicShader.setIVec2("uCellSize", kCellSize);
      }
      else
      {
        blitShader.use();
        glBindTexture(GL_TEXTURE_2D, sceneFbo.colorTexture());
        blitShader.setInt("uScene", 0);
      }
      glDrawArrays(GL_TRIANGLES, 0, 3);
    };

    while (!glfwWindowShouldClose(window))
    {
      int windowWidth, windowHeight;
      glfwGetFramebufferSize(window, &windowWidth, &windowHeight);
      if (windowWidth == 0 || windowHeight == 0)
      {
        // Minimized
        glfwWaitEvents();
        continue;
      }

      // Size the canvas: an image sets the aspect, otherwise the window does
      float contentAspect = state.useImage
                                ? static_cast<float>(image.width()) / image.height()
                                : static_cast<float>(windowWidth) / windowHeight;
      glm::ivec2 grid = canvasGrid(state.columns, contentAspect, maxTexture);
      if (grid != state.grid)
      {
        state.grid = grid;
        updateTitle(window, state);
      }
      glm::ivec2 canvasSize = grid * kCellSize;
      sceneFbo.resize(canvasSize.x, canvasSize.y);
      cellFbo.resize(grid.x, grid.y);
      canvasFbo.resize(canvasSize.x, canvasSize.y);

      // Scene pass -> offscreen FBO
      sceneFbo.bind();
      glEnable(GL_DEPTH_TEST);
      glClearColor(kBackground.r, kBackground.g, kBackground.b, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      if (state.useImage)
      {
        glDisable(GL_DEPTH_TEST);
        imageShader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, image.id());
        imageShader.setInt("uImage", 0);
        imageShader.setFloat("uImageAspect", static_cast<float>(image.width()) / image.height());
        // The canvas aspect only differs from the image's by cell rounding, so fill it.
        imageShader.setFloat("uViewAspect", static_cast<float>(image.width()) / image.height());
        imageShader.setVec3("uBackground", kBackground);
        glBindVertexArray(emptyVao);
        glDrawArrays(GL_TRIANGLES, 0, 3);
      }
      else
      {
        float t = static_cast<float>(glfwGetTime());
        float aspect = static_cast<float>(sceneFbo.width()) / sceneFbo.height();

        sceneShader.use();
        sceneShader.setMat4("uModel", glm::rotate(glm::mat4(1.0f), t * 0.5f, glm::vec3(0.0f, 1.0f, 0.0f)));
        sceneShader.setMat4("uView", glm::lookAt(cameraPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f)));
        sceneShader.setMat4("uProjection", glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f));
        sceneShader.setVec3("uLightDir", glm::normalize(glm::vec3(-0.6f, -0.5f, -0.7f)));
        sceneShader.setVec3("uViewPos", cameraPos);
        sceneShader.setVec3("uColor", glm::vec3(0.9f, 0.5f, 0.2f));
        sphere.draw();
      }

      glDisable(GL_DEPTH_TEST);
      glBindVertexArray(emptyVao);

      // Cell pass -> one texel per character cell
      cellFbo.bind();
      cellShader.use();
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, sceneFbo.colorTexture());
      cellShader.setInt("uScene", 0);
      cellShader.setIVec2("uCellSize", kCellSize);
      glDrawArrays(GL_TRIANGLES, 0, 3);

      // Auto-levels: ease toward the measured range, or back to full range when off
      glm::vec2 targetLevels(0.0f, 1.0f);
      if (state.autoLevels)
      {
        estimateLevels(cellFbo, targetLevels.x, targetLevels.y);
      }
      levels = glm::mix(levels, targetLevels, 0.15f);

      // Export: always the ASCII view, at canvas size
      if (state.exportRequested)
      {
        state.exportRequested = false;
        renderCanvas(View::Ascii);

        std::string path = timestampedPngName();
        if (writePng(canvasFbo, path))
        {
          std::cout << "Exported " << path << " (" << canvasSize.x << "x" << canvasSize.y << ")" << std::endl;
        }
      }

      renderCanvas(state.view);

      // Present pass -> window, canvas fitted with bars
      Framebuffer::unbind();
      glViewport(0, 0, windowWidth, windowHeight);
      glClear(GL_COLOR_BUFFER_BIT);

      imageShader.use();
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, canvasFbo.colorTexture());
      imageShader.setInt("uImage", 0);
      imageShader.setFloat("uImageAspect", static_cast<float>(canvasSize.x) / canvasSize.y);
      imageShader.setFloat("uViewAspect", static_cast<float>(windowWidth) / windowHeight);
      imageShader.setVec3("uBackground", kBackground);
      glDrawArrays(GL_TRIANGLES, 0, 3);
      glBindVertexArray(0);

      glfwSwapBuffers(window);
      glfwPollEvents();
    }

    glDeleteVertexArrays(1, &emptyVao);
  }

  glfwTerminate();

  return 0;
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
  if (action == GLFW_RELEASE)
  {
    return;
  }

  auto *state = static_cast<AppState *>(glfwGetWindowUserPointer(window));

  // Gain and contrast adjust on key repeat too, so holding the key sweeps them.
  if (key == GLFW_KEY_UP || key == GLFW_KEY_DOWN)
  {
    float delta = key == GLFW_KEY_UP ? 0.05f : -0.05f;
    state->gain = glm::clamp(state->gain + delta, 0.1f, 5.0f);
    updateTitle(window, *state);
    return;
  }
  if (key == GLFW_KEY_RIGHT || key == GLFW_KEY_LEFT)
  {
    float delta = key == GLFW_KEY_RIGHT ? 0.05f : -0.05f;
    state->contrast = glm::clamp(state->contrast + delta, 0.2f, 4.0f);
    updateTitle(window, *state);
    return;
  }

  if (key == GLFW_KEY_LEFT_BRACKET || key == GLFW_KEY_RIGHT_BRACKET)
  {
    int delta = key == GLFW_KEY_RIGHT_BRACKET ? 10 : -10;
    state->columns = glm::clamp(state->columns + delta, kMinColumns, kMaxColumns);
    return;
  }

  if (action != GLFW_PRESS)
  {
    return;
  }

  if (key == GLFW_KEY_ESCAPE)
  {
    glfwSetWindowShouldClose(window, true);
  }
  else if (key == GLFW_KEY_M)
  {
    state->view = static_cast<View>((static_cast<int>(state->view) + 1) % 3);
  }
  else if (key == GLFW_KEY_S)
  {
    // Only switch to the image when one has been loaded.
    state->useImage = !state->useImage && state->image && state->image->valid();
    updateTitle(window, *state);
  }
  else if (key == GLFW_KEY_E)
  {
    state->exportRequested = true;
  }
  else if (key == GLFW_KEY_A)
  {
    state->autoLevels = !state->autoLevels;
    updateTitle(window, *state);
  }
  else if (key == GLFW_KEY_C)
  {
    state->colorMode = static_cast<ColorMode>((static_cast<int>(state->colorMode) + 1) % 4);
    updateTitle(window, *state);
  }
}

void drop_callback(GLFWwindow *window, int count, const char **paths)
{
  auto *state = static_cast<AppState *>(glfwGetWindowUserPointer(window));
  if (!state || !state->image)
  {
    return;
  }

  // Use the first dropped file that loads as an image.
  for (int i = 0; i < count; ++i)
  {
    Texture loaded(paths[i]);
    if (loaded.valid())
    {
      *state->image = std::move(loaded);
      state->useImage = true;
      updateTitle(window, *state);
      return;
    }
  }
}
