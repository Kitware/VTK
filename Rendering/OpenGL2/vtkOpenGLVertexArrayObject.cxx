/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOpenGLVertexArrayObject.h"
#include "vtkObjectFactory.h"

#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLBufferObject.h"
#include "vtkOpenGLVertexBufferObject.h"
#include "vtkShaderProgram.h"

#include <map>
#include <vector>

#include "vtk_glew.h"

vtkStandardNewMacro(vtkOpenGLVertexArrayObject)

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

struct VertexAttributes
{
  GLint Index;
  GLint  Size;
  GLenum Type;
  GLboolean Normalize;
  GLsizei Stride;
  GLuint Offset;
  int Divisor;
  bool IsMatrix;
};

} // end anonymous


class vtkOpenGLVertexArrayObject::Private
{
public:
  Private()
  {
    this->HandleVAO = 0;
    this->HandleProgram = 0;
    this->Supported = true;
    this->ForceEmulation = false;
  }
  ~Private()
  {
    if (this->HandleVAO)
    {
      glDeleteVertexArrays(1, &this->HandleVAO);
    }
  }

  void Initialize()
  {
    if (!this->ForceEmulation &&
        (GLEW_ARB_vertex_array_object ||
            vtkOpenGLRenderWindow::GetContextSupportsOpenGL32()))
    {
      this->Supported = true;
      glGenVertexArrays(1, &this->HandleVAO);
    }
    else
    {
      this->Supported = false;
    }
  }

  bool IsReady() const
  {
    // We either probed and allocated a VAO, or are falling back as the current
    // hardware does not support VAOs.
    return (this->HandleVAO != 0 || this->Supported == false);
  }

  void ReleaseGraphicsResources()
  {
    if (this->HandleVAO)
    {
      glDeleteVertexArrays(1, &this->HandleVAO);
    }
    this->HandleVAO = 0;
    this->Supported = true;
    this->HandleProgram = 0;
  }

  GLuint HandleVAO;
  GLuint HandleProgram;
  bool Supported;
  bool ForceEmulation;

  typedef std::map< GLuint, std::vector<VertexAttributes> > AttributeMap;
  AttributeMap Attributes;
};

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

vtkOpenGLVertexArrayObject::vtkOpenGLVertexArrayObject()
{
  this->Internal = new vtkOpenGLVertexArrayObject::Private;
}

vtkOpenGLVertexArrayObject::~vtkOpenGLVertexArrayObject()
{
  delete this->Internal;
}

void vtkOpenGLVertexArrayObject::SetForceEmulation(bool val)
{
  this->Internal->ForceEmulation = val;
}

