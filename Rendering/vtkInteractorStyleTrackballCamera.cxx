/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleTrackballCamera.cxx
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
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkObjectFactory.h"
#include "vtkMath.h"
#include "vtkCommand.h"

vtkCxxRevisionMacro(vtkInteractorStyleTrackballCamera, "1.17");
vtkStandardNewMacro(vtkInteractorStyleTrackballCamera);

//----------------------------------------------------------------------------
vtkInteractorStyleTrackballCamera::vtkInteractorStyleTrackballCamera() 
{
  this->MotionFactor   = 10.0;
  this->RadianToDegree = 180.0 / vtkMath::Pi();

  // This prevent vtkInteractorStyle::StartState to fire the timer
  // that is used to handle joystick mode

  this->NoTimerInStartState = 1;
}

//----------------------------------------------------------------------------
vtkInteractorStyleTrackballCamera::~vtkInteractorStyleTrackballCamera() 
{
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTrackballCamera::OnMouseMove(int vtkNotUsed(ctrl), 
                                                    int vtkNotUsed(shift),
                                                    int x, 
                                                    int y) 
{ 
  switch (this->State) 
    {
    case VTKIS_ROTATE:
      this->FindPokedCamera(x, y);
      this->RotateXY(x - this->LastPos[0], y - this->LastPos[1]);
      break;

    case VTKIS_PAN:
      this->FindPokedCamera(x, y);
      this->PanXY(x, y, this->LastPos[0], this->LastPos[1]);
      break;

    case VTKIS_DOLLY:
      this->FindPokedCamera(x, y);
      this->DollyXY(x - this->LastPos[0], y - this->LastPos[1]);
      break;

    case VTKIS_SPIN:
      this->FindPokedCamera(x, y);
      this->SpinXY(x, y, this->LastPos[0], this->LastPos[1]);
      break;
    }

  this->LastPos[0] = x;
  this->LastPos[1] = y;
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTrackballCamera::OnLeftButtonDown(int ctrl, 
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
void vtkInteractorStyleTrackballCamera::OnLeftButtonUp(int vtkNotUsed(ctrl),
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
void vtkInteractorStyleTrackballCamera::OnMiddleButtonDown(int vtkNotUsed(ctrl),
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
void vtkInteractorStyleTrackballCamera::OnMiddleButtonUp(int vtkNotUsed(ctrl),
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
void vtkInteractorStyleTrackballCamera::OnRightButtonDown(int vtkNotUsed(ctrl),
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
void vtkInteractorStyleTrackballCamera::OnRightButtonUp(int vtkNotUsed(ctrl),
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
void vtkInteractorStyleTrackballCamera::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

}

//----------------------------------------------------------------------------
void vtkInteractorStyleTrackballCamera::RotateXY(int dx, int dy)
{
  double rxf;
  double ryf;
  vtkCamera *cam;
  
  if (this->CurrentRenderer == NULL)
    {
    return;
    }
  
  int *size = this->CurrentRenderer->GetRenderWindow()->GetSize();
  this->DeltaElevation = -20.0 / size[1];
  this->DeltaAzimuth = -20.0 / size[0];
  
  rxf = (double)dx * this->DeltaAzimuth *  this->MotionFactor;
  ryf = (double)dy * this->DeltaElevation * this->MotionFactor;
  
  cam = this->CurrentRenderer->GetActiveCamera();
  cam->Azimuth(rxf);
  cam->Elevation(ryf);
  cam->OrthogonalizeViewUp();
  this->ResetCameraClippingRange();
  vtkRenderWindowInteractor *rwi = this->Interactor;
  if (this->CurrentLight)
    {
    // get the first light
    this->CurrentLight->SetPosition(cam->GetPosition());
    this->CurrentLight->SetFocalPoint(cam->GetFocalPoint());
    }   
  rwi->Render();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTrackballCamera::PanXY(int x, int y, int oldX, int oldY)
{
  vtkCamera *cam;
  double viewFocus[4], focalDepth, viewPoint[3];
  float newPickPoint[4], oldPickPoint[4], motionVector[3];
  
  if (this->CurrentRenderer == NULL)
    {
    return;
    }

  // calculate the focal depth since we'll be using it a lot
  cam = this->CurrentRenderer->GetActiveCamera();
  cam->GetFocalPoint(viewFocus);
  this->ComputeWorldToDisplay(viewFocus[0], viewFocus[1],
                              viewFocus[2], viewFocus);
  focalDepth = viewFocus[2];

  this->ComputeDisplayToWorld(double(x), double(y),
                              focalDepth, newPickPoint);
    
  // has to recalc old mouse point since the viewport has moved,
  // so can't move it outside the loop
  this->ComputeDisplayToWorld(double(oldX),double(oldY),
                              focalDepth, oldPickPoint);
  
  // camera motion is reversed
  motionVector[0] = oldPickPoint[0] - newPickPoint[0];
  motionVector[1] = oldPickPoint[1] - newPickPoint[1];
  motionVector[2] = oldPickPoint[2] - newPickPoint[2];
  
  cam->GetFocalPoint(viewFocus);
  cam->GetPosition(viewPoint);
  cam->SetFocalPoint(motionVector[0] + viewFocus[0],
                     motionVector[1] + viewFocus[1],
                     motionVector[2] + viewFocus[2]);
  cam->SetPosition(motionVector[0] + viewPoint[0],
                   motionVector[1] + viewPoint[1],
                   motionVector[2] + viewPoint[2]);
      
  vtkRenderWindowInteractor *rwi = this->Interactor;
  if (this->CurrentLight)
    {
    /* get the first light */
    this->CurrentLight->SetPosition(cam->GetPosition());
    this->CurrentLight->SetFocalPoint(cam->GetFocalPoint());
    }
    
  rwi->Render();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTrackballCamera::DollyXY(int vtkNotUsed(dx), int dy)
{
  vtkCamera *cam;
  double dyf = this->MotionFactor * (double)(dy) / (double)(this->Center[1]);
  double zoomFactor = pow((double)1.1, dyf);
  
  if (this->CurrentRenderer == NULL)
    {
    return;
    }
  
  cam = this->CurrentRenderer->GetActiveCamera();
  if (cam->GetParallelProjection())
    {
    cam->SetParallelScale(cam->GetParallelScale()/zoomFactor);
    }
  else
    {
    cam->Dolly(zoomFactor);
    this->ResetCameraClippingRange();
    }
  
  vtkRenderWindowInteractor *rwi = this->Interactor;
  if (this->CurrentLight)
    {
    /* get the first light */
    this->CurrentLight->SetPosition(cam->GetPosition());
    this->CurrentLight->SetFocalPoint(cam->GetFocalPoint());
    }
  
  rwi->Render();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTrackballCamera::SpinXY(int x, int y, int oldX, int oldY)
{
  vtkRenderWindowInteractor *rwi = this->Interactor;
  vtkCamera *cam;

  if (this->CurrentRenderer == NULL)
    {
    return;
    }

  double newAngle = atan2((double)(y - this->Center[1]),
                         (double)(x - this->Center[0]));
  double oldAngle = atan2((double)(oldY -this->Center[1]),
                         (double)(oldX - this->Center[0]));
  
  newAngle *= this->RadianToDegree;
  oldAngle *= this->RadianToDegree;

  cam = this->CurrentRenderer->GetActiveCamera();
  cam->Roll(newAngle - oldAngle);
  cam->OrthogonalizeViewUp();
      
  rwi->Render();
}

