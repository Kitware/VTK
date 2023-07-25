// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) 2015, Valve Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOpenXRRenderWindow.h"

#include "vtkObjectFactory.h"
#include "vtkOpenGLState.h"
#include "vtkOpenXR.h"
#include "vtkOpenXRManager.h"
#include "vtkOpenXRModel.h"
#include "vtkOpenXRRenderWindowInteractor.h"
#include "vtkOpenXRRenderer.h"
#include "vtkOpenXRUtilities.h"
#include "vtkRendererCollection.h"
#include "vtkVRCamera.h"

// include what we need for the helper window
#if defined(_WIN32)
#include "vtkWin32OpenGLRenderWindow.h"
#endif

#if defined(VTK_USE_X)
#include "vtkXOpenGLRenderWindow.h"
#endif

#if !defined(_WIN32) || defined(__CYGWIN__)
#define stricmp strcasecmp
#endif

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkOpenXRRenderWindow);

//------------------------------------------------------------------------------
vtkOpenXRRenderWindow::vtkOpenXRRenderWindow()
{
  this->StereoCapableWindow = 1;
  this->StereoRender = 1;
  this->UseOffScreenBuffers = true;
  this->Size[0] = 640;
  this->Size[1] = 720;
  this->Position[0] = 100;
  this->Position[1] = 100;
}

//------------------------------------------------------------------------------
vtkOpenXRRenderWindow::~vtkOpenXRRenderWindow()
{
  this->Finalize();

  vtkRenderer* ren;
  vtkCollectionSimpleIterator rit;
  this->Renderers->InitTraversal(rit);
  while ((ren = this->Renderers->GetNextRenderer(rit)))
  {
    ren->SetRenderWindow(nullptr);
  }
}

//------------------------------------------------------------------------------
// Create an interactor that will work with this renderer.
vtkRenderWindowInteractor* vtkOpenXRRenderWindow::MakeRenderWindowInteractor()
{
  this->Interactor = vtkOpenXRRenderWindowInteractor::New();
  this->Interactor->SetRenderWindow(this);
  return this->Interactor;
}

//------------------------------------------------------------------------------
bool vtkOpenXRRenderWindow::GetSizeFromAPI()
{
  vtkOpenXRManager& xrManager = vtkOpenXRManager::GetInstance();

  std::tie(this->Size[0], this->Size[1]) = xrManager.GetRecommendedImageRectSize();

  return true;
}

//------------------------------------------------------------------------------
// Add a renderer to the list of renderers.
void vtkOpenXRRenderWindow::AddRenderer(vtkRenderer* ren)
{
  if (ren && !vtkOpenXRRenderer::SafeDownCast(ren))
  {
    vtkErrorMacro("vtkOpenXRRenderWindow::AddRenderer: Failed to add renderer of type "
      << ren->GetClassName() << ": A vtkOpenXRRenderer is expected");
    return;
  }
  this->Superclass::AddRenderer(ren);
}

//------------------------------------------------------------------------------
// Initialize the rendering window.
void vtkOpenXRRenderWindow::Initialize()
{
  if (this->VRInitialized)
  {
    return;
  }

  if (!this->HelperWindow)
  {
    vtkErrorMacro(<< "HelperWindow is not set");
    return;
  }

  // No need to set size of helper window as we own the window
  this->HelperWindow->SetDisplayId(this->GetGenericDisplayId());
  this->HelperWindow->SetShowWindow(false);
  this->HelperWindow->Initialize();

  this->MakeCurrent();
  this->OpenGLInit();

  vtkOpenXRManager& xrManager = vtkOpenXRManager::GetInstance();
  if (!xrManager.Initialize(this->HelperWindow))
  {
    // Set to false because the above init of the HelperWindow sets it to true
    vtkErrorMacro(<< "Failed to initialize OpenXRManager");
    return;
  }

  // Create one framebuffer per view
  this->CreateFramebuffers();

  std::tie(this->Size[0], this->Size[1]) = xrManager.GetRecommendedImageRectSize();

  vtkDebugMacro(<< "Size : " << this->Size[0] << ", " << this->Size[1]);

  std::string strWindowTitle = "VTK - " + xrManager.GetOpenXRPropertiesAsString();
  this->SetWindowName(strWindowTitle.c_str());

  this->VRInitialized = true;
}

//------------------------------------------------------------------------------
void vtkOpenXRRenderWindow::Finalize()
{
  if (!this->VRInitialized)
  {
    return;
  }

  if (this->HelperWindow && this->HelperWindow->GetGenericContext())
  {
    this->HelperWindow->Finalize();
  }

  vtkOpenXRManager::GetInstance().Finalize();

  this->ReleaseGraphicsResources(this);

  this->VRInitialized = false;
}

