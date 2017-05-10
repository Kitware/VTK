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
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLIndexBufferObject.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLTexture.h"
#include "vtkOpenGLVertexArrayObject.h"
#include "vtkOpenGLVertexBufferObject.h"
#include "vtkOpenVRCamera.h"
#include "vtkOpenVRDefaultOverlay.h"
#include "vtkOpenVRModel.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRendererCollection.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkShaderProgram.h"
#include "vtkTextureObject.h"
#include "vtkTransform.h"

#include <cmath>
#include <sstream>

#include "vtkOpenGLError.h"

#if !defined(_WIN32) || defined(__CYGWIN__)
# define stricmp strcasecmp
#endif

vtkStandardNewMacro(vtkOpenVRRenderWindow);

vtkCxxSetObjectMacro(vtkOpenVRRenderWindow, DashboardOverlay, vtkOpenVROverlay);

vtkOpenVRRenderWindow::vtkOpenVRRenderWindow()
{
  this->SetInitialViewDirection(0.0, 0.0, -1.0);
  this->SetInitialViewUp(0.0, 1.0, 0.0);

  this->StereoCapableWindow = 1;
  this->StereoRender = 1;
  this->Size[0] = 640;
  this->Size[1] = 720;
  this->Position[0] = 100;
  this->Position[1] = 100;
  this->OpenVRRenderModels = NULL;
  this->HMD = NULL;
  this->HMDTransform = vtkTransform::New();
  this->ContextId = 0;
  this->WindowId = 0;
  memset(this->TrackedDeviceToRenderModel, 0, sizeof(this->TrackedDeviceToRenderModel));
  this->DashboardOverlay = vtkOpenVRDefaultOverlay::New();
}

vtkOpenVRRenderWindow::~vtkOpenVRRenderWindow()
{
  if (this->DashboardOverlay)
  {
    this->DashboardOverlay->Delete();
    this->DashboardOverlay = 0;
  }

  this->Finalize();

  vtkRenderer *ren;
  vtkCollectionSimpleIterator rit;
  this->Renderers->InitTraversal(rit);
  while ( (ren = this->Renderers->GetNextRenderer(rit)) )
  {
    ren->SetRenderWindow(NULL);
  }
  this->HMDTransform->Delete();
  this->HMDTransform = 0;
}

// ----------------------------------------------------------------------------
void vtkOpenVRRenderWindow::ReleaseGraphicsResources(vtkRenderWindow *renWin)
{
  this->Superclass::ReleaseGraphicsResources(renWin);
  for( std::vector< vtkOpenVRModel * >::iterator i = this->VTKRenderModels.begin();
       i != this->VTKRenderModels.end(); ++i )
  {
    (*i)->ReleaseGraphicsResources(renWin);
  }
}

