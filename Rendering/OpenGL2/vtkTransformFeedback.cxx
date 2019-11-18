/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransformFeedback.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkTransformFeedback.h"

#include "vtkObjectFactory.h"
#include "vtkOpenGLBufferObject.h"
#include "vtkOpenGLError.h"
#include "vtkShaderProgram.h"

#include "vtk_glew.h"

vtkStandardNewMacro(vtkTransformFeedback);

//------------------------------------------------------------------------------
void vtkTransformFeedback::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
size_t vtkTransformFeedback::GetBytesPerVertex() const
{
  size_t result = 0;

  typedef std::vector<VaryingMetaData>::const_iterator IterT;
  for (IterT it = this->Varyings.begin(), itEnd = this->Varyings.end(); it != itEnd; ++it)
  {
    result += this->GetBytesPerVertex(it->Role);
  }

  return result;
}

//------------------------------------------------------------------------------
void vtkTransformFeedback::ClearVaryings()
{
  this->Varyings.clear();
  this->VaryingsBound = false;
}

//------------------------------------------------------------------------------
void vtkTransformFeedback::AddVarying(VaryingRole role, const std::string& var)
{
  this->Varyings.push_back(VaryingMetaData(role, var));
  this->VaryingsBound = false;
}

//------------------------------------------------------------------------------
void vtkTransformFeedback::SetNumberOfVertices(int drawMode, size_t inputVerts)
{
  switch (static_cast<GLenum>(drawMode))
  {
    case GL_POINTS:
      this->SetNumberOfVertices(inputVerts);
      this->SetPrimitiveMode(GL_POINTS);
      return;
    case GL_LINE_STRIP:
      this->SetNumberOfVertices(inputVerts < 2 ? 0 : (2 * (inputVerts - 1)));
      this->SetPrimitiveMode(GL_LINES);
      return;
    case GL_LINE_LOOP:
      this->SetNumberOfVertices(2 * inputVerts);
      this->SetPrimitiveMode(GL_LINES);
      return;
    case GL_LINES:
      this->SetNumberOfVertices(inputVerts);
      this->SetPrimitiveMode(GL_LINES);
      return;
    case GL_TRIANGLE_STRIP:
      this->SetNumberOfVertices(inputVerts < 3 ? 0 : (3 * (inputVerts - 2)));
      this->SetPrimitiveMode(GL_TRIANGLES);
      return;
    case GL_TRIANGLE_FAN:
      this->SetNumberOfVertices(inputVerts < 3 ? 0 : (3 * (inputVerts - 2)));
      this->SetPrimitiveMode(GL_TRIANGLES);
      return;
    case GL_TRIANGLES:
      this->SetNumberOfVertices(inputVerts);
      this->SetPrimitiveMode(GL_TRIANGLES);
      return;
  }

  vtkErrorMacro("Unknown draw mode enum value: " << drawMode);
  this->SetNumberOfVertices(0);
  this->SetPrimitiveMode(GL_POINTS);
}

//------------------------------------------------------------------------------
size_t vtkTransformFeedback::GetBufferSize() const
{
  return this->GetBytesPerVertex() * this->NumberOfVertices;
}

//------------------------------------------------------------------------------
void vtkTransformFeedback::BindVaryings(vtkShaderProgram* prog)
{
  if (this->Varyings.empty())
  {
    vtkErrorMacro(<< "No capture varyings specified.");
    return;
  }

  vtkOpenGLClearErrorMacro();

  std::vector<const char*> vars;
  vars.reserve(this->Varyings.size());
  for (size_t i = 0; i < this->Varyings.size(); ++i)
  {
    vars.push_back(this->Varyings[i].Identifier.c_str());
  }

  glTransformFeedbackVaryings(static_cast<GLuint>(prog->GetHandle()),
    static_cast<GLsizei>(vars.size()), &vars[0], static_cast<GLenum>(this->BufferMode));

  this->VaryingsBound = true;

  vtkOpenGLCheckErrorMacro("OpenGL errors detected after "
                           "glTransformFeedbackVaryings.");
}

