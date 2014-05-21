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
#include "vtkMatrix3x3.h"
#include "vtkMatrix4x4.h"

namespace vtkgl {

namespace {

inline GLenum convertTypeToGL(int type)
{
  switch (type)
    {
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

inline GLenum LookupTextureUnit(GLint index)
{
#define MAKE_TEXTURE_UNIT_CASE(i) case i: return GL_TEXTURE##i;
  switch (index)
    {
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

ShaderProgram::ShaderProgram() : Handle(0), VertexShaderHandle(0),
  FragmentShaderHandle(0), Linked(false), Bound(false)
{
  this->InitializeTextureUnits();
}

ShaderProgram::~ShaderProgram()
{
}

bool ShaderProgram::AttachShader(const Shader &shader)
{
  if (shader.GetHandle() == 0)
    {
    this->Error = "Shader object was not initialized, cannot attach it.";
    return false;
    }
  if (shader.GetType() == Shader::Unknown)
    {
    this->Error = "Shader object is of type Unknown and cannot be used.";
    return false;
    }

  if (Handle == 0)
    {
    GLuint handle_ = glCreateProgram();
    if (handle_ == 0)
      {
      this->Error = "Could not create shader program.";
      return false;
      }
    this->Handle = static_cast<int>(handle_);
    this->Linked = false;
    }

  if (shader.GetType() == Shader::Vertex)
    {
    if (VertexShaderHandle != 0)
      {
      glDetachShader(static_cast<GLuint>(Handle),
                     static_cast<GLuint>(VertexShaderHandle));
      }
    this->VertexShaderHandle = shader.GetHandle();
    }
  else if (shader.GetType() == Shader::Fragment)
    {
    if (FragmentShaderHandle != 0)
      {
      glDetachShader(static_cast<GLuint>(Handle),
                     static_cast<GLuint>(FragmentShaderHandle));
      }
    this->FragmentShaderHandle = shader.GetHandle();
    }
  else
    {
    this->Error = "Unknown shader type encountered - this should not happen.";
    return false;
  }

  glAttachShader(static_cast<GLuint>(this->Handle),
                 static_cast<GLuint>(shader.GetHandle()));
  this->Linked = false;
  return true;
}

bool ShaderProgram::DetachShader(const Shader &shader)
{
  if (shader.GetHandle() == 0)
    {
    this->Error = "Shader object was not initialized, cannot attach it.";
    return false;
    }
  if (shader.GetType() == Shader::Unknown)
    {
    this->Error = "Shader object is of type Unknown and cannot be used.";
    return false;
    }
  if (this->Handle == 0)
    {
    this->Error = "This shader prorgram has not been initialized yet.";
    }

  switch (shader.GetType())
    {
    case Shader::Vertex:
      if (this->VertexShaderHandle != shader.GetHandle())
        {
        Error = "The supplied shader was not attached to this program.";
        return false;
        }
      else
        {
        glDetachShader(static_cast<GLuint>(this->Handle),
                       static_cast<GLuint>(shader.GetHandle()));
        this->VertexShaderHandle = 0;
        this->Linked = false;
        return true;
        }
    case Shader::Fragment:
      if (FragmentShaderHandle != shader.GetHandle())
        {
        this->Error = "The supplied shader was not attached to this program.";
        return false;
        }
      else
        {
        glDetachShader(static_cast<GLuint>(this->Handle),
                       static_cast<GLuint>(shader.GetHandle()));
        this->FragmentShaderHandle = 0;
        this->Linked = false;
        return true;
        }
    default:
      return false;
    }
}

bool ShaderProgram::Link()
{
  if (this->Linked)
    {
    return true;
    }

  if (this->Handle == 0)
    {
    this->Error = "Program has not been initialized, and/or does not have shaders.";
    return false;
    }

  GLint isCompiled;
  glLinkProgram(static_cast<GLuint>(this->Handle));
  glGetProgramiv(static_cast<GLuint>(this->Handle), GL_LINK_STATUS, &isCompiled);
  if (isCompiled == 0)
    {
    GLint length(0);
    glGetShaderiv(static_cast<GLuint>(this->Handle), GL_INFO_LOG_LENGTH, &length);
    if (length > 1)
      {
      char *logMessage = new char[length];
      glGetShaderInfoLog(static_cast<GLuint>(this->Handle), length, NULL, logMessage);
      this->Error = logMessage;
      delete[] logMessage;
      }
    return false;
    }
  this->Linked = true;
  this->Attributes.clear();
  return true;
}

bool ShaderProgram::Bind()
{
  if (!this->Linked && !this->Link())
    {
    return false;
    }

  glUseProgram(static_cast<GLuint>(this->Handle));
  this->Bound = true;
  return true;
}

void ShaderProgram::Release()
{
  glUseProgram(0);
  this->Bound = false;
  ReleaseAllTextureUnits();
}

bool ShaderProgram::EnableAttributeArray(const std::string &name)
{
  GLint location = static_cast<GLint>(this->FindAttributeArray(name));
  if (location == -1)
    {
    this->Error = "Could not enable attribute " + name + ". No such attribute.";
    return false;
    }
  glEnableVertexAttribArray(location);
  return true;
}

bool ShaderProgram::DisableAttributeArray(const std::string &name)
{
  GLint location = static_cast<GLint>(this->FindAttributeArray(name));
  if (location == -1)
    {
    this->Error = "Could not disable attribute " + name + ". No such attribute.";
    return false;
    }
  glDisableVertexAttribArray(location);
  return true;
}

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

bool ShaderProgram::UseAttributeArray(const std::string &name, int offset,
                                      size_t stride, int elementType,
                                      int elementTupleSize,
                                      NormalizeOption normalize)
{
  GLint location = static_cast<GLint>(this->FindAttributeArray(name));
  if (location == -1)
    {
    this->Error = "Could not use attribute " + name + ". No such attribute.";
    return false;
    }
  glVertexAttribPointer(location, elementTupleSize, convertTypeToGL(elementType),
                        normalize == Normalize ? GL_TRUE : GL_FALSE,
                        static_cast<GLsizei>(stride), BUFFER_OFFSET(offset));
  return true;
}

bool ShaderProgram::SetTextureSampler(const std::string &name,
                                      const Texture2D &texture)
{
  // Look up sampler location:
  GLint location = static_cast<GLint>(this->FindUniform(name));
  if (location == -1)
    {
    this->Error = "Could not set sampler " + name + ". No uniform with that name.";
    return false;
    }

  // Check if the texture is already bound:
  GLint textureUnitId = 0;
  typedef std::map<const Texture2D*, int>::const_iterator TMapIter;
  TMapIter result = TextureUnitBindings.find(&texture);
  if (result == TextureUnitBindings.end())
    {
    // Not bound. Attempt to bind the texture to an available texture unit.
    // We'll leave GL_TEXTURE0 unbound, as it is used for manipulating
    // textures.
    std::vector<bool>::iterator begin = BoundTextureUnits.begin() + 1;
    std::vector<bool>::iterator end = BoundTextureUnits.end();
    std::vector<bool>::iterator available = std::find(begin, end, false);

    if (available == end)
      {
      this->Error = "Could not set sampler " + name + ". No remaining texture "
          "units available.";
      return false;
      }

    textureUnitId = static_cast<GLint>(available - begin);

    GLenum textureUnit = LookupTextureUnit(textureUnitId);
    if (textureUnit == 0)
      {
      this->Error = "Could not set sampler " + name
          + ". Texture unit lookup failed.";
      return false;
      }

    glActiveTexture(textureUnit);
    if (!texture.bind())
      {
      this->Error = "Could not set sampler " + name + ": Error while binding "
          "texture: '" + texture.error() + "'.";
      glActiveTexture(GL_TEXTURE0);
      return false;
      }
    glActiveTexture(GL_TEXTURE0);

    // Mark texture unit as in-use.
    TextureUnitBindings.insert(std::make_pair(&texture, textureUnitId));
    *available = true;
    }
  else
    {
    // Texture is already bound.
    textureUnitId = result->second;
    }

  // Set the texture unit uniform
  glUniform1i(location, textureUnitId);

  return true;
}

bool ShaderProgram::SetUniformValue(const std::string &name, int i)
{
  GLint location = static_cast<GLint>(this->FindUniform(name));
  if (location == -1)
    {
    this->Error = "Could not set uniform " + name + ". No such uniform.";
    return false;
    }
  glUniform1i(location, static_cast<GLint>(i));
  return true;
}

bool ShaderProgram::SetUniformValue(const std::string &name, float f)
{
  GLint location = static_cast<GLint>(this->FindUniform(name));
  if (location == -1)
    {
    this->Error = "Could not set uniform " + name + ". No such uniform.";
    return false;
    }
  glUniform1f(location, static_cast<GLfloat>(f));
  return true;
}

bool ShaderProgram::SetUniformValue(const std::string &name,
                                    const Matrix3f &matrix)
{
  GLint location = static_cast<GLint>(this->FindUniform(name));
  if (location == -1)
    {
    this->Error = "Could not set uniform " + name + ". No such uniform.";
    return false;
    }
  glUniformMatrix3fv(location, 1, GL_FALSE,
                     static_cast<const GLfloat *>(matrix.data()));
  return true;
}

bool ShaderProgram::SetUniformValue(const std::string &name,
                                    const Matrix4f &matrix)
{
  GLint location = static_cast<GLint>(this->FindUniform(name));
  if (location == -1)
    {
    this->Error = "Could not set uniform " + name + ". No such uniform.";
    return false;
    }
  glUniformMatrix4fv(location, 1, GL_FALSE,
                     static_cast<const GLfloat *>(matrix.data()));
  return true;
}

bool ShaderProgram::SetUniformValue(const std::string &name,
                                    vtkMatrix4x4 *matrix)
{
  GLint location = static_cast<GLint>(this->FindUniform(name));
  if (location == -1)
    {
    this->Error = "Could not set uniform " + name + ". No such uniform.";
    return false;
    }
  float data[16];
  for (int i = 0; i < 16; ++i)
    {
    data[i] = matrix->Element[i / 4][i % 4];
    }
  glUniformMatrix4fv(location, 1, GL_FALSE, data);
  return true;
}

bool ShaderProgram::SetUniformValue(const std::string &name,
                                    vtkMatrix3x3 *matrix)
{
  GLint location = static_cast<GLint>(this->FindUniform(name));
  if (location == -1)
    {
    this->Error = "Could not set uniform " + name + ". No such uniform.";
    return false;
    }
  float data[9];
  for (int i = 0; i < 9; ++i)
    {
    data[i] = matrix->GetElement(i / 3, i % 3);
    }
  glUniformMatrix3fv(location, 1, GL_FALSE, data);
  return true;
}

bool ShaderProgram::SetUniformValue(const std::string &name, const int count,
                                    const float *v)
{
  GLint location = static_cast<GLint>(this->FindUniform(name));
  if (location == -1)
    {
    this->Error = "Could not set uniform " + name + ". No such uniform.";
    return false;
    }
  glUniform1fv(location, count, static_cast<const GLfloat *>(v));
  return true;
}

bool ShaderProgram::SetUniformValue(const std::string &name, const int count,
                                    const int *v)
{
  GLint location = static_cast<GLint>(this->FindUniform(name));
  if (location == -1)
    {
    this->Error = "Could not set uniform " + name + ". No such uniform.";
    return false;
    }
  glUniform1iv(location, count, static_cast<const GLint *>(v));
  return true;
}

bool ShaderProgram::SetUniformValue(const std::string &name, const int count,
                                    const float (*v)[3])
{
  GLint location = static_cast<GLint>(this->FindUniform(name));
  if (location == -1)
    {
    Error = "Could not set uniform " + name + ". No such uniform.";
    return false;
    }
  glUniform3fv(location, count, (const GLfloat *)v);
  return true;
}

bool ShaderProgram::SetUniformValue(const std::string &name, const Vector3f &v)
{
  GLint location = static_cast<GLint>(this->FindUniform(name));
  if (location == -1)
    {
    this->Error = "Could not set uniform " + name + ". No such uniform.";
    return false;
    }
  glUniform3fv(location, 1, v.data());
  return true;
}

bool ShaderProgram::SetUniformValue(const std::string &name, const Vector2i &v)
{
  GLint location = static_cast<GLint>(this->FindUniform(name));
  if (location == -1)
    {
    this->Error = "Could not set uniform " + name + ". No such uniform.";
    return false;
    }
  glUniform2iv(location, 1, v.data());
  return true;
}

bool ShaderProgram::SetUniformValue(const std::string &name,
                                    const Vector3ub &v)
{
  GLint location = static_cast<GLint>(this->FindUniform(name));
  if (location == -1)
    {
    this->Error = "Could not set uniform " + name + ". No such uniform.";
    return false;
    }
  vtkColor3f colorf(v[0] / 255.0f, v[1] / 255.0f, v[2] / 255.0f);
  glUniform3fv(location, 1, colorf.GetData());
  return true;
}

bool ShaderProgram::SetUniformValue(const std::string &name,
                                    const Vector4ub &v)
{
  GLint location = static_cast<GLint>(this->FindUniform(name));
  if (location == -1)
    {
    this->Error = "Could not set uniform " + name + ". No such uniform.";
    return false;
    }
  vtkColor4f colorf(v[0] / 255.0f, v[1] / 255.0f, v[2] / 255.0f, v[3] / 255.0f);
  glUniform4fv(location, 1, colorf.GetData());
  return true;
}

bool ShaderProgram::SetAttributeArrayInternal(
    const std::string &name, void *buffer, int type, int tupleSize,
    ShaderProgram::NormalizeOption normalize)
{
  if (type == -1)
    {
    this->Error = "Unrecognized data type for attribute " + name + ".";
    return false;
    }
  GLint location = static_cast<GLint>(this->FindAttributeArray(name));
  if (location == -1)
    {
    this->Error = "Could not set attribute " + name + ". No such attribute.";
    return false;
    }
  const GLvoid *data = static_cast<const GLvoid *>(buffer);
  glVertexAttribPointer(location, tupleSize, convertTypeToGL(type),
                        normalize == Normalize ? GL_TRUE : GL_FALSE, 0, data);
  return true;
}

void ShaderProgram::InitializeTextureUnits()
{
  GLint numTextureUnits;
  glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &numTextureUnits);

  // We'll impose a a hard limit of 32 texture units for symbolic lookups.
  // This seems to be about the maximum available on current hardware.
  // If increasing this limit, modify the lookupTextureUnit method
  // appropriately.
  numTextureUnits = std::min(std::max(numTextureUnits, 0), 32);

  BoundTextureUnits.clear();
  BoundTextureUnits.resize(numTextureUnits, false);
  TextureUnitBindings.clear();
}

void ShaderProgram::ReleaseAllTextureUnits()
{
  std::fill(BoundTextureUnits.begin(), BoundTextureUnits.end(), false);
  TextureUnitBindings.clear();
}

inline int ShaderProgram::FindAttributeArray(const std::string &name)
{
  if (name.empty() || !this->Linked)
    {
    return -1;
    }
  const GLchar *namePtr = static_cast<const GLchar *>(name.c_str());
  GLint location =
      static_cast<int>(glGetAttribLocation(static_cast<GLuint>(Handle),
                                           namePtr));
  if (location == -1)
    {
    this->Error = "Specified attribute not found in current shader program: ";
    this->Error += name;
    }

  return location;
}

inline int ShaderProgram::FindUniform(const std::string &name)
{
  if (name.empty() || !this->Linked)
    return -1;
  const GLchar *namePtr = static_cast<const GLchar *>(name.c_str());
  GLint location =
      static_cast<int>(glGetUniformLocation(static_cast<GLuint>(Handle),
                                            namePtr));
  if (location == -1)
    {
    this->Error = "Uniform " + name + " not found in current shader program.";
    }

  return location;
}

}
