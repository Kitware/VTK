// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOrientationRepresentation.h"

#include "vtkActor.h"
#include "vtkAppendPolyData.h"
#include "vtkArrowSource.h"
#include "vtkAssemblyPath.h"
#include "vtkBox.h"
#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkCellPicker.h"
#include "vtkEventData.h"
#include "vtkInteractorObserver.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPickingManager.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyDataNormals.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkSuperquadricSource.h"
#include "vtkTransform.h"
#include "vtkTransformFilter.h"
#include "vtkWindow.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkOrientationRepresentation);

namespace
{
constexpr int NUMBER_OF_TORUS = 3;
constexpr int NUMBER_OF_ARROWS = 4 * NUMBER_OF_TORUS;

constexpr double X_VECTOR[3] = { 1.0, 0.0, 0.0 };
constexpr double Y_VECTOR[3] = { 0.0, 1.0, 0.0 };
constexpr double Z_VECTOR[3] = { 0.0, 0.0, 1.0 };

constexpr int TORUS_RESOLUTION = 64;
constexpr int TORUS_PHI_ROUNDNESS = 0;
constexpr double TORUS_CENTERS[NUMBER_OF_TORUS][3] = { { 0.0, 0.0, 0.0 }, { 0.001, 0.001, 0.001 },
  { -0.001, -0.001, -0.001 } };

constexpr int ARROW_RESOLUTION = 16;
constexpr double ARROW_ROTATION_X[NUMBER_OF_TORUS][2] = { { 0.0, 0.0 }, { 90.0, 90.0 },
  { 0.0, 0.0 } };
constexpr double ARROW_ROTATION_Y[NUMBER_OF_TORUS][2] = { { 90.0, -90.0 }, { 0.0, 0.0 },
  { 0.0, 0.0 } };
constexpr double ARROW_ROTATION_Z[NUMBER_OF_TORUS][2] = { { 90.0, -90.0 }, { 90.0, -90.0 },
  { 0.0, 180.0 } };
}

