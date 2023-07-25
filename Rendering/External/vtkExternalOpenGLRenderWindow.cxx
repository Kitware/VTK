// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtk_glew.h"

#include "vtkExternalOpenGLRenderWindow.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLFramebufferObject.h"
#include "vtkOpenGLState.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkExternalOpenGLRenderWindow);

//------------------------------------------------------------------------------
vtkExternalOpenGLRenderWindow::vtkExternalOpenGLRenderWindow()
{
  this->AutomaticWindowPositionAndResize = 1;
  this->UseExternalContent = true;
  this->FrameBlitMode = BlitToCurrent;
}

//------------------------------------------------------------------------------
vtkExternalOpenGLRenderWindow::~vtkExternalOpenGLRenderWindow() = default;

//------------------------------------------------------------------------------
void vtkExternalOpenGLRenderWindow::Start()
{
  // Use hardware acceleration
  this->SetIsDirect(1);

  auto ostate = this->GetState();

  if (this->AutomaticWindowPositionAndResize)
  {
    int info[4];
    ostate->vtkglGetIntegerv(GL_VIEWPORT, info);
    this->SetPosition(info[0], info[1]);
    this->SetSize(info[2], info[3]);
  }

  // creates or resizes the framebuffer
  this->Size[0] = (this->Size[0] > 0 ? this->Size[0] : 300);
  this->Size[1] = (this->Size[1] > 0 ? this->Size[1] : 300);
  this->CreateFramebuffers(this->Size[0], this->Size[1]);

  // For stereo, render the correct eye based on the OpenGL buffer mode
  GLint bufferType;
  ostate->vtkglGetIntegerv(GL_DRAW_BUFFER, &bufferType);
  vtkCollectionSimpleIterator sit;
  vtkRenderer* renderer;
  for (this->GetRenderers()->InitTraversal(sit);
       (renderer = this->GetRenderers()->GetNextRenderer(sit));)
  {
    if (bufferType == GL_BACK_RIGHT || bufferType == GL_RIGHT || bufferType == GL_FRONT_RIGHT)
    {
      this->StereoRenderOn();
      this->SetStereoTypeToRight();
    }
    else
    {
      this->SetStereoTypeToLeft();
    }
  }

  ostate->PushFramebufferBindings();

  if (this->UseExternalContent)
  {
    this->BlitToRenderFramebuffer(true);
  }

  this->RenderFramebuffer->Bind();
}

//------------------------------------------------------------------------------
bool vtkExternalOpenGLRenderWindow::IsCurrent()
{
  return true;
}

//------------------------------------------------------------------------------
void vtkExternalOpenGLRenderWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "UseExternalContent: " << this->UseExternalContent << endl;
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
