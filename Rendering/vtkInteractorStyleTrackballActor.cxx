/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleTrackballActor.cxx
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
#include "vtkInteractorStyleTrackballActor.h"
#include "vtkObjectFactory.h"
#include "vtkMath.h"
#include "vtkCommand.h"

vtkCxxRevisionMacro(vtkInteractorStyleTrackballActor, "1.21");
vtkStandardNewMacro(vtkInteractorStyleTrackballActor);

//----------------------------------------------------------------------------
vtkInteractorStyleTrackballActor::vtkInteractorStyleTrackballActor() 
{
  this->MotionFactor    = 10.0;
  this->RadianToDegree  = 180.0 / vtkMath::Pi();
  this->InteractionProp = NULL;

  int i;
  for (i = 0; i < 3; i++)
    {
    this->ViewUp[i]        = 0.0;
    this->ViewLook[i]      = 0.0;
    this->ViewRight[i]     = 0.0;
    this->ObjCenter[i]      = 0.0;
    this->DispObjCenter[i] = 0.0;
    this->NewPickPoint[i]  = 0.0;
    this->OldPickPoint[i]  = 0.0;
    this->MotionVector[i]  = 0.0;
    this->ViewPoint[i]     = 0.0;
    this->ViewFocus[i]     = 0.0;
    }

  this->NewPickPoint[3] = 1.0;
  this->OldPickPoint[3] = 1.0;
  
  this->Radius = 0.0;
  
  this->InteractionPicker = vtkCellPicker::New();

  // This prevent vtkInteractorStyle::StartState to fire the timer
  // that is used to handle joystick mode

  this->UseTimers = 0;
}