void vtkOpenVRRenderWindow::InitializeViewFromCamera(vtkCamera *srccam)
{
  vtkRenderer *ren = static_cast<vtkRenderer *>(
    this->GetRenderers()->GetItemAsObject(0));
  if (!ren)
  {
    vtkErrorMacro("The renderer must be set prior to calling InitializeViewFromCamera");
    return;
  }

  vtkOpenVRCamera *cam =
    static_cast<vtkOpenVRCamera *>(ren->GetActiveCamera());
  if (!cam)
  {
    vtkErrorMacro("The renderer's active camera must be set prior to calling InitializeViewFromCamera");
    return;
  }

  // make sure the view up is reasonable based on the view up
  // that was set in PV
  double distance =
    sin(vtkMath::RadiansFromDegrees(srccam->GetViewAngle())/2.0) *
    srccam->GetDistance() /
    sin(vtkMath::RadiansFromDegrees(cam->GetViewAngle())/2.0);

  double *oldVup = srccam->GetViewUp();
  int maxIdx = fabs(oldVup[0]) > fabs(oldVup[1]) ?
    (fabs(oldVup[0]) > fabs(oldVup[2]) ? 0 : 2) :
    (fabs(oldVup[1]) > fabs(oldVup[2]) ? 1 : 2);

  cam->SetViewUp(
    (maxIdx == 0 ? (oldVup[0] > 0 ? 1 : -1) : 0.0),
    (maxIdx == 1 ? (oldVup[1] > 0 ? 1 : -1) : 0.0),
    (maxIdx == 2 ? (oldVup[2] > 0 ? 1 : -1) : 0.0));
  this->SetInitialViewUp(
    (maxIdx == 0 ? (oldVup[0] > 0 ? 1 : -1) : 0.0),
    (maxIdx == 1 ? (oldVup[1] > 0 ? 1 : -1) : 0.0),
    (maxIdx == 2 ? (oldVup[2] > 0 ? 1 : -1) : 0.0));

  double *oldFP = srccam->GetFocalPoint();
  double *cvup = cam->GetViewUp();
  cam->SetFocalPoint(oldFP);
  cam->SetTranslation(
      cvup[0]*distance - oldFP[0],
      cvup[1]*distance - oldFP[1],
      cvup[2]*distance - oldFP[2]);

  double *oldDOP = srccam->GetDirectionOfProjection();
  int dopMaxIdx = fabs(oldDOP[0]) > fabs(oldDOP[1]) ?
    (fabs(oldDOP[0]) > fabs(oldDOP[2]) ? 0 : 2) :
    (fabs(oldDOP[1]) > fabs(oldDOP[2]) ? 1 : 2);
  this->SetInitialViewDirection(
    (dopMaxIdx == 0 ? (oldDOP[0] > 0 ? 1 : -1) : 0.0),
    (dopMaxIdx == 1 ? (oldDOP[1] > 0 ? 1 : -1) : 0.0),
    (dopMaxIdx == 2 ? (oldDOP[2] > 0 ? 1 : -1) : 0.0));
  double *idop = this->GetInitialViewDirection();
  cam->SetPosition(
    -idop[0]*distance + oldFP[0],
    -idop[1]*distance + oldFP[1],
    -idop[2]*distance + oldFP[2]);

  ren->ResetCameraClippingRange();
}

// Purpose: Helper to get a string from a tracked device property and turn it
//      into a std::string
std::string vtkOpenVRRenderWindow::GetTrackedDeviceString(
  vr::IVRSystem *pHmd,
  vr::TrackedDeviceIndex_t unDevice,
  vr::TrackedDeviceProperty prop,
  vr::TrackedPropertyError *peError)
{
  uint32_t unRequiredBufferLen =
    pHmd->GetStringTrackedDeviceProperty( unDevice, prop, NULL, 0, peError );
  if( unRequiredBufferLen == 0 )
  {
    return "";
  }

  char *pchBuffer = new char[ unRequiredBufferLen ];
  unRequiredBufferLen =
    pHmd->GetStringTrackedDeviceProperty( unDevice, prop, pchBuffer, unRequiredBufferLen, peError );
  std::string sResult = pchBuffer;
  delete [] pchBuffer;
  return sResult;
}

// Purpose: Finds a render model we've already loaded or loads a new one
vtkOpenVRModel *vtkOpenVRRenderWindow::FindOrLoadRenderModel(
  const char *pchRenderModelName )
{
  // create the model
  vtkOpenVRModel *pRenderModel = vtkOpenVRModel::New();
  pRenderModel->SetName(pchRenderModelName);

  // start loading the model
  if (vr::VRRenderModels()->LoadRenderModel_Async(
      pRenderModel->GetName().c_str(), &pRenderModel->RawModel)
      > vr::EVRRenderModelError::VRRenderModelError_Loading)
  {
    vtkErrorMacro("Unable to load render model " << pRenderModel->GetName() );
    pRenderModel->Delete();
    return NULL; // move on to the next tracked device
  }

  pRenderModel->SetShow(true);
  this->VTKRenderModels.push_back( pRenderModel );

  return pRenderModel;
}

