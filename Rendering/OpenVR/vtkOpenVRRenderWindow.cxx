/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkOpenVRRenderWindow.cxx

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
#include "vtkOpenVRRenderWindow.h"

#include "vtkCommand.h"
#include "vtkFloatArray.h"
#include "vtkIdList.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLIndexBufferObject.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLState.h"
#include "vtkOpenGLTexture.h"
#include "vtkOpenGLVertexArrayObject.h"
#include "vtkOpenGLVertexBufferObject.h"
#include "vtkOpenVRCamera.h"
#include "vtkOpenVRDefaultOverlay.h"
#include "vtkOpenVRModel.h"
#include "vtkOpenVRRenderWindowInteractor.h"
#include "vtkOpenVRRenderer.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRendererCollection.h"
#include "vtkShaderProgram.h"
#include "vtkTextureObject.h"
#include "vtkTransform.h"

#include <cmath>
#include <sstream>

#include "vtkOpenGLError.h"

// include what we need for the helper window
#ifdef WIN32
#include "vtkWin32OpenGLRenderWindow.h"
#endif
#ifdef VTK_USE_X
#include "vtkXOpenGLRenderWindow.h"
#endif
#ifdef VTK_USE_COCOA
#include "vtkCocoaRenderWindow.h"
#endif

#if !defined(_WIN32) || defined(__CYGWIN__)
#define stricmp strcasecmp
#endif

vtkStandardNewMacro(vtkOpenVRRenderWindow);

vtkCxxSetObjectMacro(vtkOpenVRRenderWindow, DashboardOverlay, vtkOpenVROverlay);

//------------------------------------------------------------------------------
vtkOpenVRRenderWindow::vtkOpenVRRenderWindow()
{
  this->SetPhysicalViewDirection(0.0, 0.0, -1.0);
  this->SetPhysicalViewUp(0.0, 1.0, 0.0);
  this->SetPhysicalTranslation(0.0, 0.0, 0.0);
  this->PhysicalScale = 1.0;

  this->TrackHMD = true;

  this->StereoCapableWindow = 1;
  this->StereoRender = 1;
  this->UseOffScreenBuffers = 1;
  this->Size[0] = 640;
  this->Size[1] = 720;
  this->Position[0] = 100;
  this->Position[1] = 100;
  this->OpenVRRenderModels = nullptr;
  this->HMD = nullptr;
  this->HMDTransform = vtkTransform::New();
  memset(this->TrackedDeviceToRenderModel, 0, sizeof(this->TrackedDeviceToRenderModel));

#ifdef WIN32
  this->HelperWindow = vtkWin32OpenGLRenderWindow::New();
#endif
#ifdef VTK_USE_X
  this->HelperWindow = vtkXOpenGLRenderWindow::New();
#endif
#ifdef VTK_USE_COCOA
  this->HelperWindow = vtkCocoaRenderWindow::New();
#endif

  this->DashboardOverlay = vtkOpenVRDefaultOverlay::New();
}

//------------------------------------------------------------------------------
vtkOpenVRRenderWindow::~vtkOpenVRRenderWindow()
{
  if (this->DashboardOverlay)
  {
    this->DashboardOverlay->Delete();
    this->DashboardOverlay = 0;
  }
  this->Finalize();

  vtkRenderer* ren;
  vtkCollectionSimpleIterator rit;
  this->Renderers->InitTraversal(rit);
  while ((ren = this->Renderers->GetNextRenderer(rit)))
  {
    ren->SetRenderWindow(nullptr);
  }
  this->HMDTransform->Delete();
  this->HMDTransform = 0;

  if (this->HelperWindow)
  {
    this->HelperWindow->Delete();
    this->HelperWindow = 0;
  }
}

// ----------------------------------------------------------------------------
void vtkOpenVRRenderWindow::ReleaseGraphicsResources(vtkWindow* renWin)
{
  // this->HelperWindow->ReleaseGraphicsResources(renWin);
  this->Superclass::ReleaseGraphicsResources(renWin);
  for (std::vector<vtkOpenVRModel*>::iterator i = this->VTKRenderModels.begin();
       i != this->VTKRenderModels.end(); ++i)
  {
    (*i)->ReleaseGraphicsResources(renWin);
  }
}

//------------------------------------------------------------------------------
void vtkOpenVRRenderWindow::SetHelperWindow(vtkOpenGLRenderWindow* win)
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

