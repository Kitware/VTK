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

vtkCxxRevisionMacro(vtkInteractorStyleJoystickCamera, "1.18");
vtkStandardNewMacro(vtkInteractorStyleJoystickCamera);

//----------------------------------------------------------------------------
vtkInteractorStyleJoystickCamera::vtkInteractorStyleJoystickCamera() 
{
}

//----------------------------------------------------------------------------
vtkInteractorStyleJoystickCamera::~vtkInteractorStyleJoystickCamera() 
{
}

//----------------------------------------------------------------------------
// Mouse events
//----------------------------------------------------------------------------
void vtkInteractorStyleJoystickCamera::OnMouseMove(int vtkNotUsed(ctrl), 
                                                   int vtkNotUsed(shift),
                                                   int x, 
                                                   int y) 
{ 
  switch (this->State) 
    {
    case VTKIS_ROTATE:
      this->FindPokedCamera(x, y);
      break;

    case VTKIS_PAN:
      this->FindPokedCamera(x, y);
      break;

    case VTKIS_DOLLY:
      this->FindPokedCamera(x, y);
      break;

    case VTKIS_SPIN:
      this->FindPokedCamera(x, y);
      break;
    }

  this->LastPos[0] = x;
  this->LastPos[1] = y;
}

//----------------------------------------------------------------------------
void vtkInteractorStyleJoystickCamera::OnLeftButtonDown(int ctrl, 
                                                        int shift, 
                                                        int x, 
                                                        int y) 
{
  this->FindPokedRenderer(x, y);
  if (this->CurrentRenderer == NULL)
    {
    return;
    }

  if (shift) 
    {
    if (ctrl) 
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
    if (ctrl) 
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
void vtkInteractorStyleJoystickCamera::OnLeftButtonUp(int vtkNotUsed(ctrl), 
                                                      int vtkNotUsed(shift), 
                                                      int vtkNotUsed(x), 
                                                      int vtkNotUsed(y))
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
void vtkInteractorStyleJoystickCamera::OnMiddleButtonDown(int vtkNotUsed(ctrl),
                                                          int vtkNotUsed(shift), 
                                                          int x, 
                                                          int y)
{
  this->FindPokedRenderer(x, y);
  if (this->CurrentRenderer == NULL)
    {
    return;
    }
  
  this->StartPan();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleJoystickCamera::OnMiddleButtonUp(int vtkNotUsed(ctrl),
                                                        int vtkNotUsed(shift), 
                                                        int vtkNotUsed(x),
                                                        int vtkNotUsed(y)) 
{
  switch (this->State) 
    {
    case VTKIS_PAN:
      this->EndPan();
      break;
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleJoystickCamera::OnRightButtonDown(int vtkNotUsed(ctrl),
                                                         int vtkNotUsed(shift), 
                                                         int x, 
                                                         int y)
{
  this->FindPokedRenderer(x, y);
  if (this->CurrentRenderer == NULL)
    {
    return;
    }
  
  this->StartDolly();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleJoystickCamera::OnRightButtonUp(int vtkNotUsed(ctrl), 
                                                       int vtkNotUsed(shift), 
                                                       int vtkNotUsed(x), 
                                                       int vtkNotUsed(y))
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

  double rxf = ((double)(this->LastPos[0]) - (double)(this->Center[0])) * this->DeltaAzimuth;
  double ryf = ((double)(this->LastPos[1]) - (double)(this->Center[1])) * this->DeltaElevation;

  this->CurrentCamera->Azimuth(rxf);
  this->CurrentCamera->Elevation(ryf);
  this->CurrentCamera->OrthogonalizeViewUp();

  this->ResetCameraClippingRange();

  if (rwi->GetLightFollowCamera())
    {
    this->CurrentLight->SetPosition(this->CurrentCamera->GetPosition());
    this->CurrentLight->SetFocalPoint(this->CurrentCamera->GetFocalPoint());
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

  // spin is based on y value
  double yf = ((double)(this->LastPos[1]) - (double)(this->Center[1])) / 
                       (double)(this->Center[1]);
  if (yf > 1)
    {
    yf = 1;
    }
  else if (yf < -1)
    {
    yf = -1;
    }

  double newAngle = asin(yf) * 180.0 / 3.1415926;

  this->CurrentCamera->Roll(newAngle);
  this->CurrentCamera->OrthogonalizeViewUp();

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
  
  // calculate the focal depth since we'll be using it a lot
  this->CurrentCamera->GetFocalPoint(ViewFocus);
  this->ComputeWorldToDisplay(ViewFocus[0], ViewFocus[1],
                              ViewFocus[2], ViewFocus);
  double focalDepth = ViewFocus[2];

  double NewPickPoint[4];
  this->ComputeDisplayToWorld((float)this->LastPos[0], (float)this->LastPos[1],
                              focalDepth, NewPickPoint);

  // get the current focal point and position
  this->CurrentCamera->GetFocalPoint(ViewFocus);
  double *ViewPoint = this->CurrentCamera->GetPosition();

  /*
   * Compute a translation vector, moving everything 1/10
   * the distance to the cursor. (Arbitrary scale factor)
   */
  double MotionVector[3];
  MotionVector[0] = 0.1*(ViewFocus[0] - NewPickPoint[0]);
  MotionVector[1] = 0.1*(ViewFocus[1] - NewPickPoint[1]);
  MotionVector[2] = 0.1*(ViewFocus[2] - NewPickPoint[2]);

  this->CurrentCamera->SetFocalPoint(MotionVector[0] + ViewFocus[0],
                                     MotionVector[1] + ViewFocus[1],
                                     MotionVector[2] + ViewFocus[2]);

  this->CurrentCamera->SetPosition(MotionVector[0] + ViewPoint[0],
                                   MotionVector[1] + ViewPoint[1],
                                   MotionVector[2] + ViewPoint[2]);

  if (rwi->GetLightFollowCamera())
    {
    this->CurrentLight->SetPosition(this->CurrentCamera->GetPosition());
    this->CurrentLight->SetFocalPoint(this->CurrentCamera->GetFocalPoint());
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

  double dyf = 0.5 * ((double)(this->LastPos[1]) - (double)(this->Center[1])) /
    (double)(this->Center[1]);
  double zoomFactor = pow((double)1.1, dyf);
  if (zoomFactor < 0.5 || zoomFactor > 1.5)
    {
    vtkErrorMacro("Bad zoom factor encountered");
    }
  
  if (this->CurrentCamera->GetParallelProjection())
    {
    this->CurrentCamera->
      SetParallelScale(this->CurrentCamera->GetParallelScale()/zoomFactor);
    }
  else
    {
    this->CurrentCamera->Dolly(zoomFactor);
    this->ResetCameraClippingRange();
    }

  if (rwi->GetLightFollowCamera())
    {
    this->CurrentLight->SetPosition(this->CurrentCamera->GetPosition());
    this->CurrentLight->SetFocalPoint(this->CurrentCamera->GetFocalPoint());
    }

  rwi->Render();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleJoystickCamera::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
