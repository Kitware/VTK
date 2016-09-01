/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyle3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInteractorStyle3D.h"

#include "vtkCallbackCommand.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPropPicker3D.h"
#include "vtkRenderWindowInteractor3D.h"
#include "vtkProp3D.h"
#include "vtkQuaternion.h"
#include "vtkRenderer.h"
#include "vtkMatrix3x3.h"
#include "vtkTransform.h"
#include "vtkCamera.h"

vtkStandardNewMacro(vtkInteractorStyle3D);

//----------------------------------------------------------------------------
vtkInteractorStyle3D::vtkInteractorStyle3D()
{
  this->InteractionProp = NULL;
  this->InteractionPicker = vtkPropPicker3D::New();
  this->TempMatrix3 = vtkMatrix3x3::New();
  this->TempMatrix4 = vtkMatrix4x4::New();
  this->AppliedTranslation[0] = 0;
  this->AppliedTranslation[1] = 0;
  this->AppliedTranslation[2] = 0;
  this->TempTransform = vtkTransform::New();
}

//----------------------------------------------------------------------------
vtkInteractorStyle3D::~vtkInteractorStyle3D()
{
  this->InteractionPicker->Delete();
  this->TempMatrix3->Delete();
  this->TempMatrix4->Delete();
  this->TempTransform->Delete();
}

//----------------------------------------------------------------------------
void vtkInteractorStyle3D::OnMouseMove()
{
  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];

  switch (this->State)
    {
    case VTKIS_ROTATE:
      this->FindPokedRenderer(x, y);
      this->Rotate();
      this->InvokeEvent(vtkCommand::InteractionEvent, NULL);
      break;
    case VTKIS_DOLLY:
      this->FindPokedRenderer(x, y);
      this->Dolly();
      this->InvokeEvent(vtkCommand::InteractionEvent, NULL);
      break;
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyle3D::OnLeftButtonDown()
{
  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];

  vtkRenderWindowInteractor3D *vriren =
    vtkRenderWindowInteractor3D::SafeDownCast(this->Interactor);

  double *wpos = vriren->GetWorldEventPosition(
    vriren->GetPointerIndex());

  this->FindPokedRenderer(x, y);
  this->FindPickedActor(wpos[0], wpos[1], wpos[2]);
  if (this->CurrentRenderer == NULL || this->InteractionProp == NULL)
    {
    return;
    }

  this->GrabFocus(this->EventCallbackCommand);
  this->StartRotate();
}

