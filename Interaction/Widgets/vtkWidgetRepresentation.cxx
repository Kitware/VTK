/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWidgetRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkWidgetRepresentation.h"

#include "vtkAbstractPropPicker.h"
#include "vtkInteractorObserver.h"
#include "vtkMatrix4x4.h"
#include "vtkPickingManager.h"
#include "vtkQuaternion.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTransform.h"

//----------------------------------------------------------------------
vtkWidgetRepresentation::vtkWidgetRepresentation()
{
  this->Renderer = nullptr;

  this->InteractionState = 0;
  this->StartEventPosition[0] = 0.0;
  this->StartEventPosition[1] = 0.0;
  this->StartEventPosition[2] = 0.0;

  this->PlaceFactor = 0.5;
  this->Placed = 0;
  this->ValidPick = 0;
  this->HandleSize = 0.01;

  this->InitialBounds[0] = this->InitialBounds[2] = this->InitialBounds[4] = 0.0;
  this->InitialBounds[1] = this->InitialBounds[3] = this->InitialBounds[5] = 1.0;

  this->InitialLength = 0.0;

  this->NeedToRender = 0;

  this->PickingManaged = true;
}

//----------------------------------------------------------------------
vtkWidgetRepresentation::~vtkWidgetRepresentation()
{
  this->UnRegisterPickers();
}

//----------------------------------------------------------------------
void vtkWidgetRepresentation::SetRenderer(vtkRenderer* ren)
{
  if (ren == this->Renderer)
  {
    return;
  }

  this->UnRegisterPickers();
  this->Renderer = ren;
  // register with potentially new picker
  if (this->Renderer)
  {
    this->RegisterPickers();
  }
  this->Modified();
}

//----------------------------------------------------------------------------
vtkRenderer* vtkWidgetRepresentation::GetRenderer()
{
  return this->Renderer;
}

//----------------------------------------------------------------------------
void vtkWidgetRepresentation::RegisterPickers() {}

//----------------------------------------------------------------------------
void vtkWidgetRepresentation::UnRegisterPickers()
{
  vtkPickingManager* pm = this->GetPickingManager();
  if (!pm)
  {
    return;
  }

  pm->RemoveObject(this);
}

//----------------------------------------------------------------------------
void vtkWidgetRepresentation::SetPickingManaged(bool managed)
{
  if (this->PickingManaged == managed)
  {
    return;
  }
  this->UnRegisterPickers();
  this->PickingManaged = managed;
  if (this->PickingManaged)
  {
    this->RegisterPickers();
  }
}

//----------------------------------------------------------------------------
vtkPickingManager* vtkWidgetRepresentation::GetPickingManager()
{
  if (!this->Renderer || !this->Renderer->GetRenderWindow() ||
    !this->Renderer->GetRenderWindow()->GetInteractor() ||
    !this->Renderer->GetRenderWindow()->GetInteractor()->GetPickingManager())
  {
    return nullptr;
  }

  return this->Renderer->GetRenderWindow()->GetInteractor()->GetPickingManager();
}

//------------------------------------------------------------------------------
vtkAssemblyPath* vtkWidgetRepresentation::GetAssemblyPath(
  double X, double Y, double Z, vtkAbstractPropPicker* picker)
{
  vtkPickingManager* pm = this->GetPickingManager();
  if (!this->PickingManaged || !pm)
  {
    picker->Pick(X, Y, Z, this->Renderer);
    return picker->GetPath();
  }

  return pm->GetAssemblyPath(X, Y, 0., picker, this->Renderer, this);
}

//------------------------------------------------------------------------------
vtkAssemblyPath* vtkWidgetRepresentation::GetAssemblyPath3DPoint(
  double pos[3], vtkAbstractPropPicker* picker)
{
  picker->Pick3DPoint(pos, this->Renderer);
  return picker->GetPath();
}

