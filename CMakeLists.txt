cmake_minimum_required(VERSION 3.15)
project(AsciiProject CXX)

add_subdirectory(external/glfw)

add_executable(opengl_ascii src/main.cpp)

# GLAD
target_include_directories(opengl_ascii PRIVATE external/glad/include)

target_sources(opengl_ascii PRIVATE
  external/glad/src/glad.c
)

# GLFW
target_link_libraries(opengl_ascii PRIVATE
  glfw
)
