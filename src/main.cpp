#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <string>
#include <utility>
#include <vector>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "ascii_filter.h"
#include "framebuffer.h"
#include "gif_export.h"
#include "mesh.h"
#include "png_export.h"
#include "shader.h"
#include "stained_glass_filter.h"
#include "texture.h"

// Clear color of the scene target, also the bars around the canvas in the window.
static const glm::vec3 kBackground(0.05f, 0.05f, 0.08f);

struct AppState
{
  Texture *image = nullptr;
  bool useImage = false;
  std::vector<Filter *> filters;
  size_t activeFilter = 0;
  Filter *filter() const { return filters[activeFilter]; }
  bool exportRequested = false;
  bool gifExportRequested = false;
  glm::ivec2 canvasSize{0, 0};  // set each frame
  double animTime = 0.0;        // seconds into an animated image
  bool playing = true;
};

static void updateTitle(GLFWwindow *window, const AppState &state)
{
  char source[64] = "sphere";
  if (state.useImage)
  {
    const Texture &image = *state.image;
    if (!image.animated())
    {
      snprintf(source, sizeof(source), "image");
    }
    else if (state.playing)
    {
      snprintf(source, sizeof(source), "gif, %d frames", image.frameCount());
    }
    else
    {
      snprintf(source, sizeof(source), "gif paused, frame %d/%d", image.frameAt(state.animTime) + 1, image.frameCount());
    }
  }

  std::string title = std::string("OpenGL Filter - ") + state.filter()->name() + " - " + source + " " +
                      state.filter()->status();
  glfwSetWindowTitle(window, title.c_str());
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
void drop_callback(GLFWwindow *window, int count, const char **paths);

int main(int argc, char **argv)
{
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow *window = glfwCreateWindow(1280, 720, "OpenGL Filter", NULL, NULL);
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

    // Both are sized to the filter's canvas at the top of every frame; the
    // window only displays the result.
    Framebuffer sceneFbo(1, 1);
    Framebuffer canvasFbo(1, 1, false);
    // Filter pixels should stay sharp when the canvas is scaled to the window.
    canvasFbo.setFilter(GL_NEAREST);

    AsciiFilter ascii;
    StainedGlassFilter stainedGlass;

    AppState state;
    state.filters = {&ascii, &stainedGlass};
    glfwSetWindowUserPointer(window, &state);

    Shader sceneShader("scene.vert", "scene.frag");
    Shader imageShader("fullscreen.vert", "image.frag");
    Shader sourceShader("fullscreen.vert", "source.frag");
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

    // Source pass and the filter's own prepare pass for one frame: draws the
    // sphere, or the given layer of the image, then lets the filter reduce it.
    auto renderFrame = [&](int layer)
    {
      // Source pass -> offscreen FBO
      sceneFbo.bind();
      glEnable(GL_DEPTH_TEST);
      glClearColor(kBackground.r, kBackground.g, kBackground.b, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      if (state.useImage)
      {
        glDisable(GL_DEPTH_TEST);
        sourceShader.use();
        glActiveTexture(GL_TEXTURE0);
        // Frames are layers of an array texture; stills have just one.
        glBindTexture(GL_TEXTURE_2D_ARRAY, image.id());
        sourceShader.setInt("uSource", 0);
        sourceShader.setInt("uLayer", layer);
        sourceShader.setVec3("uBackground", kBackground);
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

      state.filter()->prepare(sceneFbo.colorTexture(), state.canvasSize);
    };

    double lastTime = glfwGetTime();

    while (!glfwWindowShouldClose(window))
    {
      double now = glfwGetTime();
      if (state.playing && state.useImage && image.animated())
      {
        state.animTime += now - lastTime;
      }
      lastTime = now;

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
      glm::ivec2 canvasSize = state.filter()->canvasSize(contentAspect, maxTexture);
      if (canvasSize != state.canvasSize)
      {
        state.canvasSize = canvasSize;
        updateTitle(window, state);
      }
      sceneFbo.resize(canvasSize.x, canvasSize.y);
      canvasFbo.resize(canvasSize.x, canvasSize.y);

      renderFrame(state.useImage ? image.frameAt(state.animTime) : 0);

      // Export: the filter's export view, at canvas size
      if (state.exportRequested)
      {
        state.exportRequested = false;
        state.filter()->warnExport(canvasSize);
        state.filter()->renderExport(canvasFbo, sceneFbo.colorTexture());

        std::string path = timestampedPngName();
        if (writePng(canvasFbo, path))
        {
          std::cout << "Exported " << path << " (" << canvasSize.x << "x" << canvasSize.y << ")" << std::endl;
        }
      }

      state.filter()->render(canvasFbo, sceneFbo.colorTexture());

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

      // GIF export: replay every frame through the whole pipeline. It re-renders
      // the scene and filter targets, so it runs after this frame is on screen.
      if (state.gifExportRequested)
      {
        state.gifExportRequested = false;
        if (!state.useImage || !image.animated())
        {
          std::cout << "GIF export needs an animated image (E exports a PNG)" << std::endl;
        }
        else
        {
          state.filter()->warnExport(canvasSize);
          std::string path = timestampedGifName();
          GifExporter gif(path, canvasSize.x, canvasSize.y);
          if (!gif.ok())
          {
            std::cout << "Failed to write " << path << std::endl;
          }
          else
          {
            state.filter()->beginExport();
            const int count = image.frameCount();
            bool failed = false;
            for (int frame = 0; frame < count && !failed; ++frame)
            {
              renderFrame(frame);
              state.filter()->renderExport(canvasFbo, sceneFbo.colorTexture());
              // Viewers ignore delays under 2 hundredths of a second.
              int delay = std::max(2, static_cast<int>(std::lround(image.frameDuration(frame) * 100.0)));
              failed = !gif.addFrame(canvasFbo, delay);

              std::cout << "\rExporting GIF: frame " << frame + 1 << "/" << count << std::flush;
            }
            gif.finish();
            state.filter()->endExport();

            std::cout << (failed ? "\nExport failed: " : "\nExported ") << path << " (" << canvasSize.x << "x"
                      << canvasSize.y << ", " << count << " frames)" << std::endl;
          }
        }
      }
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

  if (action == GLFW_PRESS)
  {
    if (key == GLFW_KEY_ESCAPE)
    {
      glfwSetWindowShouldClose(window, true);
      return;
    }
    else if (key == GLFW_KEY_S)
    {
      // Only switch to the image when one has been loaded.
      state->useImage = !state->useImage && state->image && state->image->valid();
      updateTitle(window, *state);
      return;
    }
    else if (key == GLFW_KEY_SPACE)
    {
      state->playing = !state->playing;
      updateTitle(window, *state);
      return;
    }
    else if (key == GLFW_KEY_COMMA || key == GLFW_KEY_PERIOD)
    {
      // Stepping pauses, so the frame stays put.
      if (state->useImage && state->image->animated())
      {
        int count = state->image->frameCount();
        int frame = state->image->frameAt(state->animTime);
        frame = (frame + (key == GLFW_KEY_PERIOD ? 1 : count - 1)) % count;
        state->animTime = state->image->frameStart(frame);
        state->playing = false;
        updateTitle(window, *state);
      }
      return;
    }
    else if (key == GLFW_KEY_TAB)
    {
      state->activeFilter = (state->activeFilter + 1) % state->filters.size();
      // The title is refreshed next frame, once the new filter has sized itself.
      state->canvasSize = glm::ivec2(0, 0);
      return;
    }
    else if (key == GLFW_KEY_E)
    {
      state->exportRequested = true;
      return;
    }
    else if (key == GLFW_KEY_G)
    {
      state->gifExportRequested = true;
      return;
    }
  }

  // Everything else belongs to the active filter.
  if (state->filter()->onKey(key, action))
  {
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
      state->animTime = 0.0;
      state->playing = true;
      updateTitle(window, *state);
      return;
    }
  }
}
