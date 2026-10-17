cmake_minimum_required(VERSION 3.15)
project(AsciiProject CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(external/glfw)

add_executable(opengl_ascii
  src/main.cpp
  src/shader.cpp
  src/mesh.cpp
  src/framebuffer.cpp
  src/glyph_atlas.cpp
  src/levels.cpp
  src/texture.cpp
)

# GLAD
target_include_directories(opengl_ascii PRIVATE external/glad/include)

target_sources(opengl_ascii PRIVATE
  external/glad/src/glad.c
)

# GLM (header-only)
target_include_directories(opengl_ascii PRIVATE external/glm)

# stb (header-only image loading/writing)
target_include_directories(opengl_ascii PRIVATE external/stb)

# Shaders are read from the source tree at runtime
target_compile_definitions(opengl_ascii PRIVATE
  SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/shaders/"
)

# GLFW
target_link_libraries(opengl_ascii PRIVATE
  glfw
)