//------------------------------------------------------------------------------
void vtkOpenXRRenderWindow::Render()
{
  vtkOpenXRManager& xrManager = vtkOpenXRManager::GetInstance();

  if (!xrManager.WaitAndBeginFrame())
  {
    return;
  }

  if (this->TrackHMD)
  {
    this->UpdateHMDMatrixPose();
  }

  if (xrManager.GetShouldRenderCurrentFrame())
  {
    // Start rendering
    this->Superclass::Render();
  }
  else
  {
    vtkWarningMacro(<< "Not rendered");
  }

  xrManager.EndFrame();
}

//------------------------------------------------------------------------------
void vtkOpenXRRenderWindow::UpdateHMDMatrixPose()
{
  auto handle = this->GetDeviceHandleForOpenXRHandle(vtkOpenXRManager::ControllerIndex::Head);
  auto device = this->GetDeviceForOpenXRHandle(vtkOpenXRManager::ControllerIndex::Head);
  this->AddDeviceHandle(handle, device);

  // use left eye as stand in for HMD right now
  // todo add event for head pose

  const XrPosef* xrPose = vtkOpenXRManager::GetInstance().GetViewPose(LEFT_EYE);
  if (xrPose == nullptr)
  {
    vtkErrorMacro(<< "No pose for left eye");
    return;
  }
  // Convert a XrPosef to a vtk view matrix
  vtkMatrix4x4* hmdToPhysicalMatrix = this->GetDeviceToPhysicalMatrixForDeviceHandle(handle);
  vtkOpenXRUtilities::SetMatrixFromXrPose(hmdToPhysicalMatrix, *xrPose);

  // update the camera values based on the pose
  vtkNew<vtkMatrix4x4> d2wMat;
  this->GetDeviceToWorldMatrixForDeviceHandle(handle, d2wMat);

  vtkRenderer* ren;
  vtkCollectionSimpleIterator rit;
  this->Renderers->InitTraversal(rit);
  while ((ren = this->Renderers->GetNextRenderer(rit)))
  {
    vtkVRCamera* cam = vtkVRCamera::SafeDownCast(ren->GetActiveCamera());
    cam->SetCameraFromDeviceToWorldMatrix(d2wMat, this->GetPhysicalScale());
    if (ren->GetLightFollowCamera())
    {
      ren->UpdateLightsGeometryToFollowCamera();
    }
  }
}

//------------------------------------------------------------------------------
void vtkOpenXRRenderWindow::StereoUpdate()
{
  this->Superclass::StereoUpdate();
}

//------------------------------------------------------------------------------
void vtkOpenXRRenderWindow::StereoMidpoint()
{
  this->GetState()->vtkglDisable(GL_MULTISAMPLE);

  if (this->SwapBuffers)
  {
    this->RenderOneEye(LEFT_EYE);
  }
}

//------------------------------------------------------------------------------
void vtkOpenXRRenderWindow::StereoRenderComplete()
{
  this->GetState()->vtkglDisable(GL_MULTISAMPLE);

  if (this->SwapBuffers)
  {
    this->RenderOneEye(RIGHT_EYE);
  }
}

//------------------------------------------------------------------------------
void vtkOpenXRRenderWindow::RenderOneEye(uint32_t eye)
{
  vtkOpenXRManager& xrManager = vtkOpenXRManager::GetInstance();

  FramebufferDesc& eyeFramebufferDesc = this->FramebufferDescs[eye];

  if (!xrManager.PrepareRendering(
        eye, &eyeFramebufferDesc.ResolveColorTextureId, &eyeFramebufferDesc.ResolveDepthTextureId))
  {
    return;
  }

  this->RenderModels();

  // When binding texture, the color texture id stored in the
  // framebufferDesc must be set
  this->BindTextureToFramebuffer(eyeFramebufferDesc);

  // For this eye, the rendering resources of OpenXRUtilities and the texture ids are set
  // we can render
  this->RenderFramebuffer(eyeFramebufferDesc);

  // Release this swapchain image
  xrManager.ReleaseSwapchainImage(eye);
}

