/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkglVertexArrayObject.h"
#include "vtk_glew.h"

#include "vtkOpenGLRenderWindow.h"
#include "vtkglBufferObject.h"
#include "vtkShaderProgram.h"

#include <map>
#include <vector>

namespace vtkgl
{

namespace
{
// Copied from vtkglShaderProgram, time to move into a common header?
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
#ifdef GL_DOUBLE
      return GL_DOUBLE;
#else
      vtkGenericWarningMacro(<< "Attempt to use GL_DOUBLE when not supported");
      return 0;
#endif
    default:
      return 0;
    }
}
}

struct VertexAttributes
{
  GLint index;
  GLint  size;
  GLenum type;
  GLboolean normalize;
  GLsizei stride;
  GLuint offset;
  int divisor;
  bool isMatrix;
};

class VertexArrayObject::Private
{
public:
  Private() : handleVAO(0), handleProgram(0), supported(true)
  {
    this->ForceEmulation = false;
  }
  ~Private()
  {
    if (this->handleVAO)
      {
      glDeleteVertexArrays(1, &this->handleVAO);
      }
  }

  void Initialize()
  {
    if (!this->ForceEmulation && GLEW_ARB_vertex_array_object)
      {
      this->supported = true;
      glGenVertexArrays(1, &this->handleVAO);
      }
    else
      {
      this->supported = false;
      }
  }

  bool IsReady() const
  {
    // We either probed and allocated a VAO, or are falling back as the current
    // hardware does not support VAOs.
    return (this->handleVAO != 0 || this->supported == false);
  }

  void ReleaseGraphicsResources()
  {
    if (this->handleVAO)
      {
      glDeleteVertexArrays(1, &this->handleVAO);
      }
    this->handleVAO = 0;
    this->supported = true;
    this->handleProgram = 0;
  }

  GLuint handleVAO;
  GLuint handleProgram;
  bool supported;
  bool ForceEmulation;

  typedef std::map< GLuint, std::vector<VertexAttributes> > AttributeMap;
  AttributeMap attributes;
};

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

VertexArrayObject::VertexArrayObject() : d(new Private)
{
}

VertexArrayObject::~VertexArrayObject()
{
  delete d;
}

void VertexArrayObject::SetForceEmulation(bool val)
{
  this->d->ForceEmulation = val;
}

void VertexArrayObject::Bind()
{
  // Either simply bind the VAO, or emulate behavior by binding all attributes.
  if (!this->d->IsReady())
    {
    this->d->Initialize();
    }
  if (this->d->IsReady() && this->d->supported)
    {
    glBindVertexArray(this->d->handleVAO);
    }
  else if (this->d->IsReady())
    {
    Private::AttributeMap::const_iterator it;
    for (it = this->d->attributes.begin(); it != this->d->attributes.end();
         ++it)
      {
      std::vector<VertexAttributes>::const_iterator attrIt;
      glBindBuffer(GL_ARRAY_BUFFER, it->first);
      for (attrIt = it->second.begin(); attrIt != it->second.end(); ++attrIt)
        {
        int matrixCount = attrIt->isMatrix ? attrIt->size : 1;
        for (int i = 0; i < matrixCount; ++i)
          {
          glEnableVertexAttribArray(attrIt->index+i);
          glVertexAttribPointer(attrIt->index+i, attrIt->size, attrIt->type,
                                attrIt->normalize, attrIt->stride,
                                BUFFER_OFFSET(attrIt->offset + attrIt->stride*i/attrIt->size));
          if (attrIt->divisor > 0)
            {
#if GL_ES_VERSION_2_0 != 1 || GL_ES_VERSION_3_0 == 1
#if GL_ES_VERSION_3_0 == 1
            glVertexAttribDivisor(attrIt->index+i, 1);
#else
            if (vtkOpenGLRenderWindow::GetContextSupportsOpenGL32())
              {
              glVertexAttribDivisor(attrIt->index+i, 1);
              }
            else if (GLEW_ARB_instanced_arrays)
              {
              glVertexAttribDivisorARB(attrIt->index+i, 1);
              }
#endif
#endif
            }
          }
        }
      glBindBuffer(GL_ARRAY_BUFFER, 0);
      }
    }
}

