#include <cstdio>
#include <iostream>
#include <utility>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "framebuffer.h"
#include "glyph_atlas.h"
#include "levels.h"
#include "mesh.h"
#include "shader.h"
#include "texture.h"

// Character cell footprint in window pixels.
static const glm::ivec2 kCellSize(GlyphAtlas::kGlyphWidth, GlyphAtlas::kGlyphHeight);

// Clear color of the scene target, also the bars around a letterboxed image.
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

struct AppState
{
  Framebuffer *scene = nullptr;
  Framebuffer *cells = nullptr;
  Texture *image = nullptr;
  bool useImage = false;
  View view = View::Ascii;
  ColorMode colorMode = ColorMode::Truecolor;
  float gain = kDefaultGain;
  float contrast = 1.0f;
  bool autoLevels = false;
};

static glm::ivec2 gridSize(int width, int height)
{
  return glm::ivec2((width + kCellSize.x - 1) / kCellSize.x,
                    (height + kCellSize.y - 1) / kCellSize.y);
}

static void updateTitle(GLFWwindow *window, const AppState &state)
{
  char title[128];
  snprintf(title, sizeof(title), "OpenGL ASCII Filter - %s, %s, gain %.2f, contrast %.2f, auto-levels %s",
           state.useImage ? "image" : "sphere",
           kColorModeNames[static_cast<int>(state.colorMode)], state.gain, state.contrast,
           state.autoLevels ? "on" : "off");
  glfwSetWindowTitle(window, title);
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
void drop_callback(GLFWwindow *window, int count, const char **paths);

int main(int argc, char **argv)
{
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow *window = glfwCreateWindow(800, 600, "OpenGL ASCII Filter", NULL, NULL);
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

  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
  glfwSetKeyCallback(window, key_callback);
  glfwSetDropCallback(window, drop_callback);

  // Scope so GL objects are destroyed before the context goes away.
  {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    glm::ivec2 grid = gridSize(width, height);
    Framebuffer sceneFbo(width, height);
    Framebuffer cellFbo(grid.x, grid.y, false);

    AppState state;
    state.scene = &sceneFbo;
    state.cells = &cellFbo;
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
    updateTitle(window, state);

    // Core profile requires a bound VAO even for vertex-less draws.
    GLuint emptyVao;
    glGenVertexArrays(1, &emptyVao);

    const glm::vec3 cameraPos(0.0f, 0.0f, 3.0f);

    // Smoothed luminance range, so auto-levels doesn't flicker on a moving scene.
    glm::vec2 levels(0.0f, 1.0f);

    while (!glfwWindowShouldClose(window))
    {
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
        imageShader.setFloat("uViewAspect", static_cast<float>(sceneFbo.width()) / sceneFbo.height());
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

      // Present pass -> window
      Framebuffer::unbind();
      glViewport(0, 0, sceneFbo.width(), sceneFbo.height());
      glClear(GL_COLOR_BUFFER_BIT);

      if (state.view == View::Ascii)
      {
        glyphShader.use();
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
      }
      else if (state.view == View::Mosaic)
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
      glBindVertexArray(0);

      glfwSwapBuffers(window);
      glfwPollEvents();
    }

    glDeleteVertexArrays(1, &emptyVao);
  }

  glfwTerminate();

  return 0;
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
  auto *state = static_cast<AppState *>(glfwGetWindowUserPointer(window));
  if (state && width > 0 && height > 0)
  {
    glm::ivec2 grid = gridSize(width, height);
    state->scene->resize(width, height);
    state->cells->resize(grid.x, grid.y);
  }
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