//----------------------------------------------------------------------------
// Create an interactor that will work with this renderer.
vtkRenderWindowInteractor* vtkOpenVRRenderWindow::MakeRenderWindowInteractor()
{
  this->Interactor = vtkOpenVRRenderWindowInteractor::New();
  this->Interactor->SetRenderWindow(this);
  return this->Interactor;
}

//------------------------------------------------------------------------------
void vtkOpenVRRenderWindow::InitializeViewFromCamera(vtkCamera* srccam)
{
  vtkRenderer* ren = static_cast<vtkRenderer*>(this->GetRenderers()->GetItemAsObject(0));
  if (!ren)
  {
    vtkErrorMacro("The renderer must be set prior to calling InitializeViewFromCamera");
    return;
  }

  vtkOpenVRCamera* cam = static_cast<vtkOpenVRCamera*>(ren->GetActiveCamera());
  if (!cam)
  {
    vtkErrorMacro(
      "The renderer's active camera must be set prior to calling InitializeViewFromCamera");
    return;
  }

  // make sure the view up is reasonable based on the view up
  // that was set in PV
  double distance = sin(vtkMath::RadiansFromDegrees(srccam->GetViewAngle()) / 2.0) *
    srccam->GetDistance() / sin(vtkMath::RadiansFromDegrees(cam->GetViewAngle()) / 2.0);

  double* oldVup = srccam->GetViewUp();
  int maxIdx = fabs(oldVup[0]) > fabs(oldVup[1]) ? (fabs(oldVup[0]) > fabs(oldVup[2]) ? 0 : 2)
                                                 : (fabs(oldVup[1]) > fabs(oldVup[2]) ? 1 : 2);

  cam->SetViewUp((maxIdx == 0 ? (oldVup[0] > 0 ? 1 : -1) : 0.0),
    (maxIdx == 1 ? (oldVup[1] > 0 ? 1 : -1) : 0.0), (maxIdx == 2 ? (oldVup[2] > 0 ? 1 : -1) : 0.0));
  this->SetPhysicalViewUp((maxIdx == 0 ? (oldVup[0] > 0 ? 1 : -1) : 0.0),
    (maxIdx == 1 ? (oldVup[1] > 0 ? 1 : -1) : 0.0), (maxIdx == 2 ? (oldVup[2] > 0 ? 1 : -1) : 0.0));

  double* oldFP = srccam->GetFocalPoint();
  double* cvup = cam->GetViewUp();
  cam->SetFocalPoint(oldFP);
  this->SetPhysicalTranslation(
    cvup[0] * distance - oldFP[0], cvup[1] * distance - oldFP[1], cvup[2] * distance - oldFP[2]);
  this->SetPhysicalScale(distance);

  double* oldDOP = srccam->GetDirectionOfProjection();
  int dopMaxIdx = fabs(oldDOP[0]) > fabs(oldDOP[1]) ? (fabs(oldDOP[0]) > fabs(oldDOP[2]) ? 0 : 2)
                                                    : (fabs(oldDOP[1]) > fabs(oldDOP[2]) ? 1 : 2);
  this->SetPhysicalViewDirection((dopMaxIdx == 0 ? (oldDOP[0] > 0 ? 1 : -1) : 0.0),
    (dopMaxIdx == 1 ? (oldDOP[1] > 0 ? 1 : -1) : 0.0),
    (dopMaxIdx == 2 ? (oldDOP[2] > 0 ? 1 : -1) : 0.0));
  double* idop = this->GetPhysicalViewDirection();
  cam->SetPosition(
    -idop[0] * distance + oldFP[0], -idop[1] * distance + oldFP[1], -idop[2] * distance + oldFP[2]);

  ren->ResetCameraClippingRange();
}

//------------------------------------------------------------------------------
// Purpose: Helper to get a string from a tracked device property and turn it
//      into a std::string
std::string vtkOpenVRRenderWindow::GetTrackedDeviceString(vr::IVRSystem* pHmd,
  vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop,
  vr::TrackedPropertyError* peError)
{
  uint32_t unRequiredBufferLen =
    pHmd->GetStringTrackedDeviceProperty(unDevice, prop, nullptr, 0, peError);
  if (unRequiredBufferLen == 0)
  {
    return "";
  }

  char* pchBuffer = new char[unRequiredBufferLen];
  unRequiredBufferLen =
    pHmd->GetStringTrackedDeviceProperty(unDevice, prop, pchBuffer, unRequiredBufferLen, peError);
  std::string sResult = pchBuffer;
  delete[] pchBuffer;
  return sResult;
}

