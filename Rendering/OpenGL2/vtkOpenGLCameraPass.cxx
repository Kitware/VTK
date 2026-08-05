// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkOpenGLCameraPass.h"
#include "vtkCamera.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLFramebufferObject.h"
#include "vtkOpenGLRenderUtilities.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLState.h"
#include "vtkOverrideAttribute.h"
#include "vtkRenderState.h"
#include <cassert>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkOpenGLCameraPass);

//------------------------------------------------------------------------------
vtkOpenGLCameraPass::vtkOpenGLCameraPass() = default;

//------------------------------------------------------------------------------
vtkOpenGLCameraPass::~vtkOpenGLCameraPass() = default;

//------------------------------------------------------------------------------
vtkOverrideAttribute* vtkOpenGLCameraPass::CreateOverrideAttributes()
{
  auto* renderingBackendAttribute =
    vtkOverrideAttribute::CreateAttributeChain("RenderingBackend", "OpenGL", nullptr);
  return renderingBackendAttribute;
}

//------------------------------------------------------------------------------
void vtkOpenGLCameraPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
// Description:
// Perform rendering according to a render state \p s.
// \pre s_exists: s!=0
void vtkOpenGLCameraPass::Render(const vtkRenderState* s)
{
  assert("pre: s_exists" && s != nullptr);

  vtkOpenGLClearErrorMacro();

  this->NumberOfRenderedProps = 0;

  vtkRenderer* ren = s->GetRenderer();

  if (!ren->IsActiveCameraCreated())
  {
    vtkDebugMacro(<< "No cameras are on, creating one.");
    // the get method will automagically create a camera
    // and reset it since one hasn't been specified yet.
    // If is very unlikely that this can occur - if this
    // renderer is part of a vtkRenderWindow, the camera
    // will already have been created as part of the
    // DoStereoRender() method.

    // this is ren->GetActiveCameraAndResetIfCreated();
    ren->GetActiveCamera();
    ren->ResetCamera();
  }

  int lowerLeft[2];
  int usize;
  int vsize;
  vtkOpenGLFramebufferObject* fbo = vtkOpenGLFramebufferObject::SafeDownCast(s->GetFrameBuffer());

  vtkOpenGLRenderWindow* win = vtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow());
  win->MakeCurrent();
  vtkOpenGLState* ostate = win->GetState();

  // Unlike vtkOpenGLRenderer's no-pass code path, which calls
  // vtkRenderer::UpdateCamera() -> vtkCamera::Render(ren) (and so, for an
  // OpenGL camera, vtkOpenGLCamera::Render(), which updates the camera's
  // Stereo flag), this pass never calls vtkCamera::Render(). Update the
  // Stereo flag here so the cached view/projection matrices computed later
  // by vtkCamera::GetKeyMatrices() reflect the current stereo state instead
  // of whatever some other renderer sharing this camera happened to leave
  // it as.
  ren->GetActiveCamera()->UpdateStereo(ren);

  if (fbo == nullptr)
  {
    this->GetTiledSizeAndOrigin(s, &usize, &vsize, lowerLeft, lowerLeft + 1);
  }
  else
  {
    // FBO size. This is the renderer size as a renderstate is per renderer.
    int size[2];
    fbo->GetLastSize(size);
    usize = size[0];
    vsize = size[1];
    lowerLeft[0] = 0;
    lowerLeft[1] = 0;
    // we assume the drawbuffer state is already initialized before.
  }

  // Save the current viewport and camera matrices.
  vtkOpenGLState::ScopedglViewport vsaver(ostate);
  vtkOpenGLState::ScopedglScissor ssaver(ostate);
  vtkOpenGLState::ScopedglEnableDisable stsaver(ostate, GL_SCISSOR_TEST);

  ostate->vtkglViewport(lowerLeft[0], lowerLeft[1], usize, vsize);
  ostate->vtkglEnable(GL_SCISSOR_TEST);
  ostate->vtkglScissor(lowerLeft[0], lowerLeft[1], usize, vsize);

  if ((ren->GetRenderWindow())->GetErase() && ren->GetErase())
  {
    ren->Clear();
  }

  // Done with camera initialization. The delegate can be called.
  vtkOpenGLCheckErrorMacro("failed after camera initialization");

  if (this->DelegatePass != nullptr)
  {
    vtkOpenGLRenderUtilities::MarkDebugEvent("Start vtkCameraPass delegate");
    this->DelegatePass->Render(s);
    vtkOpenGLRenderUtilities::MarkDebugEvent("End vtkCameraPass delegate");
    this->NumberOfRenderedProps += this->DelegatePass->GetNumberOfRenderedProps();
  }
  else
  {
    vtkWarningMacro(<< " no delegate.");
  }
  vtkOpenGLCheckErrorMacro("failed after delegate pass");
}
VTK_ABI_NAMESPACE_END