//----------------------------------------------------------------------
void vtkWidgetRepresentation::AdjustBounds(double bounds[6], double newBounds[6], double center[3])
{
  center[0] = (bounds[0] + bounds[1]) / 2.0;
  center[1] = (bounds[2] + bounds[3]) / 2.0;
  center[2] = (bounds[4] + bounds[5]) / 2.0;

  newBounds[0] = center[0] + this->PlaceFactor * (bounds[0] - center[0]);
  newBounds[1] = center[0] + this->PlaceFactor * (bounds[1] - center[0]);
  newBounds[2] = center[1] + this->PlaceFactor * (bounds[2] - center[1]);
  newBounds[3] = center[1] + this->PlaceFactor * (bounds[3] - center[1]);
  newBounds[4] = center[2] + this->PlaceFactor * (bounds[4] - center[2]);
  newBounds[5] = center[2] + this->PlaceFactor * (bounds[5] - center[2]);
}

//----------------------------------------------------------------------
void vtkWidgetRepresentation::ShallowCopy(vtkProp* prop)
{
  vtkWidgetRepresentation* rep = vtkWidgetRepresentation::SafeDownCast(prop);
  if (rep)
  {
    this->SetPlaceFactor(rep->GetPlaceFactor());
    this->SetHandleSize(rep->GetHandleSize());
  }
  this->Superclass::ShallowCopy(prop);
}

//----------------------------------------------------------------------
int vtkWidgetRepresentation::ComputeInteractionState(int, int, int)
{
  return 0;
}

int vtkWidgetRepresentation::ComputeComplexInteractionState(
  vtkRenderWindowInteractor*, vtkAbstractWidget*, unsigned long, void*, int)
{
  return 0;
}

//----------------------------------------------------------------------
double vtkWidgetRepresentation::SizeHandlesInPixels(double factor, double pos[3])
{
  //
  int i;
  vtkRenderer* renderer;

  if (!this->ValidPick || !(renderer = this->Renderer) || !renderer->GetActiveCamera())
  {
    return (this->HandleSize * factor * this->InitialLength);
  }
  else
  {
    double radius, z;
    double lowerLeft[4], upperRight[4];
    double focalPoint[4];

    vtkInteractorObserver::ComputeWorldToDisplay(
      this->Renderer, pos[0], pos[1], pos[2], focalPoint);
    z = focalPoint[2];

    double x = focalPoint[0] - this->HandleSize / 2.0;
    double y = focalPoint[1] - this->HandleSize / 2.0;
    vtkInteractorObserver::ComputeDisplayToWorld(this->Renderer, x, y, z, lowerLeft);

    x = focalPoint[0] + this->HandleSize / 2.0;
    y = focalPoint[1] + this->HandleSize / 2.0;
    vtkInteractorObserver::ComputeDisplayToWorld(this->Renderer, x, y, z, upperRight);

    for (radius = 0.0, i = 0; i < 3; i++)
    {
      radius += (upperRight[i] - lowerLeft[i]) * (upperRight[i] - lowerLeft[i]);
    }
    return (factor * (sqrt(radius) / 2.0));
  }
}

//----------------------------------------------------------------------
double vtkWidgetRepresentation::SizeHandlesRelativeToViewport(double factor, double pos[3])
{
  int i;
  vtkRenderer* renderer;

  if (!this->ValidPick || !(renderer = this->Renderer) || !renderer->GetActiveCamera())
  {
    return (this->HandleSize * factor * this->InitialLength);
  }
  else
  {
    double radius, z;
    double windowLowerLeft[4], windowUpperRight[4];
    double* viewport = renderer->GetViewport();
    const int* winSize = renderer->GetRenderWindow()->GetSize();
    double focalPoint[4];

    vtkInteractorObserver::ComputeWorldToDisplay(
      this->Renderer, pos[0], pos[1], pos[2], focalPoint);
    z = focalPoint[2];

    double x = winSize[0] * viewport[0];
    double y = winSize[1] * viewport[1];
    vtkInteractorObserver::ComputeDisplayToWorld(this->Renderer, x, y, z, windowLowerLeft);

    x = winSize[0] * viewport[2];
    y = winSize[1] * viewport[3];
    vtkInteractorObserver::ComputeDisplayToWorld(this->Renderer, x, y, z, windowUpperRight);

    for (radius = 0.0, i = 0; i < 3; i++)
    {
      radius +=
        (windowUpperRight[i] - windowLowerLeft[i]) * (windowUpperRight[i] - windowLowerLeft[i]);
    }

    return (sqrt(radius) * factor * this->HandleSize);
  }
}

