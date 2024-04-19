// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOpenGLHelper.h"

#include "vtkOpenGLIndexBufferObject.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLVertexArrayObject.h"
#include "vtkShaderProgram.h"

VTK_ABI_NAMESPACE_BEGIN
vtkOpenGLHelper::vtkOpenGLHelper()
{
  this->Program = nullptr;
  this->IBO = vtkOpenGLIndexBufferObject::New();
  this->VAO = vtkOpenGLVertexArrayObject::New();
  this->ShaderChangeValue = 0;
}

vtkOpenGLHelper::~vtkOpenGLHelper()
{
  this->IBO->Delete();
  this->VAO->Delete();
}

void vtkOpenGLHelper::ReleaseGraphicsResources(vtkWindow* win)
{
  vtkOpenGLRenderWindow* rwin = vtkOpenGLRenderWindow::SafeDownCast(win);
  if (rwin)
  {
    // Ensure that the context is current before releasing any
    // graphics resources tied to it.
    rwin->MakeCurrent();
  }

  if (this->Program)
  {
    // Let ShaderCache release the graphics resources as it is
    // responsible for creation and deletion.
    this->Program = nullptr;
  }
  this->IBO->ReleaseGraphicsResources();
  this->VAO->ReleaseGraphicsResources();
}
VTK_ABI_NAMESPACE_END
