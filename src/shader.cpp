#include "shader.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <glm/gtc/type_ptr.hpp>

static std::string readFile(const std::string &name)
{
  std::ifstream file(std::string(SHADER_DIR) + name);
  if (!file)
  {
    std::cout << "Failed to open shader: " << name << std::endl;
    return "";
  }
  std::stringstream ss;
  ss << file.rdbuf();
  return ss.str();
}

static GLuint compile(GLenum type, const std::string &name)
{
  std::string source = readFile(name);
  const char *src = source.c_str();

  GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, &src, NULL);
  glCompileShader(shader);

  GLint ok = 0;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
  if (!ok)
  {
    char log[1024];
    glGetShaderInfoLog(shader, sizeof(log), NULL, log);
    std::cout << "Shader compile error (" << name << "):\n" << log << std::endl;
  }
  return shader;
}

Shader::Shader(const std::string &vertexFile, const std::string &fragmentFile)
{
  GLuint vs = compile(GL_VERTEX_SHADER, vertexFile);
  GLuint fs = compile(GL_FRAGMENT_SHADER, fragmentFile);

  id_ = glCreateProgram();
  glAttachShader(id_, vs);
  glAttachShader(id_, fs);
  glLinkProgram(id_);

  GLint ok = 0;
  glGetProgramiv(id_, GL_LINK_STATUS, &ok);
  if (!ok)
  {
    char log[1024];
    glGetProgramInfoLog(id_, sizeof(log), NULL, log);
    std::cout << "Program link error:\n" << log << std::endl;
  }

  glDeleteShader(vs);
  glDeleteShader(fs);
}

Shader::~Shader()
{
  glDeleteProgram(id_);
}

void Shader::use() const
{
  glUseProgram(id_);
}

void Shader::setInt(const char *name, int value) const
{
  glUniform1i(glGetUniformLocation(id_, name), value);
}

void Shader::setMat4(const char *name, const glm::mat4 &value) const
{
  glUniformMatrix4fv(glGetUniformLocation(id_, name), 1, GL_FALSE, glm::value_ptr(value));
}

void Shader::setVec3(const char *name, const glm::vec3 &value) const
{
  glUniform3fv(glGetUniformLocation(id_, name), 1, glm::value_ptr(value));
}
