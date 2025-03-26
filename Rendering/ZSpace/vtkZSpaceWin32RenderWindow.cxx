// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkZSpaceWin32RenderWindow.h"

#include "vtkObjectFactory.h"
#include "vtkOpenGLFramebufferObject.h"
#include "vtkOpenGLState.h"
#include "vtkTextureObject.h"
#include "vtkZSpaceSDKManager.h"

#include "vtk_glew.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkZSpaceWin32RenderWindow);

//------------------------------------------------------------------------------
vtkZSpaceWin32RenderWindow::vtkZSpaceWin32RenderWindow()
{
  // Indicate to Windows that this window is per-monitor DPI aware.
  // This allows this window to actually fit the current physical resolution
  // of the display in fullscreen (and ignore scaling).
  SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

  // zSpace Core Compatibility SDK requires multisample to be disabled
  this->SetMultiSamples(0);

  // Blitting to the backbuffer is delegated to zSpace Core Compatibility SDK
  this->SetFrameBlitModeToNoBlit();
}

//------------------------------------------------------------------------------
vtkZSpaceWin32RenderWindow::~vtkZSpaceWin32RenderWindow() = default;

//------------------------------------------------------------------------------
void vtkZSpaceWin32RenderWindow::Start()
{
  vtkZSpaceSDKManager* sdkManager = vtkZSpaceSDKManager::GetInstance();
  if (sdkManager)
  {
    sdkManager->BeginFrame();
  }
  this->Superclass::Start();

  // The zSpace Inspire requires that left and right eyes textures use linear filtering.
  // We do it here, right after framebuffers have been populated.
  vtkTextureObject* leftTex = this->RenderFramebuffer->GetColorAttachmentAsTextureObject(0);
  leftTex->SetMinificationFilter(vtkTextureObject::Linear);
  leftTex->SetMagnificationFilter(vtkTextureObject::Linear);

  vtkTextureObject* rightTex = this->DisplayFramebuffer->GetColorAttachmentAsTextureObject(0);
  rightTex->SetMinificationFilter(vtkTextureObject::Linear);
  rightTex->SetMagnificationFilter(vtkTextureObject::Linear);
}

//------------------------------------------------------------------------------
void vtkZSpaceWin32RenderWindow::Initialize()
{
  this->Superclass::Initialize();

  vtkZSpaceSDKManager* sdkManager = vtkZSpaceSDKManager::GetInstance();
  if (sdkManager)
  {
    sdkManager->EnableGraphicsBinding();
  }
}

//------------------------------------------------------------------------------
void vtkZSpaceWin32RenderWindow::StereoMidpoint()
{
  // DisplayFramebuffer: right eye
  this->DisplayFramebuffer->Bind();
  this->DisplayFramebuffer->ActivateDrawBuffer(0);
}

//------------------------------------------------------------------------------
void vtkZSpaceWin32RenderWindow::StereoRenderComplete()
{
  // RenderFramebuffer: left eye
  this->RenderFramebuffer->Bind();
  this->RenderFramebuffer->ActivateDrawBuffer(0);
}

//------------------------------------------------------------------------------
void vtkZSpaceWin32RenderWindow::Frame()
{
  this->MakeCurrent();

  auto ostate = this->GetState();
  ostate->Push();

  // Bind draw buffer to back buffer. Should be done before submitting textures
  // to the zSpace API in order to let it blit the final woven image in it.
  ostate->vtkglBindFramebuffer(GL_FRAMEBUFFER, 0);
  ostate->vtkglDrawBuffer(GL_BACK_LEFT);

  // Send textures to the zSpace API
  vtkZSpaceSDKManager* sdkManager = vtkZSpaceSDKManager::GetInstance();
  // Ensure at this point that stereo is enabled. If not, textures aren't
  // configured properly and the zSpace SubmitFrame method cannot handle it.
  if (sdkManager && this->GetStereoRender())
  {
    vtkTextureObject* leftTex = this->RenderFramebuffer->GetColorAttachmentAsTextureObject(0);
    auto leftId = leftTex->GetHandle();
    vtkTextureObject* rightTex = this->DisplayFramebuffer->GetColorAttachmentAsTextureObject(0);
    auto rightId = rightTex->GetHandle();

    leftTex->Activate();
    rightTex->Activate();

    sdkManager->SubmitFrame(leftId, rightId);

    // Should be done right before swapping buffers.
    // Used alongside BeginFrame method to help the ZSpace API
    // do enhanced (predictive) tracking of eyes / stylus.
    sdkManager->EndFrame();
  }

  ostate->Pop();

  // Swap buffers, since zSpace API does not do it by itself
  if (!this->AbortRender && this->DoubleBuffer && this->SwapBuffers)
  {
    // If this check is not enforced, we crash in offscreen rendering
    if (this->DeviceContext && !this->UseOffScreenBuffers)
    {
      // use global scope to get Win32 API SwapBuffers and not be
      // confused with this->SwapBuffers
      ::SwapBuffers(this->DeviceContext);
    }
  }
}

//------------------------------------------------------------------------------
void vtkZSpaceWin32RenderWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
