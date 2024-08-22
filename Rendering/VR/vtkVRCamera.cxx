// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkVRCamera.h"
#include "vtkMatrix4x4.h"
#include "vtkVRInteractorStyle.h"
#include "vtkVRRenderWindow.h"
#include "vtkVRRenderWindowInteractor.h"
#include "vtkVector.h"

VTK_ABI_NAMESPACE_BEGIN
vtkVRCamera::vtkVRCamera() = default;
vtkVRCamera::~vtkVRCamera() = default;

namespace
{

// A simple herlper function to return an unit vector closest to the input
// vector that is orthogonal to the normal vector
vtkVector3d sanitizeVector(vtkVector3d& in, vtkVector3d& normal)
{
  vtkVector3d result;

  // handle coincident vectors
  if (fabs(in.Dot(normal)) > 0.999) // some epsilon
  {
    if (fabs(normal[0]) < 0.1)
    {
      result.Set(1.0, 0.0, 0.0);
    }
    else
    {
      result.Set(0.0, 1.0, 0.0);
    }
  }
  else
  {
    result = in - (in.Dot(normal)) * normal;
    result.Normalize();
  }
  return result;
}
} // end anonymous namespace

// fairly simply we just save the current physical and view properties
void vtkVRCamera::SetPoseFromCamera(vtkVRCamera::Pose* pose, vtkVRRenderWindow* win)
{
  win->GetPhysicalTranslation(pose->Translation);
  win->GetPhysicalViewUp(pose->PhysicalViewUp);
  pose->Distance = win->GetPhysicalScale();
  vtkVRInteractorStyle* is =
    static_cast<vtkVRInteractorStyle*>(win->GetInteractor()->GetInteractorStyle());
  pose->MotionFactor = is->GetDollyPhysicalSpeed();

  this->GetPosition(pose->Position);

  win->GetPhysicalViewDirection(pose->PhysicalViewDirection);
  this->GetDirectionOfProjection(pose->ViewDirection);
}

// much more complicated as we cannot simply set the camera based on
// the pose as the camera is head tracked (the HMD) and whatever we set will
// be instantly overridden with the latest HMD matrix. So instead we adjust
// the physical space properties to best reproduce the pose based on the HMDs
// current pose.
void vtkVRCamera::ApplyPoseToCamera(vtkVRCamera::Pose* pose, vtkVRRenderWindow* win)
{
  // newPhysicalViewUp is always the same as what was saved
  vtkVector3d newPhysicalViewUp(pose->PhysicalViewUp);
  win->SetPhysicalViewUp(newPhysicalViewUp.GetData());

  //==========================================================
  // (1) Get the saved values (some sanitizing)
  vtkVector3d savedTranslation(pose->Translation);
  vtkVector3d savedPosition(pose->Position);
  double savedDistance = pose->Distance;

  // sanitize the savedViewDirection, must be orthogonal to newPhysicalViewUp
  vtkVector3d savedViewDirection(pose->ViewDirection);
  savedViewDirection = sanitizeVector(savedViewDirection, newPhysicalViewUp);

  //==========================================================
  // (2) Get the current values (some sanitizing)
  // c = current values
  vtkVector3d cPosition;
  this->GetPosition(cPosition.GetData());
  vtkVector3d cTranslation;
  win->GetPhysicalTranslation(cTranslation.GetData());
  double cDistance = win->GetPhysicalScale();

  // sanitize cViewDirection and cPhysicalViewDirection, must be orthogonal
  // to newPhysicalViewUp
  vtkVector3d cViewDirection;
  this->GetDirectionOfProjection(cViewDirection.GetData());
  cViewDirection = sanitizeVector(cViewDirection, newPhysicalViewUp);
  vtkVector3d cPhysicalViewDirection;
  win->GetPhysicalViewDirection(cPhysicalViewDirection.GetData());
  cPhysicalViewDirection = sanitizeVector(cPhysicalViewDirection, newPhysicalViewUp);
  vtkVector3d cPhysicalViewRight = cPhysicalViewDirection.Cross(newPhysicalViewUp);

  //==========================================================
  // (3) start doing all the calculations

  // find the newPhysicalViewDirection
  vtkVector3d newPhysicalViewDirection;
  double theta = acos(savedViewDirection.Dot(cViewDirection));
  if (newPhysicalViewUp.Dot(cViewDirection.Cross(savedViewDirection)) < 0.0)
  {
    theta = -theta;
  }
  // rotate cPhysicalViewDirection by theta
  newPhysicalViewDirection = cPhysicalViewDirection * cos(theta) - cPhysicalViewRight * sin(theta);
  win->SetPhysicalViewDirection(newPhysicalViewDirection.GetData());
  vtkVector3d newPhysicalViewRight = newPhysicalViewDirection.Cross(newPhysicalViewUp);

  // adjust translation so that we are in the same spot
  // as when the camera was saved
  vtkVector3d newTranslation;
  vtkVector3d cppwc;
  cppwc = cPosition + cTranslation;
  double x = cppwc.Dot(cPhysicalViewDirection) / cDistance;
  double y = cppwc.Dot(cPhysicalViewRight) / cDistance;

  newTranslation = savedTranslation * newPhysicalViewUp +
    newPhysicalViewDirection * (x * savedDistance - savedPosition.Dot(newPhysicalViewDirection)) +
    newPhysicalViewRight * (y * savedDistance - savedPosition.Dot(newPhysicalViewRight));

  win->SetPhysicalTranslation(newTranslation.GetData());
  this->SetPosition(cPosition.GetData());

  // this really only sets the distance as the render loop
  // sets focal point and position every frame
  vtkVector3d newFocalPoint;
  newFocalPoint = cPosition + newPhysicalViewDirection * savedDistance;
  this->SetFocalPoint(newFocalPoint.GetData());
  win->SetPhysicalScale(savedDistance);

  win->SetPhysicalViewUp(newPhysicalViewUp.GetData());
  vtkInteractorStyle3D* is =
    static_cast<vtkInteractorStyle3D*>(win->GetInteractor()->GetInteractorStyle());
  is->SetDollyPhysicalSpeed(pose->MotionFactor);
}

// extract the camera ivars from the provided device/view matrix
void vtkVRCamera::SetCameraFromWorldToDeviceMatrix(vtkMatrix4x4* mat, double distance)
{
  // the input matrix should be a pose/view matrix without projection
  this->TempMatrix4x4->DeepCopy(mat);
  this->TempMatrix4x4->Invert();
  this->SetCameraFromDeviceToWorldMatrix(this->TempMatrix4x4, distance);
}

void vtkVRCamera::SetCameraFromDeviceToWorldMatrix(vtkMatrix4x4* mat, double distance)
{
  double* ele = mat->GetData();

  // position is the last column of the matrix
  this->SetPosition(ele[3], ele[7], ele[11]);

  // view up is the second column of the matrix
  this->SetViewUp(ele[1], ele[5], ele[9]);

  // direction of projection is the third column of the matrix
  // but we set it by setting the focal point
  this->SetFocalPoint(
    ele[3] - distance * ele[2], ele[7] - distance * ele[6], ele[11] - distance * ele[10]);
}
VTK_ABI_NAMESPACE_END