void vtkOpenVRRenderWindow::RenderModels()
{
  bool bIsInputCapturedByAnotherProcess =
    this->HMD->IsInputFocusCapturedByAnotherProcess();

  // for each device
  for (uint32_t unTrackedDevice = vr::k_unTrackedDeviceIndex_Hmd + 1;
       unTrackedDevice < vr::k_unMaxTrackedDeviceCount; unTrackedDevice++ )
  {
    // is it not connected?
    if (!this->HMD->IsTrackedDeviceConnected( unTrackedDevice ) )
    {
      continue;
    }
    // do we not have a model loaded yet? try loading one
    if (!this->TrackedDeviceToRenderModel[ unTrackedDevice ])
    {
      std::string sRenderModelName =
        this->GetTrackedDeviceString(this->HMD, unTrackedDevice, vr::Prop_RenderModelName_String );
      vtkOpenVRModel *pRenderModel = this->FindOrLoadRenderModel( sRenderModelName.c_str() );
      if( pRenderModel )
      {
        this->TrackedDeviceToRenderModel[ unTrackedDevice ] = pRenderModel;
      }
    }
    // if we still have no model or it is not set to show
    if( !this->TrackedDeviceToRenderModel[ unTrackedDevice ]
      || !this->TrackedDeviceToRenderModel[ unTrackedDevice ]->GetShow())
    {
      continue;
    }
    // is the model's pose not valid?
    const vr::TrackedDevicePose_t &pose = this->TrackedDevicePose[ unTrackedDevice ];
    if( !pose.bPoseIsValid )
    {
      continue;
    }

    if( bIsInputCapturedByAnotherProcess &&
        this->HMD->GetTrackedDeviceClass( unTrackedDevice ) == vr::TrackedDeviceClass_Controller )
    {
      continue;
    }

    this->TrackedDeviceToRenderModel[ unTrackedDevice ]->Render(this, pose);
  }
}

void vtkOpenVRRenderWindow::Clean()
{
  /* finish OpenGL rendering */
  if (this->OwnContext && this->ContextId)
  {
    this->MakeCurrent();
    this->ReleaseGraphicsResources(this);
  }

  this->ContextId = NULL;
}

// ----------------------------------------------------------------------------
void vtkOpenVRRenderWindow::MakeCurrent()
{
  SDL_GL_MakeCurrent(this->WindowId, this->ContextId);
}

// ----------------------------------------------------------------------------
// Description:
// Tells if this window is the current OpenGL context for the calling thread.
bool vtkOpenVRRenderWindow::IsCurrent()
{
  return this->ContextId!=0 && this->ContextId==SDL_GL_GetCurrentContext();
}


// ----------------------------------------------------------------------------
void vtkOpenVRRenderWindow::SetSize(int x, int y)
{
  static int resizing = 0;
  if ((this->Size[0] != x) || (this->Size[1] != y))
  {
    this->Superclass::SetSize(x, y);

    if (this->Interactor)
    {
      this->Interactor->SetSize(x, y);
    }

    if (this->Mapped)
    {
      if (!resizing)
      {
        resizing = 1;
        SDL_SetWindowSize(this->WindowId, this->Size[0], this->Size[1]);
        resizing = 0;
      }
    }
  }
}

// Get the size of the whole screen.
int *vtkOpenVRRenderWindow::GetScreenSize(void)
{
  return this->Size;
}


void vtkOpenVRRenderWindow::SetPosition(int x, int y)
{
  static int resizing = 0;

  if ((this->Position[0] != x) || (this->Position[1] != y))
  {
    this->Modified();
    this->Position[0] = x;
    this->Position[1] = y;
    if (this->Mapped)
    {
      if (!resizing)
      {
        resizing = 1;
        SDL_SetWindowPosition(this->WindowId,x,y);
        resizing = 0;
      }
    }
  }
}

