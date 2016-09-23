/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkOculusRenderWindow.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

Parts Copyright Valve Coproration from hellovr_opengl_main.cpp
under their BSD license found here:
https://github.com/ValveSoftware/Oculus/blob/master/LICENSE

=========================================================================*/
#include "vtkOculusRenderWindow.h"

#include "vtkIdList.h"
#include "vtkCommand.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkRendererCollection.h"
#include "vtkStringOutputWindow.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkOpenGLVertexBufferObject.h"
#include "vtkOpenGLIndexBufferObject.h"
#include "vtkOpenGLVertexArrayObject.h"
#include "vtkTransform.h"
#include "vtkFloatArray.h"
#include "vtkPointData.h"
#include "vtkOpenGLTexture.h"
#include "vtkTextureObject.h"
#include "vtkShaderProgram.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkOculusCamera.h"
#include "vtkQuaternion.h"

#include <cmath>
#include <sstream>

#include "vtkOpenGLError.h"

vtkStandardNewMacro(vtkOculusRenderWindow);

vtkOculusRenderWindow::vtkOculusRenderWindow()
{
  this->StereoCapableWindow = 1;
  this->StereoRender = 1;
  this->Size[0] = 100;
  this->Size[1] = 100;
  this->Position[0] = 100;
  this->Position[1] = 100;
  this->Session = NULL;
  this->HMDTransform = vtkTransform::New();
  this->ContextId = 0;
  this->WindowId = 0;
  this->MultiSamples = 0;
}

vtkOculusRenderWindow::~vtkOculusRenderWindow()
{
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
void vtkOculusRenderWindow::ReleaseGraphicsResources(vtkRenderWindow *renWin)
{
  this->Superclass::ReleaseGraphicsResources(renWin);
}

void vtkOculusRenderWindow::Clean()
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
void vtkOculusRenderWindow::MakeCurrent()
{
  SDL_GL_MakeCurrent(this->WindowId, this->ContextId);
}

// ----------------------------------------------------------------------------
// Description:
// Tells if this window is the current OpenGL context for the calling thread.
bool vtkOculusRenderWindow::IsCurrent()
{
  return this->ContextId!=0 && this->ContextId==SDL_GL_GetCurrentContext();
}


// ----------------------------------------------------------------------------
void vtkOculusRenderWindow::SetSize(int x, int y)
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
int *vtkOculusRenderWindow::GetScreenSize(void)
{
  return this->Size;
}


void vtkOculusRenderWindow::SetPosition(int x, int y)
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

void vtkOculusRenderWindow::UpdateHMDMatrixPose()
{
  // double displayMidpointSeconds = GetPredictedDisplayTime(this->Session, 0);
  // ovrTrackingState hmdState = ovr_GetTrackingState(this->Session, displayMidpointSeconds, ovrTrue);
  ovrTrackingState hmdState = ovr_GetTrackingState(this->Session, ovr_GetTimeInSeconds(), ovrTrue);
  ovr_CalcEyePoses(hmdState.HeadPose.ThePose, this->HMDToEyeViewOffsets, this->OVRLayer.RenderPose);

  // Query the HMD for ts current tracking state.
  if (hmdState.StatusFlags & (ovrStatus_OrientationTracked | ovrStatus_PositionTracked))
  {
    // update the camera values based on the pose
    ovrPosef pose = hmdState.HeadPose.ThePose;

    vtkQuaternion<double> quat;
    quat.Set(pose.Orientation.w, pose.Orientation.x, pose.Orientation.y, pose.Orientation.z);
    double axis[3];
    double angle;
    angle = quat.GetRotationAngleAndAxis(axis);
    this->HMDTransform->Identity();
    this->HMDTransform->Translate(pose.Position.x, pose.Position.y, pose.Position.z);
    this->HMDTransform->RotateWXYZ(180.0*angle/3.1415926, axis[0], axis[1], axis[2]);
    double elems[16];
    vtkMatrix4x4::DeepCopy(elems,this->HMDTransform->GetMatrix());

    vtkRenderer *ren;
    vtkCollectionSimpleIterator rit;
    this->Renderers->InitTraversal(rit);
    while ( (ren = this->Renderers->GetNextRenderer(rit)) )
    {
      vtkOculusCamera *cam = static_cast<vtkOculusCamera *>(ren->GetActiveCamera());
      this->HMDTransform->Identity();
      double *trans = cam->GetTranslation();
      this->HMDTransform->Translate(-trans[0],-trans[1],-trans[2]);
      double scale = cam->GetDistance();
      this->HMDTransform->Scale(scale,scale,scale);

      this->HMDTransform->Concatenate(elems);

      cam->SetFocalPoint(0,0,-1);
      cam->SetPosition(0,0,0);
      cam->SetViewUp(0,1,0);
      cam->ApplyTransform(this->HMDTransform);
    }
  }
}

void vtkOculusRenderWindow::Render()
{
  this->vtkRenderWindow::Render();
}

void vtkOculusRenderWindow::StereoUpdate()
{
  // camera handles what we need
  this->UpdateHMDMatrixPose();

  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->LeftEyeDesc.m_nResolveFramebufferId );
  int currentIndex = 0;
  ovr_GetTextureSwapChainCurrentIndex(this->Session, this->LeftEyeDesc.TextureSwapChain, &currentIndex);
  unsigned int texId;
  ovr_GetTextureSwapChainBufferGL(this->Session, this->LeftEyeDesc.TextureSwapChain, currentIndex, &texId);
  glBindTexture(GL_TEXTURE_2D, texId );
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texId, 0);
}