//------------------------------------------------------------------------------
void vtkTransformFeedback::Allocate(int nbBuffers, size_t size, unsigned int hint)
{
  vtkOpenGLClearErrorMacro();
  this->ReleaseGraphicsResources();

  this->Buffers.resize(nbBuffers);

  for (int i = 0; i < nbBuffers; i++)
  {
    this->Buffers[i] = vtkOpenGLBufferObject::New();
    this->Buffers[i]->GenerateBuffer(vtkOpenGLBufferObject::ArrayBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, this->Buffers[i]->GetHandle());
    glBufferData(
      GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(size), nullptr, static_cast<GLenum>(hint));
    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, i, this->Buffers[i]->GetHandle());
  }
}

//------------------------------------------------------------------------------
void vtkTransformFeedback::BindBuffer(bool allocateOneBuffer)
{
  if (!this->VaryingsBound)
  {
    vtkErrorMacro("Varyings not yet bound!");
    return;
  }

  vtkOpenGLClearErrorMacro();

  if (allocateOneBuffer)
  {
    this->Allocate(1, this->GetBufferSize(), GL_STATIC_READ);
  }

  for (size_t i = 0; i < this->Buffers.size(); i++)
  {
    glBindBufferBase(
      GL_TRANSFORM_FEEDBACK_BUFFER, static_cast<GLuint>(i), this->Buffers[i]->GetHandle());
  }

  glBeginTransformFeedback(static_cast<GLenum>(this->PrimitiveMode));

  vtkOpenGLCheckErrorMacro("OpenGL errors detected.");
}

//------------------------------------------------------------------------------
void vtkTransformFeedback::ReadBuffer(int index)
{
  if (this->Buffers.empty())
  {
    vtkErrorMacro("Buffers not set by BindBuffer().");
    return;
  }

  glEndTransformFeedback();

  if (index >= 0)
  {
    size_t bufferSize = this->GetBufferSize();
    this->ReleaseBufferData();
    this->BufferData = new unsigned char[bufferSize];

    unsigned char* glBuffer(nullptr);
    glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, this->Buffers[index]->GetHandle());
    glMapBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, bufferSize, GL_MAP_READ_BIT);
    glGetBufferPointerv(
      GL_TRANSFORM_FEEDBACK_BUFFER, GL_BUFFER_MAP_POINTER, reinterpret_cast<GLvoid**>(&glBuffer));
    std::copy(glBuffer, glBuffer + bufferSize, this->BufferData);
    glUnmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
  }

  vtkOpenGLCheckErrorMacro("OpenGL errors detected.");
}

//------------------------------------------------------------------------------
vtkOpenGLBufferObject* vtkTransformFeedback::GetBuffer(int index)
{
  return this->Buffers[index];
}

//------------------------------------------------------------------------------
int vtkTransformFeedback::GetBufferHandle(int index)
{
  return this->Buffers[index]->GetHandle();
}

//------------------------------------------------------------------------------
void vtkTransformFeedback::ReleaseGraphicsResources()
{
  if (!this->Buffers.empty())
  {
    for (auto v : this->Buffers)
    {
      v->ReleaseGraphicsResources();
      v->Delete();
    }
    this->Buffers.clear();
  }
}

//------------------------------------------------------------------------------
void vtkTransformFeedback::ReleaseBufferData(bool freeBuffer)
{
  if (freeBuffer)
  {
    delete[] this->BufferData;
  }
  this->BufferData = nullptr;
}

//------------------------------------------------------------------------------
vtkTransformFeedback::vtkTransformFeedback()
  : VaryingsBound(false)
  , Varyings()
  , NumberOfVertices(0)
  , BufferMode(GL_INTERLEAVED_ATTRIBS)
  , PrimitiveMode(GL_POINTS)
  , BufferData(nullptr)
{
}

//------------------------------------------------------------------------------
vtkTransformFeedback::~vtkTransformFeedback()
{
  this->ReleaseGraphicsResources();
  this->ReleaseBufferData();
}