void VertexArrayObject::Release()
{
  if (this->d->IsReady() && this->d->supported)
    {
    glBindVertexArray(0);
    }
  else if (this->d->IsReady())
    {
    Private::AttributeMap::const_iterator it;
    for (it = this->d->attributes.begin(); it != this->d->attributes.end();
         ++it)
      {
      std::vector<VertexAttributes>::const_iterator attrIt;
      for (attrIt = it->second.begin(); attrIt != it->second.end(); ++attrIt)
        {
        int matrixCount = attrIt->isMatrix ? attrIt->size : 1;
        for (int i = 0; i < matrixCount; ++i)
          {
          if (attrIt->divisor > 0)
            {
#if GL_ES_VERSION_2_0 != 1 || GL_ES_VERSION_3_0 == 1
#if GL_ES_VERSION_3_0 == 1
            glVertexAttribDivisor(attrIt->index+i, 0);
#else
            if (vtkOpenGLRenderWindow::GetContextSupportsOpenGL32())
              {
              glVertexAttribDivisor(attrIt->index+i, 0);
              }
            else if (GLEW_ARB_instanced_arrays)
              {
              glVertexAttribDivisorARB(attrIt->index+i, 0);
              }
#endif
#endif
            }
          glDisableVertexAttribArray(attrIt->index+i);
          }
        }
      }
    }
}

void VertexArrayObject::ShaderProgramChanged()
{
  this->Release();

  Private::AttributeMap::iterator it;
  for (it = this->d->attributes.begin(); it != this->d->attributes.end();
       ++it)
    {
    it->second.clear();
    }
  this->d->attributes.clear();

  this->d->handleProgram = 0;
}

void VertexArrayObject::ReleaseGraphicsResources()
{
  this->ShaderProgramChanged();
  this->d->ReleaseGraphicsResources();
}

bool VertexArrayObject::AddAttributeArrayWithDivisor(vtkShaderProgram *program,
                                          BufferObject &buffer,
                                          const std::string &name,
                                          int offset, size_t stride,
                                          int elementType, int elementTupleSize,
                                          bool normalize,
                                          int divisor, bool isMatrix)
{
  // Check the program is bound, and the buffer is valid.
  if (!program->isBound() || buffer.GetHandle() == 0 ||
      buffer.GetType() != BufferObject::ArrayBuffer)
    {
    return false;
    }

  // Perform initalization if necessary, ensure program matches VAOs.
  if (this->d->handleProgram == 0)
    {
    this->d->handleProgram = static_cast<GLuint>(program->GetHandle());
    }
  if (!this->d->IsReady() ||
      this->d->handleProgram != static_cast<GLuint>(program->GetHandle()))
    {
    return false;
    }

  const GLchar *namePtr = static_cast<const GLchar *>(name.c_str());
  VertexAttributes attribs;
  attribs.index = glGetAttribLocation(this->d->handleProgram, namePtr);
  attribs.offset = offset;
  attribs.stride = static_cast<GLsizei>(stride);
  attribs.type = convertTypeToGL(elementType);
  attribs.size = elementTupleSize;
  attribs.normalize = normalize;
  attribs.isMatrix = isMatrix;
  attribs.divisor = divisor;

  if (attribs.index == -1)
    {
    return false;
    }

  // Always make the call as even the first use wants the attrib pointer setting
  // up when we are emulating.
  glEnableVertexAttribArray(attribs.index);
  glVertexAttribPointer(attribs.index, attribs.size, attribs.type,
                        attribs.normalize, attribs.stride,
                        BUFFER_OFFSET(attribs.offset));


  if (divisor > 0)
    {
#if GL_ES_VERSION_2_0 != 1 || GL_ES_VERSION_3_0 == 1
#if GL_ES_VERSION_3_0 == 1
    glVertexAttribDivisor(attribs.index, 1);
#else
    if (vtkOpenGLRenderWindow::GetContextSupportsOpenGL32())
      {
      glVertexAttribDivisor(attribs.index, 1);
      }
    else if (GLEW_ARB_instanced_arrays)
      {
      glVertexAttribDivisorARB(attribs.index, 1);
      }
#endif
#endif
    }

  // If vertex array objects are not supported then build up our list.
  if (!this->d->supported)
    {
    GLuint handleBuffer = buffer.GetHandle();
    Private::AttributeMap::iterator it = this->d->attributes.find(handleBuffer);
    if (it != this->d->attributes.end())
      {
      std::vector<VertexAttributes> &attribsVector = it->second;
      std::vector<VertexAttributes>::iterator it2;
      for (it2 = attribsVector.begin(); it2 != attribsVector.end(); ++it2)
        {
        if (it2->index == attribs.index)
          {
          *it2 = attribs;
          return true;
          }
        }
      // Attribute not found, add it.
      attribsVector.push_back(attribs);
      }
    else
      {
      // a single handle can have multiple attribs
      std::vector<VertexAttributes> attribsVector;
      attribsVector.push_back(attribs);
      this->d->attributes[handleBuffer] = attribsVector;
      }
    }

  return true;
}