void vtkOpenVRRenderWindow::UpdateHMDMatrixPose()
{
  if (!this->HMD)
  {
    return;
  }
  vr::VRCompositor()->WaitGetPoses(this->TrackedDevicePose,
    vr::k_unMaxTrackedDeviceCount, NULL, 0 );

  // update the camera values based on the pose
  if ( this->TrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid )
  {
    vtkRenderer *ren;
    vtkCollectionSimpleIterator rit;
    this->Renderers->InitTraversal(rit);
    while ( (ren = this->Renderers->GetNextRenderer(rit)) )
    {
      vtkOpenVRCamera *cam = static_cast<vtkOpenVRCamera *>(ren->GetActiveCamera());
      this->HMDTransform->Identity();

      // get the position and orientation of the HMD
      vr::TrackedDevicePose_t &tdPose =
        this->TrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd];
      double pos[3];

      // Vive to world axes
      double *vup = this->InitialViewUp;
      double *dop = this->InitialViewDirection;
      double vright[3];
      vtkMath::Cross(dop, vup, vright);

      // extract HMD axes
      double hvright[3];
      hvright[0] = tdPose.mDeviceToAbsoluteTracking.m[0][0];
      hvright[1] = tdPose.mDeviceToAbsoluteTracking.m[1][0];
      hvright[2] = tdPose.mDeviceToAbsoluteTracking.m[2][0];
      double hvup[3];
      hvup[0] = tdPose.mDeviceToAbsoluteTracking.m[0][1];
      hvup[1] = tdPose.mDeviceToAbsoluteTracking.m[1][1];
      hvup[2] = tdPose.mDeviceToAbsoluteTracking.m[2][1];

      pos[0] = tdPose.mDeviceToAbsoluteTracking.m[0][3];
      pos[1] = tdPose.mDeviceToAbsoluteTracking.m[1][3];
      pos[2] = tdPose.mDeviceToAbsoluteTracking.m[2][3];

      double distance = cam->GetDistance();
      double *trans = cam->GetTranslation();

      // convert position to world coordinates
      double npos[3];
      npos[0] = pos[0]*vright[0] + pos[1]*vup[0] - pos[2]*dop[0];
      npos[1] = pos[0]*vright[1] + pos[1]*vup[1] - pos[2]*dop[1];
      npos[2] = pos[0]*vright[2] + pos[1]*vup[2] - pos[2]*dop[2];
      // now adjust for scale and translation
      for (int i = 0; i < 3; i++)
      {
        pos[i] = npos[i]*distance - trans[i];
      }

      // convert axes to world coordinates
      double fvright[3]; // final vright
      fvright[0] = hvright[0]*vright[0] + hvright[1]*vup[0] - hvright[2]*dop[0];
      fvright[1] = hvright[0]*vright[1] + hvright[1]*vup[1] - hvright[2]*dop[1];
      fvright[2] = hvright[0]*vright[2] + hvright[1]*vup[2] - hvright[2]*dop[2];
      double fvup[3]; // final vup
      fvup[0] = hvup[0]*vright[0] + hvup[1]*vup[0] - hvup[2]*dop[0];
      fvup[1] = hvup[0]*vright[1] + hvup[1]*vup[1] - hvup[2]*dop[1];
      fvup[2] = hvup[0]*vright[2] + hvup[1]*vup[2] - hvup[2]*dop[2];
      double fdop[3];
      vtkMath::Cross(fvup, fvright, fdop);

      cam->SetPosition(pos);
      cam->SetFocalPoint(
        pos[0] + fdop[0]*distance,
        pos[1] + fdop[1]*distance,
        pos[2] + fdop[2]*distance);
      cam->SetViewUp(fvup);
      ren->UpdateLightsGeometryToFollowCamera();
    }
  }
}

void vtkOpenVRRenderWindow::Render()
{
  this->UpdateHMDMatrixPose();
  this->vtkRenderWindow::Render();
}

void vtkOpenVRRenderWindow::StereoUpdate()
{
  // camera handles what we need
}

void vtkOpenVRRenderWindow::StereoMidpoint()
{
  // render the left eye models
  this->RenderModels();

  glBindFramebuffer( GL_FRAMEBUFFER, 0 );
  glDisable( GL_MULTISAMPLE );

  glBindFramebuffer(GL_READ_FRAMEBUFFER, this->LeftEyeDesc.m_nRenderFramebufferId);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->LeftEyeDesc.m_nResolveFramebufferId );

  glBlitFramebuffer(0, 0, this->Size[0], this->Size[1],
    0, 0, this->Size[0], this->Size[1],
    GL_COLOR_BUFFER_BIT,
    GL_LINEAR);

  glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0 );
}

