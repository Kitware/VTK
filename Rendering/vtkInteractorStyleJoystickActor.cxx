/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleJoystickActor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInteractorStyleJoystickActor.h"

#include "vtkCamera.h"
#include "vtkCallbackCommand.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkProp3D.h"
#include "vtkRenderer.h"
#include "vtkCellPicker.h"
#include "vtkTransform.h"
#include "vtkMatrix4x4.h"

vtkStandardNewMacro(vtkInteractorStyleJoystickActor);

//----------------------------------------------------------------------------
vtkInteractorStyleJoystickActor::vtkInteractorStyleJoystickActor() 
{
  this->MotionFactor    = 10.0;
  this->InteractionProp = NULL;
  this->InteractionPicker = vtkCellPicker::New();
  this->InteractionPicker->SetTolerance(0.001);

  // Use timers to handle continous interaction
  this->UseTimers = 1;
}

//----------------------------------------------------------------------------
vtkInteractorStyleJoystickActor::~vtkInteractorStyleJoystickActor() 
{
  this->InteractionPicker->Delete();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleJoystickActor::OnMouseMove() 
{
  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];

  switch (this->State) 
    {
    case VTKIS_ROTATE:
    case VTKIS_PAN:
    case VTKIS_DOLLY:
    case VTKIS_SPIN:
    case VTKIS_USCALE:
      this->FindPokedRenderer(x, y);
      this->InvokeEvent(vtkCommand::InteractionEvent, NULL);
      break;
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleJoystickActor::OnLeftButtonDown() 
{
  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];

  this->FindPokedRenderer(x, y);
  this->FindPickedActor(x, y);
  if (this->CurrentRenderer == NULL || this->InteractionProp == NULL)
    {
    return;
    }

  this->GrabFocus(this->EventCallbackCommand);
  if (this->Interactor->GetShiftKey())
    {
    this->StartPan();
    }
  else if (this->Interactor->GetControlKey())
    {
    this->StartSpin();
    }
  else
    {
    this->StartRotate();
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleJoystickActor::OnLeftButtonUp() 
{
  switch (this->State) 
    {
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
  if ( this->Interactor )
    {
    this->ReleaseFocus();
    }
}


//----------------------------------------------------------------------------
void vtkInteractorStyleJoystickActor::OnMiddleButtonDown() 
{
  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];

  this->FindPokedRenderer(x, y);
  this->FindPickedActor(x, y);
  if (this->CurrentRenderer == NULL || this->InteractionProp == NULL)
    {
    return;
    }

  this->GrabFocus(this->EventCallbackCommand);
  if (this->Interactor->GetControlKey())
    {
    this->StartDolly();
    }
  else
    {
    this->StartPan();
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleJoystickActor::OnMiddleButtonUp() 
{
  switch (this->State) 
    {
    case VTKIS_DOLLY:
      this->EndDolly();
      break;

    case VTKIS_PAN:
      this->EndPan();
      break;
    }

  if ( this->Interactor )
    {
    this->ReleaseFocus();
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleJoystickActor::OnRightButtonDown() 
{
  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];

  this->FindPokedRenderer(x, y);
  this->FindPickedActor(x, y);
  if (this->CurrentRenderer == NULL || this->InteractionProp == NULL)
    {
    return;
    }
  
  this->GrabFocus(this->EventCallbackCommand);
  this->StartUniformScale();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleJoystickActor::OnRightButtonUp() 
{
  switch (this->State) 
    {
    case VTKIS_USCALE:
      this->EndUniformScale();
      if ( this->Interactor )
        {
        this->ReleaseFocus();
        }
      break;
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleJoystickActor::Rotate()
{
  if ( this->CurrentRenderer == NULL || this->InteractionProp == NULL )
    {
    return;
    }

  vtkRenderWindowInteractor *rwi = this->Interactor;
  vtkCamera *cam = this->CurrentRenderer->GetActiveCamera();
  
  // First get the origin of the assembly
  double *obj_center = this->InteractionProp->GetCenter();
  
  // GetLength gets the length of the diagonal of the bounding box
  double boundRadius = this->InteractionProp->GetLength() * 0.5;
  
  // Get the view up and view right vectors
  double view_up[3], view_look[3], view_right[3];

  cam->OrthogonalizeViewUp();
  cam->ComputeViewPlaneNormal();
  cam->GetViewUp(view_up);
  vtkMath::Normalize(view_up);
  cam->GetViewPlaneNormal(view_look);
  vtkMath::Cross(view_up, view_look, view_right);
  vtkMath::Normalize(view_right);
  
  // Get the furtherest point from object bounding box center
  double outsidept[3];

  outsidept[0] = obj_center[0] + view_right[0] * boundRadius;
  outsidept[1] = obj_center[1] + view_right[1] * boundRadius;
  outsidept[2] = obj_center[2] + view_right[2] * boundRadius;
  
  // Convert to display coord
  double disp_obj_center[3];

  this->ComputeWorldToDisplay(obj_center[0], obj_center[1], obj_center[2], 
                              disp_obj_center);

  this->ComputeWorldToDisplay(outsidept[0], outsidept[1], outsidept[2], 
                              outsidept);
  
  double radius = sqrt(vtkMath::Distance2BetweenPoints(disp_obj_center,
                                                       outsidept));
  
  double nxf = 
    (rwi->GetEventPosition()[0] - disp_obj_center[0]) / radius;

  double nyf = 
    (rwi->GetEventPosition()[1] - disp_obj_center[1]) / radius;
  
  if (nxf > 1.0)
    {
    nxf = 1.0;
    }
  else if (nxf < -1.0)
    {
    nxf = -1.0;
    }
  
  if (nyf > 1.0)
    {
    nyf = 1.0;
    }
  else if (nyf < -1.0)
    {
    nyf = -1.0;
    }
  
  double newXAngle = 
    vtkMath::DegreesFromRadians( asin( nxf ) ) / this->MotionFactor;

  double newYAngle = 
    vtkMath::DegreesFromRadians( asin( nyf ) ) / this->MotionFactor;
  
  double scale[3];
  scale[0] = scale[1] = scale[2] = 1.0;

  double **rotate = new double*[2];

  rotate[0] = new double[4];
  rotate[1] = new double[4];
  
  rotate[0][0] = newXAngle;
  rotate[0][1] = view_up[0];
  rotate[0][2] = view_up[1];
  rotate[0][3] = view_up[2];
  
  rotate[1][0] = -newYAngle;
  rotate[1][1] = view_right[0];
  rotate[1][2] = view_right[1];
  rotate[1][3] = view_right[2];
  
  this->Prop3DTransform(this->InteractionProp,
                        obj_center,
                        2, 
                        rotate, 
                        scale);
  
  delete [] rotate[0];
  delete [] rotate[1];
  delete [] rotate;
  
  if (this->AutoAdjustCameraClippingRange)
    {
    this->CurrentRenderer->ResetCameraClippingRange();
    }

  rwi->Render();
}
  
//----------------------------------------------------------------------------
void vtkInteractorStyleJoystickActor::Spin()
{
  if ( this->CurrentRenderer == NULL || this->InteractionProp == NULL )
    {
    return;
    }

  vtkRenderWindowInteractor *rwi = this->Interactor;
  vtkCamera *cam = this->CurrentRenderer->GetActiveCamera();
  
  // Get the axis to rotate around = vector from eye to origin
  double *obj_center = this->InteractionProp->GetCenter();
  
  double motion_vector[3];
  double view_point[3];

  if (cam->GetParallelProjection())
    {
    // If parallel projection, want to get the view plane normal...
    cam->ComputeViewPlaneNormal();
    cam->GetViewPlaneNormal(motion_vector);
    }
  else
    {
    // Perspective projection, get vector from eye to center of actor
    cam->GetPosition(view_point);
    motion_vector[0] = view_point[0] - obj_center[0];
    motion_vector[1] = view_point[1] - obj_center[1];
    motion_vector[2] = view_point[2] - obj_center[2];
    vtkMath::Normalize(motion_vector);
    }

  double disp_obj_center[3];
  
  this->ComputeWorldToDisplay(obj_center[0], obj_center[1],obj_center[2], 
                              disp_obj_center);
  
  double *center = this->CurrentRenderer->GetCenter();
  
  double yf = (rwi->GetEventPosition()[1] - disp_obj_center[1]) / center[1];

  if (yf > 1.0)
    {
    yf = 1.0;
    }
  else if (yf < -1.0)
    {
    yf = -1.0;
    }

  double newAngle = 
    vtkMath::DegreesFromRadians( asin( yf ) ) / this->MotionFactor;

  double scale[3];
  scale[0] = scale[1] = scale[2] = 1.0;

  double **rotate = new double*[1];
  rotate[0] = new double[4];

  rotate[0][0] = newAngle;
  rotate[0][1] = motion_vector[0];
  rotate[0][2] = motion_vector[1];
  rotate[0][3] = motion_vector[2];

  this->Prop3DTransform(this->InteractionProp,
                        obj_center,
                        1, 
                        rotate, 
                        scale);

  delete [] rotate[0];
  delete [] rotate;

  if (this->AutoAdjustCameraClippingRange)
    {
    this->CurrentRenderer->ResetCameraClippingRange();
    }
  
  rwi->Render();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleJoystickActor::Pan()
{
  if (this->CurrentRenderer == NULL || this->InteractionProp == NULL)
    {
    return;
    }

  vtkRenderWindowInteractor *rwi = this->Interactor;
  
  // Use initial center as the origin from which to pan
  double *obj_center = this->InteractionProp->GetCenter();

  double disp_obj_center[3], new_pick_point[4], motion_vector[3];
  
  this->ComputeWorldToDisplay(obj_center[0], obj_center[1], obj_center[2], 
                              disp_obj_center);

  this->ComputeDisplayToWorld(rwi->GetEventPosition()[0], 
                              rwi->GetEventPosition()[1],
                              disp_obj_center[2],
                              new_pick_point);
  
  // Compute a translation vector, moving everything 1/10
  // the distance to the cursor. (Arbitrary scale factor)
  motion_vector[0] = (new_pick_point[0] - obj_center[0]) / this->MotionFactor;
  motion_vector[1] = (new_pick_point[1] - obj_center[1]) / this->MotionFactor;
  motion_vector[2] = (new_pick_point[2] - obj_center[2]) / this->MotionFactor;
  
  if (this->InteractionProp->GetUserMatrix() != NULL)
    {
    vtkTransform *t = vtkTransform::New();
    t->PostMultiply();
    t->SetMatrix(this->InteractionProp->GetUserMatrix());
    t->Translate(motion_vector[0], motion_vector[1], motion_vector[2]);
    this->InteractionProp->GetUserMatrix()->DeepCopy(t->GetMatrix());
    t->Delete();
    }
  else
    {
    this->InteractionProp->AddPosition(motion_vector[0],
                                       motion_vector[1],
                                       motion_vector[2]);
    }
  
  rwi->Render();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleJoystickActor::Dolly()
{
  if (this->CurrentRenderer == NULL || this->InteractionProp == NULL)
    {
    return;
    }
  
  vtkRenderWindowInteractor *rwi = this->Interactor;
  vtkCamera *cam = this->CurrentRenderer->GetActiveCamera();

  // Dolly is based on distance from center of screen,
  // and the upper half is positive, lower half is negative
  double view_point[3], view_focus[3];
  double motion_vector[3];

  cam->GetPosition(view_point);
  cam->GetFocalPoint(view_focus);
  
  // Use initial center as the origin from which to pan
  double *obj_center = this->InteractionProp->GetCenter();

  double disp_obj_center[3];

  this->ComputeWorldToDisplay(obj_center[0], obj_center[1], obj_center[2], 
                              disp_obj_center);
  
  double *center = this->CurrentRenderer->GetCenter();
  
  double yf = (rwi->GetEventPosition()[1] - disp_obj_center[1]) / center[1];
  double dollyFactor = pow(1.1, yf);

  dollyFactor -= 1.0;
  motion_vector[0] = (view_point[0] - view_focus[0]) * dollyFactor;
  motion_vector[1] = (view_point[1] - view_focus[1]) * dollyFactor;
  motion_vector[2] = (view_point[2] - view_focus[2]) * dollyFactor;

  if (this->InteractionProp->GetUserMatrix() != NULL)
    {
    vtkTransform *t = vtkTransform::New();
    t->PostMultiply();
    t->SetMatrix(this->InteractionProp->GetUserMatrix());
    t->Translate(motion_vector[0], motion_vector[1], motion_vector[2]);
    this->InteractionProp->GetUserMatrix()->DeepCopy(t->GetMatrix());
    t->Delete();
    }
  else
    {
    this->InteractionProp->AddPosition(motion_vector[0],
                                       motion_vector[1],
                                       motion_vector[2]);
    }

  if (this->AutoAdjustCameraClippingRange)
    {
    this->CurrentRenderer->ResetCameraClippingRange();
    }

  rwi->Render();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleJoystickActor::UniformScale()
{
  if (this->CurrentRenderer == NULL || this->InteractionProp == NULL)
    {
    return;
    }
  
  vtkRenderWindowInteractor *rwi = this->Interactor;

  // Uniform scale is based on distance from center of screen,
  // and the upper half is positive, lower half is negative
  // use bounding box center as the origin from which to pan
  double *obj_center = this->InteractionProp->GetCenter();
  
  double disp_obj_center[3];

  this->ComputeWorldToDisplay(obj_center[0], obj_center[1], obj_center[2], 
                              disp_obj_center);
  
  double *center = this->CurrentRenderer->GetCenter();
    
  double yf = (rwi->GetEventPosition()[1] - disp_obj_center[1]) / center[1];
  double scaleFactor = pow(1.1, yf);
  
  double **rotate = NULL;
  
  double scale[3];
  scale[0] = scale[1] = scale[2] = scaleFactor;
  
  this->Prop3DTransform(this->InteractionProp,
                        obj_center,
                        0, 
                        rotate, 
                        scale);
  
  if (this->AutoAdjustCameraClippingRange)
    {
    this->CurrentRenderer->ResetCameraClippingRange();
    }
  
  rwi->Render();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleJoystickActor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkInteractorStyleJoystickActor::FindPickedActor(int x, int y)
{
  this->InteractionPicker->Pick(x, y, 0.0, this->CurrentRenderer);
  vtkProp *prop = this->InteractionPicker->GetViewProp();
  if (prop != NULL)
    {
    this->InteractionProp = vtkProp3D::SafeDownCast(prop);
    }
  else
    {
    this->InteractionProp = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleJoystickActor::Prop3DTransform(vtkProp3D *prop3D,
                                                      double *boxCenter,
                                                      int numRotation,
                                                      double **rotate,
                                                      double *scale)
{
  vtkMatrix4x4 *oldMatrix = vtkMatrix4x4::New();
  prop3D->GetMatrix(oldMatrix);
  
  double orig[3];
  prop3D->GetOrigin(orig);
  
  vtkTransform *newTransform = vtkTransform::New();
  newTransform->PostMultiply();
  if (prop3D->GetUserMatrix() != NULL) 
    {
    newTransform->SetMatrix(prop3D->GetUserMatrix());
    }
  else 
    {
    newTransform->SetMatrix(oldMatrix);
    }
  
  newTransform->Translate(-(boxCenter[0]), -(boxCenter[1]), -(boxCenter[2]));
  
  for (int i = 0; i < numRotation; i++) 
    {
    newTransform->RotateWXYZ(rotate[i][0], rotate[i][1],
                             rotate[i][2], rotate[i][3]);
    }
  
  if ((scale[0] * scale[1] * scale[2]) != 0.0) 
    {
    newTransform->Scale(scale[0], scale[1], scale[2]);
    }
  
  newTransform->Translate(boxCenter[0], boxCenter[1], boxCenter[2]);
  
  // now try to get the composit of translate, rotate, and scale
  newTransform->Translate(-(orig[0]), -(orig[1]), -(orig[2]));
  newTransform->PreMultiply();
  newTransform->Translate(orig[0], orig[1], orig[2]);
  
  if (prop3D->GetUserMatrix() != NULL) 
    {
    newTransform->GetMatrix(prop3D->GetUserMatrix());
    }
  else 
    {
    prop3D->SetPosition(newTransform->GetPosition());
    prop3D->SetScale(newTransform->GetScale());
    prop3D->SetOrientation(newTransform->GetOrientation());
    }
  oldMatrix->Delete();
  newTransform->Delete();
}
