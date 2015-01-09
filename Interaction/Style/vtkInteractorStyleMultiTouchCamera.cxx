/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleMultiTouchCamera.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInteractorStyleMultiTouchCamera.h"

#include "vtkCamera.h"
#include "vtkCallbackCommand.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

vtkStandardNewMacro(vtkInteractorStyleMultiTouchCamera);

//----------------------------------------------------------------------------
vtkInteractorStyleMultiTouchCamera::vtkInteractorStyleMultiTouchCamera()
{
  this->MotionFactor   = 10.0;
  this->PointersDownCount = 0;
  for (int i = 0; i < VTKI_MAX_POINTERS; ++i)
    {
    this->PointersDown[i] = 0;
    }
}

//----------------------------------------------------------------------------
vtkInteractorStyleMultiTouchCamera::~vtkInteractorStyleMultiTouchCamera()
{
}

//----------------------------------------------------------------------------
void vtkInteractorStyleMultiTouchCamera::OnMouseMove()
{
  int pointer = this->Interactor->GetPointerIndex();

  this->FindPokedRenderer(this->Interactor->GetEventPositions(pointer)[0],
                          this->Interactor->GetEventPositions(pointer)[1]);
  if (this->State == VTKIS_TWO_POINTER)
    {
    this->AdjustCamera();
    this->InvokeEvent(vtkCommand::InteractionEvent, NULL);
    }
  else
    {
    this->Superclass::OnMouseMove();
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleMultiTouchCamera::OnLeftButtonDown()
{
  int pointer = this->Interactor->GetPointerIndex();

  // if it is already down ignore this event
  if (this->PointersDown[pointer])
    {
    return;
    }

  this->FindPokedRenderer(this->Interactor->GetEventPositions(pointer)[0],
                          this->Interactor->GetEventPositions(pointer)[1]);
  if (this->CurrentRenderer == NULL)
    {
    return;
    }

  this->PointersDown[pointer] = 1;
  this->PointersDownCount++;

  // do the standard single pointer event handling
  if (this->PointersDownCount == 1)
    {
    this->Superclass::OnLeftButtonDown();
    return;
    }

  // if going from 1 to 2 pointers stop the one pointer action
  if (this->PointersDownCount == 2)
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
    // start the multipointer action
    this->StartTwoPointer();
    return;
    }

  // if going from 2 to 3 pointers stop the two pointer action
  if (this->PointersDownCount == 3 && this->State == VTKIS_TWO_POINTER)
    {
    this->EndTwoPointer();
    }

}

//----------------------------------------------------------------------------
void vtkInteractorStyleMultiTouchCamera::OnLeftButtonUp()
{
  int pointer = this->Interactor->GetPointerIndex();

  // if it is already up, ignore this event
  if (!this->PointersDown[pointer])
    {
    return;
    }

  this->PointersDownCount--;
  this->PointersDown[pointer] = 0;

  // if we were just one pointer then do the usual handling
  if (this->PointersDownCount == 0)
    {
    this->Superclass::OnLeftButtonUp();
    return;
    }

  switch (this->State)
    {
    case VTKIS_TWO_POINTER:
      this->EndTwoPointer();
      break;
    }

  if ( this->Interactor )
    {
    this->ReleaseFocus();
    }
}

double distance2D(int *a, int *b)
{
  return sqrt((double)(a[0] - b[0])*(a[0] - b[0]) + (double)(a[1]-b[1])*(a[1]-b[1]));
}

//----------------------------------------------------------------------------
void vtkInteractorStyleMultiTouchCamera::AdjustCamera()
{
  if ( this->CurrentRenderer == NULL )
    {
    return;
    }

  vtkRenderWindowInteractor *rwi = this->Interactor;

  // OK we have two pointers, that means 4 constraints
  // P1.x P1.y P2.x P2.y
  //
  // We use those 4 contraints to control:
  // Zoom (variant is the distance between points - 1 DOF)
  // Roll (variant is the angle formed by the line connecting the points - 1 DOF)
  // Position (variant is the X,Y position of the midpoint of the line - 2 DOF)
  //


  // find the moving and non moving points
  int eventPI = rwi->GetPointerIndex();
  int otherPI = 0;

  for (int i = 0; i < VTKI_MAX_POINTERS; ++i)
    {
    if (this->PointersDown[i] > 0 && i != eventPI)
      {
      otherPI = i;
      break;
      }
    }

  // compute roll - 1 DOF
  double oldAngle =
    vtkMath::DegreesFromRadians( atan2( (double)rwi->GetLastEventPositions(eventPI)[1] - rwi->GetLastEventPositions(otherPI)[1],
                                        (double)rwi->GetLastEventPositions(eventPI)[0] - rwi->GetLastEventPositions(otherPI)[0] ) );

  double newAngle =
    vtkMath::DegreesFromRadians( atan2( (double)rwi->GetEventPositions(eventPI)[1] - rwi->GetEventPositions(otherPI)[1],
                                        (double)rwi->GetEventPositions(eventPI)[0] - rwi->GetEventPositions(otherPI)[0] ) );


  vtkCamera *camera = this->CurrentRenderer->GetActiveCamera();
  camera->Roll( newAngle - oldAngle );

  // compute dolly/scale - 1 DOF
  double oldDist = distance2D(rwi->GetLastEventPositions(otherPI), rwi->GetLastEventPositions(eventPI));
  double newDist = distance2D(rwi->GetEventPositions(otherPI), rwi->GetEventPositions(eventPI));

  double dyf = newDist/oldDist;
  if (camera->GetParallelProjection())
    {
    camera->SetParallelScale(camera->GetParallelScale() / dyf);
    }
  else
    {
    camera->Dolly(dyf);
    if (this->AutoAdjustCameraClippingRange)
      {
      this->CurrentRenderer->ResetCameraClippingRange();
      }
    }

  // handle panning - 2 DOF
  double viewFocus[4], focalDepth, viewPoint[3];
  double newPickPoint[4], oldPickPoint[4], motionVector[3];

  // Calculate the focal depth since we'll be using it a lot
  camera->GetFocalPoint(viewFocus);
  this->ComputeWorldToDisplay(viewFocus[0], viewFocus[1], viewFocus[2],
                              viewFocus);
  focalDepth = viewFocus[2];

  this->ComputeDisplayToWorld((rwi->GetEventPositions(eventPI)[0] + rwi->GetEventPositions(otherPI)[0])/2.0,
                              (rwi->GetEventPositions(eventPI)[1] + rwi->GetEventPositions(otherPI)[1])/2.0,
                              focalDepth,
                              newPickPoint);

  // Has to recalc old mouse point since the viewport has moved,
  // so can't move it outside the loop
  this->ComputeDisplayToWorld((rwi->GetLastEventPositions(eventPI)[0] + rwi->GetLastEventPositions(otherPI)[0])/2.0,
                              (rwi->GetLastEventPositions(eventPI)[1] + rwi->GetLastEventPositions(otherPI)[1])/2.0,
                              focalDepth,
                              oldPickPoint);

  // Camera motion is reversed
  motionVector[0] = oldPickPoint[0] - newPickPoint[0];
  motionVector[1] = oldPickPoint[1] - newPickPoint[1];
  motionVector[2] = oldPickPoint[2] - newPickPoint[2];

  camera->GetFocalPoint(viewFocus);
  camera->GetPosition(viewPoint);
  camera->SetFocalPoint(motionVector[0] + viewFocus[0],
                        motionVector[1] + viewFocus[1],
                        motionVector[2] + viewFocus[2]);

  camera->SetPosition(motionVector[0] + viewPoint[0],
                      motionVector[1] + viewPoint[1],
                      motionVector[2] + viewPoint[2]);

  // clean up
  if (this->Interactor->GetLightFollowCamera())
    {
    this->CurrentRenderer->UpdateLightsGeometryToFollowCamera();
    }
  camera->OrthogonalizeViewUp();

  rwi->Render();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleMultiTouchCamera::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "MotionFactor: " << this->MotionFactor << "\n";
}