void  vtkOpenVRRenderWindow::StereoRenderComplete()
{
  // render the right eye models
  this->RenderModels();

  glBindFramebuffer( GL_FRAMEBUFFER, 0 );
  glDisable( GL_MULTISAMPLE );

  glBindFramebuffer(GL_READ_FRAMEBUFFER, this->RightEyeDesc.m_nRenderFramebufferId );
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->RightEyeDesc.m_nResolveFramebufferId );

  glBlitFramebuffer(0, 0, this->Size[0], this->Size[1],
    0, 0, this->Size[0], this->Size[1],
    GL_COLOR_BUFFER_BIT,
    GL_LINEAR);

  glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0 );

  // reset the camera to a neutral position
  vtkRenderer *ren = static_cast<vtkRenderer *>(
    this->GetRenderers()->GetItemAsObject(0));
  if (ren)
  {
    vtkOpenVRCamera *cam =
      static_cast<vtkOpenVRCamera *>(ren->GetActiveCamera());
    cam->ApplyEyePose(false, -1.0);
  }
}

// End the rendering process and display the image.
void vtkOpenVRRenderWindow::Frame(void)
{
  this->MakeCurrent();
  if (!this->AbortRender && this->DoubleBuffer && this->SwapBuffers)
  {
    // for now as fast as possible
    if ( this->HMD )
    {
      vr::Texture_t leftEyeTexture = {(void*)(long)this->LeftEyeDesc.m_nResolveTextureId, vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
      vr::VRCompositor()->Submit(vr::Eye_Left, &leftEyeTexture );
      vr::Texture_t rightEyeTexture = {(void*)(long)this->RightEyeDesc.m_nResolveTextureId, vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
      vr::VRCompositor()->Submit(vr::Eye_Right, &rightEyeTexture );
    }

    glBindFramebuffer(GL_READ_FRAMEBUFFER, this->RightEyeDesc.m_nResolveFramebufferId );
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0 );
    glBlitFramebuffer(0, 0, this->Size[0], this->Size[1],
      0, 0, this->Size[0] / 2, this->Size[1] / 2,
      GL_COLOR_BUFFER_BIT,
      GL_LINEAR);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

    SDL_GL_SwapWindow( this->WindowId );
  }
}

bool vtkOpenVRRenderWindow::CreateFrameBuffer( int nWidth, int nHeight, FramebufferDesc &framebufferDesc )
{
  glGenFramebuffers(1, &framebufferDesc.m_nRenderFramebufferId );
  glBindFramebuffer(GL_FRAMEBUFFER, framebufferDesc.m_nRenderFramebufferId);

  glGenRenderbuffers(1, &framebufferDesc.m_nDepthBufferId);
  glBindRenderbuffer(GL_RENDERBUFFER, framebufferDesc.m_nDepthBufferId);
  if (this->GetMultiSamples())
  {
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH_COMPONENT, nWidth, nHeight );
  }
  else
  {
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, nWidth, nHeight );
  }
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, framebufferDesc.m_nDepthBufferId );

  glGenTextures(1, &framebufferDesc.m_nRenderTextureId );
  if (this->GetMultiSamples())
  {
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, framebufferDesc.m_nRenderTextureId );
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA8, nWidth, nHeight, true);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, framebufferDesc.m_nRenderTextureId, 0);
  }
  else
  {
    glBindTexture(GL_TEXTURE_2D, framebufferDesc.m_nRenderTextureId );
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, nWidth, nHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebufferDesc.m_nRenderTextureId, 0);
  }
  glGenFramebuffers(1, &framebufferDesc.m_nResolveFramebufferId );
  glBindFramebuffer(GL_FRAMEBUFFER, framebufferDesc.m_nResolveFramebufferId);

  glGenTextures(1, &framebufferDesc.m_nResolveTextureId );
  glBindTexture(GL_TEXTURE_2D, framebufferDesc.m_nResolveTextureId );
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, nWidth, nHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebufferDesc.m_nResolveTextureId, 0);

  // check FBO status
  GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if (status != GL_FRAMEBUFFER_COMPLETE)
  {
    return false;
  }

  glBindFramebuffer( GL_FRAMEBUFFER, 0 );

  return true;
}