//------------------------------------------------------------------------------
// Purpose: Finds a render model we've already loaded or loads a new one
vtkOpenVRModel* vtkOpenVRRenderWindow::FindOrLoadRenderModel(const char* pchRenderModelName)
{
  // create the model
  vtkOpenVRModel* pRenderModel = vtkOpenVRModel::New();
  pRenderModel->SetName(pchRenderModelName);

  // start loading the model
  auto status = vr::VRRenderModels()->LoadRenderModel_Async(
    pRenderModel->GetName().c_str(), &pRenderModel->RawModel);
  if (status == vr::EVRRenderModelError::VRRenderModelError_NoShapes)
  {
    pRenderModel->SetVisibility(false);
    this->VTKRenderModels.push_back(pRenderModel);

    return pRenderModel;
  }

  if (status > vr::EVRRenderModelError::VRRenderModelError_Loading)
  {
    vtkErrorMacro(
      "Unable to load render model " << pRenderModel->GetName() << " with status " << status);
    pRenderModel->Delete();
    return nullptr; // move on to the next tracked device
  }

  pRenderModel->SetVisibility(true);
  this->VTKRenderModels.push_back(pRenderModel);

  return pRenderModel;
}

//------------------------------------------------------------------------------
void vtkOpenVRRenderWindow::RenderModels()
{
  vtkOpenGLState* ostate = this->GetState();
  ostate->vtkglEnable(GL_DEPTH_TEST);

  // for each device
  for (uint32_t unTrackedDevice = vr::k_unTrackedDeviceIndex_Hmd + 1;
       unTrackedDevice < vr::k_unMaxTrackedDeviceCount; unTrackedDevice++)
  {
    // is it not connected?
    if (!this->HMD->IsTrackedDeviceConnected(unTrackedDevice))
    {
      continue;
    }
    // do we not have a model loaded yet? try loading one
    if (!this->TrackedDeviceToRenderModel[unTrackedDevice])
    {
      std::string sRenderModelName =
        this->GetTrackedDeviceString(this->HMD, unTrackedDevice, vr::Prop_RenderModelName_String);
      vtkOpenVRModel* pRenderModel = this->FindOrLoadRenderModel(sRenderModelName.c_str());
      if (pRenderModel)
      {
        this->TrackedDeviceToRenderModel[unTrackedDevice] = pRenderModel;
        pRenderModel->TrackedDevice = unTrackedDevice;
      }
    }
    // if we still have no model or it is not set to show
    if (!this->TrackedDeviceToRenderModel[unTrackedDevice] ||
      !this->TrackedDeviceToRenderModel[unTrackedDevice]->GetVisibility())
    {
      continue;
    }
    // is the model's pose not valid?
    const vr::TrackedDevicePose_t& pose = this->TrackedDevicePose[unTrackedDevice];
    if (!pose.bPoseIsValid)
    {
      continue;
    }

    this->TrackedDeviceToRenderModel[unTrackedDevice]->Render(this, pose);
  }
}

// ----------------------------------------------------------------------------
void vtkOpenVRRenderWindow::MakeCurrent()
{
  if (this->HelperWindow)
  {
    this->HelperWindow->MakeCurrent();
  }
}

//------------------------------------------------------------------------------
vtkOpenGLState* vtkOpenVRRenderWindow::GetState()
{
  if (this->HelperWindow)
  {
    return this->HelperWindow->GetState();
  }
  return this->Superclass::GetState();
}

// ----------------------------------------------------------------------------
// Description:
// Tells if this window is the current OpenGL context for the calling thread.
bool vtkOpenVRRenderWindow::IsCurrent()
{
  return this->HelperWindow ? this->HelperWindow->IsCurrent() : false;
}

// ----------------------------------------------------------------------------
void vtkOpenVRRenderWindow::SetSize(int width, int height)
{
  if ((this->Size[0] != width) || (this->Size[1] != height))
  {
    this->Superclass::SetSize(width, height);

    if (this->Interactor)
    {
      this->Interactor->SetSize(width, height);
    }
  }
}

