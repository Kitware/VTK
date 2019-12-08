/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenGLQuadHelper.h"

#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLResourceFreeCallback.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLVertexArrayObject.h"
#include "vtkShaderProgram.h"
#include "vtk_glew.h"

vtkOpenGLQuadHelper::vtkOpenGLQuadHelper(
  vtkOpenGLRenderWindow* renWin, const char* vs, const char* fs, const char* gs)
  : Program(nullptr)
  , VAO(nullptr)
  , ResourceCallback(new vtkOpenGLResourceFreeCallback<vtkOpenGLQuadHelper>(
      this, &vtkOpenGLQuadHelper::ReleaseGraphicsResources))
{
  if (!fs)
  {
    vtkGenericWarningMacro("A fragment shader is required");
    return;
  }

  this->ResourceCallback->RegisterGraphicsResources(renWin);

  static const char* defaultVS = "//VTK::System::Dec\n"
                                 "in vec4 ndCoordIn;\n"
                                 "in vec2 texCoordIn;\n"
                                 "out vec2 texCoord;\n"
                                 "void main()\n"
                                 "{\n"
                                 "  gl_Position = ndCoordIn;\n"
                                 "  texCoord = texCoordIn;\n"
                                 "}\n";

  this->Program =
    renWin->GetShaderCache()->ReadyShaderProgram((vs ? vs : defaultVS), fs, (gs ? gs : ""));

  this->VAO = vtkOpenGLVertexArrayObject::New();
  this->ShaderChangeValue = 0;

  this->VAO->Bind();

  vtkOpenGLBufferObject* vertBuf = renWin->GetTQuad2DVBO();
  bool res = this->VAO->AddAttributeArray(
    this->Program, vertBuf, "ndCoordIn", 0, 4 * sizeof(float), VTK_FLOAT, 2, false);
  if (!res)
  {
    this->VAO->Release();
    vtkGenericWarningMacro("Error binding ndCoords to VAO.");
    return;
  }

  res = this->VAO->AddAttributeArray(this->Program, vertBuf, "texCoordIn", 2 * sizeof(float),
    4 * sizeof(float), VTK_FLOAT, 2, false);
  if (!res)
  {
    this->VAO->Release();
    vtkGenericWarningMacro("Error binding texCoords to VAO.");
    return;
  }

  this->VAO->Release();
}

vtkOpenGLQuadHelper::~vtkOpenGLQuadHelper()
{
  this->ResourceCallback->Release();
  if (this->VAO)
  {
    this->VAO->Delete();
    this->VAO = nullptr;
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLQuadHelper::ReleaseGraphicsResources(vtkWindow*)
{
  if (!this->ResourceCallback->IsReleasing())
  {
    this->ResourceCallback->Release();
    return;
  }

  if (this->VAO)
  {
    this->VAO->ReleaseGraphicsResources();
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLQuadHelper::Render()
{
  if (this->VAO)
  {
    this->VAO->Bind();
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    this->VAO->Release();
  }
}