void vtkWidgetRepresentation::UpdatePropPose(vtkProp3D* prop3D, const double* pos1,
  const double* orient1, const double* pos2, const double* orient2)
{
  double trans[3];
  for (int i = 0; i < 3; i++)
  {
    trans[i] = pos2[i] - pos1[i];
  }

  vtkTransform* newTransform = this->TempTransform;
  if (prop3D->GetUserMatrix() != nullptr)
  {
    vtkTransform* t = newTransform;
    t->Identity();
    t->PostMultiply();
    t->Concatenate(prop3D->GetUserMatrix());
    t->Translate(trans);
    prop3D->GetUserMatrix()->DeepCopy(t->GetMatrix());
  }
  else
  {
    prop3D->AddPosition(trans);
  }

  // compute the net rotation
  vtkQuaternion<double> q1;
  q1.SetRotationAngleAndAxis(
    vtkMath::RadiansFromDegrees(orient1[0]), orient1[1], orient1[2], orient1[3]);
  vtkQuaternion<double> q2;
  q2.SetRotationAngleAndAxis(
    vtkMath::RadiansFromDegrees(orient2[0]), orient2[1], orient2[2], orient2[3]);
  q1.Conjugate();
  q2 = q2 * q1;
  double axis[4];
  axis[0] = vtkMath::DegreesFromRadians(q2.GetRotationAngleAndAxis(axis + 1));

  vtkMatrix4x4* oldMatrix = this->TempMatrix;
  prop3D->GetMatrix(oldMatrix);

  double orig[3];
  prop3D->GetOrigin(orig);

  newTransform->Identity();
  newTransform->PostMultiply();
  if (prop3D->GetUserMatrix() != nullptr)
  {
    newTransform->Concatenate(prop3D->GetUserMatrix());
  }
  else
  {
    newTransform->Concatenate(oldMatrix);
  }

  newTransform->Translate(-(pos1[0]), -(pos1[1]), -(pos1[2]));

  newTransform->RotateWXYZ(axis[0], axis[1], axis[2], axis[3]);

  newTransform->Translate(pos1[0], pos1[1], pos1[2]);

  // now try to get the composite of translate, rotate, and scale
  newTransform->Translate(-(orig[0]), -(orig[1]), -(orig[2]));
  newTransform->PreMultiply();
  newTransform->Translate(orig[0], orig[1], orig[2]);

  if (prop3D->GetUserMatrix() != nullptr)
  {
    prop3D->GetUserMatrix()->DeepCopy(newTransform->GetMatrix());
  }
  else
  {
    prop3D->SetPosition(newTransform->GetPosition());
    prop3D->SetOrientation(newTransform->GetOrientation());
  }
}

//----------------------------------------------------------------------
bool vtkWidgetRepresentation::NearbyEvent(int X, int Y, double bounds[6])
{
  double focus[3], z, pickPoint[4], dFocus[4], length, dist;

  focus[0] = (bounds[0] + bounds[1]) / 2.0;
  focus[1] = (bounds[2] + bounds[3]) / 2.0;
  focus[2] = (bounds[4] + bounds[5]) / 2.0;

  vtkInteractorObserver::ComputeWorldToDisplay(
    this->Renderer, focus[0], focus[1], focus[2], dFocus);
  z = dFocus[2];
  vtkInteractorObserver::ComputeDisplayToWorld(this->Renderer, X, Y, z, pickPoint);
  length = sqrt((bounds[1] - bounds[0]) * (bounds[1] - bounds[0]) +
    (bounds[3] - bounds[2]) * (bounds[3] - bounds[2]) +
    (bounds[5] - bounds[4]) * (bounds[5] - bounds[4]));
  dist = sqrt((pickPoint[0] - focus[0]) * (pickPoint[0] - focus[0]) +
    (pickPoint[1] - focus[1]) * (pickPoint[1] - focus[1]) +
    (pickPoint[2] - focus[2]) * (pickPoint[2] - focus[2]));

  return ((dist > 0.75 * length) ? false : true);
}

//----------------------------------------------------------------------
void vtkWidgetRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  // Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Renderer: " << this->Renderer << "\n";
  os << indent << "Interaction State: " << this->InteractionState << "\n";
  os << indent << "Handle Size: " << this->HandleSize << "\n";
  os << indent << "Need to Render: " << (this->NeedToRender ? "On\n" : "Off\n");
  os << indent << "Place Factor: " << this->PlaceFactor << "\n";
}
