// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOpenGLBufferObject.h"
#include "vtkObjectFactory.h"

#include "vtk_glew.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkOpenGLBufferObject);

namespace
{
inline GLenum convertType(vtkOpenGLBufferObject::ObjectType type)
{
  switch (type)
  {
    case vtkOpenGLBufferObject::ElementArrayBuffer:
      return GL_ELEMENT_ARRAY_BUFFER;
    case vtkOpenGLBufferObject::TextureBuffer:
#if defined(GL_TEXTURE_BUFFER)
      return GL_TEXTURE_BUFFER;
      // intentional fall through when not defined
#endif
    default:
    case vtkOpenGLBufferObject::ArrayBuffer:
      return GL_ARRAY_BUFFER;
  }
}

inline GLenum convertUsage(vtkOpenGLBufferObject::ObjectUsage type)
{
  switch (type)
  {
    case vtkOpenGLBufferObject::StreamDraw:
      return GL_STREAM_DRAW;
    case vtkOpenGLBufferObject::StreamRead:
      return GL_STREAM_READ;
    case vtkOpenGLBufferObject::StreamCopy:
      return GL_STREAM_COPY;
    case vtkOpenGLBufferObject::StaticDraw:
      return GL_STATIC_DRAW;
    case vtkOpenGLBufferObject::StaticRead:
      return GL_STATIC_READ;
    case vtkOpenGLBufferObject::StaticCopy:
      return GL_STATIC_COPY;
    case vtkOpenGLBufferObject::DynamicDraw:
      return GL_DYNAMIC_DRAW;
    case vtkOpenGLBufferObject::DynamicRead:
      return GL_DYNAMIC_READ;
    case vtkOpenGLBufferObject::DynamicCopy:
    default:
      return GL_DYNAMIC_COPY;
  }
}
}

struct vtkOpenGLBufferObject::Private
{
  Private()
  {
    this->Handle = 0;
    this->Type = GL_ARRAY_BUFFER;
    this->Usage = GL_STATIC_DRAW;
    this->Size = 0;
  }
  GLenum Type;
  GLenum Usage;
  GLuint Handle;
  size_t Size;
};

//------------------------------------------------------------------------------
vtkOpenGLBufferObject::vtkOpenGLBufferObject()
{
  this->Dirty = true;
  this->Internal = new Private;
  this->Internal->Type = convertType(vtkOpenGLBufferObject::ArrayBuffer);
}

//------------------------------------------------------------------------------
vtkOpenGLBufferObject::~vtkOpenGLBufferObject()
{
  if (this->Internal->Handle != 0)
  {
    glDeleteBuffers(1, &this->Internal->Handle);
  }
  delete this->Internal;
}

