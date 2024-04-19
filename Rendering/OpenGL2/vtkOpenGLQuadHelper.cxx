// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOpenGLQuadHelper.h"

#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLResourceFreeCallback.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLVertexArrayObject.h"
#include "vtkShaderProgram.h"
#include "vtk_glew.h"

VTK_ABI_NAMESPACE_BEGIN
vtkOpenGLQuadHelper::vtkOpenGLQuadHelper(
  vtkOpenGLRenderWindow* renWin, const char* vs, const char* fs, const char* gs, bool flipY)
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
                                 "  //VTK::TCoord::Flip\n"
                                 "}\n";

  std::string VS = (vs ? vs : defaultVS);

  if (flipY)
  {
    vtkShaderProgram::Substitute(
      VS, "//VTK::TCoord::Flip\n", "texCoord.y = 1.0 - texCoord.y;\n", true);
  }

  this->Program = renWin->GetShaderCache()->ReadyShaderProgram(VS.c_str(), fs, (gs ? gs : ""));

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
    vtkGenericWarningMacro("Error binding texCoordIn to VAO.");
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

  // Owner is shader cache. When the render window releases it's graphic ressources,
  // OpenGL state is deleted, so the cache is deleted as well.
  this->Program = nullptr;
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
VTK_ABI_NAMESPACE_END