//------------------------------------------------------------------------------
vtkOrientationRepresentation::vtkOrientationRepresentation()
{
  this->InteractionState = vtkOrientationRepresentation::Outside;
  this->PlaceFactor = 1.0;
  this->ValidPick = 1;

  this->BaseTransform->PostMultiply();
  this->OrientationTransform->PostMultiply();

  // Set up the initial properties
  this->CreateDefaultProperties();

  // Create the torus and arrows
  this->InitSources();
  this->InitTransforms();

  for (int i = Axis::X_AXIS; i <= Axis::Z_AXIS; ++i)
  {
    Axis axis = static_cast<Axis>(i);

    vtkNew<vtkTransformFilter> torusOrientationTransformFilter;
    torusOrientationTransformFilter->SetTransform(this->OrientationTransform);
    torusOrientationTransformFilter->SetInputConnection(this->TorusSources[i]->GetOutputPort());
    vtkNew<vtkTransformFilter> torusBaseTransformFilter;
    torusBaseTransformFilter->SetTransform(this->BaseTransform);
    torusBaseTransformFilter->SetInputConnection(torusOrientationTransformFilter->GetOutputPort());

    vtkSmartPointer<vtkPolyDataNormals> arrows = this->GetArrowsOutput(i);
    vtkNew<vtkTransformFilter> arrowsOrientationTransformFilter;
    arrowsOrientationTransformFilter->SetTransform(this->OrientationTransform);
    arrowsOrientationTransformFilter->SetInputConnection(arrows->GetOutputPort());
    vtkNew<vtkTransformFilter> arrowsBaseTransformFilter;
    arrowsBaseTransformFilter->SetTransform(this->BaseTransform);
    arrowsBaseTransformFilter->SetInputConnection(
      arrowsOrientationTransformFilter->GetOutputPort());

    vtkNew<vtkPolyDataMapper> torusMapper;
    torusMapper->SetInputConnection(torusBaseTransformFilter->GetOutputPort());
    vtkNew<vtkPolyDataMapper> arrowsMapper;
    arrowsMapper->SetInputConnection(arrowsBaseTransformFilter->GetOutputPort());

    this->TorusActors[axis]->SetMapper(torusMapper);
    this->TorusActors[axis]->SetProperty(this->Properties[axis]);
    this->HandlePicker->AddPickList(this->TorusActors[axis]);

    this->ArrowsActors[axis]->SetMapper(arrowsMapper);
    this->ArrowsActors[axis]->SetProperty(this->Properties[axis]);
    this->HandlePicker->AddPickList(this->ArrowsActors[axis]);
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
      this->Rotate(prevPickPoint, pickPoint, ::X_VECTOR);
      break;
    case vtkOrientationRepresentation::RotatingY:
      this->Rotate(prevPickPoint, pickPoint, ::Y_VECTOR);
      break;
    case vtkOrientationRepresentation::RotatingZ:
      this->Rotate(prevPickPoint, pickPoint, ::Z_VECTOR);
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
  this->Properties[Axis::X_AXIS] = vtkSmartPointer<vtkProperty>::New();
  this->Properties[Axis::Y_AXIS] = vtkSmartPointer<vtkProperty>::New();
  this->Properties[Axis::Z_AXIS] = vtkSmartPointer<vtkProperty>::New();
  this->SelectedProperties[Axis::X_AXIS] = vtkSmartPointer<vtkProperty>::New();
  this->SelectedProperties[Axis::Y_AXIS] = vtkSmartPointer<vtkProperty>::New();
  this->SelectedProperties[Axis::Z_AXIS] = vtkSmartPointer<vtkProperty>::New();

  this->Properties[Axis::X_AXIS]->SetColor(1.0, 0.0, 0.0);
  this->Properties[Axis::Y_AXIS]->SetColor(0.0, 1.0, 0.0);
  this->Properties[Axis::Z_AXIS]->SetColor(0.0, 0.0, 1.0);
  this->SelectedProperties[Axis::X_AXIS]->SetColor(1.0, 0.0, 0.0);
  this->SelectedProperties[Axis::Y_AXIS]->SetColor(0.0, 1.0, 0.0);
  this->SelectedProperties[Axis::Z_AXIS]->SetColor(0.0, 0.0, 1.0);
  this->SelectedProperties[Axis::X_AXIS]->SetAmbient(1.0);
  this->SelectedProperties[Axis::Y_AXIS]->SetAmbient(1.0);
  this->SelectedProperties[Axis::Z_AXIS]->SetAmbient(1.0);
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
int vtkOrientationRepresentation::ComputeInteractionState(int X, int Y, int vtkNotUsed(modify))
{
  if (!this->Renderer || !this->Renderer->IsInViewport(X, Y))
  {
    return this->InteractionState = vtkOrientationRepresentation::Outside;
  }

  this->LastHandle = this->CurrentHandle;
  this->CurrentHandle = nullptr;
  vtkAssemblyPath* path = this->GetAssemblyPath(X, Y, 0., this->HandlePicker);

  if (path)
  {
    this->ValidPick = 1;
    this->CurrentHandle = path->GetFirstNode()->GetViewProp();
  }
  if (!this->CurrentHandle)
  {
    return this->InteractionState = vtkOrientationRepresentation::Outside;
  }

  if (this->CurrentHandle.Get() == this->TorusActors[Axis::X_AXIS] ||
    this->CurrentHandle.Get() == this->ArrowsActors[Axis::X_AXIS])
  {
    return this->InteractionState = vtkOrientationRepresentation::RotatingX;
  }
  else if (this->CurrentHandle.Get() == this->TorusActors[Axis::Y_AXIS] ||
    this->CurrentHandle.Get() == this->ArrowsActors[Axis::Y_AXIS])
  {
    return this->InteractionState = vtkOrientationRepresentation::RotatingY;
  }
  else if (this->CurrentHandle.Get() == this->TorusActors[Axis::Z_AXIS] ||
    this->CurrentHandle.Get() == this->ArrowsActors[Axis::Z_AXIS])
  {
    return this->InteractionState = vtkOrientationRepresentation::RotatingZ;
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
  this->HighlightHandle();
}

//------------------------------------------------------------------------------
double* vtkOrientationRepresentation::GetBounds()
{
  this->BuildRepresentation();
  this->BoundingBox->SetBounds(this->TorusActors[Axis::X_AXIS]->GetBounds());
  this->BoundingBox->AddBounds(this->TorusActors[Axis::Y_AXIS]->GetBounds());
  this->BoundingBox->AddBounds(this->TorusActors[Axis::Z_AXIS]->GetBounds());
  if (this->ShowArrows)
  {
    this->BoundingBox->AddBounds(this->ArrowsActors[Axis::X_AXIS]->GetBounds());
    this->BoundingBox->AddBounds(this->ArrowsActors[Axis::Y_AXIS]->GetBounds());
    this->BoundingBox->AddBounds(this->ArrowsActors[Axis::Z_AXIS]->GetBounds());
  }
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
    if (this->SelectedProperties[clampedAxis] != property)
    {
      this->SelectedProperties[clampedAxis] = property;
      this->Modified();
    }
  }
  else if (this->Properties[clampedAxis] != property)
  {
    // Overwrite actors current property to avoid having to
    // highlight them for the property to update
    this->TorusActors[clampedAxis]->SetProperty(property);
    this->ArrowsActors[clampedAxis]->SetProperty(property);

    this->Properties[clampedAxis] = property;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
vtkProperty* vtkOrientationRepresentation::GetProperty(int axis, bool selected)
{
  Axis clampedAxis = axis < Axis::X_AXIS
    ? Axis::X_AXIS
    : (axis > Axis::Z_AXIS ? Axis::Z_AXIS : static_cast<Axis>(axis));
  return selected ? this->SelectedProperties[clampedAxis] : this->Properties[clampedAxis];
}

//------------------------------------------------------------------------------
void vtkOrientationRepresentation::BuildRepresentation()
{
  // Rebuild only if necessary
  if (this->GetMTime() > this->BuildTime)
  {
    this->UpdateGeometry();
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
  for (const auto& arrowsActor : this->ArrowsActors)
  {
    arrowsActor.second->ReleaseGraphicsResources(w);
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
  if (this->ShowArrows)
  {
    for (const auto& arrowsActor : this->ArrowsActors)
    {
      count += arrowsActor.second->RenderOpaqueGeometry(v);
    }
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
  if (this->ShowArrows)
  {
    for (const auto& arrowsActor : this->ArrowsActors)
    {
      count += arrowsActor.second->RenderTranslucentPolygonalGeometry(v);
    }
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
  if (this->ShowArrows)
  {
    for (const auto& arrowsActor : this->ArrowsActors)
    {
      result |= arrowsActor.second->HasTranslucentPolygonalGeometry();
    }
  }

  return result;
}

//------------------------------------------------------------------------------
void vtkOrientationRepresentation::UpdateGeometry()
{
  for (const auto& torus : this->TorusSources)
  {
    torus->SetThickness(this->TorusThickness);
    torus->SetScale(1.0, 1.0, this->TorusLength);
  }
  for (const auto& arrow : this->ArrowSources)
  {
    arrow->SetTipLength(this->ArrowTipLength);
    arrow->SetTipRadius(this->ArrowTipRadius);
    arrow->SetShaftRadius(this->ArrowShaftRadius);
  }

  this->InitTransforms();
}

//------------------------------------------------------------------------------
void vtkOrientationRepresentation::HighlightHandle()
{
  if (vtkActor* actor = vtkActor::SafeDownCast(this->LastHandle))
  {
    if (actor == this->TorusActors[Axis::X_AXIS] || actor == this->ArrowsActors[Axis::X_AXIS])
    {
      this->TorusActors[Axis::X_AXIS]->SetProperty(this->Properties[Axis::X_AXIS]);
      this->ArrowsActors[Axis::X_AXIS]->SetProperty(this->Properties[Axis::X_AXIS]);
    }
    else if (actor == this->TorusActors[Axis::Y_AXIS] || actor == this->ArrowsActors[Axis::Y_AXIS])
    {
      this->TorusActors[Axis::Y_AXIS]->SetProperty(this->Properties[Axis::Y_AXIS]);
      this->ArrowsActors[Axis::Y_AXIS]->SetProperty(this->Properties[Axis::Y_AXIS]);
    }
    else if (actor == this->TorusActors[Axis::Z_AXIS] || actor == this->ArrowsActors[Axis::Z_AXIS])
    {
      this->TorusActors[Axis::Z_AXIS]->SetProperty(this->Properties[Axis::Z_AXIS]);
      this->ArrowsActors[Axis::Z_AXIS]->SetProperty(this->Properties[Axis::Z_AXIS]);
    }
  }

  if (vtkActor* actor = vtkActor::SafeDownCast(this->CurrentHandle))
  {
    if (actor == this->TorusActors[Axis::X_AXIS] || actor == this->ArrowsActors[Axis::X_AXIS])
    {
      this->TorusActors[Axis::X_AXIS]->SetProperty(this->SelectedProperties[Axis::X_AXIS]);
      this->ArrowsActors[Axis::X_AXIS]->SetProperty(this->SelectedProperties[Axis::X_AXIS]);
    }
    else if (actor == this->TorusActors[Axis::Y_AXIS] || actor == this->ArrowsActors[Axis::Y_AXIS])
    {
      this->TorusActors[Axis::Y_AXIS]->SetProperty(this->SelectedProperties[Axis::Y_AXIS]);
      this->ArrowsActors[Axis::Y_AXIS]->SetProperty(this->SelectedProperties[Axis::Y_AXIS]);
    }
    else if (actor == this->TorusActors[Axis::Z_AXIS] || actor == this->ArrowsActors[Axis::Z_AXIS])
    {
      this->TorusActors[Axis::Z_AXIS]->SetProperty(this->SelectedProperties[Axis::Z_AXIS]);
      this->ArrowsActors[Axis::Z_AXIS]->SetProperty(this->SelectedProperties[Axis::Z_AXIS]);
    }
  }
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkPolyDataNormals> vtkOrientationRepresentation::GetArrowsOutput(int axisIndex)
{
  int arrowIndex = 4 * axisIndex;

  vtkNew<vtkTransform> arrowTransform;
  arrowTransform->Identity();
  arrowTransform->RotateX(::ARROW_ROTATION_X[axisIndex][0]);
  arrowTransform->RotateY(::ARROW_ROTATION_Y[axisIndex][0]);
  arrowTransform->RotateZ(::ARROW_ROTATION_Z[axisIndex][0]);

  vtkNew<vtkTransformFilter> arrowUFTransformFilter;
  vtkNew<vtkTransformFilter> arrowUFTransformFilterBase;
  arrowUFTransformFilterBase->SetTransform(this->ArrowPosTransform);
  arrowUFTransformFilterBase->SetInputConnection(this->ArrowSources[arrowIndex++]->GetOutputPort());
  arrowUFTransformFilter->SetTransform(arrowTransform);
  arrowUFTransformFilter->SetInputConnection(arrowUFTransformFilterBase->GetOutputPort());

  vtkNew<vtkTransformFilter> arrowUBTransformFilter;
  vtkNew<vtkTransformFilter> arrowUBTransformFilterBase;
  arrowUBTransformFilterBase->SetTransform(this->ArrowPosInvTransform);
  arrowUBTransformFilterBase->SetInputConnection(this->ArrowSources[arrowIndex++]->GetOutputPort());
  arrowUBTransformFilter->SetTransform(arrowTransform);
  arrowUBTransformFilter->SetInputConnection(arrowUBTransformFilterBase->GetOutputPort());

  vtkNew<vtkTransform> arrowInvTransform;
  arrowInvTransform->Identity();
  arrowInvTransform->RotateX(::ARROW_ROTATION_X[axisIndex][1]);
  arrowInvTransform->RotateY(::ARROW_ROTATION_Y[axisIndex][1]);
  arrowInvTransform->RotateZ(::ARROW_ROTATION_Z[axisIndex][1]);

  vtkNew<vtkTransformFilter> arrowDFTransformFilter;
  vtkNew<vtkTransformFilter> arrowDFTransformFilterBase;
  arrowDFTransformFilterBase->SetTransform(this->ArrowPosTransform);
  arrowDFTransformFilterBase->SetInputConnection(this->ArrowSources[arrowIndex++]->GetOutputPort());
  arrowDFTransformFilter->SetTransform(arrowInvTransform);
  arrowDFTransformFilter->SetInputConnection(arrowDFTransformFilterBase->GetOutputPort());

  vtkNew<vtkTransformFilter> arrowDBTransformFilter;
  vtkNew<vtkTransformFilter> arrowDBTransformFilterBase;
  arrowDBTransformFilterBase->SetTransform(this->ArrowPosInvTransform);
  arrowDBTransformFilterBase->SetInputConnection(this->ArrowSources[arrowIndex++]->GetOutputPort());
  arrowDBTransformFilter->SetTransform(arrowInvTransform);
  arrowDBTransformFilter->SetInputConnection(arrowDBTransformFilterBase->GetOutputPort());

  vtkSmartPointer<vtkAppendPolyData> appendArrows = vtkSmartPointer<vtkAppendPolyData>::New();
  appendArrows->AddInputConnection(arrowUFTransformFilter->GetOutputPort());
  appendArrows->AddInputConnection(arrowUBTransformFilter->GetOutputPort());
  appendArrows->AddInputConnection(arrowDFTransformFilter->GetOutputPort());
  appendArrows->AddInputConnection(arrowDBTransformFilter->GetOutputPort());

  // For a better rendering, generate normals (torus already generates its own)
  vtkSmartPointer<vtkPolyDataNormals> arrowNormals = vtkSmartPointer<vtkPolyDataNormals>::New();
  arrowNormals->SetInputConnection(appendArrows->GetOutputPort());

  return arrowNormals;
}

//------------------------------------------------------------------------------
void vtkOrientationRepresentation::InitSources()
{
  this->TorusSources.clear();
  this->ArrowSources.clear();
  this->TorusSources.reserve(::NUMBER_OF_TORUS);
  this->ArrowSources.reserve(::NUMBER_OF_ARROWS);

  for (int i = 0; i < ::NUMBER_OF_TORUS; ++i)
  {
    vtkSmartPointer<vtkSuperquadricSource> torus = vtkSmartPointer<vtkSuperquadricSource>::New();
    torus->SetToroidal(1);
    torus->SetAxisOfSymmetry(i);
    torus->SetThetaResolution(::TORUS_RESOLUTION);
    torus->SetPhiRoundness(::TORUS_PHI_ROUNDNESS);
    torus->SetThickness(this->TorusThickness);
    torus->SetScale(1.0, 1.0, this->TorusLength);
    torus->SetCenter(::TORUS_CENTERS[i]);
    this->TorusSources.push_back(torus);
  }
  for (int i = 0; i < ::NUMBER_OF_ARROWS; ++i)
  {
    vtkSmartPointer<vtkArrowSource> arrow = vtkSmartPointer<vtkArrowSource>::New();
    arrow->SetTipResolution(::ARROW_RESOLUTION);
    arrow->SetShaftResolution(::ARROW_RESOLUTION);
    arrow->SetTipLength(this->ArrowTipLength);
    arrow->SetTipRadius(this->ArrowTipRadius);
    arrow->SetShaftRadius(this->ArrowShaftRadius);
    this->ArrowSources.push_back(arrow);
  }
}

//------------------------------------------------------------------------------
void vtkOrientationRepresentation::InitTransforms()
{
  this->ArrowPosTransform->Identity();
  this->ArrowPosTransform->Translate(0.0, 0.5 + this->ArrowDistance, 0.0);
  this->ArrowPosTransform->Scale(this->ArrowLength, 1.0, 1.0);

  this->ArrowPosInvTransform->Identity();
  this->ArrowPosInvTransform->Translate(0.0, -0.5 - this->ArrowDistance, 0.0);
  this->ArrowPosInvTransform->Scale(this->ArrowLength, 1.0, 1.0);
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
    if (this->ShowArrows)
    {
      for (const auto& arrowsActor : this->ArrowsActors)
      {
        arrowsActor.second->GetActors(pc);
      }
    }
  }
  this->Superclass::GetActors(pc);
}

//------------------------------------------------------------------------------
void vtkOrientationRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Bounding Box: "
     << "(" << this->BoundingBox[0] << "," << this->BoundingBox[1] << ") "
     << "(" << this->BoundingBox[2] << "," << this->BoundingBox[3] << ") "
     << "(" << this->BoundingBox[4] << "," << this->BoundingBox[5] << ")\n";
  os << indent << "Initial Bounds: "
     << "(" << this->InitialBounds[0] << "," << this->InitialBounds[1] << ") "
     << "(" << this->InitialBounds[2] << "," << this->InitialBounds[3] << ") "
     << "(" << this->InitialBounds[4] << "," << this->InitialBounds[5] << ")\n";
  os << indent << "Initial Length: " << this->InitialLength << std::endl;
  os << indent << "Torus Thickness: " << this->TorusThickness << std::endl;
  os << indent << "Torus Length: " << this->TorusLength << std::endl;
  os << indent << "Show Arrows: " << (this->ShowArrows ? "On" : "Off") << std::endl;
  if (this->ShowArrows)
  {
    os << indent << "Arrow Length: " << this->ArrowLength << std::endl;
    os << indent << "Arrow Tip Length: " << this->ArrowTipLength << std::endl;
    os << indent << "Arrow Tip Radius: " << this->ArrowTipRadius << std::endl;
    os << indent << "Arrow Shaft Radius: " << this->ArrowShaftRadius << std::endl;
    os << indent << "Arrow Distance: " << this->ArrowDistance << std::endl;
  }
}
VTK_ABI_NAMESPACE_END
