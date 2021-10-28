/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkOpenXRRenderWindow.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

Parts Copyright Valve Coproration from hellovr_opengl_main.cpp
under their BSD license found here:
https://github.com/ValveSoftware/openvr/blob/master/LICENSE

=========================================================================*/
#include "vtkOpenXRRenderWindow.h"

#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLState.h"
#include "vtkOpenXRCamera.h"
#include "vtkOpenXRManager.h"
#include "vtkOpenXRModel.h"
#include "vtkOpenXRRenderWindowInteractor.h"
#include "vtkOpenXRRenderer.h"
#include "vtkOpenXRUtilities.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkTransform.h"

// include what we need for the helper window
#ifdef _WIN32
#include "vtkWin32OpenGLRenderWindow.h"
#elif VTK_USE_X
#include "vtkXOpenGLRenderWindow.h"
#endif

#if !defined(_WIN32) || defined(__CYGWIN__)
#define stricmp strcasecmp
#endif

vtkStandardNewMacro(vtkOpenXRRenderWindow);

//------------------------------------------------------------------------------
vtkOpenXRRenderWindow::vtkOpenXRRenderWindow()
{
  this->StereoCapableWindow = 1;
  this->StereoRender = 1;
  this->UseOffScreenBuffers = 1;
  this->Size[0] = 640;
  this->Size[1] = 720;
  this->Position[0] = 100;
  this->Position[1] = 100;

  // Initialize the models
  // For instance models are only a ray
  // But for the future, we should create a vtkOpenXRModel
  // That contain a ray and a mesh for a controller
  // (mesh loaded using the XR_MSFT_controller_model OpenXR extension)
  this->Models[0] = vtkSmartPointer<vtkOpenXRModel>::New();
  this->Models[1] = vtkSmartPointer<vtkOpenXRModel>::New();
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
void vtkOpenXRRenderWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkOpenXRRenderWindow::ReleaseGraphicsResources(vtkWindow* renWin)
{
  this->Superclass::ReleaseGraphicsResources(renWin);

  for (FramebufferDesc& fbo : this->FramebufferDescs)
  {
    glDeleteFramebuffers(1, &fbo.ResolveFramebufferId);
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
bool vtkOpenXRRenderWindow::GetPoseMatrixWorldFromDevice(
  vtkEventDataDevice vtkNotUsed(device), vtkMatrix4x4* poseMatrixWorld)
{
  vtkOpenXRManager* xrManager = vtkOpenXRManager::GetInstance();
  // Not sure if LeftEye should be used here. Probably should have some midpoint eye
  // or something.
  const XrPosef* viewPose = xrManager->GetViewPose(vtkVRRenderWindow::LeftEye);
  if (viewPose)
  {
    this->ConvertOpenXRPoseToMatrices(*viewPose, poseMatrixWorld);
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
bool vtkOpenXRRenderWindow::GetSizeFromAPI()
{
  vtkOpenXRManager* xrManager = vtkOpenXRManager::GetInstance();
  if (!xrManager)
  {
    return false;
  }
  std::tie(this->Size[0], this->Size[1]) = xrManager->GetRecommandedImageRectSize();

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
  if (this->Initialized)
  {
    return;
  }

  // No need to set size of helper window as we own the window
  this->HelperWindow->SetDisplayId(this->GetGenericDisplayId());
  this->HelperWindow->SetShowWindow(false);
  this->HelperWindow->Initialize();

  this->MakeCurrent();
  this->OpenGLInit();

  vtkOpenXRManager* xrManager = vtkOpenXRManager::GetInstance();
  if (!xrManager->Initialize(this->HelperWindow))
  {
    vtkErrorMacro(<< "Failed to initialize OpenXRManager");
    return;
  }

  // Create one framebuffer per view
  this->CreateFramebuffers();

  std::tie(this->Size[0], this->Size[1]) = xrManager->GetRecommandedImageRectSize();

  vtkDebugMacro(<< "Size : " << this->Size[0] << ", " << this->Size[1]);

  std::string strWindowTitle = "VTK - " + xrManager->GetOpenXRPropertiesAsString();
  this->SetWindowName(strWindowTitle.c_str());

  this->Initialized = true;

  vtkDebugMacro(<< "End of OpenXRRenderWindow Initialization");
}

//------------------------------------------------------------------------------
void vtkOpenXRRenderWindow::Finalize()
{
  this->ReleaseGraphicsResources(this);

  vtkOpenXRManager::GetInstance()->Finalize();

  if (this->HelperWindow && this->HelperWindow->GetGenericContext())
  {
    this->HelperWindow->Finalize();
  }
}

//------------------------------------------------------------------------------
void vtkOpenXRRenderWindow::Render()
{
  vtkOpenXRManager* xrManager = vtkOpenXRManager::GetInstance();

  if (!xrManager->WaitAndBeginFrame())
  {
    return;
  }

  if (xrManager->GetShouldRenderCurrentFrame())
  {
    // Start rendering
    this->Superclass::Render();
  }
  else
  {
    vtkWarningMacro(<< "Not rendered");
  }

  xrManager->EndFrame();
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
void vtkOpenXRRenderWindow::RenderOneEye(const uint32_t eye)
{
  vtkOpenXRManager* xrManager = vtkOpenXRManager::GetInstance();

  FramebufferDesc& eyeFramebufferDesc = this->FramebufferDescs[eye];
  if (!xrManager->PrepareRendering(
        eye, eyeFramebufferDesc.ResolveColorTextureId, eyeFramebufferDesc.ResolveDepthTextureId))
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
  xrManager->ReleaseSwapchainImage(eye);
}

//------------------------------------------------------------------------------
void vtkOpenXRRenderWindow::RenderModels()
{
  vtkOpenGLState* ostate = this->GetState();
  ostate->vtkglEnable(GL_DEPTH_TEST);

  vtkNew<vtkMatrix4x4> poseMatrix;
  auto iren = vtkOpenXRRenderWindowInteractor::SafeDownCast(this->Interactor);
  for (uint32_t hand :
    { vtkOpenXRManager::ControllerIndex::Left, vtkOpenXRManager::ControllerIndex::Right })
  {
    XrPosef handPose = iren->GetHandPose(hand);

    // this could be done without a new every time
    vtkNew<vtkMatrix4x4> poseMatrixPhysical;
    vtkOpenXRUtilities::CreateModelMatrix(poseMatrixPhysical, handPose);

    float fMatrix[3][4];
    for (int j = 0; j < 3; j++)
    {
      for (int i = 0; i < 4; i++)
      {
        fMatrix[j][i] = poseMatrixPhysical->GetElement(j, i);
      }
    }

    this->Models[hand]->Render(this, fMatrix);
  }
}

//------------------------------------------------------------------------------
bool vtkOpenXRRenderWindow::CreateFramebuffers()
{
  // With OpenXR, textures are created by the runtime because the compositor / runtime
  // knows better how to allocate a texture/buffer that will perform well
  // So we call glFrameBufferTexture2D at each frame with the texture provided by
  // the runtime
  // That's why we only generate framebuffers here
  vtkOpenXRManager* xrManager = vtkOpenXRManager::GetInstance();
  uint32_t viewCount = xrManager->GetViewCount();
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

  if (vtkOpenXRManager::GetInstance()->IsDepthExtensionSupported())
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

  if (vtkOpenXRManager::GetInstance()->IsDepthExtensionSupported())
  {
    glBlitFramebuffer(0, 0, this->Size[0], this->Size[1], 0, 0, this->Size[0], this->Size[1],
      GL_DEPTH_BUFFER_BIT, GL_NEAREST);
  }

  this->GetState()->PopDrawFramebufferBinding();
}

//------------------------------------------------------------------------------
void vtkOpenXRRenderWindow::ConvertOpenXRPoseToWorldCoordinates(const XrPosef& xrPose,
  double pos[3],  // Output world position
  double wxyz[4], // Output world orientation quaternion
  double ppos[3], // Output physical position
  double wdir[3]) // Output world view direction (-Z)
{
  vtkNew<vtkMatrix4x4> poseMatrixPhysical;
  vtkNew<vtkMatrix4x4> poseMatrixWorld;
  this->ConvertOpenXRPoseToMatrices(xrPose, poseMatrixWorld, poseMatrixPhysical);

  ppos[0] = poseMatrixPhysical->Element[0][3];
  ppos[1] = poseMatrixPhysical->Element[1][3];
  ppos[2] = poseMatrixPhysical->Element[2][3];

  vtkNew<vtkTransform> worldT;
  worldT->SetMatrix(poseMatrixWorld);

  worldT->GetPosition(pos);
  worldT->GetOrientationWXYZ(wxyz);
  worldT->GetOrientation(wdir);
}

//------------------------------------------------------------------------------
void vtkOpenXRRenderWindow::ConvertOpenXRPoseToMatrices(const XrPosef& xrPose,
  vtkMatrix4x4* poseMatrixWorld, vtkMatrix4x4* poseMatrixPhysical /*=nullptr*/)
{
  if (!poseMatrixWorld && !poseMatrixPhysical)
  {
    return;
  }

  vtkNew<vtkMatrix4x4> modelMatrix;
  vtkOpenXRUtilities::CreateModelMatrix(modelMatrix, xrPose);

  if (poseMatrixPhysical)
  {
    poseMatrixPhysical->DeepCopy(modelMatrix);
  }

  if (poseMatrixWorld)
  {
    // Transform from physical to world space
    vtkNew<vtkMatrix4x4> physicalToWorldMatrix;
    this->GetPhysicalToWorldMatrix(physicalToWorldMatrix);
    vtkMatrix4x4::Multiply4x4(modelMatrix, physicalToWorldMatrix, poseMatrixWorld);
  }
}

//------------------------------------------------------------------------------
const int vtkOpenXRRenderWindow::GetTrackedDeviceIndexForDevice(vtkEventDataDevice controller)
{
  if (controller == vtkEventDataDevice::HeadMountedDisplay)
  {
    vtkWarningMacro(<< "Controller is a HeadMountedDisplay, not supported yet");
    return vtkOpenXRManager::ControllerIndex::Inactive;
  }
  if (controller == vtkEventDataDevice::LeftController)
  {
    return vtkOpenXRManager::ControllerIndex::Left;
  }
  if (controller == vtkEventDataDevice::RightController)
  {
    return vtkOpenXRManager::ControllerIndex::Right;
  }
  if (controller == vtkEventDataDevice::GenericTracker)
  {
    vtkWarningMacro(<< "Controller is a GenericTracker, not supported yet");
    return vtkOpenXRManager::ControllerIndex::Inactive;
  }

  return vtkOpenXRManager::ControllerIndex::Inactive;
}

//------------------------------------------------------------------------------
vtkVRModel* vtkOpenXRRenderWindow::GetTrackedDeviceModel(
  vtkEventDataDevice vtkNotUsed(dev), uint32_t idx)
{
  if (idx == vtkOpenXRManager::ControllerIndex::Inactive)
  {
    return nullptr;
  }
  if (idx > vtkOpenXRManager::ControllerIndex::Right)
  {
    vtkWarningMacro(<< "Unsupported tracked device index");
    return nullptr;
  }

  // Now, check if the model is active and return nullptr if not
  if (!this->ModelsActiveState[idx])
  {
    return nullptr;
  }
  return this->Models[idx];
}
