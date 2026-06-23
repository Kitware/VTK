// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkWebXRRenderWindow.h"

#include "vtkEventData.h"
#include "vtkIndent.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLFramebufferObject.h"
#include "vtkOpenGLState.h"
#include "vtkRendererCollection.h"
#include "vtkVRCamera.h"
#include "vtkWebXR.h"
#include "vtkWebXRModel.h"
#include "vtkWebXRRenderWindowInteractor.h"
#include "vtkWebXRRenderer.h"
#include "vtkWebXRUtilities.h"
#include <iostream>

#ifdef __EMSCRIPTEN__
#include "vtkWebAssemblyOpenGLRenderWindow.h"
#include <emscripten/emscripten.h>
#endif

VTK_ABI_NAMESPACE_BEGIN

class vtkWebXRRenderWindow::vtkInternals
{
  vtkWebXRRenderWindow* rw;

public:
  vtkInternals(vtkWebXRRenderWindow* RenderWindow)
    : rw(RenderWindow)
  {
  }

  bool XRRendering = false;

  //------------------------------------------------------------------------------
  void onFrame()
  {
    vtkWebXRRenderWindowInteractor* rwi =
      vtkWebXRRenderWindowInteractor::SafeDownCast(this->rw->GetInteractor());
    if (!rwi)
    {
      return;
    }
    this->XRRendering = true;
    rwi->DoOneEvent(this->rw, this->rw->GetRenderers()->GetFirstRenderer());
    this->XRRendering = false;
  }

  //------------------------------------------------------------------------------
  void onStart() { this->rw->GetSizeFromAPI(); }

  //------------------------------------------------------------------------------
  void onEnd() {}

  //------------------------------------------------------------------------------
  void onError(int error_code) { std::cerr << "[WebXR] Error " << error_code << std::endl; }
};

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkWebXRRenderWindow);

//------------------------------------------------------------------------------
vtkWebXRRenderWindow::vtkWebXRRenderWindow()
  : Internal(new vtkWebXRRenderWindow::vtkInternals(this))
{
  this->SetStereoCapableWindow(true);
  this->SetStereoRender(true);
}

//------------------------------------------------------------------------------
vtkWebXRRenderWindow::~vtkWebXRRenderWindow()
{
  this->Finalize();

  vtkRenderer* ren;
  vtkCollectionSimpleIterator rit;
  this->Renderers->InitTraversal(rit);
  while ((ren = this->Renderers->GetNextRenderer(rit)))
  {
    ren->SetRenderWindow(nullptr);
  }

  delete this->Internal;
}

//------------------------------------------------------------------------------
// Create an interactor that will work with this renderer.
vtkRenderWindowInteractor* vtkWebXRRenderWindow::MakeRenderWindowInteractor()
{
  this->Interactor = vtkWebXRRenderWindowInteractor::New();
  this->Interactor->SetRenderWindow(this);
  return this->Interactor;
}

//------------------------------------------------------------------------------
bool vtkWebXRRenderWindow::GetSizeFromAPI()
{
#ifdef __EMSCRIPTEN__
  webxr_get_framebuffer_size(&this->Size[0], &this->Size[1]);
  return true;
#else
  return false;
#endif
}

//------------------------------------------------------------------------------
// Add a renderer to the list of renderers.
void vtkWebXRRenderWindow::AddRenderer(vtkRenderer* ren)
{
  if (ren && !vtkWebXRRenderer::SafeDownCast(ren))
  {
    vtkErrorMacro("vtkWebXRRenderWindow::AddRenderer: Failed to add renderer of type "
      << ren->GetClassName() << ": A vtkWebXRRenderer is expected");
    return;
  }
  this->Superclass::AddRenderer(ren);
}

//------------------------------------------------------------------------------
// Initialize the rendering window.
void vtkWebXRRenderWindow::Initialize()
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

  this->HelperWindow->SetDisplayId(this->GetGenericDisplayId());
  this->HelperWindow->SetShowWindow(false);
  this->HelperWindow->Initialize();

  this->MakeCurrent();
  this->OpenGLInit();

  vtkDebugMacro(<< "Size : " << this->Size[0] << ", " << this->Size[1]);

#ifdef __EMSCRIPTEN__
  webxr_init([](void* data, int) { static_cast<vtkWebXRRenderWindow*>(data)->Internal->onFrame(); },
    [](void* data, int) { static_cast<vtkWebXRRenderWindow*>(data)->Internal->onStart(); },
    [](void* data, int) { static_cast<vtkWebXRRenderWindow*>(data)->Internal->onEnd(); },
    [](void* data, int error_code)
    { static_cast<vtkWebXRRenderWindow*>(data)->Internal->onError(error_code); }, this);
#endif

  this->VRInitialized = true;
}

//------------------------------------------------------------------------------
void vtkWebXRRenderWindow::Finalize()
{
  if (!this->VRInitialized)
  {
    return;
  }

  vtkWebXRRenderWindowInteractor::StopXR();

  if (this->HelperWindow && this->HelperWindow->GetGenericContext())
  {
    this->HelperWindow->Finalize();
  }

  this->ReleaseGraphicsResources(this);

  this->VRInitialized = false;
}

