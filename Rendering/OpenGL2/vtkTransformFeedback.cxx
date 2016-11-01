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
#include "vtkOpenGLError.h"
#include "vtkShaderProgram.h"

#include "vtk_glew.h"

vtkStandardNewMacro(vtkTransformFeedback)

//------------------------------------------------------------------------------
void vtkTransformFeedback::PrintSelf(std::ostream &os,
                                     vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
size_t vtkTransformFeedback::GetBytesPerVertex() const
{
  size_t result = 0;

  typedef std::vector<VaryingMetaData>::const_iterator IterT;
  for (IterT it = this->Varyings.begin(), itEnd = this->Varyings.end();
       it != itEnd; ++it)
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
void vtkTransformFeedback::AddVarying(VaryingRole role,
                                      const std::string &var)
{
  this->Varyings.push_back(VaryingMetaData(role, var));
  this->VaryingsBound = false;
}

//------------------------------------------------------------------------------
void vtkTransformFeedback::SetNumberOfVertices(int drawMode,
                                               size_t inputVerts)
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
void vtkTransformFeedback::BindVaryings(vtkShaderProgram *prog)
{
  if (this->Varyings.empty())
  {
    vtkErrorMacro(<<"No capture varyings specified.");
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
                              static_cast<GLsizei>(vars.size()),
                              &vars[0], static_cast<GLenum>(this->BufferMode));

  this->VaryingsBound = true;

  vtkOpenGLCheckErrorMacro("OpenGL errors detected after "
                           "glTransformFeedbackVaryings.");
}

//------------------------------------------------------------------------------
void vtkTransformFeedback::BindBuffer()
{
  if (!this->VaryingsBound)
  {
    vtkErrorMacro("Varyings not yet bound!");
    return;
  }

  vtkOpenGLClearErrorMacro();
  this->ReleaseGraphicsResources();

  GLuint tbo;
  glGenBuffers(1, &tbo);
  this->BufferHandle = static_cast<int>(tbo);
  glBindBuffer(GL_ARRAY_BUFFER, tbo);
  glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(this->GetBufferSize()),
               NULL, GL_STATIC_READ);
  glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, tbo);
  glBeginTransformFeedback(static_cast<GLenum>(this->PrimitiveMode));

  vtkOpenGLCheckErrorMacro("OpenGL errors detected.");
}

//------------------------------------------------------------------------------
void vtkTransformFeedback::ReadBuffer()
{
  if (this->BufferHandle == 0)
  {
    vtkErrorMacro("BufferHandle not set by BindBuffer().");
    return;
  }

  glEndTransformFeedback();

  size_t bufferSize = this->GetBufferSize();
  this->ReleaseBufferData();
  this->BufferData = new unsigned char[bufferSize];

  unsigned char *glBuffer(NULL);
  glMapBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, bufferSize,
                   GL_MAP_READ_BIT);
  glGetBufferPointerv(GL_TRANSFORM_FEEDBACK_BUFFER, GL_BUFFER_MAP_POINTER,
                      reinterpret_cast<GLvoid**>(&glBuffer));
  std::copy(glBuffer, glBuffer + bufferSize, this->BufferData);
  glUnmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
  this->ReleaseGraphicsResources();

  vtkOpenGLCheckErrorMacro("OpenGL errors detected.");
}

//------------------------------------------------------------------------------
void vtkTransformFeedback::ReleaseGraphicsResources()
{
  if (this->BufferHandle)
  {
    GLuint tbo = static_cast<GLuint>(this->BufferHandle);
    glDeleteBuffers(1, &tbo);
    this->BufferHandle = 0;
  }
}

//------------------------------------------------------------------------------
void vtkTransformFeedback::ReleaseBufferData(bool freeBuffer)
{
  if (freeBuffer)
  {
    delete [] this->BufferData;
  }
  this->BufferData = NULL;
}

//------------------------------------------------------------------------------
vtkTransformFeedback::vtkTransformFeedback()
  : VaryingsBound(false),
    Varyings(),
    NumberOfVertices(0),
    BufferMode(GL_INTERLEAVED_ATTRIBS),
    BufferHandle(0),
    PrimitiveMode(GL_POINTS),
    BufferData(NULL)
{
}

//------------------------------------------------------------------------------
vtkTransformFeedback::~vtkTransformFeedback()
{
  this->ReleaseGraphicsResources();
  this->ReleaseBufferData();
}