bool VertexArrayObject::AddAttributeMatrixWithDivisor(vtkShaderProgram *program,
                                          BufferObject &buffer,
                                          const std::string &name,
                                          int offset, size_t stride,
                                          int elementType, int elementTupleSize,
                                          bool normalize,
                                          int divisor)
{
  // bind the first row of values
  bool result =
    this->AddAttributeArrayWithDivisor(program, buffer, name,
      offset, stride, elementType, elementTupleSize, normalize, divisor, true);

  if (!result)
    {
    return result;
    }

  const GLchar *namePtr = static_cast<const GLchar *>(name.c_str());
  VertexAttributes attribs;
  attribs.index = glGetAttribLocation(this->d->handleProgram, namePtr);

  for (int i = 1; i < elementTupleSize; i++)
    {
    glEnableVertexAttribArray(attribs.index+i);
    glVertexAttribPointer(attribs.index + i, elementTupleSize, convertTypeToGL(elementType),
                          normalize, static_cast<GLsizei>(stride),
                          BUFFER_OFFSET(offset + stride*i/elementTupleSize));
    if (divisor > 0)
      {
#if GL_ES_VERSION_2_0 != 1 || GL_ES_VERSION_3_0 == 1
#if GL_ES_VERSION_3_0 == 1
      glVertexAttribDivisor(attribs.index+i, 1);
#else
      if (vtkOpenGLRenderWindow::GetContextSupportsOpenGL32())
        {
        glVertexAttribDivisor(attribs.index+i, 1);
        }
      else if (GLEW_ARB_instanced_arrays)
        {
        glVertexAttribDivisorARB(attribs.index+i, 1);
       }
#endif
#endif
      }
    }

  return true;
}

bool VertexArrayObject::RemoveAttributeArray(const std::string &name)
{
  if (!this->d->IsReady() || this->d->handleProgram == 0)
    {
    return false;
    }

  const GLchar *namePtr = static_cast<const GLchar *>(name.c_str());
  GLint location = glGetAttribLocation(this->d->handleProgram, namePtr);
  if (location == -1)
    {
    return false;
    }

  glDisableVertexAttribArray(location);
  // If we don't have real VAOs find the entry and remove it too.
  if (!this->d->supported)
    {
    Private::AttributeMap::iterator it;
    for (it = this->d->attributes.begin(); it != this->d->attributes.end();
         ++it)
      {
      std::vector<VertexAttributes>::iterator attrIt;
      for (attrIt = it->second.begin(); attrIt != it->second.end(); ++attrIt)
        {
        if (attrIt->index == location)
          {
          it->second.erase(attrIt);
          return true;
          }
        }
      }
    }

  return true;
}

} // End of vtkgl namespace