//------------------------------------------------------------------------------
void vtkWebXRRenderWindow::Render()
{
  // Disable rendering when not in a WebXR callback
  if (!this->Internal->XRRendering)
  {
    return;
  }

  this->UpdateHMDMatrixPose();

  this->Superclass::Render();
}

//------------------------------------------------------------------------------
void vtkWebXRRenderWindow::UpdateHMDMatrixPose()
{
#ifdef __EMSCRIPTEN__
  WebXRRigidTransform pose;
  webxr_get_viewer_pose(&pose);

  uint32_t handle = static_cast<uint32_t>(vtkEventDataDevice::HeadMountedDisplay);
  this->AddDeviceHandle(handle, vtkEventDataDevice::HeadMountedDisplay);
  vtkMatrix4x4* hmdToPhysicalMatrix = this->GetDeviceToPhysicalMatrixForDeviceHandle(handle);
  vtkWebXRUtilities::SetMatrixFromWebXRMatrix(hmdToPhysicalMatrix, pose.matrix);

  // update the camera values based on the pose
  this->GetDeviceToWorldMatrixForDeviceHandle(handle, TempMatrix4x4);

  vtkRenderer* ren;
  vtkCollectionSimpleIterator rit;
  this->Renderers->InitTraversal(rit);
  while ((ren = this->Renderers->GetNextRenderer(rit)))
  {
    vtkVRCamera* cam = vtkVRCamera::SafeDownCast(ren->GetActiveCamera());
    if (cam && cam->GetTrackHMD())
    {
      cam->SetCameraFromDeviceToWorldMatrix(TempMatrix4x4, this->GetPhysicalScale());
      if (ren->GetLightFollowCamera())
      {
        ren->UpdateLightsGeometryToFollowCamera();
      }
    }
  }
#endif
}

//------------------------------------------------------------------------------
void vtkWebXRRenderWindow::StereoMidpoint()
{
  // render models for left eye
  this->RenderModels();
}

//------------------------------------------------------------------------------
void vtkWebXRRenderWindow::StereoRenderComplete()
{
  // render models for right eye
  this->RenderModels();

#ifdef __EMSCRIPTEN__
  // blit render framebuffer to HMD's framebuffer
  this->GetState()->PushDrawFramebufferBinding();
  this->GetRenderFramebuffer()->Bind(GL_READ_FRAMEBUFFER);
  this->GetState()->vtkglBindFramebuffer(GL_DRAW_FRAMEBUFFER, webxr_get_framebuffer());
  this->GetState()->vtkglBlitFramebuffer(0, 0, this->Size[0], this->Size[1], 0, 0, this->Size[0],
    this->Size[1], GL_COLOR_BUFFER_BIT, GL_LINEAR);
  this->GetState()->PopDrawFramebufferBinding();
#endif
}

//------------------------------------------------------------------------------
void vtkWebXRRenderWindow::RenderModels()
{
  for (vtkEventDataDevice hand :
    { vtkEventDataDevice::LeftController, vtkEventDataDevice::RightController })
  {
    uint32_t handle = static_cast<uint32_t>(hand);
    this->AddDeviceHandle(handle, hand);
    auto* pRenderModel = this->GetModelForDeviceHandle(handle);
    if (!pRenderModel)
    {
      vtkNew<vtkWebXRModel> newModel;
      this->SetModelForDeviceHandle(handle, newModel);
      pRenderModel = newModel;
    }

    // if we have a model and it is visible
    if (pRenderModel && pRenderModel->GetVisibility())
    {
      vtkMatrix4x4* tdPose = this->GetDeviceToPhysicalMatrixForDeviceHandle(handle);
      pRenderModel->Render(this, tdPose);
    }
  }
}

//------------------------------------------------------------------------------
int vtkWebXRRenderWindow::GetColorBufferSizes(int* rgba)
{
  return this->HelperWindow->GetColorBufferSizes(rgba);
}

//------------------------------------------------------------------------------
char* vtkWebXRRenderWindow::GetCanvasSelector()
{
#ifdef __EMSCRIPTEN__
  if (auto hw = vtkWebAssemblyOpenGLRenderWindow::SafeDownCast(this->HelperWindow))
  {
    return hw->GetCanvasSelector();
  }
#endif
  return nullptr;
}

//------------------------------------------------------------------------------
void vtkWebXRRenderWindow::SetCanvasSelector([[maybe_unused]] const char* _arg)
{
#ifdef __EMSCRIPTEN__
  if (auto hw = vtkWebAssemblyOpenGLRenderWindow::SafeDownCast(this->HelperWindow))
  {
    hw->SetCanvasSelector(_arg);
  }
#endif
}

//------------------------------------------------------------------------------
void vtkWebXRRenderWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

VTK_ABI_NAMESPACE_END
