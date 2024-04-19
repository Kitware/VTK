// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkInteractorStyle3D);

//------------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkInteractorStyle3D, InteractionPicker, vtkAbstractPropPicker);

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
vtkInteractorStyle3D::~vtkInteractorStyle3D()
{
  this->InteractionPicker->Delete();
  this->TempMatrix3->Delete();
  this->TempMatrix4->Delete();
  this->TempTransform->Delete();
}

//------------------------------------------------------------------------------
// We handle all adjustments here
void vtkInteractorStyle3D::PositionProp(vtkEventData* ed, double* lwpos, double* lwori)
{
  if (this->CurrentRenderer == nullptr || this->InteractionProp == nullptr)
  {
    return;
  }

  if (ed->GetType() != vtkCommand::Move3DEvent)
  {
    return;
  }

  vtkEventDataDevice3D* edd = static_cast<vtkEventDataDevice3D*>(ed);
  double wpos[3];
  edd->GetWorldPosition(wpos);
  double wori[4];
  edd->GetWorldOrientation(wori);

  // If no user defined last world event and last world orientation,
  // use the ones stored by vtkRenderWindowInteractor3D
  if (lwpos == nullptr || lwori == nullptr)
  {
    vtkRenderWindowInteractor3D* rwi = static_cast<vtkRenderWindowInteractor3D*>(this->Interactor);
    if (rwi == nullptr)
    {
      vtkErrorMacro("vtkRenderWindowInteractor3D is necessary without setting lwpos and lwori.");
      return;
    }
    lwpos = rwi->GetLastWorldEventPosition(rwi->GetPointerIndex());
    lwori = rwi->GetLastWorldEventOrientation(rwi->GetPointerIndex());
  }

  // the code below computes newModelToWorld and then sets the prop3D from
  // it

  // we need another temp matrix for these calculations
  vtkNew<vtkMatrix4x4> tmpMatrix;

  // the basic gist is
  // newModelToWorld = oldModelToWorld -> worldToLastPose -> newPoseToWorld

  // first use it to store newModelToWorld
  vtkMatrix4x4* oldModelToLastPose = this->TempMatrix4;

  // create a scope here so that some usages of TempMatrix4 and tmpMatrix
  // go out of scope and will not be accidentally reused.
  {
    vtkMatrix4x4* oldModelToWorld = this->TempMatrix4;
    this->InteractionProp->GetModelToWorldMatrix(oldModelToWorld);

    vtkMatrix4x4* worldToLastPose = tmpMatrix;
    vtkMatrix4x4::PoseToMatrix(lwpos, lwori, worldToLastPose);
    worldToLastPose->Invert();

    vtkMatrix4x4::Multiply4x4(worldToLastPose, oldModelToWorld, oldModelToLastPose);
  }
  // oldModelToWorld and worldToLastPose are gone now

  vtkMatrix4x4* newPoseToWorld = tmpMatrix;
  vtkMatrix4x4::PoseToMatrix(wpos, wori, newPoseToWorld);

  vtkMatrix4x4* newModelToWorld = this->TempMatrix4;
  vtkMatrix4x4::Multiply4x4(newPoseToWorld, oldModelToLastPose, newModelToWorld);

  this->InteractionProp->SetPropertiesFromModelToWorldMatrix(newModelToWorld);

  if (this->AutoAdjustCameraClippingRange)
  {
    this->CurrentRenderer->ResetCameraClippingRange();
  }
}

//------------------------------------------------------------------------------
void vtkInteractorStyle3D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
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
  // update touchpad/joystick if we have the data
  if (edd->GetType() == vtkCommand::ViewerMovement3DEvent)
  {
    edd->GetTrackPadPosition(this->LastTrackPadPosition);
  }
  double speedScaleFactor = this->LastTrackPadPosition[1]; // -1 to +1 (the Y axis of the trackpad)
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
VTK_ABI_NAMESPACE_END
