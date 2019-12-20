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

#include "vtkAssemblyPath.h"
#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkEventData.h"
#include "vtkMapper.h"
#include "vtkMath.h"
#include "vtkMatrix3x3.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPlane.h"
#include "vtkProp3D.h"
#include "vtkPropPicker.h"
#include "vtkQuaternion.h"
#include "vtkRenderWindowInteractor3D.h"
#include "vtkRenderer.h"
#include "vtkTimerLog.h"
#include "vtkTransform.h"

vtkStandardNewMacro(vtkInteractorStyle3D);

//----------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkInteractorStyle3D, InteractionPicker, vtkAbstractPropPicker);

//----------------------------------------------------------------------------
vtkInteractorStyle3D::vtkInteractorStyle3D()
{
  this->InteractionProp = nullptr;
  this->InteractionPicker = vtkPropPicker::New();
  this->TempMatrix3 = vtkMatrix3x3::New();
  this->TempMatrix4 = vtkMatrix4x4::New();
  this->AppliedTranslation[0] = 0;
  this->AppliedTranslation[1] = 0;
  this->AppliedTranslation[2] = 0;
  this->TempTransform = vtkTransform::New();
  this->DollyPhysicalSpeed = 1.6666;
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
// We handle all adjustments here
void vtkInteractorStyle3D::PositionProp(vtkEventData* ed)
{
  if (this->CurrentRenderer == nullptr || this->InteractionProp == nullptr)
  {
    return;
  }

  vtkRenderWindowInteractor3D* rwi = static_cast<vtkRenderWindowInteractor3D*>(this->Interactor);

  if (ed->GetType() != vtkCommand::Move3DEvent)
  {
    return;
  }
  vtkEventDataDevice3D* edd = static_cast<vtkEventDataDevice3D*>(ed);
  double wpos[3];
  edd->GetWorldPosition(wpos);

  double* lwpos = rwi->GetLastWorldEventPosition(rwi->GetPointerIndex());

  double trans[3];
  for (int i = 0; i < 3; i++)
  {
    trans[i] = wpos[i] - lwpos[i];
  }

  if (this->InteractionProp->GetUserTransform() != nullptr)
  {
    vtkTransform* t = this->TempTransform;
    t->PostMultiply();
    t->Identity();
    t->Concatenate(this->InteractionProp->GetUserMatrix());
    t->Translate(trans);
    vtkNew<vtkMatrix4x4> n;
    n->DeepCopy(t->GetMatrix());
    this->InteractionProp->SetUserMatrix(n);
  }
  else
  {
    this->InteractionProp->AddPosition(trans);
  }

  double* wori = rwi->GetWorldEventOrientation(rwi->GetPointerIndex());

  double* lwori = rwi->GetLastWorldEventOrientation(rwi->GetPointerIndex());

  // compute the net rotation
  vtkQuaternion<double> q1;
  q1.SetRotationAngleAndAxis(vtkMath::RadiansFromDegrees(lwori[0]), lwori[1], lwori[2], lwori[3]);
  vtkQuaternion<double> q2;
  q2.SetRotationAngleAndAxis(vtkMath::RadiansFromDegrees(wori[0]), wori[1], wori[2], wori[3]);
  q1.Conjugate();
  q2 = q2 * q1;
  double axis[4];
  axis[0] = vtkMath::DegreesFromRadians(q2.GetRotationAngleAndAxis(axis + 1));

  double scale[3];
  scale[0] = scale[1] = scale[2] = 1.0;

  double* rotate = axis;
  this->Prop3DTransform(this->InteractionProp, wpos, 1, &rotate, scale);

  if (this->AutoAdjustCameraClippingRange)
  {
    this->CurrentRenderer->ResetCameraClippingRange();
  }
}

//----------------------------------------------------------------------------
void vtkInteractorStyle3D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkInteractorStyle3D::FindPickedActor(double pos[3], double orient[4])
{
  if (!orient)
  {
    this->InteractionPicker->Pick3DPoint(pos, this->CurrentRenderer);
  }
  else
  {
    this->InteractionPicker->Pick3DRay(pos, orient, this->CurrentRenderer);
  }
  vtkProp* prop = this->InteractionPicker->GetViewProp();
  if (prop != nullptr)
  {
    this->InteractionProp = vtkProp3D::SafeDownCast(prop);
  }
  else
  {
    this->InteractionProp = nullptr;
  }
}

//----------------------------------------------------------------------------
void vtkInteractorStyle3D::Prop3DTransform(
  vtkProp3D* prop3D, double* boxCenter, int numRotation, double** rotate, double* scale)
{
  vtkMatrix4x4* oldMatrix = this->TempMatrix4;
  prop3D->GetMatrix(oldMatrix);

  double orig[3];
  prop3D->GetOrigin(orig);

  vtkTransform* newTransform = this->TempTransform;
  newTransform->PostMultiply();
  newTransform->Identity();
  if (prop3D->GetUserMatrix() != nullptr)
  {
    newTransform->Concatenate(prop3D->GetUserMatrix());
  }
  else
  {
    newTransform->Concatenate(oldMatrix);
  }

  newTransform->Translate(-(boxCenter[0]), -(boxCenter[1]), -(boxCenter[2]));

  for (int i = 0; i < numRotation; i++)
  {
    newTransform->RotateWXYZ(rotate[i][0], rotate[i][1], rotate[i][2], rotate[i][3]);
  }

  if ((scale[0] * scale[1] * scale[2]) != 0.0)
  {
    newTransform->Scale(scale[0], scale[1], scale[2]);
  }

  newTransform->Translate(boxCenter[0], boxCenter[1], boxCenter[2]);

  // now try to get the composite of translate, rotate, and scale
  newTransform->Translate(-(orig[0]), -(orig[1]), -(orig[2]));
  newTransform->PreMultiply();
  newTransform->Translate(orig[0], orig[1], orig[2]);

  if (prop3D->GetUserMatrix() != nullptr)
  {
    vtkNew<vtkMatrix4x4> n;
    n->DeepCopy(newTransform->GetMatrix());
    prop3D->SetUserMatrix(n);
  }
  else
  {
    prop3D->SetPosition(newTransform->GetPosition());
    prop3D->SetScale(newTransform->GetScale());
    prop3D->SetOrientation(newTransform->GetOrientation());
  }
}

void vtkInteractorStyle3D::Dolly3D(vtkEventData* ed)
{
  if (this->CurrentRenderer == nullptr)
  {
    return;
  }

  vtkRenderWindowInteractor3D* rwi = static_cast<vtkRenderWindowInteractor3D*>(this->Interactor);

  if (ed->GetType() != vtkCommand::Move3DEvent)
  {
    return;
  }
  vtkEventDataDevice3D* edd = static_cast<vtkEventDataDevice3D*>(ed);
  const double* wori = edd->GetWorldOrientation();

  // move HMD world in the direction of the controller
  vtkQuaternion<double> q1;
  q1.SetRotationAngleAndAxis(vtkMath::RadiansFromDegrees(wori[0]), wori[1], wori[2], wori[3]);

  double elem[3][3];
  q1.ToMatrix3x3(elem);
  double vdir[3] = { 0.0, 0.0, -1.0 };
  vtkMatrix3x3::MultiplyPoint(elem[0], vdir, vdir);

  double* trans = rwi->GetPhysicalTranslation(this->CurrentRenderer->GetActiveCamera());

  // scale speed by thumb position on the touchpad along Y axis
  float tpos[3];
  rwi->GetTouchPadPosition(edd->GetDevice(), vtkEventDataDeviceInput::Unknown, tpos);
  if (fabs(tpos[0]) > fabs(tpos[1]))
  {
    // do not dolly if pressed direction is not up or down but left or right
    return;
  }
  double speedScaleFactor = tpos[1]; // -1 to +1 (the Y axis of the trackpad)
  double physicalScale = rwi->GetPhysicalScale();

  this->LastDolly3DEventTime->StopTimer();
  double distanceTravelled_World = speedScaleFactor * this->DollyPhysicalSpeed /* m/sec */ *
    physicalScale * /* world/physical */
    this->LastDolly3DEventTime->GetElapsedTime() /* sec */;

  this->LastDolly3DEventTime->StartTimer();

  rwi->SetPhysicalTranslation(this->CurrentRenderer->GetActiveCamera(),
    trans[0] - vdir[0] * distanceTravelled_World, trans[1] - vdir[1] * distanceTravelled_World,
    trans[2] - vdir[2] * distanceTravelled_World);

  if (this->AutoAdjustCameraClippingRange)
  {
    this->CurrentRenderer->ResetCameraClippingRange();
  }
}

void vtkInteractorStyle3D::SetScale(vtkCamera* camera, double newScale)
{
  vtkRenderWindowInteractor3D* rwi = static_cast<vtkRenderWindowInteractor3D*>(this->Interactor);

  double* trans = rwi->GetPhysicalTranslation(camera);
  double physicalScale = rwi->GetPhysicalScale();
  double* dop = camera->GetDirectionOfProjection();
  double* pos = camera->GetPosition();
  double hmd[3];
  hmd[0] = (pos[0] + trans[0]) / physicalScale;
  hmd[1] = (pos[1] + trans[1]) / physicalScale;
  hmd[2] = (pos[2] + trans[2]) / physicalScale;

  double newPos[3];
  newPos[0] = hmd[0] * newScale - trans[0];
  newPos[1] = hmd[1] * newScale - trans[1];
  newPos[2] = hmd[2] * newScale - trans[2];

  // Note: New camera properties are overridden by virtual reality render
  // window if head-mounted display is tracked
  camera->SetFocalPoint(
    newPos[0] + dop[0] * newScale, newPos[1] + dop[1] * newScale, newPos[2] + dop[2] * newScale);
  camera->SetPosition(newPos[0], newPos[1], newPos[2]);

  rwi->SetPhysicalScale(newScale);

  if (this->AutoAdjustCameraClippingRange && this->CurrentRenderer)
  {
    this->CurrentRenderer->ResetCameraClippingRange();
  }
}
