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
  this->SetPhysicalViewDirection(0.0, 0.0, -1.0);
  this->SetPhysicalViewUp(0.0, 1.0, 0.0);
  this->SetPhysicalTranslation(0.0, 0.0, 0.0);
  this->PhysicalScale = 1.0;

  this->StereoCapableWindow = 1;
  this->StereoRender = 1;
  this->UseOffScreenBuffers = 1;
  this->Size[0] = 640;
  this->Size[1] = 720;
  this->Position[0] = 100;
  this->Position[1] = 100;

  this->HelperWindow = vtkOpenGLRenderWindow::SafeDownCast(vtkRenderWindow::New());

  if (this->HelperWindow == nullptr)
  {
    vtkErrorMacro(<< "Failed to create render window");
  }

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

  if (this->HelperWindow)
  {
    this->HelperWindow->Delete();
  }
}

//------------------------------------------------------------------------------
void vtkOpenXRRenderWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Initialized: " << this->Initialized << "\n";
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
void vtkOpenXRRenderWindow::SetHelperWindow(vtkOpenGLRenderWindow* win)
{
  if (this->HelperWindow == win)
  {
    return;
  }

  if (this->HelperWindow)
  {
    this->ReleaseGraphicsResources(this);
    this->HelperWindow->Delete();
    this->HelperWindow = nullptr;
  }

  this->HelperWindow = win;
  if (win)
  {
    win->Register(this);
  }

  this->Modified();
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
void vtkOpenXRRenderWindow::MakeCurrent()
{
  if (this->HelperWindow)
  {
    this->HelperWindow->MakeCurrent();
  }
}

//------------------------------------------------------------------------------
vtkOpenGLState* vtkOpenXRRenderWindow::GetState()
{
  if (this->HelperWindow)
  {
    return this->HelperWindow->GetState();
  }
  return this->Superclass::GetState();
}

//------------------------------------------------------------------------------
// Tells if this window is the current OpenGL context for the calling thread.
bool vtkOpenXRRenderWindow::IsCurrent()
{
  return this->HelperWindow ? this->HelperWindow->IsCurrent() : false;
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
// Begin the rendering process.
void vtkOpenXRRenderWindow::Start()
{
  // if the renderer has not been initialized, do so now
  if (this->HelperWindow && !this->Initialized)
  {
    this->Initialize();
  }

  if (!this->Initialized)
  {
    vtkErrorMacro(<< "Failed to Initialize vtkOpenXRRenderWindow");
    return;
  }

  this->Superclass::Start();
}

//------------------------------------------------------------------------------
void vtkOpenXRRenderWindow::SetPhysicalViewDirection(double x, double y, double z)
{
  if (this->PhysicalViewDirection[0] != x || this->PhysicalViewDirection[1] != y ||
    this->PhysicalViewDirection[2] != z)
  {
    this->PhysicalViewDirection[0] = x;
    this->PhysicalViewDirection[1] = y;
    this->PhysicalViewDirection[2] = z;
    this->InvokeEvent(vtkOpenXRRenderWindow::PhysicalToWorldMatrixModified);
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkOpenXRRenderWindow::SetPhysicalViewDirection(double dir[3])
{
  this->SetPhysicalViewDirection(dir[0], dir[1], dir[2]);
}

//------------------------------------------------------------------------------
void vtkOpenXRRenderWindow::SetPhysicalViewUp(double x, double y, double z)
{
  if (this->PhysicalViewUp[0] != x || this->PhysicalViewUp[1] != y || this->PhysicalViewUp[2] != z)
  {
    this->PhysicalViewUp[0] = x;
    this->PhysicalViewUp[1] = y;
    this->PhysicalViewUp[2] = z;
    this->InvokeEvent(vtkOpenXRRenderWindow::PhysicalToWorldMatrixModified);
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkOpenXRRenderWindow::SetPhysicalViewUp(double dir[3])
{
  this->SetPhysicalViewUp(dir[0], dir[1], dir[2]);
}

//------------------------------------------------------------------------------
void vtkOpenXRRenderWindow::SetPhysicalTranslation(double x, double y, double z)
{
  if (this->PhysicalTranslation[0] != x || this->PhysicalTranslation[1] != y ||
    this->PhysicalTranslation[2] != z)
  {
    this->PhysicalTranslation[0] = x;
    this->PhysicalTranslation[1] = y;
    this->PhysicalTranslation[2] = z;
    this->InvokeEvent(vtkOpenXRRenderWindow::PhysicalToWorldMatrixModified);
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkOpenXRRenderWindow::SetPhysicalTranslation(double trans[3])
{
  this->SetPhysicalTranslation(trans[0], trans[1], trans[2]);
}

//------------------------------------------------------------------------------
void vtkOpenXRRenderWindow::SetPhysicalScale(double scale)
{
  if (this->PhysicalScale != scale)
  {
    this->PhysicalScale = scale;
    this->InvokeEvent(vtkOpenXRRenderWindow::PhysicalToWorldMatrixModified);
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkOpenXRRenderWindow::SetPhysicalToWorldMatrix(vtkMatrix4x4* matrix)
{
  if (!matrix)
  {
    return;
  }
  vtkNew<vtkMatrix4x4> currentPhysicalToWorldMatrix;
  this->GetPhysicalToWorldMatrix(currentPhysicalToWorldMatrix);
  bool matrixDifferent = false;
  for (int i = 0; i < 4; i++)
  {
    for (int j = 0; j < 4; j++)
    {
      if (fabs(matrix->GetElement(i, j) - currentPhysicalToWorldMatrix->GetElement(i, j)) >= 1e-3)
      {
        matrixDifferent = true;
        break;
      }
    }
  }
  if (!matrixDifferent)
  {
    return;
  }

  vtkNew<vtkTransform> hmdToWorldTransform;
  hmdToWorldTransform->SetMatrix(matrix);

  double translation[3] = { 0.0 };
  hmdToWorldTransform->GetPosition(translation);
  this->PhysicalTranslation[0] = (-1.0) * translation[0];
  this->PhysicalTranslation[1] = (-1.0) * translation[1];
  this->PhysicalTranslation[2] = (-1.0) * translation[2];

  double scale[3] = { 0.0 };
  hmdToWorldTransform->GetScale(scale);
  this->PhysicalScale = scale[0];

  this->PhysicalViewUp[0] = matrix->GetElement(0, 1);
  this->PhysicalViewUp[1] = matrix->GetElement(1, 1);
  this->PhysicalViewUp[2] = matrix->GetElement(2, 1);
  vtkMath::Normalize(this->PhysicalViewUp);
  this->PhysicalViewDirection[0] = (-1.0) * matrix->GetElement(0, 2);
  this->PhysicalViewDirection[1] = (-1.0) * matrix->GetElement(1, 2);
  this->PhysicalViewDirection[2] = (-1.0) * matrix->GetElement(2, 2);
  vtkMath::Normalize(this->PhysicalViewDirection);

  this->InvokeEvent(vtkOpenXRRenderWindow::PhysicalToWorldMatrixModified);
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkOpenXRRenderWindow::GetPhysicalToWorldMatrix(vtkMatrix4x4* physicalToWorldMatrix)
{
  if (!physicalToWorldMatrix)
  {
    return;
  }

  physicalToWorldMatrix->Identity();

  // construct physical to non-scaled world axes (scaling is applied later)
  double physicalZ_NonscaledWorld[3] = { -this->PhysicalViewDirection[0],
    -this->PhysicalViewDirection[1], -this->PhysicalViewDirection[2] };
  double* physicalY_NonscaledWorld = this->PhysicalViewUp;
  double physicalX_NonscaledWorld[3] = { 0.0 };
  vtkMath::Cross(physicalY_NonscaledWorld, physicalZ_NonscaledWorld, physicalX_NonscaledWorld);

  for (int row = 0; row < 3; ++row)
  {
    physicalToWorldMatrix->SetElement(row, 0, physicalX_NonscaledWorld[row] * this->PhysicalScale);
    physicalToWorldMatrix->SetElement(row, 1, physicalY_NonscaledWorld[row] * this->PhysicalScale);
    physicalToWorldMatrix->SetElement(row, 2, physicalZ_NonscaledWorld[row] * this->PhysicalScale);
    physicalToWorldMatrix->SetElement(row, 3, -this->PhysicalTranslation[row]);
  }
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
  this->CreateFramebuffers(xrManager->GetViewCount());

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
    this->MakeCurrent();
    this->GetState()->ResetGLViewportState();
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
void vtkOpenXRRenderWindow::CreateFramebuffers(uint32_t viewCount)
{
  // With OpenXR, textures are created by the runtime because the compositor / runtime
  // knows better how to allocate a texture/buffer that will perform well
  // So we call glFrameBufferTexture2D at each frame with the texture provided by
  // the runtime
  // That's why we only generate framebuffers here
  this->FramebufferDescs.resize(viewCount);
  for (size_t i = 0; i < viewCount; ++i)
  {
    glGenFramebuffers(1, &this->FramebufferDescs[i].ResolveFramebufferId);
  }
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
vtkVRModel* vtkOpenXRRenderWindow::GetTrackedDeviceModel(const int idx)
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

  // Now, check if the model is active adn return nullptr if not
  if (!this->ModelsActiveState[idx])
  {
    return nullptr;
  }
  return this->Models[idx];
}
