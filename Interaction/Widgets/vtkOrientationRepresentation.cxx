/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOrientationRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOrientationRepresentation.h"

#include "vtkActor.h"
#include "vtkAssemblyPath.h"
#include "vtkBox.h"
#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkCellPicker.h"
#include "vtkEventData.h"
#include "vtkInteractorObserver.h"
#include "vtkLineSource.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPickingManager.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSuperquadricSource.h"
#include "vtkTransform.h"
#include "vtkTransformFilter.h"
#include "vtkVectorOperators.h"
#include "vtkWindow.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkOrientationRepresentation);

namespace
{
constexpr double X_VECTOR[3] = { 1.0, 0.0, 0.0 };
constexpr double Y_VECTOR[3] = { 0.0, 1.0, 0.0 };
constexpr double Z_VECTOR[3] = { 0.0, 0.0, 1.0 };

constexpr double MINIMUM_THICKNESS = 0.001;
constexpr double MAXIMUM_THICKNESS = 0.1;
constexpr double MINIMUM_LENGTH = 0.01;
constexpr double MAXIMUM_LENGTH = 100.0;
}

//------------------------------------------------------------------------------
vtkOrientationRepresentation::vtkOrientationRepresentation()
{
  this->InteractionState = vtkOrientationRepresentation::Outside;
  this->PlaceFactor = 1.0;
  this->ValidPick = 1;

  this->BaseTransform = vtkSmartPointer<vtkTransform>::New();
  this->OrientationTransform = vtkSmartPointer<vtkTransform>::New();
  this->BaseTransform->PostMultiply();
  this->OrientationTransform->PostMultiply();

  // Set up the initial properties
  this->CreateDefaultProperties();

  // Create the torus
  for (int i = Axis::X_AXIS; i <= Axis::Z_AXIS; ++i)
  {
    Axis axis = static_cast<Axis>(i);

    this->TorusSources[axis]->SetToroidal(1);
    this->TorusSources[axis]->SetAxisOfSymmetry(i);
    this->TorusSources[axis]->SetThetaResolution(64);
    this->TorusSources[axis]->SetPhiRoundness(0.0);
    this->TorusSources[axis]->SetThickness(this->Thickness);
    this->TorusSources[axis]->SetScale(1.0, 1.0, this->Length);

    vtkNew<vtkTransformFilter> orientationTransform;
    orientationTransform->SetTransform(this->OrientationTransform);
    orientationTransform->SetInputConnection(this->TorusSources[axis]->GetOutputPort());
    vtkNew<vtkTransformFilter> torusTransform;
    torusTransform->SetTransform(this->BaseTransform);
    torusTransform->SetInputConnection(orientationTransform->GetOutputPort());

    vtkNew<vtkPolyDataMapper> torusMapper;
    torusMapper->SetInputConnection(torusTransform->GetOutputPort());

    this->TorusActors[axis]->SetMapper(torusMapper);
    this->TorusActors[axis]->SetProperty(this->TorusProperties[axis]);
    this->HandlePicker->AddPickList(this->TorusActors[axis]);
  }
  this->HandlePicker->SetTolerance(0.001);
  this->HandlePicker->PickFromListOn();
}

//------------------------------------------------------------------------------
vtkOrientationRepresentation::~vtkOrientationRepresentation() = default;

//------------------------------------------------------------------------------
void vtkOrientationRepresentation::StartWidgetInteraction(double e[2])
{
  this->StartEventPosition[0] = e[0];
  this->StartEventPosition[1] = e[1];
  this->StartEventPosition[2] = 0.0;

  this->LastEventPosition[0] = e[0];
  this->LastEventPosition[1] = e[1];
  this->LastEventPosition[2] = 0.0;
}