//----------------------------------------------------------------------------
void vtkInteractorStyle3D::OnLeftButtonUp()
{
  this->AppliedTranslation[0] = 0;
  this->AppliedTranslation[1] = 0;
  this->AppliedTranslation[2] = 0;

  switch (this->State)
    {
    // in our case roate state is used for actor pose adjustments
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
// We handle all adjustments here
void vtkInteractorStyle3D::Rotate()
{
  if (this->CurrentRenderer == NULL || this->InteractionProp == NULL)
    {
    return;
    }

  vtkRenderWindowInteractor3D *rwi =
    static_cast<vtkRenderWindowInteractor3D *>(this->Interactor);

  double *wpos = rwi->GetWorldEventPosition(
    rwi->GetPointerIndex());

  double *lwpos = rwi->GetLastWorldEventPosition(
    rwi->GetPointerIndex());

  double trans[3];
  for (int i = 0; i < 3; i++)
    {
    trans[i] = wpos[i] - lwpos[i];
    }

  if (this->InteractionProp->GetUserMatrix() != NULL)
    {
    vtkTransform *t = this->TempTransform;
    t->PostMultiply();
    t->SetMatrix(this->InteractionProp->GetUserMatrix());
    t->Translate(trans);
    this->InteractionProp->SetUserMatrix(t->GetMatrix());
    }
  else
    {
    this->InteractionProp->AddPosition(trans);
    }

  double *wori = rwi->GetWorldEventOrientation(
    rwi->GetPointerIndex());

  double *lwori = rwi->GetLastWorldEventOrientation(
    rwi->GetPointerIndex());

  // compute the net rotation
  vtkQuaternion<double> q1;
  q1.SetRotationAngleAndAxis(
    vtkMath::RadiansFromDegrees(lwori[0]), lwori[1], lwori[2], lwori[3]);
  vtkQuaternion<double> q2;
  q2.SetRotationAngleAndAxis(
    vtkMath::RadiansFromDegrees(wori[0]), wori[1], wori[2], wori[3]);
  q1.Conjugate();
  q2 = q2*q1;
  double axis[4];
  axis[0] = vtkMath::DegreesFromRadians(q2.GetRotationAngleAndAxis(axis+1));

  double scale[3];
  scale[0] = scale[1] = scale[2] = 1.0;

  double *rotate = axis;
  this->Prop3DTransform(this->InteractionProp,
                        wpos,
                        1,
                        &rotate,
                        scale);

  if (this->AutoAdjustCameraClippingRange)
    {
    this->CurrentRenderer->ResetCameraClippingRange();
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyle3D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkInteractorStyle3D::FindPickedActor(double x, double y, double z)
{
  this->InteractionPicker->Pick(x, y, z, this->CurrentRenderer);
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
void vtkInteractorStyle3D::Prop3DTransform(vtkProp3D *prop3D,
                                                       double *boxCenter,
                                                       int numRotation,
                                                       double **rotate,
                                                       double *scale)
{
  vtkMatrix4x4 *oldMatrix = this->TempMatrix4;
  prop3D->GetMatrix(oldMatrix);

  double orig[3];
  prop3D->GetOrigin(orig);

  vtkTransform *newTransform = this->TempTransform;
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
    prop3D->SetUserMatrix(newTransform->GetMatrix());
    }
  else
    {
    prop3D->SetPosition(newTransform->GetPosition());
    prop3D->SetScale(newTransform->GetScale());
    prop3D->SetOrientation(newTransform->GetOrientation());
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyle3D::OnRightButtonDown()
{
  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];

  this->FindPokedRenderer(x, y);
  if (this->CurrentRenderer == NULL)
    {
    return;
    }

  this->GrabFocus(this->EventCallbackCommand);
  this->StartDolly();
}

//----------------------------------------------------------------------------
void vtkInteractorStyle3D::OnRightButtonUp()
{
  switch (this->State)
    {
    // in our case roate state is used for all adjustments
    case VTKIS_DOLLY:
      this->EndDolly();
      break;
    }

  if ( this->Interactor )
    {
    this->ReleaseFocus();
    }
}

void vtkInteractorStyle3D::Dolly()
{
  if (this->CurrentRenderer == NULL)
    {
    return;
    }

  vtkRenderWindowInteractor3D *rwi =
    static_cast<vtkRenderWindowInteractor3D *>(this->Interactor);

  double *wori = rwi->GetWorldEventOrientation(
    rwi->GetPointerIndex());

  // move HMD world in the direction of the controller
  vtkQuaternion<double> q1;
  q1.SetRotationAngleAndAxis(
    vtkMath::RadiansFromDegrees(wori[0]), wori[1], wori[2], wori[3]);

  double elem[3][3];
  q1.ToMatrix3x3(elem);
  double vdir[3] = {0.0,0.0,-1.0};
  vtkMatrix3x3::MultiplyPoint(
    elem[0],vdir,vdir);

  double *trans = rwi->GetPhysicalTranslation(
    this->CurrentRenderer->GetActiveCamera());
  double scale = rwi->GetPhysicalScale(
    this->CurrentRenderer->GetActiveCamera());

  // move at a max rate of 0.1 meters per frame in HMD space
  // this works out to about 10 meters per second
  // which is quick. The world coordinate speed of
  // movement can be determined from the camera scale.
  // movement speed is scaled by the touchpad
  // y coordinate

  float *tpos = rwi->GetTouchPadPosition();
  rwi->SetPhysicalTranslation(
    this->CurrentRenderer->GetActiveCamera(),
    trans[0]-vdir[0]*0.05*tpos[1]/scale,
    trans[1]-vdir[1]*0.05*tpos[1]/scale,
    trans[2]-vdir[2]*0.05*tpos[1]/scale);

  if (this->AutoAdjustCameraClippingRange)
    {
    this->CurrentRenderer->ResetCameraClippingRange();
    }
}


//----------------------------------------------------------------------------
void vtkInteractorStyle3D::OnPinch()
{
  int pointer = this->Interactor->GetPointerIndex();

  this->FindPokedRenderer(this->Interactor->GetEventPositions(pointer)[0],
                          this->Interactor->GetEventPositions(pointer)[1]);

  if ( this->CurrentRenderer == NULL )
    {
    return;
    }

  vtkCamera *camera = this->CurrentRenderer->GetActiveCamera();
  vtkRenderWindowInteractor3D *rwi =
    static_cast<vtkRenderWindowInteractor3D *>(this->Interactor);

  double dyf = this->Interactor->GetScale()/this->Interactor->GetLastScale();

  double scale = rwi->GetPhysicalScale(camera);
  rwi->SetPhysicalScale(camera, scale*dyf);
}


//----------------------------------------------------------------------------
void vtkInteractorStyle3D::OnPan()
{
  int pointer = this->Interactor->GetPointerIndex();

  this->FindPokedRenderer(this->Interactor->GetEventPositions(pointer)[0],
                          this->Interactor->GetEventPositions(pointer)[1]);

  if ( this->CurrentRenderer == NULL )
    {
    return;
    }

  vtkCamera *camera = this->CurrentRenderer->GetActiveCamera();

  vtkRenderWindowInteractor3D *rwi =
    static_cast<vtkRenderWindowInteractor3D *>(this->Interactor);

  double *trans = rwi->GetTranslation3D();

  double *ptrans = rwi->GetPhysicalTranslation(camera);
  double scale = rwi->GetPhysicalScale(camera);
  rwi->SetPhysicalTranslation(camera,
    ptrans[0] - this->AppliedTranslation[0] + trans[0]/scale,
    ptrans[1] - this->AppliedTranslation[1] + trans[1]/scale,
    ptrans[2] - this->AppliedTranslation[2] + trans[2]/scale);
  this->AppliedTranslation[0] = trans[0]/scale;
  this->AppliedTranslation[1] = trans[1]/scale;
  this->AppliedTranslation[2] = trans[2]/scale;

  // clean up
  if (this->Interactor->GetLightFollowCamera())
    {
    this->CurrentRenderer->UpdateLightsGeometryToFollowCamera();
    }
}
