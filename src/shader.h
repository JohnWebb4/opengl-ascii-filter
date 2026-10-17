#pragma once

#include <string>
#include <glad/glad.h>
#include <glm/glm.hpp>

class Shader
{
public:
  // Loads, compiles and links the given files from SHADER_DIR.
  Shader(const std::string &vertexFile, const std::string &fragmentFile);
  ~Shader();

  Shader(const Shader &) = delete;
  Shader &operator=(const Shader &) = delete;

  void use() const;
  void setInt(const char *name, int value) const;
  void setIVec2(const char *name, const glm::ivec2 &value) const;
  void setFloat(const char *name, float value) const;
  void setMat4(const char *name, const glm::mat4 &value) const;
  void setVec2(const char *name, const glm::vec2 &value) const;
  void setVec3(const char *name, const glm::vec3 &value) const;

private:
  GLuint id_ = 0;
};
