/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleJoystickActor.cxx
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
#include "vtkInteractorStyleJoystickActor.h"
#include "vtkObjectFactory.h"
#include "vtkMath.h"
#include "vtkCommand.h"

vtkCxxRevisionMacro(vtkInteractorStyleJoystickActor, "1.20");
vtkStandardNewMacro(vtkInteractorStyleJoystickActor);

//----------------------------------------------------------------------------
vtkInteractorStyleJoystickActor::vtkInteractorStyleJoystickActor() 
{
  int i;
  
  this->MotionFactor    = 10.0;
  this->RadianToDegree  = 180.0 / vtkMath::Pi();
  this->InteractionProp = NULL;

  for (i = 0; i < 3; i++)
    {
    this->ViewUp[i]        = 0.0;
    this->ViewLook[i]      = 0.0;
    this->ViewRight[i]     = 0.0;
    this->ObjCenter[i]     = 0.0;
    this->DispObjCenter[i] = 0.0;
    this->NewPickPoint[i]  = 0.0;
    this->OldPickPoint[i]  = 0.0;
    this->MotionVector[i]  = 0.0;
    this->ViewPoint[i]     = 0.0;
    this->ViewFocus[i]     = 0.0;
    }

  this->NewPickPoint[3]    = 1.0;
  this->OldPickPoint[3]    = 1.0;
  
  this->Radius = 0.0;

  this->InteractionPicker = vtkCellPicker::New();
}