//------------------------------------------------------------------------------
void vtkOrientationRepresentation::WidgetInteraction(double e[2])
{
  // Convert events to appropriate coordinate systems
  vtkCamera* camera = this->Renderer->GetActiveCamera();
  if (!camera)
  {
    return;
  }
  double vpn[3];
  camera->GetViewPlaneNormal(vpn);

  // Compute the two points defining the motion vector
  double pos[3];
  this->HandlePicker->GetPickPosition(pos);

  double focalPoint[4], pickPoint[4], prevPickPoint[4];
  vtkInteractorObserver::ComputeWorldToDisplay(this->Renderer, pos[0], pos[1], pos[2], focalPoint);
  double z = focalPoint[2];
  vtkInteractorObserver::ComputeDisplayToWorld(
    this->Renderer, this->LastEventPosition[0], this->LastEventPosition[1], z, prevPickPoint);
  vtkInteractorObserver::ComputeDisplayToWorld(this->Renderer, e[0], e[1], z, pickPoint);

  // Process the motion
  switch (this->InteractionState)
  {
    case vtkOrientationRepresentation::RotatingX:
      this->Rotate(prevPickPoint, pickPoint, X_VECTOR);
      break;
    case vtkOrientationRepresentation::RotatingY:
      this->Rotate(prevPickPoint, pickPoint, Y_VECTOR);
      break;
    case vtkOrientationRepresentation::RotatingZ:
      this->Rotate(prevPickPoint, pickPoint, Z_VECTOR);
      break;
    case vtkOrientationRepresentation::Outside:
    default:
      break;
  }

  this->LastEventPosition[0] = e[0];
  this->LastEventPosition[1] = e[1];
  this->LastEventPosition[2] = 0.0;
}

//------------------------------------------------------------------------------
void vtkOrientationRepresentation::Rotate(
  const double p1[4], const double p2[4], const double baseVector[3])
{
  double* position = this->BaseTransform->GetPosition();
  const double centeredP1[3] = { p1[0] - position[0], p1[1] - position[1], p1[2] - position[2] };
  const double centeredP2[3] = { p2[0] - position[0], p2[1] - position[1], p2[2] - position[2] };

  double* rotationAxis = this->OrientationTransform->TransformDoubleVector(baseVector);
  double rotationAngle = vtkMath::SignedAngleBetweenVectors(centeredP1, centeredP2, rotationAxis);

  this->OrientationTransform->RotateWXYZ(vtkMath::DegreesFromRadians(rotationAngle), rotationAxis);
}

//------------------------------------------------------------------------------
void vtkOrientationRepresentation::CreateDefaultProperties()
{
  this->TorusProperties[Axis::X_AXIS] = vtkSmartPointer<vtkProperty>::New();
  this->TorusProperties[Axis::Y_AXIS] = vtkSmartPointer<vtkProperty>::New();
  this->TorusProperties[Axis::Z_AXIS] = vtkSmartPointer<vtkProperty>::New();
  this->SelectedTorusProperties[Axis::X_AXIS] = vtkSmartPointer<vtkProperty>::New();
  this->SelectedTorusProperties[Axis::Y_AXIS] = vtkSmartPointer<vtkProperty>::New();
  this->SelectedTorusProperties[Axis::Z_AXIS] = vtkSmartPointer<vtkProperty>::New();

  this->TorusProperties[Axis::X_AXIS]->SetColor(1, 0, 0);
  this->TorusProperties[Axis::Y_AXIS]->SetColor(0, 1, 0);
  this->TorusProperties[Axis::Z_AXIS]->SetColor(0, 0, 1);
  this->SelectedTorusProperties[Axis::X_AXIS]->SetColor(1, 0, 0);
  this->SelectedTorusProperties[Axis::Y_AXIS]->SetColor(0, 1, 0);
  this->SelectedTorusProperties[Axis::Z_AXIS]->SetColor(0, 0, 1);
  this->SelectedTorusProperties[Axis::X_AXIS]->SetAmbient(1.0);
  this->SelectedTorusProperties[Axis::Y_AXIS]->SetAmbient(1.0);
  this->SelectedTorusProperties[Axis::Z_AXIS]->SetAmbient(1.0);
}

