/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkVRRenderWindow.cxx

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
#include "vtkVRRenderWindow.h"

#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLState.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkTransform.h"
#include "vtkVRCamera.h"
#include "vtkVRModel.h"
#include "vtkVRRenderer.h"

#include <cstring>
#include <memory>

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

//------------------------------------------------------------------------------
vtkVRRenderWindow::vtkVRRenderWindow()
{
  this->SetPhysicalViewDirection(0.0, 0.0, -1.0);
  this->SetPhysicalViewUp(0.0, 1.0, 0.0);
  this->SetPhysicalTranslation(0.0, 0.0, 0.0);
  this->PhysicalScale = 1.0;

  this->StereoCapableWindow = 1;
  this->StereoRender = 1;
  this->UseOffScreenBuffers = true;
  this->Size[0] = 640;
  this->Size[1] = 720;
  this->Position[0] = 100;
  this->Position[1] = 100;
  this->HMDTransform = vtkTransform::New();

#ifdef WIN32
  this->HelperWindow = vtkWin32OpenGLRenderWindow::New();
#endif
#ifdef VTK_USE_X
  this->HelperWindow = vtkXOpenGLRenderWindow::New();
#endif
#ifdef VTK_USE_COCOA
  this->HelperWindow = vtkCocoaRenderWindow::New();
#endif
}

//------------------------------------------------------------------------------
vtkVRRenderWindow::~vtkVRRenderWindow()
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
    this->HelperWindow = nullptr;
  }
}

//------------------------------------------------------------------------------
void vtkVRRenderWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ContextId: " << this->HelperWindow->GetGenericContext() << "\n";
  os << indent << "Window Id: " << this->HelperWindow->GetGenericWindowId() << "\n";
  os << indent << "Initialized: " << this->Initialized << "\n";
  os << indent << "PhysicalViewDirection: (" << this->PhysicalViewDirection[0] << ", "
     << this->PhysicalViewDirection[1] << ", " << this->PhysicalViewDirection[2] << ")"
     << "\n";
  os << indent << "PhysicalViewUp: (" << this->PhysicalViewUp[0] << ", " << this->PhysicalViewUp[1]
     << ", " << this->PhysicalViewUp[2] << ")"
     << "\n";
  os << indent << "PhysicalTranslation: (" << this->PhysicalTranslation[0] << ", "
     << this->PhysicalTranslation[1] << ", " << this->PhysicalTranslation[2] << ")"
     << "\n";
  os << indent << "PhysicalScale: " << this->PhysicalScale << "\n";
}

//------------------------------------------------------------------------------
void vtkVRRenderWindow::ReleaseGraphicsResources(vtkWindow* renWin)
{
  this->Superclass::ReleaseGraphicsResources(renWin);

  for (FramebufferDesc& fbo : this->FramebufferDescs)
  {
    glDeleteFramebuffers(1, &fbo.ResolveFramebufferId);
  }

  for (std::vector<vtkVRModel*>::iterator i = this->VTKRenderModels.begin();
       i != this->VTKRenderModels.end(); ++i)
  {
    (*i)->ReleaseGraphicsResources(renWin);
  }
}