//------------------------------------------------------------------------------
// Get the size of the whole screen.
int* vtkOpenVRRenderWindow::GetScreenSize(void)
{
  if (this->HMD)
  {
    uint32_t renderWidth = 0;
    uint32_t renderHeight = 0;
    this->HMD->GetRecommendedRenderTargetSize(&renderWidth, &renderHeight);
    this->ScreenSize[0] = renderWidth;
    this->ScreenSize[1] = renderHeight;
  }

  return this->Size;
}

//------------------------------------------------------------------------------
void vtkOpenVRRenderWindow::SetPosition(int x, int y)
{
  if ((this->Position[0] != x) || (this->Position[1] != y))
  {
    this->Modified();
    this->Position[0] = x;
    this->Position[1] = y;
  }
}

//------------------------------------------------------------------------------
void vtkOpenVRRenderWindow::UpdateHMDMatrixPose()
{
  if (!this->HMD)
  {
    return;
  }
  vr::VRCompositor()->WaitGetPoses(
    this->TrackedDevicePose, vr::k_unMaxTrackedDeviceCount, nullptr, 0);

  // update the camera values based on the pose
  if (this->TrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid)
  {
    vtkRenderer* ren;
    vtkCollectionSimpleIterator rit;
    this->Renderers->InitTraversal(rit);
    while ((ren = this->Renderers->GetNextRenderer(rit)))
    {
      vtkOpenVRCamera* cam = static_cast<vtkOpenVRCamera*>(ren->GetActiveCamera());
      this->HMDTransform->Identity();

      // get the position and orientation of the HMD
      vr::TrackedDevicePose_t& tdPose = this->TrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd];

      // Note: Scaling is applied through moving the camera closer to the focal point, because
      // scaling of all actors is not feasible, and vtkCamera::ModelTransformMatrix is not supported
      // throughout VTK (clipping issues etc.). To achieve this, a new coordinate system called
      // NonScaledWorld is introduced. The relationship between Physical (in which the HMD pose
      // is given by OpenVR) and NonScaledWorld is described by the PhysicalViewUp etc. member
      // variables. After getting the HMD pose in Physical, those coordinates and axes are converted
      // to the NonScaledWorld coordinate system, on which the PhysicalScaling trick of modifying
      // the camera position is applied, resulting the World coordinate system.

      // construct physical to non-scaled world axes (scaling is used later to move camera closer)
      double physicalZ_NonscaledWorld[3] = { -this->PhysicalViewDirection[0],
        -this->PhysicalViewDirection[1], -this->PhysicalViewDirection[2] };
      double* physicalY_NonscaledWorld = this->PhysicalViewUp;
      double physicalX_NonscaledWorld[3] = { 0.0 };
      vtkMath::Cross(physicalY_NonscaledWorld, physicalZ_NonscaledWorld, physicalX_NonscaledWorld);

      // extract HMD axes and position
      double hmdX_Physical[3] = { tdPose.mDeviceToAbsoluteTracking.m[0][0],
        tdPose.mDeviceToAbsoluteTracking.m[1][0], tdPose.mDeviceToAbsoluteTracking.m[2][0] };
      double hmdY_Physical[3] = { tdPose.mDeviceToAbsoluteTracking.m[0][1],
        tdPose.mDeviceToAbsoluteTracking.m[1][1], tdPose.mDeviceToAbsoluteTracking.m[2][1] };
      double hmdPosition_Physical[3] = { tdPose.mDeviceToAbsoluteTracking.m[0][3],
        tdPose.mDeviceToAbsoluteTracking.m[1][3], tdPose.mDeviceToAbsoluteTracking.m[2][3] };

      // convert position to non-scaled world coordinates
      double hmdPosition_NonscaledWorld[3];
      hmdPosition_NonscaledWorld[0] = hmdPosition_Physical[0] * physicalX_NonscaledWorld[0] +
        hmdPosition_Physical[1] * physicalY_NonscaledWorld[0] +
        hmdPosition_Physical[2] * physicalZ_NonscaledWorld[0];
      hmdPosition_NonscaledWorld[1] = hmdPosition_Physical[0] * physicalX_NonscaledWorld[1] +
        hmdPosition_Physical[1] * physicalY_NonscaledWorld[1] +
        hmdPosition_Physical[2] * physicalZ_NonscaledWorld[1];
      hmdPosition_NonscaledWorld[2] = hmdPosition_Physical[0] * physicalX_NonscaledWorld[2] +
        hmdPosition_Physical[1] * physicalY_NonscaledWorld[2] +
        hmdPosition_Physical[2] * physicalZ_NonscaledWorld[2];
      // now adjust for scale and translation
      double hmdPosition_World[3] = { 0.0 };
      for (int i = 0; i < 3; i++)
      {
        hmdPosition_World[i] =
          hmdPosition_NonscaledWorld[i] * this->PhysicalScale - this->PhysicalTranslation[i];
      }

      // convert axes to non-scaled world coordinate system
      double hmdX_NonscaledWorld[3] = { hmdX_Physical[0] * physicalX_NonscaledWorld[0] +
          hmdX_Physical[1] * physicalY_NonscaledWorld[0] +
          hmdX_Physical[2] * physicalZ_NonscaledWorld[0],
        hmdX_Physical[0] * physicalX_NonscaledWorld[1] +
          hmdX_Physical[1] * physicalY_NonscaledWorld[1] +
          hmdX_Physical[2] * physicalZ_NonscaledWorld[1],
        hmdX_Physical[0] * physicalX_NonscaledWorld[2] +
          hmdX_Physical[1] * physicalY_NonscaledWorld[2] +
          hmdX_Physical[2] * physicalZ_NonscaledWorld[2] };
      double hmdY_NonscaledWorld[3] = { hmdY_Physical[0] * physicalX_NonscaledWorld[0] +
          hmdY_Physical[1] * physicalY_NonscaledWorld[0] +
          hmdY_Physical[2] * physicalZ_NonscaledWorld[0],
        hmdY_Physical[0] * physicalX_NonscaledWorld[1] +
          hmdY_Physical[1] * physicalY_NonscaledWorld[1] +
          hmdY_Physical[2] * physicalZ_NonscaledWorld[1],
        hmdY_Physical[0] * physicalX_NonscaledWorld[2] +
          hmdY_Physical[1] * physicalY_NonscaledWorld[2] +
          hmdY_Physical[2] * physicalZ_NonscaledWorld[2] };
      double hmdZ_NonscaledWorld[3] = { 0.0 };
      vtkMath::Cross(hmdY_NonscaledWorld, hmdX_NonscaledWorld, hmdZ_NonscaledWorld);

      cam->SetPosition(hmdPosition_World);
      cam->SetFocalPoint(hmdPosition_World[0] + hmdZ_NonscaledWorld[0] * this->PhysicalScale,
        hmdPosition_World[1] + hmdZ_NonscaledWorld[1] * this->PhysicalScale,
        hmdPosition_World[2] + hmdZ_NonscaledWorld[2] * this->PhysicalScale);
      cam->SetViewUp(hmdY_NonscaledWorld);

      ren->UpdateLightsGeometryToFollowCamera();
    }
  }
}