//------------------------------------------------------------------------------
void vtkOpenGLBufferObject::ReleaseGraphicsResources()
{
  if (this->Internal->Handle != 0)
  {
    glBindBuffer(this->Internal->Type, 0);
    glDeleteBuffers(1, &this->Internal->Handle);
    this->Internal->Handle = 0;
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLBufferObject::SetType(vtkOpenGLBufferObject::ObjectType value)
{
  this->Internal->Type = convertType(value);
}

//------------------------------------------------------------------------------
vtkOpenGLBufferObject::ObjectType vtkOpenGLBufferObject::GetType() const
{
  if (this->Internal->Type == GL_ARRAY_BUFFER)
  {
    return vtkOpenGLBufferObject::ArrayBuffer;
  }
  if (this->Internal->Type == GL_ELEMENT_ARRAY_BUFFER)
  {
    return vtkOpenGLBufferObject::ElementArrayBuffer;
  }
  else
  {
    return vtkOpenGLBufferObject::TextureBuffer;
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLBufferObject::SetUsage(vtkOpenGLBufferObject::ObjectUsage value)
{
  this->Internal->Usage = convertUsage(value);
}

//------------------------------------------------------------------------------
vtkOpenGLBufferObject::ObjectUsage vtkOpenGLBufferObject::GetUsage() const
{
  switch (this->Internal->Usage)
  {
    case GL_STREAM_DRAW:
      return vtkOpenGLBufferObject::StreamDraw;
    case GL_STREAM_READ:
      return vtkOpenGLBufferObject::StreamRead;
    case GL_STREAM_COPY:
      return vtkOpenGLBufferObject::StreamCopy;
    case GL_STATIC_DRAW:
      return vtkOpenGLBufferObject::StaticDraw;
    case GL_STATIC_READ:
      return vtkOpenGLBufferObject::StaticRead;
    case GL_STATIC_COPY:
      return vtkOpenGLBufferObject::StaticCopy;
    case GL_DYNAMIC_DRAW:
      return vtkOpenGLBufferObject::DynamicDraw;
    case GL_DYNAMIC_READ:
      return vtkOpenGLBufferObject::DynamicRead;
    case GL_DYNAMIC_COPY:
    default:
      return vtkOpenGLBufferObject::DynamicCopy;
  }
}

//------------------------------------------------------------------------------
int vtkOpenGLBufferObject::GetHandle() const
{
  return static_cast<int>(this->Internal->Handle);
}

//------------------------------------------------------------------------------
bool vtkOpenGLBufferObject::Allocate(size_t size, ObjectType objectType, ObjectUsage objectUsage)
{
  const bool generated = this->GenerateBuffer(objectType);
  if (!generated)
  {
    this->Error = "Trying to upload array buffer to incompatible buffer.";
    return false;
  }

  glBindBuffer(this->Internal->Type, this->Internal->Handle);
  vtkDebugMacro(<< "glBufferData: " << size << " bytes");
  glBufferData(
    this->Internal->Type, static_cast<GLsizeiptr>(size), nullptr, convertUsage(objectUsage));
  this->Dirty = true;
  this->Internal->Size = size;
  return true;
}

size_t vtkOpenGLBufferObject::GetSize()
{
  return this->Internal->Size;
}

//------------------------------------------------------------------------------
bool vtkOpenGLBufferObject::Bind()
{
  if (!this->Internal->Handle)
  {
    return false;
  }

  glBindBuffer(this->Internal->Type, this->Internal->Handle);
  return true;
}

//------------------------------------------------------------------------------
bool vtkOpenGLBufferObject::BindShaderStorage(int index)
{
#ifdef GL_SHADER_STORAGE_BUFFER
  if (!this->Internal->Handle)
  {
    return false;
  }

  this->Bind();

  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, this->Internal->Handle);
#else
  (void)index;
#endif
  return true;
}

//------------------------------------------------------------------------------
bool vtkOpenGLBufferObject::Release()
{
  if (!this->Internal->Handle)
  {
    return false;
  }

  glBindBuffer(this->Internal->Type, 0);
  return true;
}

//------------------------------------------------------------------------------
bool vtkOpenGLBufferObject::GenerateBuffer(vtkOpenGLBufferObject::ObjectType objectType)
{
  GLenum objectTypeGL = convertType(objectType);
  if (this->Internal->Handle == 0)
  {
    glGenBuffers(1, &this->Internal->Handle);
    this->Internal->Type = objectTypeGL;
  }
  return (this->Internal->Type == objectTypeGL);
}

//------------------------------------------------------------------------------
bool vtkOpenGLBufferObject::UploadInternal(
  const void* buffer, size_t size, vtkOpenGLBufferObject::ObjectType objectType)
{
  this->Allocate(size, objectType, this->GetUsage());
  return this->UploadRangeInternal(buffer, 0, size, objectType);
}

//------------------------------------------------------------------------------
bool vtkOpenGLBufferObject::UploadRangeInternal(
  const void* buffer, ptrdiff_t offset, ptrdiff_t size, ObjectType objectType)
{
  const bool generated = this->GenerateBuffer(objectType);
  if (!generated)
  {
    this->Error = "Trying to upload array buffer to incompatible buffer.";
    return false;
  }

  glBindBuffer(this->Internal->Type, this->Internal->Handle);
  vtkDebugMacro(<< "glBufferSubData: "
                << "(offset: " << offset << ", size: " << size << ")");
  glBufferSubData(
    this->Internal->Type, static_cast<GLintptr>(offset), static_cast<GLsizeiptr>(size), buffer);
  this->Dirty = false;
  return true;
}

//------------------------------------------------------------------------------
bool vtkOpenGLBufferObject::DownloadRangeInternal(void* buffer, ptrdiff_t offset, size_t size)
{
  glBindBuffer(this->Internal->Type, this->Internal->Handle);
  void* ptr = glMapBufferRange(this->Internal->Type, static_cast<GLintptr>(offset),
    static_cast<GLsizeiptr>(size), GL_MAP_READ_BIT);
  memcpy(buffer, ptr, size);
  glUnmapBuffer(this->Internal->Type);
  glBindBuffer(this->Internal->Type, 0);
  return true;
}

//------------------------------------------------------------------------------
void vtkOpenGLBufferObject::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
