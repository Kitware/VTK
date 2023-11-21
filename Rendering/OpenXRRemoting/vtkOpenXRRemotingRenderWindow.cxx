// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOpenXRRemotingRenderWindow.h"

#include "vtkObjectFactory.h"
#include "vtkOpenGLFramebufferObject.h"
#include "vtkOpenXRManager.h"
#include "vtkOpenXRManagerD3DGraphics.h"
#include "vtkOpenXRManagerRemoteConnection.h"
#include "vtkTextureObject.h"
#include "vtkWin32OpenGLDXRenderWindow.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkOpenXRRemotingRenderWindow);

//------------------------------------------------------------------------------
vtkOpenXRRemotingRenderWindow::vtkOpenXRRemotingRenderWindow()
{
  // Flip the texture before presenting to D3D which using a different convention
  // for texture orientation.
  this->FramebufferFlipY = true;

  if (this->HelperWindow) // Clear previous window allocated by vtkVRRenderWindow
  {
    this->HelperWindow->Delete();
  }

  // Use an OpenGL-DX render window to allow streaming VTK rendering in a D3D texture.
  this->HelperWindow = vtkWin32OpenGLDXRenderWindow::New();

  // Use a D3D rendering backend in OpenXR
  vtkNew<vtkOpenXRManagerD3DGraphics> D3Dgraphics;
  vtkOpenXRManager::GetInstance().SetGraphicsStrategy(D3Dgraphics);

  // Use the OpenXR remoting connection strategy
  vtkNew<vtkOpenXRManagerRemoteConnection> remoteConnection;
  vtkOpenXRManager::GetInstance().SetConnectionStrategy(remoteConnection);
}

//------------------------------------------------------------------------------
void vtkOpenXRRemotingRenderWindow::SetRemotingIPAddress(const char* host)
{
  vtkOpenXRManager::GetInstance().GetConnectionStrategy()->SetIPAddress(host);
}

//------------------------------------------------------------------------------
void vtkOpenXRRemotingRenderWindow::Initialize()
{
  if (this->VRInitialized)
  {
    return;
  }

  this->Superclass::Initialize();

  // Set the sample count to the value recommended by the runtime
  uint32_t samples = vtkOpenXRManager::GetInstance().GetRecommendedSampleCount();
  this->HelperWindow->SetMultiSamples(samples);

  vtkOpenGLRenderWindow::CreateFramebuffers(this->Size[0], this->Size[1]);

  vtkWin32OpenGLDXRenderWindow* helperWindow =
    vtkWin32OpenGLDXRenderWindow::SafeDownCast(this->HelperWindow);

  // Register this window display framebuffer with the helper window D3D texture.
  // We use the display buffer to benefit from FramebufferFlipY.
  helperWindow->RegisterSharedTexture(
    this->GetDisplayFramebuffer()->GetColorAttachmentAsTextureObject(0)->GetHandle(),
    this->GetDisplayFramebuffer()->GetDepthAttachmentAsTextureObject()->GetHandle());

  // Resize shared texture
  this->HelperWindow->SetSize(this->Size[0], this->Size[1]);
}

//------------------------------------------------------------------------------
void vtkOpenXRRemotingRenderWindow::CopyResultFrame()
{
  vtkWin32OpenGLDXRenderWindow::SafeDownCast(this->HelperWindow)->Lock();
  this->Superclass::CopyResultFrame();
  vtkWin32OpenGLDXRenderWindow::SafeDownCast(this->HelperWindow)->Unlock();
}

//------------------------------------------------------------------------------
void vtkOpenXRRemotingRenderWindow::StereoUpdate()
{
  // Lock the shared texture for rendering
  vtkWin32OpenGLDXRenderWindow::SafeDownCast(this->HelperWindow)->Lock();

  this->Superclass::StereoUpdate();
}

//------------------------------------------------------------------------------
void vtkOpenXRRemotingRenderWindow::StereoMidpoint()
{
  this->RenderModels();

  // Blit to DisplayFamebuffer with FramebufferFlipY enabled
  this->Frame();

  // RenderOneEye
  this->Superclass::StereoMidpoint();
}

//------------------------------------------------------------------------------
void vtkOpenXRRemotingRenderWindow::StereoRenderComplete()
{
  this->RenderModels();

  // Blit to DisplayFamebuffer with FramebufferFlipY enabled
  this->Frame();

  // RenderOneEye
  this->Superclass::StereoRenderComplete();

  // Unlock the shared texture
  vtkWin32OpenGLDXRenderWindow::SafeDownCast(this->HelperWindow)->Unlock();
}

//------------------------------------------------------------------------------
void vtkOpenXRRemotingRenderWindow::RenderOneEye(uint32_t eye)
{
  ID3D11Texture2D* colorTexture = nullptr;
  ID3D11Texture2D* depthTexture = nullptr;
  if (!vtkOpenXRManager::GetInstance().PrepareRendering(this, &colorTexture, &depthTexture))
  {
    return;
  }

  // D3D11 Rendering
  vtkWin32OpenGLDXRenderWindow* helperWindow =
    vtkWin32OpenGLDXRenderWindow::SafeDownCast(this->HelperWindow);
  helperWindow->Unlock();
  helperWindow->BlitToTexture(colorTexture, depthTexture);
  helperWindow->Lock();

  // Release this swapchain image
  vtkOpenXRManager::GetInstance().ReleaseSwapchainImage(eye);
}
VTK_ABI_NAMESPACE_END