//------------------------------------------------------------------------------
void vtkVRRenderWindow::SetHelperWindow(vtkOpenGLRenderWindow* win)
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
void vtkVRRenderWindow::InitializeViewFromCamera(vtkCamera* srccam)
{
  vtkRenderer* ren = static_cast<vtkRenderer*>(this->GetRenderers()->GetItemAsObject(0));
  if (!ren)
  {
    vtkErrorMacro("The renderer must be set prior to calling InitializeViewFromCamera");
    return;
  }

  vtkVRCamera* cam = static_cast<vtkVRCamera*>(ren->GetActiveCamera());
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
void vtkVRRenderWindow::MakeCurrent()
{
  if (this->HelperWindow)
  {
    this->HelperWindow->MakeCurrent();
  }
}

//------------------------------------------------------------------------------
void vtkVRRenderWindow::ReleaseCurrent()
{
  if (this->HelperWindow)
  {
    this->HelperWindow->ReleaseCurrent();
  }
}

//------------------------------------------------------------------------------
vtkOpenGLState* vtkVRRenderWindow::GetState()
{
  if (this->HelperWindow)
  {
    return this->HelperWindow->GetState();
  }
  return this->Superclass::GetState();
}

//------------------------------------------------------------------------------
// Description:
// Tells if this window is the current OpenGL context for the calling thread.
bool vtkVRRenderWindow::IsCurrent()
{
  return this->HelperWindow ? this->HelperWindow->IsCurrent() : false;
}

//------------------------------------------------------------------------------
// Add a renderer to the list of renderers.
void vtkVRRenderWindow::AddRenderer(vtkRenderer* ren)
{
  if (ren && !vtkVRRenderer::SafeDownCast(ren))
  {
    vtkErrorMacro("vtkVRRenderWindow::AddRenderer: Failed to add renderer of type "
      << ren->GetClassName() << ": A subclass of vtkVRRenderer is expected");
    return;
  }
  this->Superclass::AddRenderer(ren);
}

//------------------------------------------------------------------------------
// Begin the rendering process.
void vtkVRRenderWindow::Start()
{
  // if the renderer has not been initialized, do so now
  if (this->HelperWindow && !this->Initialized)
  {
    this->Initialize();
  }

  this->Superclass::Start();
}

//------------------------------------------------------------------------------
// Initialize the rendering window.
void vtkVRRenderWindow::Initialize()
{
  if (this->Initialized)
  {
    return;
  }
  this->Initialized = false;

  this->GetSizeFromAPI();

  this->HelperWindow->SetDisplayId(this->GetGenericDisplayId());
  this->HelperWindow->SetShowWindow(false);
  this->HelperWindow->Initialize();

  this->MakeCurrent();

  this->OpenGLInit();

  // some classes override the ivar in a getter :-(
  this->MaximumHardwareLineWidth = this->HelperWindow->GetMaximumHardwareLineWidth();

  glDepthRange(0., 1.);

  // TODO: make sure vsync is off
  // this->HelperWindow->SetSwapControl(0);

  this->SetWindowName(this->GetWindowTitleFromAPI().c_str());

  this->CreateFramebuffers();

  this->Initialized = true;
  vtkDebugMacro(<< "End of VRRenderWindow Initialization");
}

//------------------------------------------------------------------------------
void vtkVRRenderWindow::Finalize()
{
  this->ReleaseGraphicsResources(this);

  for (std::vector<vtkVRModel*>::iterator i = this->VTKRenderModels.begin();
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
void vtkVRRenderWindow::Render()
{
  this->MakeCurrent();
  this->GetState()->ResetGLViewportState();
  this->Superclass::Render();
}

//------------------------------------------------------------------------------
void vtkVRRenderWindow::RenderFramebuffer(FramebufferDesc& framebufferDesc)
{
  this->GetState()->PushDrawFramebufferBinding();
  this->GetState()->vtkglBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebufferDesc.ResolveFramebufferId);

  glBlitFramebuffer(0, 0, this->Size[0], this->Size[1], 0, 0, this->Size[0], this->Size[1],
    GL_COLOR_BUFFER_BIT, GL_LINEAR);

  if (framebufferDesc.ResolveDepthTextureId != 0)
  {
    glBlitFramebuffer(0, 0, this->Size[0], this->Size[1], 0, 0, this->Size[0], this->Size[1],
      GL_DEPTH_BUFFER_BIT, GL_NEAREST);
  }

  this->GetState()->PopDrawFramebufferBinding();
}

//------------------------------------------------------------------------------
void vtkVRRenderWindow::SetPhysicalViewDirection(double x, double y, double z)
{
  if (this->PhysicalViewDirection[0] != x || this->PhysicalViewDirection[1] != y ||
    this->PhysicalViewDirection[2] != z)
  {
    this->PhysicalViewDirection[0] = x;
    this->PhysicalViewDirection[1] = y;
    this->PhysicalViewDirection[2] = z;
    this->InvokeEvent(vtkVRRenderWindow::PhysicalToWorldMatrixModified);
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkVRRenderWindow::SetPhysicalViewDirection(double dir[3])
{
  this->SetPhysicalViewDirection(dir[0], dir[1], dir[2]);
}

//------------------------------------------------------------------------------
void vtkVRRenderWindow::SetPhysicalViewUp(double x, double y, double z)
{
  if (this->PhysicalViewUp[0] != x || this->PhysicalViewUp[1] != y || this->PhysicalViewUp[2] != z)
  {
    this->PhysicalViewUp[0] = x;
    this->PhysicalViewUp[1] = y;
    this->PhysicalViewUp[2] = z;
    this->InvokeEvent(vtkVRRenderWindow::PhysicalToWorldMatrixModified);
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkVRRenderWindow::SetPhysicalViewUp(double dir[3])
{
  this->SetPhysicalViewUp(dir[0], dir[1], dir[2]);
}

//------------------------------------------------------------------------------
void vtkVRRenderWindow::SetPhysicalTranslation(double x, double y, double z)
{
  if (this->PhysicalTranslation[0] != x || this->PhysicalTranslation[1] != y ||
    this->PhysicalTranslation[2] != z)
  {
    this->PhysicalTranslation[0] = x;
    this->PhysicalTranslation[1] = y;
    this->PhysicalTranslation[2] = z;
    this->InvokeEvent(vtkVRRenderWindow::PhysicalToWorldMatrixModified);
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkVRRenderWindow::SetPhysicalTranslation(double trans[3])
{
  this->SetPhysicalTranslation(trans[0], trans[1], trans[2]);
}

//------------------------------------------------------------------------------
void vtkVRRenderWindow::SetPhysicalScale(double scale)
{
  if (this->PhysicalScale != scale)
  {
    this->PhysicalScale = scale;
    this->InvokeEvent(vtkVRRenderWindow::PhysicalToWorldMatrixModified);
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkVRRenderWindow::SetPhysicalToWorldMatrix(vtkMatrix4x4* matrix)
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

  this->InvokeEvent(vtkVRRenderWindow::PhysicalToWorldMatrixModified);
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkVRRenderWindow::GetPhysicalToWorldMatrix(vtkMatrix4x4* physicalToWorldMatrix)
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
// Get the size of the whole screen.
int* vtkVRRenderWindow::GetScreenSize()
{
  if (this->GetSizeFromAPI())
  {
    this->ScreenSize[0] = this->Size[0];
    this->ScreenSize[1] = this->Size[1];
  }
  return this->ScreenSize;
}

//------------------------------------------------------------------------------
void vtkVRRenderWindow::SetSize(int width, int height)
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
