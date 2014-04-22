/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkglBufferObject.h"

#include <GL/glew.h>

namespace vtkgl {

namespace {
inline GLenum convertType(BufferObject::ObjectType type)
{
  switch (type) {
  default:
  case BufferObject::ArrayBuffer:
    return GL_ARRAY_BUFFER;
  case BufferObject::ElementArrayBuffer:
    return GL_ELEMENT_ARRAY_BUFFER;
  }
}
}

struct BufferObject::Private
{
  Private() : handle(0) {}
  GLenum type;
  GLuint handle;
};

BufferObject::BufferObject(ObjectType type_)
  : d(new Private), m_dirty(true)
{
  if (type_ == ArrayBuffer)
    d->type = GL_ARRAY_BUFFER;
  else
    d->type = GL_ELEMENT_ARRAY_BUFFER;
}

BufferObject::~BufferObject()
{
  if (d->handle != 0)
    glDeleteBuffers(1, &d->handle);
  delete d;
}

BufferObject::ObjectType BufferObject::type() const
{
  if (d->type == GL_ARRAY_BUFFER)
    return ArrayBuffer;
  else
    return ElementArrayBuffer;
}

int BufferObject::handle() const
{
  return static_cast<int>(d->handle);
}

bool BufferObject::bind()
{
  if (!d->handle)
    return false;

  glBindBuffer(d->type, d->handle);
  return true;
}

bool BufferObject::release()
{
  if (!d->handle)
    return false;

  glBindBuffer(d->type, 0);
  return true;
}

bool BufferObject::uploadInternal(const void *buffer, size_t size,
                                  ObjectType objectType)
{
  GLenum objectTypeGl = convertType(objectType);
  if (d->handle == 0) {
    glGenBuffers(1, &d->handle);
    d->type = objectTypeGl;
  }
  else if (d->type != objectTypeGl) {
    m_error = "Trying to upload array buffer to incompatible buffer.";
    return false;
  }
  glBindBuffer(d->type, d->handle);
  glBufferData(d->type, size, static_cast<const GLvoid *>(buffer),
               GL_STATIC_DRAW);
  m_dirty = false;
  return true;
}

}
