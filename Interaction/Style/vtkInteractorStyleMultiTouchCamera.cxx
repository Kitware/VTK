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
}

//----------------------------------------------------------------------------
vtkInteractorStyleMultiTouchCamera::~vtkInteractorStyleMultiTouchCamera()
{
}

//----------------------------------------------------------------------------
void vtkInteractorStyleMultiTouchCamera::OnRotate()
{
  int pointer = this->Interactor->GetPointerIndex();

  this->FindPokedRenderer(this->Interactor->GetEventPositions(pointer)[0],
                          this->Interactor->GetEventPositions(pointer)[1]);

  if ( this->CurrentRenderer == NULL )
  {
    return;
  }

  vtkCamera *camera = this->CurrentRenderer->GetActiveCamera();
  camera->Roll( this->Interactor->GetRotation() - this->Interactor->GetLastRotation() );

  camera->OrthogonalizeViewUp();

  this->Interactor->Render();
}


//----------------------------------------------------------------------------
void vtkInteractorStyleMultiTouchCamera::OnPinch()
{
  int pointer = this->Interactor->GetPointerIndex();

  this->FindPokedRenderer(this->Interactor->GetEventPositions(pointer)[0],
                          this->Interactor->GetEventPositions(pointer)[1]);

  if ( this->CurrentRenderer == NULL )
  {
    return;
  }

  vtkCamera *camera = this->CurrentRenderer->GetActiveCamera();

  double dyf = this->Interactor->GetScale()/this->Interactor->GetLastScale();
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

  if (this->Interactor->GetLightFollowCamera())
  {
    this->CurrentRenderer->UpdateLightsGeometryToFollowCamera();
  }
  this->Interactor->Render();
}


//----------------------------------------------------------------------------
void vtkInteractorStyleMultiTouchCamera::OnPan()
{
  int pointer = this->Interactor->GetPointerIndex();

  this->FindPokedRenderer(this->Interactor->GetEventPositions(pointer)[0],
                          this->Interactor->GetEventPositions(pointer)[1]);

  if ( this->CurrentRenderer == NULL )
  {
    return;
  }

  vtkCamera *camera = this->CurrentRenderer->GetActiveCamera();
  vtkRenderWindowInteractor *rwi = this->Interactor;

  // handle panning - 2 DOF
  double viewFocus[4], focalDepth, viewPoint[3];
  double newPickPoint[4], oldPickPoint[4], motionVector[3];

  // Calculate the focal depth since we'll be using it a lot
  camera->GetFocalPoint(viewFocus);
  this->ComputeWorldToDisplay(viewFocus[0], viewFocus[1], viewFocus[2],
                              viewFocus);
  focalDepth = viewFocus[2];

  double *trans = this->Interactor->GetTranslation();
  double *lastTrans = this->Interactor->GetLastTranslation();
  this->ComputeDisplayToWorld(viewFocus[0] + trans[0] - lastTrans[0],
                              viewFocus[1] + trans[1] - lastTrans[1],
                              focalDepth,
                              newPickPoint);

  // Has to recalc old mouse point since the viewport has moved,
  // so can't move it outside the loop
  this->ComputeDisplayToWorld(viewFocus[0],
                              viewFocus[1],
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
}