//----------------------------------------------------------------------------
vtkInteractorStyleTrackballActor::~vtkInteractorStyleTrackballActor() 
{
  this->InteractionPicker->Delete();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTrackballActor::OnMouseMove(int vtkNotUsed(ctrl), 
                                                   int vtkNotUsed(shift),
                                                   int x, 
                                                   int y) 
{
  switch (this->State) 
    {
    case VTKIS_ROTATE:
      this->FindPokedCamera(x, y);
      this->Rotate();
      break;

    case VTKIS_PAN:
      this->FindPokedCamera(x, y);
      this->Pan();
      break;

    case VTKIS_DOLLY:
      this->FindPokedCamera(x, y);
      this->Dolly();
      break;

    case VTKIS_SPIN:
      this->FindPokedCamera(x, y);
      this->Spin();
      break;

    case VTKIS_USCALE:
      this->FindPokedCamera(x, y);
      this->UniformScale();
      break;
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTrackballActor::OnLeftButtonDown(int ctrl, 
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
void vtkInteractorStyleTrackballActor::OnLeftButtonUp(int vtkNotUsed(ctrl),
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
void vtkInteractorStyleTrackballActor::OnMiddleButtonDown(int ctrl, 
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
void vtkInteractorStyleTrackballActor::OnMiddleButtonUp(int vtkNotUsed(ctrl),
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
void vtkInteractorStyleTrackballActor::OnRightButtonDown(int vtkNotUsed(ctrl),
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
void vtkInteractorStyleTrackballActor::OnRightButtonUp(int vtkNotUsed(ctrl),
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
void vtkInteractorStyleTrackballActor::Rotate()
{
  if (this->CurrentRenderer == NULL || this->InteractionProp == NULL)
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
  
  // get the furtherest point from object position+origin
  double outsidept[3];
  outsidept[0] = this->ObjCenter[0] + this->ViewRight[0] * boundRadius;
  outsidept[1] = this->ObjCenter[1] + this->ViewRight[1] * boundRadius;
  outsidept[2] = this->ObjCenter[2] + this->ViewRight[2] * boundRadius;
  
  // convert them to display coord
  this->ComputeWorldToDisplay(this->ObjCenter[0], this->ObjCenter[1],
                              this->ObjCenter[2], this->DispObjCenter);

  this->ComputeWorldToDisplay(outsidept[0], outsidept[1],
                              outsidept[2], outsidept);
  
  // get the radius in display coord
  double ftmp[3];
  ftmp[0] = this->DispObjCenter[0];
  ftmp[1] = this->DispObjCenter[1];
  ftmp[2] = this->DispObjCenter[2];
  
  this->Radius = sqrt(vtkMath::Distance2BetweenPoints(ftmp, outsidept));
  this->HighlightProp3D(NULL);
  
  double nxf = ((double)(x) - (double)(this->DispObjCenter[0])) / (double)(this->Radius);
  double nyf = ((double)(y) - (double)(this->DispObjCenter[1])) / (double)(this->Radius);
  double oxf = ((double)(rwi->GetLastEventPosition()[0]) - (double)(this->DispObjCenter[0])) / (double)(this->Radius);
  double oyf = ((double)(rwi->GetLastEventPosition()[1]) - (double)(this->DispObjCenter[1])) / (double)(this->Radius);

  if (((nxf * nxf + nyf * nyf) <= 1.0) &&
      ((oxf * oxf + oyf * oyf) <= 1.0))
    {
    double newXAngle = asin(nxf) * this->RadianToDegree;
    double newYAngle = asin(nyf) * this->RadianToDegree;
    double oldXAngle = asin(oxf) * this->RadianToDegree;
    double oldYAngle = asin(oyf) * this->RadianToDegree;
    
    double scale[3];
    scale[0] = scale[1] = scale[2] = 1.0;
    double **rotate = new double*[2];
    rotate[0] = new double[4];
    rotate[1] = new double[4];
    
    rotate[0][0] = newXAngle - oldXAngle;
    rotate[0][1] = this->ViewUp[0];
    rotate[0][2] = this->ViewUp[1];
    rotate[0][3] = this->ViewUp[2];
    
    rotate[1][0] = oldYAngle - newYAngle;
    rotate[1][1] = this->ViewRight[0];
    rotate[1][2] = this->ViewRight[1];
    rotate[1][3] = this->ViewRight[2];
    
    
    this->Prop3DTransform(this->InteractionProp,
                          this->ObjCenter,
                          2, rotate, scale);
    
    delete [] rotate[0];
    delete [] rotate[1];
    delete [] rotate;
    
    this->ResetCameraClippingRange();
    rwi->Render();
    }
}
  
//----------------------------------------------------------------------------
void vtkInteractorStyleTrackballActor::Spin()
{
  if (this->CurrentRenderer == NULL || this->InteractionProp == NULL)
    {
    return;
    }

  vtkRenderWindowInteractor *rwi = this->Interactor;

  int x = rwi->GetEventPosition()[0];
  int y = rwi->GetEventPosition()[1];
 
  vtkCamera *cam = this->CurrentRenderer->GetActiveCamera();
  
  // get the position plus origin of the object
  float *center = this->InteractionProp->GetCenter();
  this->ObjCenter[0] = center[0];
  this->ObjCenter[1] = center[1];
  this->ObjCenter[2] = center[2];
  
  // get the axis to rotate around = vector from eye to origin
  if (cam->GetParallelProjection())
    {
    cam->ComputeViewPlaneNormal();
    cam->GetViewPlaneNormal(this->MotionVector);
    }
  else
    {   
    cam->GetPosition(this->ViewPoint);
    this->MotionVector[0] = this->ViewPoint[0] - this->ObjCenter[0];
    this->MotionVector[1] = this->ViewPoint[1] - this->ObjCenter[1];
    this->MotionVector[2] = this->ViewPoint[2] - this->ObjCenter[2];
    vtkMath::Normalize(this->MotionVector);
    }
  
  this->ComputeWorldToDisplay(this->ObjCenter[0], this->ObjCenter[1],
                              this->ObjCenter[2], this->DispObjCenter);
  
  this->HighlightProp3D(NULL);
  
  // this has to be in the loop
  double newAngle = atan2((double)(y) - this->DispObjCenter[1],
                          (double)(x) - this->DispObjCenter[0]);
  double oldAngle = atan2((double)(rwi->GetLastEventPosition()[1]) - this->DispObjCenter[1],
                          (double)(rwi->GetLastEventPosition()[0]) - this->DispObjCenter[0]);
  
  newAngle *= this->RadianToDegree;
  oldAngle *= this->RadianToDegree;
  
  double scale[3];
  scale[0] = scale[1] = scale[2] = 1.0;
  double **rotate = new double*[1];
  rotate[0] = new double[4];
  
  rotate[0][0] = newAngle - oldAngle;
  rotate[0][1] = this->MotionVector[0];
  rotate[0][2] = this->MotionVector[1];
  rotate[0][3] = this->MotionVector[2];
  
  this->Prop3DTransform(this->InteractionProp,
                        this->ObjCenter,
                        1, rotate, scale);
  
  delete [] rotate[0];
  delete [] rotate;
  
  this->ResetCameraClippingRange();
  rwi->Render();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTrackballActor::Pan()
{
  if (this->CurrentRenderer == NULL || this->InteractionProp == NULL)
    {
    return;
    }

  vtkRenderWindowInteractor *rwi = this->Interactor;
  
  int x = rwi->GetEventPosition()[0];
  int y = rwi->GetEventPosition()[1];
 
  // use initial center as the origin from which to pan
  float *center = this->InteractionProp->GetCenter();
  this->ObjCenter[0] = center[0];
  this->ObjCenter[1] = center[1];
  this->ObjCenter[2] = center[2];
  this->ComputeWorldToDisplay(this->ObjCenter[0], this->ObjCenter[1],
                              this->ObjCenter[2], this->DispObjCenter);
  this->HighlightProp3D(NULL);
  
  this->ComputeDisplayToWorld(double(x), double(y), 
                              this->DispObjCenter[2],
                              this->NewPickPoint);
  
  this->ComputeDisplayToWorld(double(rwi->GetLastEventPosition()[0]), 
                              double(rwi->GetLastEventPosition()[1]), 
                              this->DispObjCenter[2],
                              this->OldPickPoint);
  
  this->MotionVector[0] = this->NewPickPoint[0] - this->OldPickPoint[0];
  this->MotionVector[1] = this->NewPickPoint[1] - this->OldPickPoint[1];
  this->MotionVector[2] = this->NewPickPoint[2] - this->OldPickPoint[2];
  
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
void vtkInteractorStyleTrackballActor::Dolly()
{
  if (this->CurrentRenderer == NULL || this->InteractionProp == NULL)
    {
    return;
    }
  
  vtkRenderWindowInteractor *rwi = this->Interactor;

  int dy = rwi->GetEventPosition()[1] - rwi->GetLastEventPosition()[1];
 
  vtkCamera *cam = this->CurrentRenderer->GetActiveCamera();

  cam->GetPosition(this->ViewPoint);
  cam->GetFocalPoint(this->ViewFocus);
  
  this->HighlightProp3D(NULL);
  
  double yf = (double)(dy) / (double)(this->Center[1]) *
    this->MotionFactor;
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
void vtkInteractorStyleTrackballActor::UniformScale()
{
  if (this->CurrentRenderer == NULL || this->InteractionProp == NULL)
    {
    return;
    }
  
  vtkRenderWindowInteractor *rwi = this->Interactor;

  int dy = rwi->GetEventPosition()[1] - rwi->GetLastEventPosition()[1];
 
  float *center = this->InteractionProp->GetCenter();
  this->ObjCenter[0] = center[0];
  this->ObjCenter[1] = center[1];
  this->ObjCenter[2] = center[2];
  
  this->HighlightProp3D(NULL);
      
  double yf = (double)(dy) / (double)(this->Center[1]) *
    this->MotionFactor;
  double scaleFactor = pow((double)1.1, yf);
  
  double **rotate = NULL;
  
  double scale[3];
  scale[0] = scale[1] = scale[2] = scaleFactor;
  
  this->Prop3DTransform(this->InteractionProp,
                        this->ObjCenter,
                        0, rotate, scale);
  
  this->ResetCameraClippingRange();
  rwi->Render();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTrackballActor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTrackballActor::FindPickedActor(int x, int y)
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
void vtkInteractorStyleTrackballActor::Prop3DTransform(vtkProp3D *prop3D,
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
void vtkInteractorStyleTrackballActor::Prop3DTransform(vtkProp3D *prop3D,
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