void vtkOculusRenderWindow::StereoMidpoint()
{
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->RightEyeDesc.m_nResolveFramebufferId );

  int currentIndex = 0;
  ovr_GetTextureSwapChainCurrentIndex(this->Session, this->RightEyeDesc.TextureSwapChain, &currentIndex);
  unsigned int texId;
  ovr_GetTextureSwapChainBufferGL(this->Session, this->RightEyeDesc.TextureSwapChain, currentIndex, &texId);
  glBindTexture(GL_TEXTURE_2D, texId );
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texId, 0);
}

void  vtkOculusRenderWindow::StereoRenderComplete()
{
  glBindFramebuffer(GL_READ_FRAMEBUFFER, this->RightEyeDesc.m_nResolveFramebufferId );
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0 );

  ovrRecti vp = this->OVRLayer.Viewport[1];
  glBlitFramebuffer(
    vp.Pos.x, vp.Pos.y, vp.Size.w, vp.Size.h,
    0, 0, this->Size[0], this->Size[1],
    GL_COLOR_BUFFER_BIT,
    GL_LINEAR );
}

// End the rendering process and display the image.
void vtkOculusRenderWindow::Frame(void)
{
  this->MakeCurrent();
  if (!this->AbortRender && this->DoubleBuffer && this->SwapBuffers)
  {
    // for now as fast as possible
    if ( this->Session )
    {
      ovr_CommitTextureSwapChain(this->Session, this->LeftEyeDesc.TextureSwapChain);
      ovr_CommitTextureSwapChain(this->Session, this->RightEyeDesc.TextureSwapChain);
      // Submit frame with one layer we have.
      ovrLayerHeader* layers = &this->OVRLayer.Header;
      ovrResult result = ovr_SubmitFrame(this->Session, 0, nullptr, &layers, 1);
      while (result == ovrSuccess_NotVisible)
      {
        result = ovr_SubmitFrame(this->Session, 0, nullptr, &layers, 1);
      }
      if (result != ovrSuccess)
      {
        vtkWarningMacro("failed to submit frame");
      }
    }

    SDL_GL_SwapWindow( this->WindowId );
  }
}

