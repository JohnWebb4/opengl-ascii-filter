#include "framebuffer.h"

#include <iostream>

Framebuffer::Framebuffer(int width, int height, bool withDepth)
    : width_(width), height_(height), withDepth_(withDepth)
{
  glGenFramebuffers(1, &fbo_);
  glGenTextures(1, &color_);
  glGenRenderbuffers(1, &depth_);
  allocate();
}

Framebuffer::~Framebuffer()
{
  glDeleteRenderbuffers(1, &depth_);
  glDeleteTextures(1, &color_);
  glDeleteFramebuffers(1, &fbo_);
}

void Framebuffer::allocate()
{
  glBindTexture(GL_TEXTURE_2D, color_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width_, height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  if (withDepth_)
  {
    glBindRenderbuffer(GL_RENDERBUFFER, depth_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width_, height_);
  }

  glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_, 0);
  if (withDepth_)
  {
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_);
  }

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
  {
    std::cout << "Framebuffer is not complete" << std::endl;
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::resize(int width, int height)
{
  if (width == width_ && height == height_)
  {
    return;
  }
  width_ = width;
  height_ = height;
  allocate();
}

void Framebuffer::setFilter(GLenum filter)
{
  glBindTexture(GL_TEXTURE_2D, color_);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
}

void Framebuffer::bind() const
{
  glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
  glViewport(0, 0, width_, height_);
}

void Framebuffer::unbind()
{
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
