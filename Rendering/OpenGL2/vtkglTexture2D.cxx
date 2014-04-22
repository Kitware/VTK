/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkglTexture2D.h"

#include <GL/glew.h>

namespace vtkgl {

namespace {
GLint convertFilterOptionToGL(Texture2D::FilterOption opt)
{
  switch (opt) {
  case Texture2D::Nearest:
    return GL_NEAREST;
  case Texture2D::Linear:
    return GL_LINEAR;
  default:
    return -1;
  }
}

Texture2D::FilterOption convertFilterOptionFromGL(GLint opt)
{
  switch (opt) {
  case GL_NEAREST:
    return Texture2D::Nearest;
  case GL_LINEAR:
    return Texture2D::Linear;
  default:
    return Texture2D::InvalidFilter;
  }
}

GLint convertWrappingOptionToGL(Texture2D::WrappingOption opt)
{
  switch (opt) {
  case Texture2D::ClampToEdge:
    return GL_CLAMP_TO_EDGE;
  case Texture2D::MirroredRepeat:
    return GL_MIRRORED_REPEAT;
  case Texture2D::Repeat:
    return GL_REPEAT;
  default:
    return -1;
  }
}

Texture2D::WrappingOption convertWrappingOptionFromGL(GLint opt)
{
  switch (opt) {
  case GL_CLAMP_TO_EDGE:
    return Texture2D::ClampToEdge;
  case GL_MIRRORED_REPEAT:
    return Texture2D::MirroredRepeat;
  case GL_REPEAT:
    return Texture2D::Repeat;
  default:
    return Texture2D::InvalidWrapping;
  }
}

GLint convertInternalFormatToGL(Texture2D::InternalFormat format)
{
  switch (format) {
  case Texture2D::InternalDepth:
    return GL_DEPTH_COMPONENT;
  case Texture2D::InternalDepthStencil:
    return GL_DEPTH_STENCIL;
  case Texture2D::InternalR:
    return GL_RED;
  case Texture2D::InternalRG:
    return GL_RG;
  case Texture2D::InternalRGB:
    return GL_RGB;
  case Texture2D::InternalRGBA:
    return GL_RGBA;
  default:
    return -1;
  }
}

GLint convertIncomingFormatToGL(Texture2D::IncomingFormat format)
{
  switch (format) {
  case Texture2D::IncomingR:
    return GL_RED;
  case Texture2D::IncomingRG:
    return GL_RG;
  case Texture2D::IncomingRGB:
    return GL_RGB;
  case Texture2D::IncomingBGR:
    return GL_BGR;
  case Texture2D::IncomingRGBA:
    return GL_RGBA;
  case Texture2D::IncomingBGRA:
    return GL_BGRA;
  case Texture2D::IncomingDepth:
    return GL_DEPTH_COMPONENT;
  case Texture2D::IncomingDepthStencil:
    return GL_DEPTH_STENCIL;
  default:
    return -1;
  }
}

GLenum convertTypeToGL(int type)
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

} // end anon namespace

class Texture2D::Private
{
public:
  Private() : textureId(0) { }
  ~Private()
  {
    if (textureId > 0)
      glDeleteTextures(1, &textureId);
  }

  mutable GLuint textureId;
};

Texture2D::Texture2D()
  : d(new Private)
{
}

Texture2D::~Texture2D()
{
  delete d;
}

int Texture2D::handle() const
{
  return static_cast<int>(d->textureId);
}

void Texture2D::setMinFilter(Texture2D::FilterOption opt)
{
  int old = pushTexture();
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  convertFilterOptionToGL(opt));
  popTexture(old);
}

Texture2D::FilterOption Texture2D::minFilter() const
{
  int old = pushTexture();
  GLint result;
  glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &result);
  popTexture(old);

  return convertFilterOptionFromGL(result);
}

void Texture2D::setMagFilter(Texture2D::FilterOption opt)
{
  int old = pushTexture();
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                  convertFilterOptionToGL(opt));
  popTexture(old);
}

Texture2D::FilterOption Texture2D::magFilter() const
{
  int old = pushTexture();
  GLint result;
  glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, &result);
  popTexture(old);

  return convertFilterOptionFromGL(result);
}

void Texture2D::setWrappingS(Texture2D::WrappingOption opt)
{
  int old = pushTexture();
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                  convertWrappingOptionToGL(opt));
  popTexture(old);
}

Texture2D::WrappingOption Texture2D::wrappingS() const
{
  int old = pushTexture();
  GLint result;
  glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, &result);
  popTexture(old);

  return convertWrappingOptionFromGL(result);
}

void Texture2D::setWrappingT(Texture2D::WrappingOption opt)
{
  int old = pushTexture();
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                  convertWrappingOptionToGL(opt));
  popTexture(old);
}

Texture2D::WrappingOption Texture2D::wrappingT() const
{
  int old = pushTexture();
  GLint result;
  glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, &result);
  popTexture(old);

  return convertWrappingOptionFromGL(result);
}

bool Texture2D::bind() const
{
  return pushTexture() != VTK_INT_MAX;
}

bool Texture2D::release() const
{
  popTexture(0);
  return true;
}

bool Texture2D::uploadInternal(const void *buffer, const Vector2i &dims,
                               Texture2D::IncomingFormat dataFormat,
                               int dataType,
                               Texture2D::InternalFormat internalFormat)
{
  // The dataType has already been validated.
  int old = pushTexture();
  glTexImage2D(GL_TEXTURE_2D, 0, convertInternalFormatToGL(internalFormat),
               dims[0], dims[1], 0, convertIncomingFormatToGL(dataFormat),
               convertTypeToGL(dataType),
               static_cast<GLvoid*>(const_cast<void*>(buffer)));
  popTexture(old);
  return true;
}

int Texture2D::pushTexture() const
{
  GLint currentHandle;
  glGetIntegerv(GL_TEXTURE_BINDING_2D, &currentHandle);

  if (d->textureId == 0) {
    if (!const_cast<Texture2D*>(this)->generateTextureHandle())
      return VTK_INT_MAX;
  }

  glBindTexture(GL_TEXTURE_2D, d->textureId);

  return static_cast<int>(currentHandle);
}

void Texture2D::popTexture(int id) const
{
  glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(id));
}

bool Texture2D::generateTextureHandle()
{
  if (d->textureId > 0) {
    m_error = "Refusing to overwrite existing texture handle.";
    return false;
  }

  glGenTextures(1, &d->textureId);

  if (d->textureId == 0) {
    m_error = "Error generating texture handle.";
    return false;
  }

  // Set up defaults to match the documentation:
  setMinFilter(Linear);
  setMagFilter(Linear);
  setWrappingS(Repeat);
  setWrappingT(Repeat);

  return true;
}

}
