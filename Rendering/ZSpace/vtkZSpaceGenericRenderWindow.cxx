// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkZSpaceGenericRenderWindow.h"

#include "vtkObjectFactory.h"
#include "vtkZSpaceSDKManager.h"

#include "vtkOpenGLFramebufferObject.h"
#include "vtkOpenGLState.h"
#include "vtkTextureObject.h"

#include "vtk_glew.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkZSpaceGenericRenderWindow);

//------------------------------------------------------------------------------
vtkZSpaceGenericRenderWindow::vtkZSpaceGenericRenderWindow()
{
  // zSpace Core Compatibility SDK requires it to be disabled
  this->SetMultiSamples(0);

  // The blit to the backbuffer is delegated to zSpace Core Compatibility SDK
  this->SetFrameBlitModeToNoBlit();
}

//------------------------------------------------------------------------------
vtkZSpaceGenericRenderWindow::~vtkZSpaceGenericRenderWindow() = default;

//------------------------------------------------------------------------------
void vtkZSpaceGenericRenderWindow::Start()
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
void vtkZSpaceGenericRenderWindow::OpenGLInitContext()
{
  this->Superclass::OpenGLInitContext();

  this->MakeCurrent();

  vtkZSpaceSDKManager* sdkManager = vtkZSpaceSDKManager::GetInstance();
  if (sdkManager)
  {
    sdkManager->EnableGraphicsBinding();
  }
}

//------------------------------------------------------------------------------
void vtkZSpaceGenericRenderWindow::StereoMidpoint()
{
  // DisplayFramebuffer: right eye
  this->DisplayFramebuffer->Bind();
  this->DisplayFramebuffer->ActivateDrawBuffer(0);
}

//------------------------------------------------------------------------------
void vtkZSpaceGenericRenderWindow::StereoRenderComplete()
{
  // RenderFramebuffer: left eye
  this->RenderFramebuffer->Bind();
  this->RenderFramebuffer->ActivateDrawBuffer(0);
}

//------------------------------------------------------------------------------
void vtkZSpaceGenericRenderWindow::Frame()
{
  this->MakeCurrent();

  auto ostate = this->GetState();
  ostate->Push();

  // Bind draw buffer to back buffer. Should be done before submitting textures
  // to the zSpace API in order to let it blit the final woven image in it.
  ostate->vtkglBindFramebuffer(GL_FRAMEBUFFER, 0);
  ostate->vtkglDrawBuffer(GL_BACK_LEFT);

  // Send textures
  vtkZSpaceSDKManager* sdkManager = vtkZSpaceSDKManager::GetInstance();
  if (sdkManager)
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

  // Indicate listener (managing OpenGL context)
  // that buffers can be swapped
  this->InvokeEvent(vtkCommand::WindowFrameEvent, nullptr);
}

//------------------------------------------------------------------------------
void vtkZSpaceGenericRenderWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