//----------------------------------------------------------------------------
vtkInteractorStyleJoystickActor::~vtkInteractorStyleJoystickActor() 
{
  this->InteractionPicker->Delete();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleJoystickActor::OnMouseMove(int vtkNotUsed(ctrl), 
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

    case VTKIS_USCALE:
      this->FindPokedCamera(x, y);
      break;
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleJoystickActor::OnLeftButtonDown(int ctrl, 
                                                       int shift, 
                                                       int x, 
                                                       int y) 
{
  this->FindPokedRenderer(x, y);
  this->FindPickedActor(x, y);
  if (this->CurrentRenderer == NULL || this->InteractionProp == NULL)
    {
    return;
    }

  if (shift)
    {
    this->StartPan();
    }
  else if (ctrl)
    {
    this->StartSpin();
    }
  else
    {
    this->StartRotate();
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleJoystickActor::OnLeftButtonUp(int vtkNotUsed(ctrl),
                                                     int vtkNotUsed(shift), 
                                                     int vtkNotUsed(x), 
                                                     int vtkNotUsed(y)) 
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
}

//----------------------------------------------------------------------------
void vtkInteractorStyleJoystickActor::OnMiddleButtonDown(int ctrl, 
                                                         int vtkNotUsed(shift), 
                                                         int x, 
                                                         int y) 
{
  this->FindPokedRenderer(x, y);
  this->FindPickedActor(x, y);
  if (this->CurrentRenderer == NULL || this->InteractionProp == NULL)
    {
    return;
    }

  if (ctrl)
    {
    this->StartDolly();
    }
  else
    {
    this->StartPan();
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleJoystickActor::OnMiddleButtonUp(int vtkNotUsed(ctrl),
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
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleJoystickActor::OnRightButtonDown(int vtkNotUsed(ctrl),
                                                        int vtkNotUsed(shift), 
                                                        int x, 
                                                        int y) 
{
  this->FindPokedRenderer(x, y);
  this->FindPickedActor(x, y);
  if (this->CurrentRenderer == NULL || this->InteractionProp == NULL)
    {
    return;
    }
  
  this->StartUniformScale();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleJoystickActor::OnRightButtonUp(int vtkNotUsed(ctrl),
                                                      int vtkNotUsed(shift), 
                                                      int vtkNotUsed(x),
                                                      int vtkNotUsed(y)) 
{
  switch (this->State) 
    {
    case VTKIS_USCALE:
      this->EndUniformScale();
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

  int x = rwi->GetEventPosition()[0];
  int y = rwi->GetEventPosition()[1];
 
  vtkCamera *cam = this->CurrentRenderer->GetActiveCamera();
  
  // first get the origin of the assembly
  float *center = this->InteractionProp->GetCenter();
  this->ObjCenter[0] = center[0];
  this->ObjCenter[1] = center[1];
  this->ObjCenter[2] = center[2];
  
  // GetLength gets the length of the diagonal of the bounding box
  double boundRadius = this->InteractionProp->GetLength() * 0.5;
  
  // get the view up and view right vectors
  cam->OrthogonalizeViewUp();
  cam->ComputeViewPlaneNormal();
  cam->GetViewUp(this->ViewUp);
  vtkMath::Normalize(this->ViewUp);
  cam->GetViewPlaneNormal(this->ViewLook);
  vtkMath::Cross(this->ViewUp, this->ViewLook, this->ViewRight);
  vtkMath::Normalize(this->ViewRight);
  
  // get the furtherest point from object bounding box center
  float outsidept[3];
  outsidept[0] = this->ObjCenter[0] + this->ViewRight[0] * boundRadius;
  outsidept[1] = this->ObjCenter[1] + this->ViewRight[1] * boundRadius;
  outsidept[2] = this->ObjCenter[2] + this->ViewRight[2] * boundRadius;
  
  // convert to display coord
  this->ComputeWorldToDisplay(this->ObjCenter[0], this->ObjCenter[1],
                              this->ObjCenter[2], this->DispObjCenter);
  this->ComputeWorldToDisplay(outsidept[0], outsidept[1],
                              outsidept[2], outsidept);
  
  this->Radius = sqrt(vtkMath::Distance2BetweenPoints(this->DispObjCenter,
                                                      outsidept));
  
  this->HighlightProp3D(NULL);
  
  double nxf = ((double)(x) - this->DispObjCenter[0]) / this->Radius;
  double nyf = ((double)(y) - this->DispObjCenter[1]) / this->Radius;
  
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
  
  double newXAngle = asin(nxf) * this->RadianToDegree / this->MotionFactor;
  double newYAngle = asin(nyf) * this->RadianToDegree / this->MotionFactor;
  
  double scale[3];
  scale[0] = scale[1] = scale[2] = 1.0;
  double **rotate = new double*[2];
  rotate[0] = new double[4];
  rotate[1] = new double[4];
  
  rotate[0][0] = newXAngle;
  rotate[0][1] = this->ViewUp[0];
  rotate[0][2] = this->ViewUp[1];
  rotate[0][3] = this->ViewUp[2];
  
  rotate[1][0] = -newYAngle;
  rotate[1][1] = this->ViewRight[0];
  rotate[1][2] = this->ViewRight[1];
  rotate[1][3] = this->ViewRight[2];
  
  this->Prop3DTransform(this->InteractionProp,
                        this->ObjCenter,
                        2, rotate, scale);
  
  delete [] rotate[0];
  delete [] rotate[1];
  delete [] rotate;
  
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

  int y = rwi->GetEventPosition()[1];

  vtkCamera *cam = this->CurrentRenderer->GetActiveCamera();
  
  // get the axis to rotate around = vector from eye to origin
  float *obj_center = this->InteractionProp->GetCenter();
  this->ObjCenter[0] = obj_center[0];
  this->ObjCenter[1] = obj_center[1];
  this->ObjCenter[2] = obj_center[2];
  
  if (cam->GetParallelProjection())
    {
    // if parallel projection, want to get the view plane normal...
    cam->ComputeViewPlaneNormal();
    cam->GetViewPlaneNormal(this->MotionVector);
    }
  else
    {
    // perspective projection, get vector from eye to center of actor
    cam->GetPosition(this->ViewPoint);
    this->MotionVector[0] = this->ViewPoint[0] - this->ObjCenter[0];
    this->MotionVector[1] = this->ViewPoint[1] - this->ObjCenter[1];
    this->MotionVector[2] = this->ViewPoint[2] - this->ObjCenter[2];
    vtkMath::Normalize(this->MotionVector);
    }
  
  this->ComputeWorldToDisplay(this->ObjCenter[0], this->ObjCenter[1],
                              this->ObjCenter[2], this->DispObjCenter);
  
  this->HighlightProp3D(NULL);

  float *center = this->CurrentRenderer->GetCenter();
  
  double yf = ((double)(y) - this->DispObjCenter[1]) / (double)(center[1]);
  if (yf > 1.0)
    {
    yf = 1.0;
    }
  else if (yf < -1.0)
    {
    yf = -1.0;
    }

  double newAngle = asin(yf) * this->RadianToDegree / this->MotionFactor;

  double scale[3];
  scale[0] = scale[1] = scale[2] = 1.0;
  double **rotate = new double*[1];
  rotate[0] = new double[4];

  rotate[0][0] = newAngle;
  rotate[0][1] = this->MotionVector[0];
  rotate[0][2] = this->MotionVector[1];
  rotate[0][3] = this->MotionVector[2];

  this->Prop3DTransform(this->InteractionProp,
                        this->ObjCenter,
                        1, rotate, scale);

  delete [] rotate[0];
  delete [] rotate;

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
  
  int x = rwi->GetEventPosition()[0];
  int y = rwi->GetEventPosition()[1];
 
  // use initial center as the origin from which to pan
  float *obj_center = this->InteractionProp->GetCenter();
  this->ObjCenter[0] = obj_center[0];
  this->ObjCenter[1] = obj_center[1];
  this->ObjCenter[2] = obj_center[2];
  
  this->ComputeWorldToDisplay(this->ObjCenter[0], this->ObjCenter[1],
                              this->ObjCenter[2], this->DispObjCenter);

  this->HighlightProp3D(NULL);
  
  this->ComputeDisplayToWorld(double(x), double(y),
                              this->DispObjCenter[2],
                              this->NewPickPoint);
  
  /*
   * Compute a translation vector, moving everything 1/10
   * the distance to the cursor. (Arbitrary scale factor)
   */
  this->MotionVector[0] = (this->NewPickPoint[0] -
                           this->ObjCenter[0]) / this->MotionFactor;
  this->MotionVector[1] = (this->NewPickPoint[1] -
                           this->ObjCenter[1]) / this->MotionFactor;
  this->MotionVector[2] = (this->NewPickPoint[2] -
                           this->ObjCenter[2]) / this->MotionFactor;
  
  if (this->InteractionProp->GetUserMatrix() != NULL)
    {
    vtkTransform *t = vtkTransform::New();
    t->PostMultiply();
    t->SetMatrix(this->InteractionProp->GetUserMatrix());
    t->Translate(this->MotionVector[0], this->MotionVector[1],
                 this->MotionVector[2]);
    this->InteractionProp->GetUserMatrix()->DeepCopy(t->GetMatrix());
    t->Delete();
    }
  else
    {
    this->InteractionProp->AddPosition(this->MotionVector);
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

  int y = rwi->GetEventPosition()[1];
 
  vtkCamera *cam = this->CurrentRenderer->GetActiveCamera();

  // dolly is based on distance from center of screen,
  // and the upper half is positive, lower half is negative
  
  cam->GetPosition(this->ViewPoint);
  cam->GetFocalPoint(this->ViewFocus);
  
  // use initial center as the origin from which to pan
  float *obj_center = this->InteractionProp->GetCenter();
  this->ObjCenter[0] = obj_center[0];
  this->ObjCenter[1] = obj_center[1];
  this->ObjCenter[2] = obj_center[2];
  this->ComputeWorldToDisplay(this->ObjCenter[0], this->ObjCenter[1],
                              this->ObjCenter[2], this->DispObjCenter);
  
  this->HighlightProp3D(NULL);
  
  float *center = this->CurrentRenderer->GetCenter();
  
  double yf = ((double)(y) - this->DispObjCenter[1]) / (double)(center[1]);
  double dollyFactor = pow((double)1.1, yf);

  dollyFactor -= 1.0;
  this->MotionVector[0] = (this->ViewPoint[0] -
                           this->ViewFocus[0]) * dollyFactor;
  this->MotionVector[1] = (this->ViewPoint[1] -
                           this->ViewFocus[1]) * dollyFactor;
  this->MotionVector[2] = (this->ViewPoint[2] -
                           this->ViewFocus[2]) * dollyFactor;

  if (this->InteractionProp->GetUserMatrix() != NULL)
    {
    vtkTransform *t = vtkTransform::New();
    t->PostMultiply();
    t->SetMatrix(this->InteractionProp->GetUserMatrix());
    t->Translate(this->MotionVector[0], this->MotionVector[1],
                 this->MotionVector[2]);
    this->InteractionProp->GetUserMatrix()->DeepCopy(t->GetMatrix());
    t->Delete();
    }
  else
    {
    this->InteractionProp->AddPosition(this->MotionVector);
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

  int y = rwi->GetEventPosition()[1];
 
  // Uniform scale is based on distance from center of screen,
  // and the upper half is positive, lower half is negative

  // use bounding box center as the origin from which to pan
  float *obj_center = this->InteractionProp->GetCenter();
  this->ObjCenter[0] = obj_center[0];
  this->ObjCenter[1] = obj_center[1];
  this->ObjCenter[2] = obj_center[2];
  
  this->ComputeWorldToDisplay(this->ObjCenter[0], this->ObjCenter[1],
                              this->ObjCenter[2], this->DispObjCenter);
  
  this->HighlightProp3D(NULL);

  float *center = this->CurrentRenderer->GetCenter();
    
  double yf = ((double)(y) - this->DispObjCenter[1]) / (double)(center[1]);
  double scaleFactor = pow((double)1.1, yf);
  
  double **rotate = NULL;
  
  double scale[3];
  scale[0] = scale[1] = scale[2] = scaleFactor;
  
  this->Prop3DTransform(this->InteractionProp,
                        this->ObjCenter,
                        0, rotate, scale);
  
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
  vtkProp *prop = this->InteractionPicker->GetProp();
  if (prop != NULL)
    {
    this->InteractionProp = vtkProp3D::SafeDownCast(prop);
    }
  else
    {
    this->InteractionProp = NULL;
    }
  
  // refine the answer to whether an actor was picked.  CellPicker()
  // returns true from Pick() if the bounding box was picked,
  // but we only want something to be picked if a cell was actually
  // selected
  this->PropPicked = (this->InteractionProp != NULL);
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
  
  float orig[3];
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

//----------------------------------------------------------------------------
void vtkInteractorStyleJoystickActor::Prop3DTransform(vtkProp3D *prop3D,
                                                      float *boxCenter,
                                                      int numRotation,
                                                      double **rotate,
                                                      double *scale)
{
  double boxCenter2[3];
  boxCenter2[0] = boxCenter[0];
  boxCenter2[1] = boxCenter[1];
  boxCenter2[2] = boxCenter[2];
  this->Prop3DTransform(prop3D,boxCenter2,numRotation,rotate,scale);
}

