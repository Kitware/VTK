/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkglShaderProgram.h"

#include <GL/glew.h>
#include "vtkglShader.h"
#include "vtkglTexture2D.h"

namespace vtkgl {

namespace {

inline GLenum convertTypeToGL(int type)
{
  switch (type) {
  case VTK_CHAR:
    return GL_BYTE;
  case VTK_UNSIGNED_CHAR:
    return GL_UNSIGNED_BYTE;
  case VTK_SHORT:
    return GL_SHORT;
  case VTK_UNSIGNED_SHORT:
    return GL_UNSIGNED_SHORT;
  case VTK_INT:
    return GL_INT;
  case VTK_UNSIGNED_INT:
    return GL_UNSIGNED_INT;
  case VTK_FLOAT:
    return GL_FLOAT;
  case VTK_DOUBLE:
    return GL_DOUBLE;
  default:
    return 0;
  }
}

inline GLenum lookupTextureUnit(GLint index)
{
#define MAKE_TEXTURE_UNIT_CASE(i) case i: return GL_TEXTURE##i;
  switch (index) {
  MAKE_TEXTURE_UNIT_CASE(0)
  MAKE_TEXTURE_UNIT_CASE(1)
  MAKE_TEXTURE_UNIT_CASE(2)
  MAKE_TEXTURE_UNIT_CASE(3)
  MAKE_TEXTURE_UNIT_CASE(4)
  MAKE_TEXTURE_UNIT_CASE(5)
  MAKE_TEXTURE_UNIT_CASE(6)
  MAKE_TEXTURE_UNIT_CASE(7)
  MAKE_TEXTURE_UNIT_CASE(8)
  MAKE_TEXTURE_UNIT_CASE(9)
  MAKE_TEXTURE_UNIT_CASE(10)
  MAKE_TEXTURE_UNIT_CASE(11)
  MAKE_TEXTURE_UNIT_CASE(12)
  MAKE_TEXTURE_UNIT_CASE(13)
  MAKE_TEXTURE_UNIT_CASE(14)
  MAKE_TEXTURE_UNIT_CASE(15)
  MAKE_TEXTURE_UNIT_CASE(16)
  MAKE_TEXTURE_UNIT_CASE(17)
  MAKE_TEXTURE_UNIT_CASE(18)
  MAKE_TEXTURE_UNIT_CASE(19)
  MAKE_TEXTURE_UNIT_CASE(20)
  MAKE_TEXTURE_UNIT_CASE(21)
  MAKE_TEXTURE_UNIT_CASE(22)
  MAKE_TEXTURE_UNIT_CASE(23)
  MAKE_TEXTURE_UNIT_CASE(24)
  MAKE_TEXTURE_UNIT_CASE(25)
  MAKE_TEXTURE_UNIT_CASE(26)
  MAKE_TEXTURE_UNIT_CASE(27)
  MAKE_TEXTURE_UNIT_CASE(28)
  MAKE_TEXTURE_UNIT_CASE(29)
  MAKE_TEXTURE_UNIT_CASE(30)
  MAKE_TEXTURE_UNIT_CASE(31)
  default:
    return 0;
  }
}
} // end anon namespace

ShaderProgram::ShaderProgram() : m_handle(0), m_vertexShader(0),
  m_fragmentShader(0), m_linked(false)
{
  initializeTextureUnits();
}

ShaderProgram::~ShaderProgram()
{
}

bool ShaderProgram::attachShader(const Shader &shader)
{
  if (shader.handle() == 0) {
    m_error = "Shader object was not initialized, cannot attach it.";
    return false;
  }
  if (shader.type() == Shader::Unknown) {
    m_error = "Shader object is of type Unknown and cannot be used.";
    return false;
  }

  if (m_handle == 0) {
    GLuint handle_ = glCreateProgram();
    if (handle_ == 0) {
      m_error = "Could not create shader program.";
      return false;
    }
    m_handle = static_cast<int>(handle_);
    m_linked = false;
  }

  if (shader.type() == Shader::Vertex) {
    if (m_vertexShader != 0) {
      glDetachShader(static_cast<GLuint>(m_handle),
                     static_cast<GLuint>(m_vertexShader));
      m_vertexShader = shader.handle();
    }
  }
  else if (shader.type() == Shader::Fragment) {
    if (m_fragmentShader != 0) {
      glDetachShader(static_cast<GLuint>(m_handle),
                     static_cast<GLuint>(m_fragmentShader));
      m_fragmentShader = shader.handle();
    }
  }
  else {
    m_error = "Unknown shader type encountered - this should not happen.";
    return false;
  }

  glAttachShader(static_cast<GLuint>(m_handle),
                 static_cast<GLuint>(shader.handle()));
  m_linked = false;
  return true;
}

bool ShaderProgram::detachShader(const Shader &shader)
{
  if (shader.handle() == 0) {
    m_error = "Shader object was not initialized, cannot attach it.";
    return false;
  }
  if (shader.type() == Shader::Unknown) {
    m_error = "Shader object is of type Unknown and cannot be used.";
    return false;
  }
  if (m_handle == 0) {
    m_error = "This shader prorgram has not been initialized yet.";
  }

  switch (shader.type()) {
  case Shader::Vertex:
    if (m_vertexShader != shader.handle()) {
      m_error = "The supplied shader was not attached to this program.";
      return false;
    }
    else {
      glDetachShader(static_cast<GLuint>(m_handle),
                     static_cast<GLuint>(shader.handle()));
      m_vertexShader = 0;
      m_linked = false;
      return true;
    }
  case Shader::Fragment:
    if (m_fragmentShader != shader.handle()) {
      m_error = "The supplied shader was not attached to this program.";
      return false;
    }
    else {
      glDetachShader(static_cast<GLuint>(m_handle),
                     static_cast<GLuint>(shader.handle()));
      m_fragmentShader = 0;
      m_linked = false;
      return true;
    }
  default:
    return false;
  }
}

bool ShaderProgram::link()
{
  if (m_linked)
    return true;

  if (m_handle == 0) {
    m_error = "Program has not been initialized, and/or does not have shaders.";
    return false;
  }

  GLint isCompiled;
  glLinkProgram(static_cast<GLuint>(m_handle));
  glGetProgramiv(static_cast<GLuint>(m_handle), GL_LINK_STATUS, &isCompiled);
  if (isCompiled == 0) {
    GLint length(0);
    glGetShaderiv(static_cast<GLuint>(m_handle), GL_INFO_LOG_LENGTH, &length);
    if (length > 1) {
      char *logMessage = new char[length];
      glGetShaderInfoLog(static_cast<GLuint>(m_handle), length, NULL, logMessage);
      m_error = logMessage;
      delete[] logMessage;
    }
    return false;
  }
  m_linked = true;
  m_attributes.clear();
  return true;
}

bool ShaderProgram::bind()
{
  if (!m_linked && !link())
    return false;

  glUseProgram(static_cast<GLuint>(m_handle));
  return true;
}

void ShaderProgram::release()
{
  glUseProgram(0);
  releaseAllTextureUnits();
}

bool ShaderProgram::enableAttributeArray(const std::string &name)
{
  GLint location = static_cast<GLint>(findAttributeArray(name));
  if (location == -1) {
    m_error = "Could not enable attribute " + name + ". No such attribute.";
    return false;
  }
  glEnableVertexAttribArray(location);
  return true;
}

bool ShaderProgram::disableAttributeArray(const std::string &name)
{
  GLint location = static_cast<GLint>(findAttributeArray(name));
  if (location == -1) {
    m_error = "Could not disable attribute " + name + ". No such attribute.";
    return false;
  }
  glDisableVertexAttribArray(location);
  return true;
}

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

bool ShaderProgram::useAttributeArray(const std::string &name, int offset,
                                      size_t stride, int elementType,
                                      int elementTupleSize,
                                      NormalizeOption normalize)
{
  GLint location = static_cast<GLint>(findAttributeArray(name));
  if (location == -1) {
    m_error = "Could not use attribute " + name + ". No such attribute.";
    return false;
  }
  glVertexAttribPointer(location, elementTupleSize, convertTypeToGL(elementType),
                        normalize == Normalize ? GL_TRUE : GL_FALSE,
                        static_cast<GLsizei>(stride), BUFFER_OFFSET(offset));
  return true;
}

bool ShaderProgram::setTextureSampler(const std::string &name,
                                      const Texture2D &texture)
{
  // Look up sampler location:
  GLint location = static_cast<GLint>(findUniform(name));
  if (location == -1) {
    m_error = "Could not set sampler " + name + ". No uniform with that name.";
    return false;
  }

  // Check if the texture is already bound:
  GLint textureUnitId = 0;
  typedef std::map<const Texture2D*, int>::const_iterator TMapIter;
  TMapIter result = m_textureUnitBindings.find(&texture);
  if (result == m_textureUnitBindings.end()) {
    // Not bound. Attempt to bind the texture to an available texture unit.
    // We'll leave GL_TEXTURE0 unbound, as it is used for manipulating
    // textures.
    std::vector<bool>::iterator begin = m_boundTextureUnits.begin() + 1;
    std::vector<bool>::iterator end = m_boundTextureUnits.end();
    std::vector<bool>::iterator available = std::find(begin, end, false);

    if (available == end) {
      m_error = "Could not set sampler " + name + ". No remaining texture "
          "units available.";
      return false;
    }

    textureUnitId = static_cast<GLint>(available - begin);

    GLenum textureUnit = lookupTextureUnit(textureUnitId);
    if (textureUnit == 0) {
      m_error = "Could not set sampler " + name
          + ". Texture unit lookup failed.";
      return false;
    }

    glActiveTexture(textureUnit);
    if (!texture.bind()) {
      m_error = "Could not set sampler " + name + ": Error while binding "
          "texture: '" + texture.error() + "'.";
      glActiveTexture(GL_TEXTURE0);
      return false;
    }
    glActiveTexture(GL_TEXTURE0);

    // Mark texture unit as in-use.
    m_textureUnitBindings.insert(std::make_pair(&texture, textureUnitId));
    *available = true;
  }
  else {
    // Texture is already bound.
    textureUnitId = result->second;
  }

  // Set the texture unit uniform
  glUniform1i(location, textureUnitId);

  return true;
}

bool ShaderProgram::setUniformValue(const std::string &name, int i)
{
  GLint location = static_cast<GLint>(findUniform(name));
  if (location == -1) {
    m_error = "Could not set uniform " + name + ". No such uniform.";
    return false;
  }
  glUniform1i(location, static_cast<GLint>(i));
  return true;
}

bool ShaderProgram::setUniformValue(const std::string &name, float f)
{
  GLint location = static_cast<GLint>(findUniform(name));
  if (location == -1) {
    m_error = "Could not set uniform " + name + ". No such uniform.";
    return false;
  }
  glUniform1f(location, static_cast<GLfloat>(f));
  return true;
}

bool ShaderProgram::setUniformValue(const std::string &name,
                                    const Matrix3f &matrix)
{
  GLint location = static_cast<GLint>(findUniform(name));
  if (location == -1) {
    m_error = "Could not set uniform " + name + ". No such uniform.";
    return false;
  }
  glUniformMatrix3fv(location, 1, GL_FALSE,
                     static_cast<const GLfloat *>(matrix.data()));
  return true;
}

bool ShaderProgram::setUniformValue(const std::string &name,
                                    const Matrix4f &matrix)
{
  GLint location = static_cast<GLint>(findUniform(name));
  if (location == -1) {
    m_error = "Could not set uniform " + name + ". No such uniform.";
    return false;
  }
  glUniformMatrix4fv(location, 1, GL_FALSE,
                     static_cast<const GLfloat *>(matrix.data()));
  return true;
}

bool ShaderProgram::setUniformValue(const std::string &name, const int count,
                                    const float (*v)[3])
{
  GLint location = static_cast<GLint>(findUniform(name));
  if (location == -1) {
    m_error = "Could not set uniform " + name + ". No such uniform.";
    return false;
  }
  glUniform3fv(location, count, (const GLfloat *)v);
  return true;
}

bool ShaderProgram::setUniformValue(const std::string &name, const Vector3f &v)
{
  GLint location = static_cast<GLint>(findUniform(name));
  if (location == -1) {
    m_error = "Could not set uniform " + name + ". No such uniform.";
    return false;
  }
  glUniform3fv(location, 1, v.data());
  return true;
}

bool ShaderProgram::setUniformValue(const std::string &name, const Vector2i &v)
{
  GLint location = static_cast<GLint>(findUniform(name));
  if (location == -1) {
    m_error = "Could not set uniform " + name + ". No such uniform.";
    return false;
  }
  glUniform2iv(location, 1, v.data());
  return true;
}

bool ShaderProgram::setUniformValue(const std::string &name,
                                    const Vector3ub &v)
{
  GLint location = static_cast<GLint>(findUniform(name));
  if (location == -1) {
    m_error = "Could not set uniform " + name + ". No such uniform.";
    return false;
  }
  vtkColor3f colorf(v[0] / 255.0f, v[1] / 255.0f, v[2] / 255.0f);
  glUniform3fv(location, 1, colorf.GetData());
  return true;
}

bool ShaderProgram::setAttributeArrayInternal(
    const std::string &name, void *buffer, int type, int tupleSize,
    ShaderProgram::NormalizeOption normalize)
{
  if (type == -1) {
    m_error = "Unrecognized data type for attribute " + name + ".";
    return false;
  }
  GLint location = static_cast<GLint>(findAttributeArray(name));
  if (location == -1) {
    m_error = "Could not set attribute " + name + ". No such attribute.";
    return false;
  }
  const GLvoid *data = static_cast<const GLvoid *>(buffer);
  glVertexAttribPointer(location, tupleSize, convertTypeToGL(type),
                        normalize == Normalize ? GL_TRUE : GL_FALSE, 0, data);
  return true;
}

void ShaderProgram::initializeTextureUnits()
{
  GLint numTextureUnits;
  glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &numTextureUnits);