//------------------------------------------------------------------------------
void vtkOpenVRRenderWindow::Render()
{
  if (this->TrackHMD)
  {
    this->UpdateHMDMatrixPose();
  }
  else
  {
    vr::VRCompositor()->WaitGetPoses(
      this->TrackedDevicePose, vr::k_unMaxTrackedDeviceCount, nullptr, 0);
  }

  this->MakeCurrent();
  this->GetState()->ResetGLViewportState();
  this->Superclass::Render();
}

//------------------------------------------------------------------------------
void vtkOpenVRRenderWindow::StereoUpdate() {}

//------------------------------------------------------------------------------
void vtkOpenVRRenderWindow::StereoMidpoint()
{
  // render the left eye models
  this->RenderModels();

  this->GetState()->vtkglDisable(GL_MULTISAMPLE);

  if (this->HMD && this->SwapBuffers) // picking does not swap and we don't show it
  {
    this->GetState()->PushDrawFramebufferBinding();
    this->GetState()->vtkglBindFramebuffer(
      GL_DRAW_FRAMEBUFFER, this->LeftEyeDesc.m_nResolveFramebufferId);

    glBlitFramebuffer(0, 0, this->Size[0], this->Size[1], 0, 0, this->Size[0], this->Size[1],
      GL_COLOR_BUFFER_BIT, GL_LINEAR);

    vr::Texture_t leftEyeTexture = { (void*)(long)this->LeftEyeDesc.m_nResolveTextureId,
      vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
    vr::VRCompositor()->Submit(vr::Eye_Left, &leftEyeTexture);
    this->GetState()->PopDrawFramebufferBinding();
  }
}

//------------------------------------------------------------------------------
void vtkOpenVRRenderWindow::StereoRenderComplete()
{
  // render the right eye models
  this->RenderModels();

  // reset the camera to a neutral position
  vtkRenderer* ren = static_cast<vtkRenderer*>(this->GetRenderers()->GetItemAsObject(0));
  if (ren && !ren->GetSelector())
  {
    vtkOpenVRCamera* cam = static_cast<vtkOpenVRCamera*>(ren->GetActiveCamera());
    cam->ApplyEyePose(this, false, -1.0);
  }

  this->GetState()->vtkglDisable(GL_MULTISAMPLE);

  // for now as fast as possible
  if (this->HMD && this->SwapBuffers) // picking does not swap and we don't show it
  {
    this->GetState()->PushDrawFramebufferBinding();
    this->GetState()->vtkglBindFramebuffer(
      GL_DRAW_FRAMEBUFFER, this->RightEyeDesc.m_nResolveFramebufferId);

    glBlitFramebuffer(0, 0, this->Size[0], this->Size[1], 0, 0, this->Size[0], this->Size[1],
      GL_COLOR_BUFFER_BIT, GL_LINEAR);

    vr::Texture_t rightEyeTexture = { (void*)(long)this->RightEyeDesc.m_nResolveTextureId,
      vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
    vr::VRCompositor()->Submit(vr::Eye_Right, &rightEyeTexture);
    this->GetState()->PopDrawFramebufferBinding();
  }
}

//------------------------------------------------------------------------------
bool vtkOpenVRRenderWindow::CreateFrameBuffer(
  int nWidth, int nHeight, FramebufferDesc& framebufferDesc)
{
  glGenFramebuffers(1, &framebufferDesc.m_nResolveFramebufferId);
  this->GetState()->vtkglBindFramebuffer(GL_FRAMEBUFFER, framebufferDesc.m_nResolveFramebufferId);

  glGenTextures(1, &framebufferDesc.m_nResolveTextureId);
  glBindTexture(GL_TEXTURE_2D, framebufferDesc.m_nResolveTextureId);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, nWidth, nHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  glFramebufferTexture2D(
    GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebufferDesc.m_nResolveTextureId, 0);

  // check FBO status
  GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if (status != GL_FRAMEBUFFER_COMPLETE)
  {
    return false;
  }

  this->GetState()->vtkglBindFramebuffer(GL_FRAMEBUFFER, 0);

  return true;
}

//------------------------------------------------------------------------------
// Initialize the rendering window.
void vtkOpenVRRenderWindow::Initialize(void)
{
  // Loading the SteamVR Runtime
  vr::EVRInitError eError = vr::VRInitError_None;
  this->HMD = vr::VR_Init(&eError, vr::VRApplication_Scene);

  if (eError != vr::VRInitError_None)
  {
    this->HMD = nullptr;
    char buf[1024];
    snprintf(buf, sizeof(buf), "Unable to init VR runtime: %s",
      vr::VR_GetVRInitErrorAsEnglishDescription(eError));
    vtkErrorMacro(<< "VR_Init Failed" << buf);
    return;
  }

  this->OpenVRRenderModels =
    (vr::IVRRenderModels*)vr::VR_GetGenericInterface(vr::IVRRenderModels_Version, &eError);
  if (!this->OpenVRRenderModels)
  {
    this->HMD = nullptr;
    vr::VR_Shutdown();

    char buf[1024];
    snprintf(buf, sizeof(buf), "Unable to get render model interface: %s",
      vr::VR_GetVRInitErrorAsEnglishDescription(eError));
    vtkErrorMacro(<< "VR_Init Failed" << buf);
    return;
  }

  uint32_t renderWidth;
  uint32_t renderHeight;
  this->HMD->GetRecommendedRenderTargetSize(&renderWidth, &renderHeight);

  this->Size[0] = renderWidth;
  this->Size[1] = renderHeight;

  this->HelperWindow->SetDisplayId(this->GetGenericDisplayId());
  this->HelperWindow->SetShowWindow(false);
  this->HelperWindow->Initialize();

  this->MakeCurrent();

  this->OpenGLInit();

  // some classes override the ivar in a getter :-(
  this->MaximumHardwareLineWidth = this->HelperWindow->GetMaximumHardwareLineWidth();

  glDepthRange(0., 1.);

  // make sure vsync is off
  // this->HelperWindow->SetSwapControl(0);

  m_strDriver = "No Driver";
  m_strDisplay = "No Display";

  m_strDriver = GetTrackedDeviceString(
    this->HMD, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_TrackingSystemName_String);
  m_strDisplay =
    GetTrackedDeviceString(this->HMD, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_SerialNumber_String);

  std::string strWindowTitle = "VTK - " + m_strDriver + " " + m_strDisplay;
  this->SetWindowName(strWindowTitle.c_str());

  this->CreateFrameBuffer(this->Size[0], this->Size[1], this->LeftEyeDesc);
  this->CreateFrameBuffer(this->Size[0], this->Size[1], this->RightEyeDesc);

  if (!vr::VRCompositor())
  {
    vtkErrorMacro("Compositor initialization failed.");
    return;
  }

  this->DashboardOverlay->Create(this);
}

//------------------------------------------------------------------------------
void vtkOpenVRRenderWindow::Finalize(void)
{
  this->ReleaseGraphicsResources(this);
  if (this->HMD)
  {
    vr::VR_Shutdown();
    this->HMD = nullptr;
  }

  for (std::vector<vtkOpenVRModel*>::iterator i = this->VTKRenderModels.begin();
       i != this->VTKRenderModels.end(); ++i)
  {
    (*i)->Delete();
  }
  this->VTKRenderModels.clear();

  if (this->HelperWindow && this->HelperWindow->GetGenericContext())
  {
    this->HelperWindow->Finalize();
  }
}

//------------------------------------------------------------------------------
void vtkOpenVRRenderWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ContextId: " << this->HelperWindow->GetGenericContext() << "\n";
  os << indent << "Window Id: " << this->HelperWindow->GetGenericWindowId() << "\n";
}

//------------------------------------------------------------------------------
// Add a renderer to the list of renderers.
void vtkOpenVRRenderWindow::AddRenderer(vtkRenderer* ren)
{
  if (ren && !vtkOpenVRRenderer::SafeDownCast(ren))
  {
    vtkErrorMacro("vtkOpenVRRenderWindow::AddRenderer: Failed to add renderer of type "
      << ren->GetClassName() << ": A vtkOpenVRRenderer is expected");
    return;
  }
  this->Superclass::AddRenderer(ren);
}

//------------------------------------------------------------------------------
// Begin the rendering process.
void vtkOpenVRRenderWindow::Start(void)
{
  // if the renderer has not been initialized, do so now
  if (this->HelperWindow && !this->HMD)
  {
    this->Initialize();
  }

  this->Superclass::Start();
}

//------------------------------------------------------------------------------
void vtkOpenVRRenderWindow::RenderOverlay()
{
  this->DashboardOverlay->Render();
}

//------------------------------------------------------------------------------
vr::TrackedDeviceIndex_t vtkOpenVRRenderWindow::GetTrackedDeviceIndexForDevice(
  vtkEventDataDevice dev, uint32_t index)
{
  if (dev == vtkEventDataDevice::HeadMountedDisplay)
  {
    return vr::k_unTrackedDeviceIndex_Hmd;
  }
  if (dev == vtkEventDataDevice::LeftController)
  {
    return this->HMD->GetTrackedDeviceIndexForControllerRole(vr::TrackedControllerRole_LeftHand);
  }
  if (dev == vtkEventDataDevice::RightController)
  {
    return this->HMD->GetTrackedDeviceIndexForControllerRole(vr::TrackedControllerRole_RightHand);
  }
  if (dev == vtkEventDataDevice::GenericTracker)
  {
    bool notDone = true;
    uint32_t arraySize(1024);
    vr::TrackedDeviceIndex_t* devices = new vr::TrackedDeviceIndex_t[arraySize];
    uint32_t deviceCount(0);
    while (notDone)
    {
      deviceCount = this->HMD->GetSortedTrackedDeviceIndicesOfClass(
        vr::TrackedDeviceClass_GenericTracker, devices, 1024);
      if (deviceCount > arraySize)
      {
        delete[] devices;
        arraySize *= 2;
        devices = new vr::TrackedDeviceIndex_t[arraySize];
        continue;
      }
      else
      {
        notDone = false;
      }
    }

    uint32_t devIndex = devices[index];
    delete[] devices;

    if (index > deviceCount)
    {
      return vr::k_unTrackedDeviceIndexInvalid;
    }

    return devIndex;
  }
  return vr::k_unTrackedDeviceIndexInvalid;
}

//------------------------------------------------------------------------------
uint32_t vtkOpenVRRenderWindow::GetNumberOfTrackedDevicesForDevice(vtkEventDataDevice)
{
  vr::TrackedDeviceIndex_t devices[1];
  return this->HMD->GetSortedTrackedDeviceIndicesOfClass(
    vr::TrackedDeviceClass_GenericTracker, devices, 1);
}

//------------------------------------------------------------------------------
vtkOpenVRModel* vtkOpenVRRenderWindow::GetTrackedDeviceModel(vtkEventDataDevice dev, uint32_t index)
{
  vr::TrackedDeviceIndex_t idx = this->GetTrackedDeviceIndexForDevice(dev, index);
  if (idx != vr::k_unTrackedDeviceIndexInvalid)
  {
    return this->GetTrackedDeviceModel(idx);
  }
  return nullptr;
}

//------------------------------------------------------------------------------
void vtkOpenVRRenderWindow::GetTrackedDevicePose(
  vtkEventDataDevice dev, uint32_t index, vr::TrackedDevicePose_t** pose)
{
  vr::TrackedDeviceIndex_t idx = this->GetTrackedDeviceIndexForDevice(dev, index);
  *pose = nullptr;
  if (idx < vr::k_unMaxTrackedDeviceCount)
  {
    *pose = &(this->TrackedDevicePose[idx]);
  }
}

//------------------------------------------------------------------------------
void vtkOpenVRRenderWindow::SetPhysicalViewDirection(double x, double y, double z)
{
  if (this->PhysicalViewDirection[0] != x || this->PhysicalViewDirection[1] != y ||
    this->PhysicalViewDirection[2] != z)
  {
    this->PhysicalViewDirection[0] = x;
    this->PhysicalViewDirection[1] = y;
    this->PhysicalViewDirection[2] = z;
    this->InvokeEvent(vtkOpenVRRenderWindow::PhysicalToWorldMatrixModified);
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkOpenVRRenderWindow::SetPhysicalViewDirection(double dir[3])
{
  this->SetPhysicalViewDirection(dir[0], dir[1], dir[2]);
}

//------------------------------------------------------------------------------
void vtkOpenVRRenderWindow::SetPhysicalViewUp(double x, double y, double z)
{
  if (this->PhysicalViewUp[0] != x || this->PhysicalViewUp[1] != y || this->PhysicalViewUp[2] != z)
  {
    this->PhysicalViewUp[0] = x;
    this->PhysicalViewUp[1] = y;
    this->PhysicalViewUp[2] = z;
    this->InvokeEvent(vtkOpenVRRenderWindow::PhysicalToWorldMatrixModified);
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkOpenVRRenderWindow::SetPhysicalViewUp(double dir[3])
{
  this->SetPhysicalViewUp(dir[0], dir[1], dir[2]);
}

//------------------------------------------------------------------------------
void vtkOpenVRRenderWindow::SetPhysicalTranslation(double x, double y, double z)
{
  if (this->PhysicalTranslation[0] != x || this->PhysicalTranslation[1] != y ||
    this->PhysicalTranslation[2] != z)
  {
    this->PhysicalTranslation[0] = x;
    this->PhysicalTranslation[1] = y;
    this->PhysicalTranslation[2] = z;
    this->InvokeEvent(vtkOpenVRRenderWindow::PhysicalToWorldMatrixModified);
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkOpenVRRenderWindow::SetPhysicalTranslation(double trans[3])
{
  this->SetPhysicalTranslation(trans[0], trans[1], trans[2]);
}

//------------------------------------------------------------------------------
void vtkOpenVRRenderWindow::SetPhysicalScale(double scale)
{
  if (this->PhysicalScale != scale)
  {
    this->PhysicalScale = scale;
    this->InvokeEvent(vtkOpenVRRenderWindow::PhysicalToWorldMatrixModified);
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkOpenVRRenderWindow::SetPhysicalToWorldMatrix(vtkMatrix4x4* matrix)
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

  this->InvokeEvent(vtkOpenVRRenderWindow::PhysicalToWorldMatrixModified);
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkOpenVRRenderWindow::GetPhysicalToWorldMatrix(vtkMatrix4x4* physicalToWorldMatrix)
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
