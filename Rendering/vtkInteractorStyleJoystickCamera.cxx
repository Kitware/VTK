/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleJoystickCamera.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInteractorStyleJoystickCamera.h"
#include "vtkObjectFactory.h"
#include "vtkCommand.h"
#include "vtkMath.h"

vtkCxxRevisionMacro(vtkInteractorStyleJoystickCamera, "1.24");
vtkStandardNewMacro(vtkInteractorStyleJoystickCamera);

//----------------------------------------------------------------------------
vtkInteractorStyleJoystickCamera::vtkInteractorStyleJoystickCamera() 
{
  // Use timers to handle continous interaction

  this->UseTimers = 1;
}

//----------------------------------------------------------------------------
vtkInteractorStyleJoystickCamera::~vtkInteractorStyleJoystickCamera() 
{
}

//----------------------------------------------------------------------------
// Mouse events
//----------------------------------------------------------------------------
void vtkInteractorStyleJoystickCamera::OnMouseMove() 
{ 
  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];

  switch (this->State) 
    {
    case VTKIS_ROTATE:
      this->FindPokedRenderer(x, y);
      break;

    case VTKIS_PAN:
      this->FindPokedRenderer(x, y);
      break;

    case VTKIS_DOLLY:
      this->FindPokedRenderer(x, y);
      break;

    case VTKIS_SPIN:
      this->FindPokedRenderer(x, y);
      break;
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleJoystickCamera::OnLeftButtonDown() 
{
  this->FindPokedRenderer(this->Interactor->GetEventPosition()[0], 
                          this->Interactor->GetEventPosition()[1]);
  if (this->CurrentRenderer == NULL)
    {
    return;
    }

  if (this->Interactor->GetShiftKey()) 
    {
    if (this->Interactor->GetControlKey()) 
      {
      this->StartDolly();
      }
    else 
      {
      this->StartPan();
      }
    } 
  else 
    {
    if (this->Interactor->GetControlKey()) 
      {
      this->StartSpin();
      }
    else 
      {
      this->StartRotate();
      }
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleJoystickCamera::OnLeftButtonUp()
{
  switch (this->State) 
    {
    case VTKIS_DOLLY:
      this->EndDolly();
      break;

    case VTKIS_PAN:
      this->EndPan();
      break;

    case VTKIS_SPIN:
      this->EndSpin();
      break;

    case VTKIS_ROTATE:
      this->EndRotate();
      break;
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleJoystickCamera::OnMiddleButtonDown()
{
  this->FindPokedRenderer(this->Interactor->GetEventPosition()[0], 
                          this->Interactor->GetEventPosition()[1]);
  if (this->CurrentRenderer == NULL)
    {
    return;
    }
  
  this->StartPan();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleJoystickCamera::OnMiddleButtonUp() 
{
  switch (this->State) 
    {
    case VTKIS_PAN:
      this->EndPan();
      break;
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleJoystickCamera::OnRightButtonDown()
{
  this->FindPokedRenderer(this->Interactor->GetEventPosition()[0], 
                          this->Interactor->GetEventPosition()[1]);
  if (this->CurrentRenderer == NULL)
    {
    return;
    }
  
  this->StartDolly();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleJoystickCamera::OnRightButtonUp()
{
  switch (this->State) 
    {
    case VTKIS_DOLLY:
      this->EndDolly();
      break;
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleJoystickCamera::Rotate()
{
  if (this->CurrentRenderer == NULL)
    {
    return;
    }
  
  vtkRenderWindowInteractor *rwi = this->Interactor;

  float *center = this->CurrentRenderer->GetCenter();

  float dx = (float)rwi->GetEventPosition()[0] - center[0];
  float dy = (float)rwi->GetEventPosition()[1] - center[1];
  
  float *vp = this->CurrentRenderer->GetViewport();
  int *size = rwi->GetSize();

  float delta_elevation = -20.0/((vp[3] - vp[1])*size[1]);
  float delta_azimuth = -20.0/((vp[2] - vp[0])*size[0]);
  
  double rxf = (double)dx * delta_azimuth;
  double ryf = (double)dy * delta_elevation;

  vtkCamera* camera = this->CurrentRenderer->GetActiveCamera();
  camera->Azimuth(rxf);
  camera->Elevation(ryf);
  camera->OrthogonalizeViewUp();

  this->ResetCameraClippingRange();

  if (rwi->GetLightFollowCamera())
    {
    vtkLight* light = this->CurrentRenderer->GetFirstLight();
      if (light != NULL) 
        {
        light->SetPosition(camera->GetPosition());
        light->SetFocalPoint(camera->GetFocalPoint());
        }
    }

  rwi->Render();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleJoystickCamera::Spin()
{
  if (this->CurrentRenderer == NULL)
    {
    return;
    }
  
  vtkRenderWindowInteractor *rwi = this->Interactor;

  float *center = this->CurrentRenderer->GetCenter();

  // Spin is based on y value

  double yf = ((double)rwi->GetEventPosition()[1] - (double)center[1]) 
    / (double)(center[1]);

  if (yf > 1)
    {
    yf = 1;
    }
  else if (yf < -1)
    {
    yf = -1;
    }

  double newAngle = asin(yf) * vtkMath::RadiansToDegrees();

  vtkCamera* camera = this->CurrentRenderer->GetActiveCamera();
  camera->Roll(newAngle);
  camera->OrthogonalizeViewUp();

  rwi->Render();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleJoystickCamera::Pan()
{
  if (this->CurrentRenderer == NULL)
    {
    return;
    }
  
  vtkRenderWindowInteractor *rwi = this->Interactor;

  double ViewFocus[4];
  double NewPickPoint[4];
  
  // Calculate the focal depth since we'll be using it a lot

  vtkCamera* camera = this->CurrentRenderer->GetActiveCamera();
  camera->GetFocalPoint(ViewFocus);
  this->ComputeWorldToDisplay(ViewFocus[0], ViewFocus[1], ViewFocus[2], 
                              ViewFocus);
  double focalDepth = ViewFocus[2];

  this->ComputeDisplayToWorld((float)rwi->GetEventPosition()[0], 
                              (float)rwi->GetEventPosition()[1],
                              focalDepth, 
                              NewPickPoint);

  // Get the current focal point and position

  camera->GetFocalPoint(ViewFocus);
  double *ViewPoint = camera->GetPosition();

  // Compute a translation vector, moving everything 1/10
  // the distance to the cursor. (Arbitrary scale factor)

  double MotionVector[3];
  MotionVector[0] = 0.1 * (ViewFocus[0] - NewPickPoint[0]);
  MotionVector[1] = 0.1 * (ViewFocus[1] - NewPickPoint[1]);
  MotionVector[2] = 0.1 * (ViewFocus[2] - NewPickPoint[2]);

  camera->SetFocalPoint(MotionVector[0] + ViewFocus[0],
                        MotionVector[1] + ViewFocus[1],
                        MotionVector[2] + ViewFocus[2]);

  camera->SetPosition(MotionVector[0] + ViewPoint[0],
                      MotionVector[1] + ViewPoint[1],
                      MotionVector[2] + ViewPoint[2]);

  if (rwi->GetLightFollowCamera())
    {
    vtkLight* light = this->CurrentRenderer->GetFirstLight();
    if (light != NULL) 
      {
      light->SetPosition(camera->GetPosition());
      light->SetFocalPoint(camera->GetFocalPoint());
      }
    }

  rwi->Render();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleJoystickCamera::Dolly()
{
  if (this->CurrentRenderer == NULL)
    {
    return;
    }
  
  vtkRenderWindowInteractor *rwi = this->Interactor;

  float *center = this->CurrentRenderer->GetCenter();

  double dy = (double)rwi->GetEventPosition()[1] - (double)center[1];
  double dyf = 0.5 * dy / (double)center[1];
  double zoomFactor = pow((double)1.1, dyf);

  if (zoomFactor < 0.5 || zoomFactor > 1.5)
    {
    vtkErrorMacro("Bad zoom factor encountered");
    }
  
  vtkCamera* camera = this->CurrentRenderer->GetActiveCamera();
  if (camera->GetParallelProjection())
    {
    camera->SetParallelScale(camera->GetParallelScale() / zoomFactor);
    }
  else
    {
    camera->Dolly(zoomFactor);
    this->ResetCameraClippingRange();
    }

  if (rwi->GetLightFollowCamera())
    {
    vtkLight* light = this->CurrentRenderer->GetFirstLight();
    if (light != NULL) 
      {
      light->SetPosition(camera->GetPosition());
      light->SetFocalPoint(camera->GetFocalPoint());
      }
    }

  rwi->Render();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleJoystickCamera::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