//------------------------------------------------------------------------------
void vtkOrientationRepresentation::PlaceWidget(double bds[6])
{
  double center[3];
  this->AdjustBounds(bds, this->InitialBounds, center);

  this->InitialLength = sqrt((this->InitialBounds[1] - this->InitialBounds[0]) *
      (this->InitialBounds[1] - this->InitialBounds[0]) +
    (this->InitialBounds[3] - this->InitialBounds[2]) *
      (this->InitialBounds[3] - this->InitialBounds[2]) +
    (this->InitialBounds[5] - this->InitialBounds[4]) *
      (this->InitialBounds[5] - this->InitialBounds[4]));

  this->BaseTransform->Identity();
  this->BaseTransform->Scale(this->InitialLength, this->InitialLength, this->InitialLength);
  this->BaseTransform->Translate(center);

  this->OrientationTransform->Identity();
}

//------------------------------------------------------------------------------
int vtkOrientationRepresentation::ComputeInteractionState(int X, int Y, int modify)
{
  if (!this->Renderer || !this->Renderer->IsInViewport(X, Y))
  {
    return this->InteractionState = vtkOrientationRepresentation::Outside;
  }

  this->LastHandle = this->CurrentHandle;
  this->CurrentHandle = nullptr;
  vtkAssemblyPath* path = this->GetAssemblyPath(X, Y, 0., this->HandlePicker);

  if (path != nullptr)
  {
    this->ValidPick = 1;
    this->CurrentHandle = path->GetFirstNode()->GetViewProp();
    if (this->CurrentHandle == this->TorusActors[Axis::X_AXIS])
    {
      return this->InteractionState = vtkOrientationRepresentation::RotatingX;
    }
    else if (this->CurrentHandle == this->TorusActors[Axis::Y_AXIS])
    {
      return this->InteractionState = vtkOrientationRepresentation::RotatingY;
    }
    else if (this->CurrentHandle == this->TorusActors[Axis::Z_AXIS])
    {
      return this->InteractionState = vtkOrientationRepresentation::RotatingZ;
    }
  }

  return this->InteractionState = vtkOrientationRepresentation::Outside;
}

//------------------------------------------------------------------------------
void vtkOrientationRepresentation::SetInteractionState(int state)
{
  // Clamp to allowable values
  state = (state < vtkOrientationRepresentation::Outside
      ? vtkOrientationRepresentation::Outside
      : (state > vtkOrientationRepresentation::RotatingZ ? vtkOrientationRepresentation::RotatingZ
                                                         : state));

  this->InteractionState = state;
  if (state != vtkOrientationRepresentation::Outside)
  {
    this->HighlightHandle(this->CurrentHandle);
  }
  else
  {
    this->HighlightHandle(nullptr);
  }
}

//------------------------------------------------------------------------------
double* vtkOrientationRepresentation::GetBounds()
{
  this->BuildRepresentation();
  this->BoundingBox->SetBounds(this->TorusActors[Axis::X_AXIS]->GetBounds());
  this->BoundingBox->AddBounds(this->TorusActors[Axis::Y_AXIS]->GetBounds());
  this->BoundingBox->AddBounds(this->TorusActors[Axis::Z_AXIS]->GetBounds());
  return this->BoundingBox->GetBounds();
}

//------------------------------------------------------------------------------
vtkTransform* vtkOrientationRepresentation::GetTransform()
{
  return this->OrientationTransform.Get();
}

