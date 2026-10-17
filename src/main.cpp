#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "framebuffer.h"
#include "mesh.h"
#include "shader.h"

// Character cell footprint in window pixels.
static const glm::ivec2 kCellSize(8, 16);

struct AppState
{
  Framebuffer *scene = nullptr;
  Framebuffer *cells = nullptr;
  bool showCells = true;
};

static glm::ivec2 gridSize(int width, int height)
{
  return glm::ivec2((width + kCellSize.x - 1) / kCellSize.x,
                    (height + kCellSize.y - 1) / kCellSize.y);
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

int main(void)
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
    Shader mosaicShader("fullscreen.vert", "mosaic.frag");
    Shader blitShader("fullscreen.vert", "blit.frag");
    Mesh sphere = Mesh::sphere(32, 64);

    // Core profile requires a bound VAO even for vertex-less draws.
    GLuint emptyVao;
    glGenVertexArrays(1, &emptyVao);

    const glm::vec3 cameraPos(0.0f, 0.0f, 3.0f);

    while (!glfwWindowShouldClose(window))
    {
      // Scene pass -> offscreen FBO
      sceneFbo.bind();
      glEnable(GL_DEPTH_TEST);
      glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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

      // Present pass -> window
      Framebuffer::unbind();
      glViewport(0, 0, sceneFbo.width(), sceneFbo.height());
      glClear(GL_COLOR_BUFFER_BIT);

      if (state.showCells)
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
    auto *state = static_cast<AppState *>(glfwGetWindowUserPointer(window));
    state->showCells = !state->showCells;
  }
}