bool vtkOculusRenderWindow::CreateFrameBuffer( FramebufferDesc &framebufferDesc )
{
  glGenFramebuffers(1, &framebufferDesc.m_nResolveFramebufferId );
  glBindFramebuffer(GL_FRAMEBUFFER, framebufferDesc.m_nResolveFramebufferId);

  int nWidth = framebufferDesc.RecommendedTexSize.w;
  int nHeight = framebufferDesc.RecommendedTexSize.h;

  glGenRenderbuffers(1, &framebufferDesc.m_nDepthBufferId);
  glBindRenderbuffer(GL_RENDERBUFFER, framebufferDesc.m_nDepthBufferId);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, nWidth, nHeight );
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, framebufferDesc.m_nDepthBufferId );

  ovrTextureSwapChainDesc desc = {};
  desc.Type = ovrTexture_2D;
  desc.ArraySize = 1;
  desc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
  desc.Width = framebufferDesc.RecommendedTexSize.w;
  desc.Height = framebufferDesc.RecommendedTexSize.h;
  desc.MipLevels = 1;
  desc.SampleCount = 1;
  desc.StaticImage = ovrFalse;
  if (ovr_CreateTextureSwapChainGL(this->Session, &desc, &framebufferDesc.TextureSwapChain) != ovrSuccess)
  {
    vtkErrorMacro("Failed to create texture swap chain");
  }

  // Sample texture access:
  unsigned int texId;
  ovr_GetTextureSwapChainBufferGL(this->Session, framebufferDesc.TextureSwapChain, 0, &texId);
  glBindTexture(GL_TEXTURE_2D, texId );
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texId, 0);

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
void vtkOculusRenderWindow::Initialize (void)
{
  if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_TIMER ) < 0 )
  {
    vtkErrorMacro("SDL could not initialize! SDL Error: " <<  SDL_GetError());
    return;
  }

  ovrResult result = ovr_Initialize(nullptr);
  if (OVR_FAILURE(result))
  {
    vtkErrorMacro("Failed to initialize LibOVR");
    return;
  }

  ovrGraphicsLuid luid;
  result = ovr_Create(&this->Session, &luid);

  if (OVR_FAILURE(result))
  {
    vtkErrorMacro("Failed to create LibOVR session");
    return;
  }

  this->HMD = ovr_GetHmdDesc(this->Session);

  // Configure Stereo settings.
  this->LeftEyeDesc.RecommendedTexSize =
    ovr_GetFovTextureSize(this->Session, ovrEye_Left,
      this->HMD.DefaultEyeFov[0], 1.0f);
  this->RightEyeDesc.RecommendedTexSize =
    ovr_GetFovTextureSize(this->Session, ovrEye_Right,
      this->HMD.DefaultEyeFov[1], 1.0f);

  this->Size[0] = this->RightEyeDesc.RecommendedTexSize.w*0.5;
  this->Size[1] = this->RightEyeDesc.RecommendedTexSize.h*0.5;

  Uint32 unWindowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;

  SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 4 );
  SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 1 );
  SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );

  SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, 0 );
  SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, 0 );

  this->WindowId = SDL_CreateWindow( this->WindowName,
    this->Position[0], this->Position[1],
    this->Size[0], this->Size[1],
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

  if ( SDL_GL_SetSwapInterval( 0 ) < 0 )
  {
    vtkErrorMacro("Warning: Unable to set VSync! SDL Error: " << SDL_GetError() );
    return;
  }

  std::string strWindowTitle = "VTK - Oculus";
  this->SetWindowName(strWindowTitle.c_str());
  SDL_SetWindowTitle( this->WindowId, this->WindowName );

  this->CreateFrameBuffer( this->LeftEyeDesc );
  this->CreateFrameBuffer( this->RightEyeDesc );

  ovrEyeRenderDesc eyeRenderDesc[2];
  eyeRenderDesc[0] = ovr_GetRenderDesc(this->Session, ovrEye_Left, this->HMD.DefaultEyeFov[0]);
  eyeRenderDesc[1] = ovr_GetRenderDesc(this->Session, ovrEye_Right, this->HMD.DefaultEyeFov[1]);
  this->HMDToEyeViewOffsets[0] = eyeRenderDesc[0].HmdToEyeOffset;
  this->HMDToEyeViewOffsets[1] = eyeRenderDesc[1].HmdToEyeOffset;

  this->OVRLayer.Header.Type = ovrLayerType_EyeFov;
  this->OVRLayer.Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft;
  this->OVRLayer.ColorTexture[0] = this->LeftEyeDesc.TextureSwapChain;
  this->OVRLayer.ColorTexture[1] = this->RightEyeDesc.TextureSwapChain;
  this->OVRLayer.Fov[0] = eyeRenderDesc[0].Fov;
  this->OVRLayer.Fov[1] = eyeRenderDesc[1].Fov;
  this->OVRLayer.Viewport[0].Pos = {0, 0};
  this->OVRLayer.Viewport[0].Size = this->LeftEyeDesc.RecommendedTexSize;
  this->OVRLayer.Viewport[1].Pos = {0, 0};
  this->OVRLayer.Viewport[1].Size = this->RightEyeDesc.RecommendedTexSize;

  ovr_SetTrackingOriginType(this->Session, ovrTrackingOrigin_EyeLevel);
}

void vtkOculusRenderWindow::Finalize (void)
{
  this->Clean();

  if( this->Session )
  {
    ovr_Destroy(this->Session);
    ovr_Shutdown();
    this->Session = NULL;
  }

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

void vtkOculusRenderWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "ContextId: " << this->ContextId << "\n";
  os << indent << "Window Id: " << this->WindowId << "\n";
}

// Begin the rendering process.
void vtkOculusRenderWindow::Start(void)
{
  // if the renderer has not been initialized, do so now
  if (!this->ContextId)
  {
    this->Initialize();
  }

  // set the current window
  this->MakeCurrent();
}