//------------------------------------------------------------------------------
void vtkOpenXRRenderWindow::RenderModels()
{
  vtkOpenGLState* ostate = this->GetState();
  ostate->vtkglEnable(GL_DEPTH_TEST);

  auto iren = vtkOpenXRRenderWindowInteractor::SafeDownCast(this->Interactor);
  for (uint32_t hand :
    { vtkOpenXRManager::ControllerIndex::Left, vtkOpenXRManager::ControllerIndex::Right })
  {
    // do we not have a model loaded yet? try loading one
    auto handle = this->GetDeviceHandleForOpenXRHandle(hand);
    auto device = this->GetDeviceForOpenXRHandle(hand);
    this->AddDeviceHandle(handle, device);
    auto* pRenderModel = this->GetModelForDeviceHandle(handle);
    if (!pRenderModel)
    {
      vtkNew<vtkOpenXRModel> newModel;
      this->SetModelForDeviceHandle(handle, newModel);
      pRenderModel = newModel;
    }

    // if we have a model and it is visible
    if (pRenderModel && pRenderModel->GetVisibility())
    {
      XrPosef* handPose = iren->GetHandPose(hand);
      if (handPose)
      {
        vtkMatrix4x4* tdPose = this->GetDeviceToPhysicalMatrixForDeviceHandle(handle);
        vtkOpenXRUtilities::SetMatrixFromXrPose(tdPose, *handPose);
        pRenderModel->Render(this, tdPose);
      }
    }
  }
}

//------------------------------------------------------------------------------
bool vtkOpenXRRenderWindow::CreateFramebuffers(uint32_t vtkNotUsed(viewCount))
{
  // With OpenXR, textures are created by the runtime because the compositor / runtime
  // knows better how to allocate a texture/buffer that will perform well
  // So we call glFrameBufferTexture2D at each frame with the texture provided by
  // the runtime
  // That's why we only generate framebuffers here
  vtkOpenXRManager& xrManager = vtkOpenXRManager::GetInstance();
  uint32_t viewCount = xrManager.GetViewCount();
  this->FramebufferDescs.resize(viewCount);
  for (size_t i = 0; i < viewCount; ++i)
  {
    glGenFramebuffers(1, &this->FramebufferDescs[i].ResolveFramebufferId);
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkOpenXRRenderWindow::BindTextureToFramebuffer(FramebufferDesc& framebufferDesc)
{
  this->GetState()->PushFramebufferBindings();
  this->GetState()->vtkglBindFramebuffer(GL_FRAMEBUFFER, framebufferDesc.ResolveFramebufferId);

  glFramebufferTexture2D(
    GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebufferDesc.ResolveColorTextureId, 0);

  if (vtkOpenXRManager::GetInstance().IsDepthExtensionSupported())
  {
    glFramebufferTexture2D(
      GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, framebufferDesc.ResolveDepthTextureId, 0);
  }

  // check FBO status
  GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if (status != GL_FRAMEBUFFER_COMPLETE)
  {
    vtkErrorMacro(<< "Framebuffer binding is not complete");
    return false;
  }

  this->GetState()->PopFramebufferBindings();

  return true;
}

//------------------------------------------------------------------------------
void vtkOpenXRRenderWindow::RenderFramebuffer(FramebufferDesc& framebufferDesc)
{
  // Blit the render frame buffer into the draw frame buffer
  this->GetState()->PushDrawFramebufferBinding();

  // We will read from actual read buffer and draw in our framebuffer
  this->GetState()->vtkglBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebufferDesc.ResolveFramebufferId);

  glBlitFramebuffer(0, 0, this->Size[0], this->Size[1], 0, 0, this->Size[0], this->Size[1],
    GL_COLOR_BUFFER_BIT, GL_LINEAR);

  if (vtkOpenXRManager::GetInstance().IsDepthExtensionSupported())
  {
    glBlitFramebuffer(0, 0, this->Size[0], this->Size[1], 0, 0, this->Size[0], this->Size[1],
      GL_DEPTH_BUFFER_BIT, GL_NEAREST);
  }

  this->GetState()->PopDrawFramebufferBinding();
}

//------------------------------------------------------------------------------
uint32_t vtkOpenXRRenderWindow::GetDeviceHandleForOpenXRHandle(uint32_t index)
{
  return index;
}

vtkEventDataDevice vtkOpenXRRenderWindow::GetDeviceForOpenXRHandle(uint32_t ohandle)
{
  if (ohandle == vtkOpenXRManager::ControllerIndex::Left)
  {
    return vtkEventDataDevice::LeftController;
  }
  if (ohandle == vtkOpenXRManager::ControllerIndex::Right)
  {
    return vtkEventDataDevice::RightController;
  }
  if (ohandle == vtkOpenXRManager::ControllerIndex::Head)
  {
    return vtkEventDataDevice::HeadMountedDisplay;
  }

  return vtkEventDataDevice::Unknown;
}
VTK_ABI_NAMESPACE_END
