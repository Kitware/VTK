// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOpenGLFXAAPass.h"

#include "vtkObjectFactory.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLState.h"
#include "vtkRenderState.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkOpenGLFXAAPass);
vtkCxxSetObjectMacro(vtkOpenGLFXAAPass, FXAAOptions, vtkFXAAOptions);

vtkOpenGLFXAAPass::~vtkOpenGLFXAAPass()
{
  this->SetFXAAOptions(nullptr);
}

//------------------------------------------------------------------------------
void vtkOpenGLFXAAPass::Render(const vtkRenderState* s)
{
  vtkOpenGLRenderer* r = vtkOpenGLRenderer::SafeDownCast(s->GetRenderer());
  vtkOpenGLRenderWindow* renWin = vtkOpenGLRenderWindow::SafeDownCast(r->GetRenderWindow());
  vtkOpenGLState* ostate = renWin->GetState();

  vtkOpenGLState::ScopedglEnableDisable dsaver(ostate, GL_DEPTH_TEST);

  int x, y, w, h;
  r->GetTiledSizeAndOrigin(&w, &h, &x, &y);

  ostate->vtkglViewport(x, y, w, h);
  ostate->vtkglScissor(x, y, w, h);

  if (this->DelegatePass == nullptr)
  {
    vtkWarningMacro("no delegate in vtkOpenGLFXAAPass.");
    return;
  }

  this->DelegatePass->Render(s);
  this->NumberOfRenderedProps = this->DelegatePass->GetNumberOfRenderedProps();

  if (this->FXAAOptions)
  {
    this->FXAAFilter->UpdateConfiguration(this->FXAAOptions);
  }

  this->FXAAFilter->Execute(r);
}

//------------------------------------------------------------------------------
void vtkOpenGLFXAAPass::ReleaseGraphicsResources(vtkWindow* w)
{
  assert("pre: w_exists" && w != nullptr);

  this->FXAAFilter->ReleaseGraphicsResources();
  this->Superclass::ReleaseGraphicsResources(w);
}

//------------------------------------------------------------------------------
void vtkOpenGLFXAAPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