//------------------------------------------------------------------------------
void vtkOrientationRepresentation::SetOrientation(double values[3])
{
  double* orientation = this->OrientationTransform->GetOrientation();
  if (orientation[0] == values[0] && orientation[1] == values[1] && orientation[2] == values[2])
  {
    return;
  }
  this->OrientationTransform->Identity();
  this->OrientationTransform->RotateX(values[0]);
  this->OrientationTransform->RotateY(values[1]);
  this->OrientationTransform->RotateZ(values[2]);
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkOrientationRepresentation::SetOrientationX(double value)
{
  double* orientation = this->OrientationTransform->GetOrientation();
  orientation[0] = value;
  this->SetOrientation(orientation);
}

//------------------------------------------------------------------------------
void vtkOrientationRepresentation::SetOrientationY(double value)
{
  double* orientation = this->OrientationTransform->GetOrientation();
  orientation[1] = value;
  this->SetOrientation(orientation);
}

//------------------------------------------------------------------------------
void vtkOrientationRepresentation::SetOrientationZ(double value)
{
  double* orientation = this->OrientationTransform->GetOrientation();
  orientation[2] = value;
  this->SetOrientation(orientation);
}

//------------------------------------------------------------------------------
double* vtkOrientationRepresentation::GetOrientation()
{
  return this->OrientationTransform->GetOrientation();
}

//------------------------------------------------------------------------------
double vtkOrientationRepresentation::GetOrientationX()
{
  return this->GetOrientation()[0];
}

//------------------------------------------------------------------------------
double vtkOrientationRepresentation::GetOrientationY()
{
  return this->GetOrientation()[1];
}

//------------------------------------------------------------------------------
double vtkOrientationRepresentation::GetOrientationZ()
{
  return this->GetOrientation()[2];
}

//------------------------------------------------------------------------------
void vtkOrientationRepresentation::SetProperty(int axis, bool selected, vtkProperty* property)
{
  Axis clampedAxis = axis < Axis::X_AXIS
    ? Axis::X_AXIS
    : (axis > Axis::Z_AXIS ? Axis::Z_AXIS : static_cast<Axis>(axis));
  if (selected)
  {
    if (this->SelectedTorusProperties[clampedAxis] != property)
    {
      this->SelectedTorusProperties[clampedAxis] = property;
      this->Modified();
    }
  }
  else if (this->TorusProperties[clampedAxis] != property)
  {
    this->TorusProperties[clampedAxis] = property;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
vtkProperty* vtkOrientationRepresentation::GetProperty(int axis, bool selected)
{
  Axis clampedAxis = axis < Axis::X_AXIS
    ? Axis::X_AXIS
    : (axis > Axis::Z_AXIS ? Axis::Z_AXIS : static_cast<Axis>(axis));
  return selected ? this->SelectedTorusProperties[clampedAxis] : this->TorusProperties[clampedAxis];
}

//------------------------------------------------------------------------------
void vtkOrientationRepresentation::SetLength(double length)
{
  length =
    length < MINIMUM_LENGTH ? MINIMUM_LENGTH : (length > MAXIMUM_LENGTH ? MAXIMUM_LENGTH : length);
  if (this->Length != length)
  {
    this->Length = length;
    for (int i = Axis::X_AXIS; i <= Axis::Z_AXIS; ++i)
    {
      this->TorusSources[static_cast<Axis>(i)]->SetScale(1.0, 1.0, this->Length);
    }
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkOrientationRepresentation::SetThickness(double thickness)
{
  thickness = thickness < MINIMUM_THICKNESS
    ? MINIMUM_THICKNESS
    : (thickness > MAXIMUM_THICKNESS ? MAXIMUM_THICKNESS : thickness);
  if (this->Thickness != thickness)
  {
    this->Thickness = thickness;
    for (int i = Axis::X_AXIS; i <= Axis::Z_AXIS; ++i)
    {
      this->TorusSources[static_cast<Axis>(i)]->SetThickness(this->Thickness);
    }
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkOrientationRepresentation::BuildRepresentation()
{
  // Rebuild only if necessary
  if (this->GetMTime() > this->BuildTime ||
    (this->Renderer && this->Renderer->GetVTKWindow() &&
      (this->Renderer->GetVTKWindow()->GetMTime() > this->BuildTime ||
        this->Renderer->GetActiveCamera()->GetMTime() > this->BuildTime)))
  {
    this->BuildTime.Modified();
  }
}

//------------------------------------------------------------------------------
void vtkOrientationRepresentation::ReleaseGraphicsResources(vtkWindow* w)
{
  for (const auto& torusActor : this->TorusActors)
  {
    torusActor.second->ReleaseGraphicsResources(w);
  }
}

//------------------------------------------------------------------------------
int vtkOrientationRepresentation::RenderOpaqueGeometry(vtkViewport* v)
{
  this->BuildRepresentation();

  int count = 0;
  for (const auto& torusActor : this->TorusActors)
  {
    count += torusActor.second->RenderOpaqueGeometry(v);
  }

  return count;
}

//------------------------------------------------------------------------------
int vtkOrientationRepresentation::RenderTranslucentPolygonalGeometry(vtkViewport* v)
{
  this->BuildRepresentation();

  int count = 0;
  for (const auto& torusActor : this->TorusActors)
  {
    count += torusActor.second->RenderTranslucentPolygonalGeometry(v);
  }

  return count;
}

//------------------------------------------------------------------------------
vtkTypeBool vtkOrientationRepresentation::HasTranslucentPolygonalGeometry()
{
  this->BuildRepresentation();

  int result = 0;
  for (const auto& torusActor : this->TorusActors)
  {
    result |= torusActor.second->HasTranslucentPolygonalGeometry();
  }

  return result;
}

//------------------------------------------------------------------------------
void vtkOrientationRepresentation::HighlightHandle(vtkProp* prop)
{
  // first unhighlight anything picked
  if (vtkActor* actor = vtkActor::SafeDownCast(this->LastHandle))
  {
    if (actor == this->TorusActors[X_AXIS])
    {
      actor->SetProperty(this->TorusProperties[X_AXIS]);
    }
    else if (actor == this->TorusActors[Y_AXIS])
    {
      actor->SetProperty(this->TorusProperties[Y_AXIS]);
    }
    else if (actor == this->TorusActors[Z_AXIS])
    {
      actor->SetProperty(this->TorusProperties[Z_AXIS]);
    }
  }

  if (vtkActor* actor = vtkActor::SafeDownCast(this->CurrentHandle))
  {
    if (actor == this->TorusActors[X_AXIS])
    {
      actor->SetProperty(this->SelectedTorusProperties[X_AXIS]);
    }
    else if (actor == this->TorusActors[Y_AXIS])
    {
      actor->SetProperty(this->SelectedTorusProperties[Y_AXIS]);
    }
    else if (actor == this->TorusActors[Z_AXIS])
    {
      actor->SetProperty(this->SelectedTorusProperties[Z_AXIS]);
    }
  }
}

//------------------------------------------------------------------------------
void vtkOrientationRepresentation::RegisterPickers()
{
  vtkPickingManager* pm = this->GetPickingManager();
  if (!pm)
  {
    return;
  }
  pm->AddPicker(this->HandlePicker, this);
}

//------------------------------------------------------------------------------
void vtkOrientationRepresentation::GetActors(vtkPropCollection* pc)
{
  if (pc != nullptr && this->GetVisibility())
  {
    for (const auto& torusActor : this->TorusActors)
    {
      torusActor.second->GetActors(pc);
    }
  }
  this->Superclass::GetActors(pc);
}

//------------------------------------------------------------------------------
void vtkOrientationRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Initial Bounds: "
     << "(" << this->InitialBounds[0] << "," << this->InitialBounds[1] << ") "
     << "(" << this->InitialBounds[2] << "," << this->InitialBounds[3] << ") "
     << "(" << this->InitialBounds[4] << "," << this->InitialBounds[5] << ")\n";
  os << indent << "Bounding Box: "
     << "(" << this->BoundingBox[0] << "," << this->BoundingBox[1] << ") "
     << "(" << this->BoundingBox[2] << "," << this->BoundingBox[3] << ") "
     << "(" << this->BoundingBox[4] << "," << this->BoundingBox[5] << ")\n";
}
VTK_ABI_NAMESPACE_END