  // We'll impose a a hard limit of 32 texture units for symbolic lookups.
  // This seems to be about the maximum available on current hardware.
  // If increasing this limit, modify the lookupTextureUnit method
  // appropriately.
  numTextureUnits = std::min(std::max(numTextureUnits, 0), 32);

  m_boundTextureUnits.clear();
  m_boundTextureUnits.resize(numTextureUnits, false);
  m_textureUnitBindings.clear();
}

void ShaderProgram::releaseAllTextureUnits()
{
  std::fill(m_boundTextureUnits.begin(), m_boundTextureUnits.end(), false);
  m_textureUnitBindings.clear();
}

inline int ShaderProgram::findAttributeArray(const std::string &name)
{
  if (name.empty() || !m_linked)
    return -1;
  const GLchar *namePtr = static_cast<const GLchar *>(name.c_str());
  GLint location =
      static_cast<int>(glGetAttribLocation(static_cast<GLuint>(m_handle),
                                           namePtr));
  if (location == -1) {
    m_error = "Specified attribute not found in current shader program: ";
    m_error += name;
  }

  return location;
}

inline int ShaderProgram::findUniform(const std::string &name)
{
  if (name.empty() || !m_linked)
    return -1;
  const GLchar *namePtr = static_cast<const GLchar *>(name.c_str());
  GLint location =
      static_cast<int>(glGetUniformLocation(static_cast<GLuint>(m_handle),
                                            namePtr));
  if (location == -1)
    m_error = "Uniform " + name + " not found in current shader program.";

  return location;
}

}