void vtkOpenGLVertexArrayObject::Bind()
{
  // Either simply bind the VAO, or emulate behavior by binding all attributes.
  if (!this->Internal->IsReady())
  {
    this->Internal->Initialize();
  }
  if (this->Internal->IsReady() && this->Internal->Supported)
  {
    glBindVertexArray(this->Internal->HandleVAO);
  }
  else if (this->Internal->IsReady())
  {
    Private::AttributeMap::const_iterator it;
    for (it = this->Internal->Attributes.begin(); it != this->Internal->Attributes.end();
         ++it)
    {
      std::vector<VertexAttributes>::const_iterator attrIt;
      glBindBuffer(GL_ARRAY_BUFFER, it->first);
      for (attrIt = it->second.begin(); attrIt != it->second.end(); ++attrIt)
      {
        int matrixCount = attrIt->IsMatrix ? attrIt->Size : 1;
        for (int i = 0; i < matrixCount; ++i)
        {
          glEnableVertexAttribArray(attrIt->Index+i);
          glVertexAttribPointer(attrIt->Index+i, attrIt->Size, attrIt->Type,
                                attrIt->Normalize, attrIt->Stride,
                                BUFFER_OFFSET(attrIt->Offset + attrIt->Stride*i/attrIt->Size));
          if (attrIt->Divisor > 0)
          {
#if GL_ES_VERSION_3_0 == 1
            glVertexAttribDivisor(attrIt->Index+i, 1);
#else
            if (GLEW_ARB_instanced_arrays)
            {
              glVertexAttribDivisorARB(attrIt->Index+i, 1);
            }
#endif
          }
        }
      }
      glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
  }
}

void vtkOpenGLVertexArrayObject::Release()
{
  if (this->Internal->IsReady() && this->Internal->Supported)
  {
    glBindVertexArray(0);
  }
  else if (this->Internal->IsReady())
  {
    Private::AttributeMap::const_iterator it;
    for (it = this->Internal->Attributes.begin(); it != this->Internal->Attributes.end();
         ++it)
    {
      std::vector<VertexAttributes>::const_iterator attrIt;
      for (attrIt = it->second.begin(); attrIt != it->second.end(); ++attrIt)
      {
        int matrixCount = attrIt->IsMatrix ? attrIt->Size : 1;
        for (int i = 0; i < matrixCount; ++i)
        {
          if (attrIt->Divisor > 0)
          {
#if GL_ES_VERSION_3_0 == 1
            glVertexAttribDivisor(attrIt->Index+i, 0);
#else
            if (GLEW_ARB_instanced_arrays)
            {
              glVertexAttribDivisorARB(attrIt->Index+i, 0);
            }
#endif
          }
          glDisableVertexAttribArray(attrIt->Index+i);
        }
      }
    }
  }
}

void vtkOpenGLVertexArrayObject::ShaderProgramChanged()
{
  this->Release();

  Private::AttributeMap::iterator it;
  for (it = this->Internal->Attributes.begin(); it != this->Internal->Attributes.end();
       ++it)
  {
    it->second.clear();
  }
  this->Internal->Attributes.clear();

  this->Internal->HandleProgram = 0;
}

void vtkOpenGLVertexArrayObject::ReleaseGraphicsResources()
{
  this->ShaderProgramChanged();
  this->Internal->ReleaseGraphicsResources();
}

bool vtkOpenGLVertexArrayObject::AddAttributeArray(
  vtkShaderProgram *program,
  vtkOpenGLVertexBufferObject *buffer,
  const std::string &name,
  int offset, bool normalize)
{
  return this->AddAttributeArrayWithDivisor(
    program, buffer, name, offset,
    buffer->Stride, buffer->DataType, buffer->NumberOfComponents,
    normalize, 0, false);
}

bool vtkOpenGLVertexArrayObject::AddAttributeArrayWithDivisor(vtkShaderProgram *program,
                                          vtkOpenGLBufferObject *buffer,
                                          const std::string &name,
                                          int offset, size_t stride,
                                          int elementType, int elementTupleSize,
                                          bool normalize,
                                          int divisor, bool isMatrix)
{
  if(!program)
  {
    vtkErrorMacro("attempt to add attribute without a program for attribute " << name);
    return false;
  }

  // Check the program is bound, and the buffer is valid.
  if (!program->isBound())
  {
    vtkErrorMacro("attempt to add attribute without a bound program for attribute " << name);
    return false;
  }

  if (buffer->GetHandle() == 0)
  {
    vtkErrorMacro("attempt to add attribute without a handleless buffer for attribute " << name);
    return false;
  }

  if (buffer->GetType() != vtkOpenGLBufferObject::ArrayBuffer )
  {
    vtkErrorMacro("attempt to add attribute without an array buffer for attribute " << name);
    return false;
  }

  // Perform initialization if necessary, ensure program matches VAOs.
  if (this->Internal->HandleProgram == 0)
  {
    this->Internal->HandleProgram = static_cast<GLuint>(program->GetHandle());
  }
  if (!this->Internal->IsReady() ||
      this->Internal->HandleProgram != static_cast<GLuint>(program->GetHandle()))
  {
    vtkErrorMacro("attempt to add attribute when not ready for attribute " << name);
    return false;
  }

  const GLchar *namePtr = static_cast<const GLchar *>(name.c_str());
  VertexAttributes attribs;
  attribs.Index = glGetAttribLocation(this->Internal->HandleProgram, namePtr);
  attribs.Offset = offset;
  attribs.Stride = static_cast<GLsizei>(stride);
  attribs.Type = convertTypeToGL(elementType);
  attribs.Size = elementTupleSize;
  attribs.Normalize = normalize;
  attribs.IsMatrix = isMatrix;
  attribs.Divisor = divisor;

  if (attribs.Index == -1)
  {
    vtkErrorMacro("attempt to add attribute not found in program for attribute " << name);
    return false;
  }

  // Always make the call as even the first use wants the attrib pointer setting
  // up when we are emulating.
  buffer->Bind();
  glEnableVertexAttribArray(attribs.Index);
  glVertexAttribPointer(attribs.Index, attribs.Size, attribs.Type,
                        attribs.Normalize, attribs.Stride,
                        BUFFER_OFFSET(attribs.Offset));


  if (divisor > 0)
  {
#if GL_ES_VERSION_3_0 == 1
    glVertexAttribDivisor(attribs.Index, 1);
#else
    if (GLEW_ARB_instanced_arrays)
    {
      glVertexAttribDivisorARB(attribs.Index, 1);
    }
#endif
  }

  // If vertex array objects are not supported then build up our list.
  if (!this->Internal->Supported)
  {
    GLuint handleBuffer = buffer->GetHandle();
    Private::AttributeMap::iterator it = this->Internal->Attributes.find(handleBuffer);
    if (it != this->Internal->Attributes.end())
    {
      std::vector<VertexAttributes> &attribsVector = it->second;
      std::vector<VertexAttributes>::iterator it2;
      for (it2 = attribsVector.begin(); it2 != attribsVector.end(); ++it2)
      {
        if (it2->Index == attribs.Index)
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
      this->Internal->Attributes[handleBuffer] = attribsVector;
    }
  }

  return true;
}

bool vtkOpenGLVertexArrayObject::AddAttributeMatrixWithDivisor(
  vtkShaderProgram *program,
  vtkOpenGLBufferObject *buffer,
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
  attribs.Index = glGetAttribLocation(this->Internal->HandleProgram, namePtr);

  for (int i = 1; i < elementTupleSize; i++)
  {
    glEnableVertexAttribArray(attribs.Index+i);
    glVertexAttribPointer(attribs.Index + i, elementTupleSize, convertTypeToGL(elementType),
                          normalize, static_cast<GLsizei>(stride),
                          BUFFER_OFFSET(offset + stride*i/elementTupleSize));
    if (divisor > 0)
    {
#if GL_ES_VERSION_3_0 == 1
      glVertexAttribDivisor(attribs.Index+i, 1);
#else
      if (GLEW_ARB_instanced_arrays)
      {
        glVertexAttribDivisorARB(attribs.Index+i, 1);
      }
#endif
    }
  }

  return true;
}

bool vtkOpenGLVertexArrayObject::RemoveAttributeArray(const std::string &name)
{
  if (!this->Internal->IsReady() || this->Internal->HandleProgram == 0)
  {
    return false;
  }

  const GLchar *namePtr = static_cast<const GLchar *>(name.c_str());
  GLint location = glGetAttribLocation(this->Internal->HandleProgram, namePtr);
  if (location == -1)
  {
    return false;
  }

  glDisableVertexAttribArray(location);
  // If we don't have real VAOs find the entry and remove it too.
  if (!this->Internal->Supported)
  {
    Private::AttributeMap::iterator it;
    for (it = this->Internal->Attributes.begin(); it != this->Internal->Attributes.end();
         ++it)
    {
      std::vector<VertexAttributes>::iterator attrIt;
      for (attrIt = it->second.begin(); attrIt != it->second.end(); ++attrIt)
      {
        if (attrIt->Index == location)
        {
          it->second.erase(attrIt);
          return true;
        }
      }
    }
  }

  return true;
}

//-----------------------------------------------------------------------------
void vtkOpenGLVertexArrayObject::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
