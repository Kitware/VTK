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

#include "vtk_glew.h"

namespace vtkgl {

namespace {
inline GLenum convertType(BufferObject::ObjectType type)
{
  switch (type)
    {
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

BufferObject::BufferObject(ObjectType type)
  : d(new Private), Dirty(true)
{
  if (type == ArrayBuffer)
    {
    this->d->type = GL_ARRAY_BUFFER;
    }
  else
    {
    this->d->type = GL_ELEMENT_ARRAY_BUFFER;
    }
}

BufferObject::~BufferObject()
{
  if (this->d->handle != 0)
    {
    glDeleteBuffers(1, &this->d->handle);
    }
  delete this->d;
}

void BufferObject::ReleaseGraphicsResources()
{
  if (this->d->handle != 0)
    {
    glBindBuffer(this->d->type, 0);
    glDeleteBuffers(1, &this->d->handle);
    this->d->handle = 0;
    }
}

BufferObject::ObjectType BufferObject::GetType() const
{
  if (this->d->type == GL_ARRAY_BUFFER)
    {
    return ArrayBuffer;
    }
  else
    {
    return ElementArrayBuffer;
    }
}

int BufferObject::GetHandle() const
{
  return static_cast<int>(this->d->handle);
}

bool BufferObject::Bind()
{
  if (!this->d->handle)
    {
    return false;
    }

  glBindBuffer(this->d->type, this->d->handle);
  return true;
}

bool BufferObject::Release()
{
  if (!this->d->handle)
    {
    return false;
    }

  glBindBuffer(this->d->type, 0);
  return true;
}

bool BufferObject::UploadInternal(const void *buffer, size_t size,
                                  ObjectType objectType)
{
  GLenum objectTypeGl = convertType(objectType);
  if (d->handle == 0)
    {
    glGenBuffers(1, &this->d->handle);
    this->d->type = objectTypeGl;
    }
  else if (this->d->type != objectTypeGl)
    {
    this->Error = "Trying to upload array buffer to incompatible buffer.";
    return false;
    }
  glBindBuffer(this->d->type, this->d->handle);
  glBufferData(this->d->type, size, static_cast<const GLvoid *>(buffer),
               GL_STATIC_DRAW);
  this->Dirty = false;
  return true;
}

}
