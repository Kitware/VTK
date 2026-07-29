// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkImageProcessingPass.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLFramebufferObject.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLState.h"
#include "vtkRenderState.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkTextureObject.h"
#include "vtk_glad.h"

#include "vtkCamera.h"
#include "vtkMath.h"

#include <cassert>
#include <iostream>

VTK_ABI_NAMESPACE_BEGIN
vtkCxxSetObjectMacro(vtkImageProcessingPass, DelegatePass, vtkRenderPass);

//------------------------------------------------------------------------------
vtkImageProcessingPass::vtkImageProcessingPass()
{
  this->DelegatePass = nullptr;
}

//------------------------------------------------------------------------------
vtkImageProcessingPass::~vtkImageProcessingPass()
{
  if (this->DelegatePass != nullptr)
  {
    this->DelegatePass->Delete();
  }
}

//------------------------------------------------------------------------------
void vtkImageProcessingPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "DelegatePass:";
  if (this->DelegatePass != nullptr)
  {
    this->DelegatePass->PrintSelf(os, indent);
  }
  else
  {
    os << "(none)" << endl;
  }
}
//------------------------------------------------------------------------------
// Description:
// Render delegate with a image of different dimensions than the
// original one.
// \pre s_exists: s!=0
// \pre fbo_exists: fbo!=0
// \pre fbo_has_context: fbo->GetContext()!=0
// \pre target_exists: target!=0
// \pre target_has_context: target->GetContext()!=0
void vtkImageProcessingPass::RenderDelegate(const vtkRenderState* states, int width, int height,
  int newWidth, int newHeight, vtkOpenGLFramebufferObject* fbo, vtkTextureObject* target)
{
  assert("pre: s_exists" && states != nullptr);
  assert("pre: fbo_exists" && fbo != nullptr);
  assert("pre: fbo_has_context" && fbo->GetContext() != nullptr);
  assert("pre: target_exists" && target != nullptr);
  assert("pre: target_has_context" && target->GetContext() != nullptr);

  vtkRenderer* renderer = states->GetRenderer();
  vtkRenderState delegateStates(renderer);
  delegateStates.SetPropArrayAndCount(states->GetPropArray(), states->GetPropArrayCount());

  // Adapt camera to new window size
  vtkCamera* savedCamera = renderer->GetActiveCamera();
  savedCamera->Register(this);
  vtkCamera* newCamera = vtkCamera::New();
  newCamera->DeepCopy(savedCamera);

  renderer->SetActiveCamera(newCamera);

  if (newCamera->GetParallelProjection())
  {
    newCamera->SetParallelScale(
      newCamera->GetParallelScale() * newHeight / static_cast<double>(height));
  }
  else
  {
    double largeDim;
    double smallDim;
    if (newCamera->GetUseHorizontalViewAngle())
    {
      largeDim = newWidth;
      smallDim = width;
    }
    else
    {
      largeDim = newHeight;
      smallDim = height;
    }
    double angle = vtkMath::RadiansFromDegrees(newCamera->GetViewAngle());

    angle = 2.0 * atan(tan(angle / 2.0) * largeDim / smallDim);

    newCamera->SetViewAngle(vtkMath::DegreesFromRadians(angle));
  }

  if (target->GetWidth() != static_cast<unsigned int>(newWidth) ||
    target->GetHeight() != static_cast<unsigned int>(newHeight))
  {
    target->Create2D(newWidth, newHeight, 4, VTK_UNSIGNED_CHAR, false);
  }

  delegateStates.SetFrameBuffer(fbo);
  fbo->Bind();
  fbo->AddColorAttachment(0, target);

  // because the same FBO can be used in another pass but with several color
  // buffers, force this pass to use 1, to avoid side effects from the
  // render of the previous frame.
  fbo->ActivateBuffer(0);

  fbo->AddDepthAttachment();
  fbo->StartNonOrtho(newWidth, newHeight);

  this->InitializeRenderTarget(target, states);

  vtkOpenGLState* ostate =
    vtkOpenGLRenderWindow::SafeDownCast(states->GetRenderer()->GetRenderWindow())->GetState();
  ostate->vtkglViewport(0, 0, newWidth, newHeight);
  ostate->vtkglScissor(0, 0, newWidth, newHeight);

  // 2. Delegate render in FBO
  ostate->vtkglEnable(GL_DEPTH_TEST);
  this->DelegatePass->Render(&delegateStates);
  this->NumberOfRenderedProps += this->DelegatePass->GetNumberOfRenderedProps();

  newCamera->Delete();
  renderer->SetActiveCamera(savedCamera);
  savedCamera->UnRegister(this);
}

//------------------------------------------------------------------------------
void vtkImageProcessingPass::InitializeRenderTarget(
  vtkTextureObject* textureTarget, const vtkRenderState* states)
{
  vtkRenderer* renderer = states->GetRenderer();
  vtkOpenGLRenderWindow* renderWindow =
    vtkOpenGLRenderWindow::SafeDownCast(renderer->GetRenderWindow());
  vtkOpenGLState* ostate = renderWindow->GetState();

  // We want to preserve the color buffer, thus copying the existing data from the render window
  // frame in the target color texture.
  vtkOpenGLRenderer* glRen = vtkOpenGLRenderer::SafeDownCast(renderer);
  if (glRen->GetPreserveColorBuffer())
  {
    ostate->PushReadFramebufferBinding();
    renderWindow->GetRenderFramebuffer()->Bind(vtkOpenGLFramebufferObject::GetReadMode());

    int renderWindowWidth = renderWindow->GetSize()[0];
    int renderWindowHeight = renderWindow->GetSize()[1];
    int targetWidth = textureTarget->GetWidth();
    int targetHeight = textureTarget->GetHeight();
    ostate->vtkglBlitFramebuffer(0, 0, renderWindowWidth, renderWindowHeight, 0, 0, targetWidth,
      targetHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    ostate->PopReadFramebufferBinding();
  }
}

//------------------------------------------------------------------------------
// Description:
// Release graphics resources and ask components to release their own
// resources.
// \pre w_exists: w!=0
void vtkImageProcessingPass::ReleaseGraphicsResources(vtkWindow* w)
{
  assert("pre: w_exists" && w != nullptr);
  if (this->DelegatePass != nullptr)
  {
    this->DelegatePass->ReleaseGraphicsResources(w);
  }
}
VTK_ABI_NAMESPACE_END
