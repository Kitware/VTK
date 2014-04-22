/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkglShader.h"

#include <GL/glew.h>

namespace vtkgl {

Shader::Shader(Type type_, const std::string &source_)
  : m_type(type_), m_handle(0), m_dirty(true), m_source(source_)
{
}

Shader::~Shader()
{
}

void Shader::setType(Type type_)
{
  m_type = type_;
  m_dirty = true;
}

void Shader::setSource(const std::string &source_)
{
  m_source = source_;
  m_dirty = true;
}

bool Shader::compile()
{
  if (m_source.empty() || m_type == Unknown || !m_dirty)
    return false;

  // Ensure we delete the previous shader if necessary.
  if (m_handle != 0) {
    glDeleteShader(static_cast<GLuint>(m_handle));
    m_handle = 0;
  }

  GLenum type_ = m_type == Vertex ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER;
  GLuint handle_ = glCreateShader(type_);
  const GLchar *source_ = static_cast<const GLchar *>(m_source.c_str());
  glShaderSource(handle_, 1, &source_, NULL);
  glCompileShader(handle_);
  GLint isCompiled;
  glGetShaderiv(handle_, GL_COMPILE_STATUS, &isCompiled);

  // Handle shader compilation failures.
  if (!isCompiled) {
    GLint length(0);
    glGetShaderiv(handle_, GL_INFO_LOG_LENGTH, &length);
    if (length > 1) {
      char *logMessage = new char[length];
      glGetShaderInfoLog(handle_, length, NULL, logMessage);
      m_error = logMessage;
      delete[] logMessage;
    }
    glDeleteShader(handle_);
    return false;
  }

  // The shader compiled, store its handle and return success.
  m_handle = static_cast<int>(handle_);
  m_dirty = false;

  return true;
}

void Shader::cleanup()
{
  if (m_type == Unknown || m_handle == 0)
    return;

  glDeleteShader(static_cast<GLuint>(m_handle));
  m_handle = 0;
  m_dirty = false;
}

}
