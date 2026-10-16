#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "framebuffer.h"
#include "mesh.h"
#include "shader.h"

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void processInput(GLFWwindow *window);

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

  // Scope so GL objects are destroyed before the context goes away.
  {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    Framebuffer sceneFbo(width, height);
    glfwSetWindowUserPointer(window, &sceneFbo);

    Shader sceneShader("scene.vert", "scene.frag");
    Shader blitShader("fullscreen.vert", "blit.frag");
    Mesh sphere = Mesh::sphere(32, 64);

    // Core profile requires a bound VAO even for vertex-less draws.
    GLuint emptyVao;
    glGenVertexArrays(1, &emptyVao);

    const glm::vec3 cameraPos(0.0f, 0.0f, 3.0f);

    while (!glfwWindowShouldClose(window))
    {
      processInput(window);

      // Skip rendering while minimized.
      if (sceneFbo.width() == 0 || sceneFbo.height() == 0)
      {
        glfwWaitEvents();
        continue;
      }

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

      // Present pass -> window
      Framebuffer::unbind();
      glViewport(0, 0, sceneFbo.width(), sceneFbo.height());
      glDisable(GL_DEPTH_TEST);
      glClear(GL_COLOR_BUFFER_BIT);

      blitShader.use();
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, sceneFbo.colorTexture());
      blitShader.setInt("uScene", 0);
      glBindVertexArray(emptyVao);
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
  auto *fbo = static_cast<Framebuffer *>(glfwGetWindowUserPointer(window));
  if (fbo && width > 0 && height > 0)
  {
    fbo->resize(width, height);
  }
}

void processInput(GLFWwindow *window)
{
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
  {
    glfwSetWindowShouldClose(window, true);
  }
}