// Initialize the rendering window.
void vtkOpenVRRenderWindow::Initialize (void)
{
  if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_TIMER ) < 0 )
  {
    vtkErrorMacro("SDL could not initialize! SDL Error: " <<  SDL_GetError());
    return;
  }

  // Loading the SteamVR Runtime
  vr::EVRInitError eError = vr::VRInitError_None;
  this->HMD = vr::VR_Init( &eError, vr::VRApplication_Scene );

  if ( eError != vr::VRInitError_None )
  {
    this->HMD = NULL;
    char buf[1024];
    snprintf( buf, sizeof( buf ), "Unable to init VR runtime: %s", vr::VR_GetVRInitErrorAsEnglishDescription( eError ) );
    SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, "VR_Init Failed", buf, NULL );
    return;
  }

  this->OpenVRRenderModels = (vr::IVRRenderModels *)
    vr::VR_GetGenericInterface( vr::IVRRenderModels_Version, &eError );
  if( !this->OpenVRRenderModels )
  {
    this->HMD = NULL;
    vr::VR_Shutdown();

    char buf[1024];
    snprintf( buf, sizeof( buf ), "Unable to get render model interface: %s", vr::VR_GetVRInitErrorAsEnglishDescription( eError ) );
    SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, "VR_Init Failed", buf, NULL );
    return;
  }

  uint32_t renderWidth;
  uint32_t renderHeight;
  this->HMD->GetRecommendedRenderTargetSize(
    &renderWidth, &renderHeight );

  this->Size[0] = renderWidth;
  this->Size[1] = renderHeight;

  Uint32 unWindowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;
  //Uint32 unWindowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN;

  SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 4 );
  SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 1 );
  //SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY );
  SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );

  SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, 0 );
  SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, 0 );

  this->WindowId = SDL_CreateWindow( this->WindowName,
    this->Position[0], this->Position[1],
    this->Size[0] / 2, this->Size[1] / 2,
    unWindowFlags );
  if (this->WindowId == NULL)
  {
    vtkErrorMacro("Window could not be created! SDL Error: " <<  SDL_GetError());
    return;
  }

  this->ContextId = SDL_GL_CreateContext(this->WindowId);
  if (this->ContextId == NULL)
  {
    vtkErrorMacro("OpenGL context could not be created! SDL Error: " <<  SDL_GetError() );
    return;
  }

  this->OpenGLInit();
  glDepthRange(0., 1.);

  // make sure vsync is off
  if ( SDL_GL_SetSwapInterval( 0 ) < 0 )
  {
    vtkErrorMacro("Warning: Unable to set VSync! SDL Error: " << SDL_GetError() );
    return;
  }

  m_strDriver = "No Driver";
  m_strDisplay = "No Display";

  m_strDriver = GetTrackedDeviceString( this->HMD,
    vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_TrackingSystemName_String );
  m_strDisplay = GetTrackedDeviceString( this->HMD,
    vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_SerialNumber_String );

  std::string strWindowTitle = "VTK - " + m_strDriver + " " + m_strDisplay;
  this->SetWindowName(strWindowTitle.c_str());
  SDL_SetWindowTitle( this->WindowId, this->WindowName );

  this->CreateFrameBuffer(this->Size[0], this->Size[1], this->LeftEyeDesc);
  this->CreateFrameBuffer(this->Size[0], this->Size[1], this->RightEyeDesc);

  if ( !vr::VRCompositor() )
  {
    vtkErrorMacro("Compositor initialization failed." );
    return;
  }

  this->DashboardOverlay->Create(this);
}

void vtkOpenVRRenderWindow::Finalize (void)
{
  this->Clean();

  if( this->HMD )
  {
    vr::VR_Shutdown();
    this->HMD = NULL;
  }

  for( std::vector< vtkOpenVRModel * >::iterator i = this->VTKRenderModels.begin();
       i != this->VTKRenderModels.end(); ++i )
  {
    (*i)->Delete();
  }
  this->VTKRenderModels.clear();

  if( this->ContextId )
  {
  }

  if( this->WindowId )
  {
    SDL_DestroyWindow( this->WindowId );
    this->WindowId = NULL;
  }

  SDL_Quit();
}

void vtkOpenVRRenderWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "ContextId: " << this->ContextId << "\n";
  os << indent << "Window Id: " << this->WindowId << "\n";
}

// Begin the rendering process.
void vtkOpenVRRenderWindow::Start(void)
{
  // if the renderer has not been initialized, do so now
  if (!this->ContextId)
  {
    this->Initialize();
  }

  // set the current window
  this->MakeCurrent();
}

void vtkOpenVRRenderWindow::RenderOverlay()
{
  this->DashboardOverlay->Render();
}
