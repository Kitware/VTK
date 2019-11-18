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

#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPlane.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTransform.h"

vtkStandardNewMacro(vtkInteractorStyleMultiTouchCamera);

//----------------------------------------------------------------------------
vtkInteractorStyleMultiTouchCamera::vtkInteractorStyleMultiTouchCamera() = default;

//----------------------------------------------------------------------------
vtkInteractorStyleMultiTouchCamera::~vtkInteractorStyleMultiTouchCamera() = default;

//----------------------------------------------------------------------------
void vtkInteractorStyleMultiTouchCamera::OnStartRotate()
{
  this->StartGesture();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleMultiTouchCamera::OnRotate()
{
  if (this->State != VTKIS_GESTURE)
  {
    return;
  }

  int pointer = this->Interactor->GetPointerIndex();

  this->FindPokedRenderer(this->Interactor->GetEventPositions(pointer)[0],
    this->Interactor->GetEventPositions(pointer)[1]);

  if (this->CurrentRenderer == nullptr)
  {
    return;
  }

  vtkCamera* camera = this->CurrentRenderer->GetActiveCamera();

  int* pinchPositionDisplay = this->Interactor->GetEventPositions(pointer);

  // Calculate the focal depth since we'll be using it a lot
  double viewFocus[4];
  camera->GetFocalPoint(viewFocus);
  this->ComputeWorldToDisplay(viewFocus[0], viewFocus[1], viewFocus[2], viewFocus);
  double focalDepth = viewFocus[2];

  double oldPickPoint[4] = { 0, 0, 0, 0 };
  vtkInteractorObserver::ComputeDisplayToWorld(this->CurrentRenderer, pinchPositionDisplay[0],
    pinchPositionDisplay[1], focalDepth, oldPickPoint);

  camera->Roll(this->Interactor->GetRotation() - this->Interactor->GetLastRotation());

  camera->GetFocalPoint(viewFocus);
  this->ComputeWorldToDisplay(viewFocus[0], viewFocus[1], viewFocus[2], viewFocus);
  focalDepth = viewFocus[2];

  double newPickPoint[4] = { 0, 0, 0, 0 };
  vtkInteractorObserver::ComputeDisplayToWorld(this->CurrentRenderer, pinchPositionDisplay[0],
    pinchPositionDisplay[1], focalDepth, newPickPoint);

  double motionVector[3] = { 0, 0, 0 };
  vtkMath::Subtract(oldPickPoint, newPickPoint, motionVector);

  vtkNew<vtkTransform> cameraTransform;
  cameraTransform->Identity();
  cameraTransform->Translate(motionVector);
  camera->ApplyTransform(cameraTransform);

  camera->OrthogonalizeViewUp();

  this->Interactor->Render();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleMultiTouchCamera::OnEndRotate()
{
  this->EndGesture();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleMultiTouchCamera::OnStartPinch()
{
  this->StartGesture();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleMultiTouchCamera::OnPinch()
{
  if (this->State != VTKIS_GESTURE)
  {
    return;
  }

  int pointer = this->Interactor->GetPointerIndex();

  this->FindPokedRenderer(this->Interactor->GetEventPositions(pointer)[0],
    this->Interactor->GetEventPositions(pointer)[1]);

  if (this->CurrentRenderer == nullptr)
  {
    return;
  }

  vtkCamera* camera = this->CurrentRenderer->GetActiveCamera();

  int* pinchPositionDisplay = this->Interactor->GetEventPositions(pointer);

  // Calculate the focal depth since we'll be using it a lot
  double viewFocus[4], focalDepth;
  camera->GetFocalPoint(viewFocus);
  this->ComputeWorldToDisplay(viewFocus[0], viewFocus[1], viewFocus[2], viewFocus);
  focalDepth = viewFocus[2];

  // Remember the position of the center of the pinch in world coordinates
  // This position should stay in the same location on the screen after the dolly has been performed
  double oldPickPoint[4] = { 0, 0, 0, 0 };
  this->ComputeDisplayToWorld(
    pinchPositionDisplay[0], pinchPositionDisplay[1], focalDepth, oldPickPoint);

  double dyf = this->Interactor->GetScale() / this->Interactor->GetLastScale();
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

  camera->GetFocalPoint(viewFocus);
  this->ComputeWorldToDisplay(viewFocus[0], viewFocus[1], viewFocus[2], viewFocus);
  focalDepth = viewFocus[2];

  // New position at the center of the pinch gesture
  double newPickPoint[4] = { 0, 0, 0, 0 };
  this->ComputeDisplayToWorld(
    pinchPositionDisplay[0], pinchPositionDisplay[1], focalDepth, newPickPoint);

  // Determine how far the pinch center has been shifted from it's original location
  double motionVector[4] = { 0, 0, 0 };
  vtkMath::Subtract(oldPickPoint, newPickPoint, motionVector);

  // Translate the camera to compensate for the shift of the pinch center
  vtkNew<vtkTransform> cameraTransform;
  cameraTransform->Identity();
  cameraTransform->Translate(motionVector);
  camera->ApplyTransform(cameraTransform);

  if (this->Interactor->GetLightFollowCamera())
  {
    this->CurrentRenderer->UpdateLightsGeometryToFollowCamera();
  }
  this->Interactor->Render();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleMultiTouchCamera::OnEndPinch()
{
  this->EndGesture();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleMultiTouchCamera::OnStartPan()
{
  this->StartGesture();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleMultiTouchCamera::OnPan()
{
  if (this->State != VTKIS_GESTURE)
  {
    return;
  }

  int pointer = this->Interactor->GetPointerIndex();

  this->FindPokedRenderer(this->Interactor->GetEventPositions(pointer)[0],
    this->Interactor->GetEventPositions(pointer)[1]);

  if (this->CurrentRenderer == nullptr)
  {
    return;
  }

  vtkCamera* camera = this->CurrentRenderer->GetActiveCamera();
  vtkRenderWindowInteractor* rwi = this->Interactor;

  // handle panning - 2 DOF
  double viewFocus[4], focalDepth, viewPoint[3];
  double newPickPoint[4], oldPickPoint[4], motionVector[3];

  // Calculate the focal depth since we'll be using it a lot
  camera->GetFocalPoint(viewFocus);
  this->ComputeWorldToDisplay(viewFocus[0], viewFocus[1], viewFocus[2], viewFocus);
  focalDepth = viewFocus[2];

  double* trans = this->Interactor->GetTranslation();
  this->ComputeDisplayToWorld(
    viewFocus[0] + trans[0], viewFocus[1] + trans[1], focalDepth, newPickPoint);

  // Has to recalc old mouse point since the viewport has moved,
  // so can't move it outside the loop
  this->ComputeDisplayToWorld(viewFocus[0], viewFocus[1], focalDepth, oldPickPoint);

  // Camera motion is reversed
  motionVector[0] = oldPickPoint[0] - newPickPoint[0];
  motionVector[1] = oldPickPoint[1] - newPickPoint[1];
  motionVector[2] = oldPickPoint[2] - newPickPoint[2];

  camera->GetFocalPoint(viewFocus);
  camera->GetPosition(viewPoint);
  camera->SetFocalPoint(
    motionVector[0] + viewFocus[0], motionVector[1] + viewFocus[1], motionVector[2] + viewFocus[2]);

  camera->SetPosition(
    motionVector[0] + viewPoint[0], motionVector[1] + viewPoint[1], motionVector[2] + viewPoint[2]);

  // clean up
  if (this->Interactor->GetLightFollowCamera())
  {
    this->CurrentRenderer->UpdateLightsGeometryToFollowCamera();
  }
  camera->OrthogonalizeViewUp();

  rwi->Render();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleMultiTouchCamera::OnEndPan()
{
  this->EndGesture();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleMultiTouchCamera::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
