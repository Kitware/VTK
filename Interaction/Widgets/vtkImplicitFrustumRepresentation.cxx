// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkImplicitFrustumRepresentation.h"

#include "vtkActor.h"
#include "vtkAssemblyPath.h"
#include "vtkBoundingBox.h"
#include "vtkBox.h"
#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkCellPicker.h"
#include "vtkEllipseArcSource.h"
#include "vtkFrustum.h"
#include "vtkInteractorObserver.h"
#include "vtkLineSource.h"
#include "vtkMath.h"
#include "vtkPickingManager.h"
#include "vtkPlane.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkTransform.h"
#include "vtkTubeFilter.h"
#include "vtkVector.h"
#include <cmath>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkImplicitFrustumRepresentation);

namespace
{
const vtkVector3d FRUSTUM_UP_AXIS(0, 0, -1);
const vtkVector3d FRUSTUM_RIGHT_AXIS(1, 0, 0);
const vtkVector3d FRUSTUM_FORWARD_AXIS(0, 1, 0);
}

//------------------------------------------------------------------------------
vtkImplicitFrustumRepresentation::EdgeHandle::EdgeHandle()
{
  vtkNew<vtkPoints> points;
  points->SetDataTypeToDouble();
  this->PolyData->SetPoints(points);

  vtkNew<vtkCellArray> lines;
  this->PolyData->SetLines(lines);

  this->Tuber->SetInputData(this->PolyData);
  this->Tuber->SetNumberOfSides(12);

  this->Mapper->SetInputConnection(this->Tuber->GetOutputPort());
  this->Actor->SetMapper(this->Mapper);
  this->Mapper->ScalarVisibilityOff();
}

//------------------------------------------------------------------------------
vtkImplicitFrustumRepresentation::SphereHandle::SphereHandle()
{
  this->Source->SetThetaResolution(16);
  this->Source->SetPhiResolution(16);
  this->Mapper->SetInputConnection(this->Source->GetOutputPort());
  this->Actor->SetMapper(this->Mapper);
}

//------------------------------------------------------------------------------
vtkImplicitFrustumRepresentation::EllipseHandle::EllipseHandle()
{
  this->Source->SetClose(true);
  this->Source->SetStartAngle(0);
  this->Source->SetSegmentAngle(360);
  this->Tuber->SetInputConnection(this->Source->GetOutputPort());
  this->Tuber->SetNumberOfSides(12);
  this->Mapper->SetInputConnection(this->Tuber->GetOutputPort());
  this->Actor->SetMapper(this->Mapper);
}

//------------------------------------------------------------------------------
vtkImplicitFrustumRepresentation::vtkImplicitFrustumRepresentation()
{
  vtkNew<vtkTransform> transform;
  transform->Identity();
  this->Frustum->SetTransform(transform);

  // Orientation transform is in post multiply so we can edit it as we go
  this->OrientationTransform->Identity();
  this->OrientationTransform->PostMultiply();

  this->InteractionState = InteractionStateType::Outside;

  // Handle size is in pixels for this widget
  this->HandleSize = 5.0;

  // Build the representation of the widget
  vtkNew<vtkPoints> points;
  points->SetDataTypeToDouble();
  this->FrustumPD->SetPoints(points);

  vtkNew<vtkCellArray> polys;
  this->FrustumPD->SetPolys(polys);

  this->FrustumMapper->SetInputData(this->FrustumPD);
  this->FrustumActor->SetMapper(this->FrustumMapper);

  // Initialize handles
  this->YawHandle.Source->SetClose(false);
  this->YawHandle.Source->SetSegmentAngle(180);
  this->YawHandle.Source->SetStartAngle(0);

  this->PitchHandle.Source->SetClose(false);
  this->PitchHandle.Source->SetSegmentAngle(180);
  this->PitchHandle.Source->SetStartAngle(-90);

  // Initial creation of the widget, serves to initialize it
  std::array<double, 6> bounds = { -0.5, 0.5, -0.5, 0.5, -0.5, 0.5 };
  this->PlaceWidget(bounds.data());

  // Manage the picking stuff
  this->Picker->SetTolerance(0.005);
  this->Picker->AddPickList(this->OriginHandle.Actor);
  this->Picker->AddPickList(this->NearPlaneCenterHandle.Actor);
  this->Picker->AddPickList(this->NearPlaneEdgesHandle.Actor);
  this->Picker->AddPickList(this->RollHandle.Actor);
  this->Picker->AddPickList(this->PitchHandle.Actor);
  this->Picker->AddPickList(this->YawHandle.Actor);
  for (int edgeIdx = 0; edgeIdx < 4; ++edgeIdx)
  {
    this->Picker->AddPickList(this->FarPlaneHandles[edgeIdx].Actor);
  }
  this->Picker->AddPickList(this->GetOutlineActor());

  this->Picker->PickFromListOn();

  this->FrustumPicker->SetTolerance(0.005);
  this->FrustumPicker->AddPickList(this->FrustumActor);
  this->FrustumPicker->PickFromListOn();

  // Set up the initial properties
  // Frustum properties
  this->FrustumProperty->SetAmbient(1.0);
  this->FrustumProperty->SetAmbientColor(1.0, 1.0, 1.0);
  this->FrustumProperty->SetOpacity(0.5);

  // Origin handle properties
  this->OriginHandleProperty->SetAmbient(1.0);
  this->OriginHandleProperty->SetColor(1, 0, 0);

  this->SelectedOriginHandleProperty->SetAmbient(1.0);
  this->SelectedOriginHandleProperty->SetColor(0, 1, 0);

  // Edge property
  this->EdgeHandleProperty->SetAmbient(1.0);
  this->EdgeHandleProperty->SetColor(1.0, 0.0, 0.0);
  this->SelectedEdgeHandleProperty->SetAmbient(1.0);
  this->SelectedEdgeHandleProperty->SetColor(0.0, 1.0, 0.0);

  this->CreateDefaultProperties();

  // Pass the initial properties to the actors.
  this->FrustumActor->SetProperty(this->FrustumProperty);
  this->OriginHandle.Actor->SetProperty(this->OriginHandleProperty);
  this->NearPlaneCenterHandle.Actor->SetProperty(this->OriginHandleProperty);
  this->FrustumActor->SetProperty(this->FrustumProperty);
  this->NearPlaneEdgesHandle.Actor->SetProperty(this->EdgeHandleProperty);
  this->RollHandle.Actor->SetProperty(this->EdgeHandleProperty);
  this->YawHandle.Actor->SetProperty(this->EdgeHandleProperty);
  this->PitchHandle.Actor->SetProperty(this->EdgeHandleProperty);

  for (int edgeIdx = 0; edgeIdx < 4; ++edgeIdx)
  {
    this->FarPlaneHandles[edgeIdx].Actor->SetProperty(this->EdgeHandleProperty);
    this->FarPlaneHandles[edgeIdx].Actor->SetProperty(this->EdgeHandleProperty);
  }
}

//------------------------------------------------------------------------------
vtkImplicitFrustumRepresentation::~vtkImplicitFrustumRepresentation() = default;

//------------------------------------------------------------------------------
vtkVector3d vtkImplicitFrustumRepresentation::GetForwardAxis()
{
  vtkVector3d forwardAxis;
  this->OrientationTransform->TransformVector(
    ::FRUSTUM_FORWARD_AXIS.GetData(), forwardAxis.GetData());
  return forwardAxis;
}

//------------------------------------------------------------------------------
vtkVector3d vtkImplicitFrustumRepresentation::GetUpAxis()
{
  vtkVector3d upAxis;
  this->OrientationTransform->TransformVector(::FRUSTUM_UP_AXIS.GetData(), upAxis.GetData());
  return upAxis;
}

//------------------------------------------------------------------------------
vtkVector3d vtkImplicitFrustumRepresentation::GetRightAxis()
{
  vtkVector3d rightAxis;
  this->OrientationTransform->TransformVector(::FRUSTUM_RIGHT_AXIS.GetData(), rightAxis.GetData());
  return rightAxis;
}

//------------------------------------------------------------------------------
int vtkImplicitFrustumRepresentation::ComputeInteractionState(int X, int Y, int vtkNotUsed(modify))
{
  // See if anything has been selected
  vtkAssemblyPath* path = this->GetAssemblyPath(X, Y, 0., this->Picker);
  this->ActiveEdgeHandle = FrustumFace::None;

  // The second picker may need to be called. This is done because the frustum
  // wraps around things that can be picked; thus the frustum is the selection
  // of last resort.
  if (path == nullptr)
  {
    this->FrustumPicker->Pick(X, Y, 0., this->Renderer);
    this->FrustumPicker->GetCellId();
    path = this->FrustumPicker->GetPath();
  }

  if (path == nullptr) // Nothing picked
  {
    this->SetRepresentationState(InteractionStateType::Outside);
    this->InteractionState = InteractionStateType::Outside;
    return this->InteractionState;
  }

  // Something picked, continue
  this->ValidPick = true;

  // Depending on the interaction state (set by the widget) we modify
  // the state of the representation based on what is picked.
  if (this->InteractionState == InteractionStateType::Moving)
  {
    vtkProp* prop = path->GetFirstNode()->GetViewProp();
    if (prop == this->OriginHandle.Actor || prop == this->NearPlaneCenterHandle.Actor)
    {
      this->InteractionState = InteractionStateType::MovingOrigin;
      this->SetRepresentationState(InteractionStateType::MovingOrigin);
    }
    else if (prop == this->NearPlaneEdgesHandle.Actor)
    {
      this->InteractionState = InteractionStateType::AdjustingNearPlaneDistance;
      this->SetRepresentationState(InteractionStateType::AdjustingNearPlaneDistance);
    }
    else if (prop == this->FarPlaneHandles[static_cast<int>(FrustumFace::Right)].Actor)
    {
      this->ActiveEdgeHandle = FrustumFace::Right;
      this->InteractionState = InteractionStateType::AdjustingHorizontalAngle;
      this->SetRepresentationState(InteractionStateType::AdjustingHorizontalAngle);
    }
    else if (prop == this->FarPlaneHandles[static_cast<int>(FrustumFace::Left)].Actor)
    {
      this->ActiveEdgeHandle = FrustumFace::Left;
      this->InteractionState = InteractionStateType::AdjustingHorizontalAngle;
      this->SetRepresentationState(InteractionStateType::AdjustingHorizontalAngle);
    }
    else if (prop == this->FarPlaneHandles[static_cast<int>(FrustumFace::Bottom)].Actor)
    {
      this->ActiveEdgeHandle = FrustumFace::Bottom;
      this->InteractionState = InteractionStateType::AdjustingVerticalAngle;
      this->SetRepresentationState(InteractionStateType::AdjustingVerticalAngle);
    }
    else if (prop == this->FarPlaneHandles[static_cast<int>(FrustumFace::Top)].Actor)
    {
      this->ActiveEdgeHandle = FrustumFace::Top;
      this->InteractionState = InteractionStateType::AdjustingVerticalAngle;
      this->SetRepresentationState(InteractionStateType::AdjustingVerticalAngle);
    }
    else if (prop == this->FrustumActor)
    {
      // Choose rotation axis according to cell id
      vtkIdType pickedCellId = this->FrustumPicker->GetCellId();
      if (pickedCellId == static_cast<vtkIdType>(FrustumFace::Bottom) ||
        pickedCellId == static_cast<vtkIdType>(FrustumFace::Top))
      {
        this->InteractionState = InteractionStateType::AdjustingPitch;
        this->SetRepresentationState(InteractionStateType::AdjustingPitch);
      }
      else if (pickedCellId == static_cast<vtkIdType>(FrustumFace::Right) ||
        pickedCellId == static_cast<vtkIdType>(FrustumFace::Left))
      {
        this->InteractionState = InteractionStateType::AdjustingYaw;
        this->SetRepresentationState(InteractionStateType::AdjustingYaw);
      }
    }
    else if (prop == this->RollHandle.Actor)
    {
      this->InteractionState = InteractionStateType::AdjustingRoll;
      this->SetRepresentationState(InteractionStateType::AdjustingRoll);
    }
    else if (prop == this->PitchHandle.Actor)
    {
      this->InteractionState = InteractionStateType::AdjustingPitch;
      this->SetRepresentationState(InteractionStateType::AdjustingPitch);
    }
    else if (prop == this->YawHandle.Actor)
    {
      this->InteractionState = InteractionStateType::AdjustingYaw;
      this->SetRepresentationState(InteractionStateType::AdjustingYaw);
    }
    else
    {
      this->InteractionState = InteractionStateType::Outside;
      this->SetRepresentationState(InteractionStateType::Outside);
    }
  }
  else
  {
    // The widget provided a precise state, just use this one
    this->SetRepresentationState(static_cast<InteractionStateType>(this->InteractionState));
  }

  return this->InteractionState;
}

//------------------------------------------------------------------------------
void vtkImplicitFrustumRepresentation::SetRepresentationState(InteractionStateType state)
{
  if (this->RepresentationState == state)
  {
    return;
  }

  this->RepresentationState = state;
  this->Modified();

  this->HighlightOriginHandle(false);
  this->HighlightFarPlaneHorizontalHandle(false);
  this->HighlightFarPlaneVerticalHandle(false);
  this->HighlightNearPlaneHandle(false);
  this->HighlightRollHandle(false);
  this->HighlightPitchHandle(false);
  this->HighlightYawHandle(false);
  this->HighlightOutline(0);

  switch (state)
  {
    case InteractionStateType::TranslatingOriginOnAxis:
    case InteractionStateType::MovingOrigin:
      this->HighlightOriginHandle(true);
      break;

    case InteractionStateType::AdjustingHorizontalAngle:
      this->HighlightFarPlaneHorizontalHandle(true);
      break;

    case InteractionStateType::AdjustingVerticalAngle:
      this->HighlightFarPlaneVerticalHandle(true);
      break;

    case InteractionStateType::AdjustingNearPlaneDistance:
      this->HighlightNearPlaneHandle(true);
      break;

    case InteractionStateType::AdjustingRoll:
      this->HighlightRollHandle(true);
      break;

    case InteractionStateType::AdjustingPitch:
      this->HighlightPitchHandle(true);
      break;

    case InteractionStateType::AdjustingYaw:
      this->HighlightYawHandle(true);
      break;

    default:
      break;
  }
}

//------------------------------------------------------------------------------
void vtkImplicitFrustumRepresentation::StartWidgetInteraction(double e[2])
{
  this->StartEventPosition[0] = e[0];
  this->StartEventPosition[1] = e[1];
  this->StartEventPosition[2] = 0.0;
  this->LastEventPosition = { e[0], e[1], 0.0 };
}

//------------------------------------------------------------------------------
void vtkImplicitFrustumRepresentation::WidgetInteraction(double e[2])
{
  vtkCamera* camera = this->Renderer->GetActiveCamera();
  if (camera == nullptr)
  {
    return;
  }

  vtkVector2d eventPosition(e);

  vtkVector3d pickPoint = this->GetWorldPoint(this->Picker, e);
  vtkVector3d prevPickPoint = this->GetWorldPoint(this->Picker, this->LastEventPosition.GetData());
  vtkVector3d frustumPickPoint = this->GetWorldPoint(this->FrustumPicker, e);
  vtkVector3d prevFrustumPickPoint =
    this->GetWorldPoint(this->FrustumPicker, this->LastEventPosition.GetData());

  switch (this->InteractionState)
  {
    case InteractionStateType::MovingOrigin:
      this->TranslateOrigin(prevPickPoint, pickPoint);
      break;

    case InteractionStateType::TranslatingOriginOnAxis:
      this->TranslateOriginOnAxis(prevPickPoint, pickPoint);
      break;

    case InteractionStateType::AdjustingHorizontalAngle:
      this->AdjustHorizontalAngle(prevFrustumPickPoint, frustumPickPoint);
      break;

    case InteractionStateType::AdjustingVerticalAngle:
      this->AdjustVerticalAngle(prevFrustumPickPoint, frustumPickPoint);
      break;

    case InteractionStateType::AdjustingNearPlaneDistance:
      this->AdjustNearPlaneDistance(eventPosition, prevPickPoint, pickPoint);
      break;

    case InteractionStateType::AdjustingYaw:
      this->Rotate(prevPickPoint, pickPoint, vtkVector3d(0, 0, 1));
      break;

    case InteractionStateType::AdjustingPitch:
      this->Rotate(prevPickPoint, pickPoint, vtkVector3d(1, 0, 0));
      break;

    case InteractionStateType::AdjustingRoll:
      this->Rotate(prevPickPoint, pickPoint, vtkVector3d(0, 1, 0));
      break;
  }

  this->LastEventPosition = { e[0], e[1], 0.0 };

  this->BuildRepresentation();
}

//------------------------------------------------------------------------------
void vtkImplicitFrustumRepresentation::EndWidgetInteraction(double vtkNotUsed(e)[2])
{
  this->SetRepresentationState(InteractionStateType::Outside);
}

//------------------------------------------------------------------------------
double* vtkImplicitFrustumRepresentation::GetBounds()
{
  this->BuildRepresentation();

  this->BoundingBox->SetBounds(0, 0, 0, 0, 0, 0);
  this->BoundingBox->AddBounds(this->FrustumActor->GetBounds());
  this->BoundingBox->AddBounds(this->NearPlaneEdgesHandle.Actor->GetBounds());
  this->BoundingBox->AddBounds(this->OriginHandle.Actor->GetBounds());
  this->BoundingBox->AddBounds(this->NearPlaneCenterHandle.Actor->GetBounds());
  this->BoundingBox->AddBounds(this->RollHandle.Actor->GetBounds());
  this->BoundingBox->AddBounds(this->YawHandle.Actor->GetBounds());
  this->BoundingBox->AddBounds(this->PitchHandle.Actor->GetBounds());
  for (int edgeIdx = 0; edgeIdx < 4; ++edgeIdx)
  {
    this->BoundingBox->AddBounds(this->FarPlaneHandles[edgeIdx].Actor->GetBounds());
  }

  return this->BoundingBox->GetBounds();
}

//------------------------------------------------------------------------------
void vtkImplicitFrustumRepresentation::GetActors(vtkPropCollection* pc)
{
  this->NearPlaneEdgesHandle.Actor->GetActors(pc);
  this->OriginHandle.Actor->GetActors(pc);
  this->NearPlaneCenterHandle.Actor->GetActors(pc);
  this->RollHandle.Actor->GetActors(pc);
  this->YawHandle.Actor->GetActors(pc);
  this->PitchHandle.Actor->GetActors(pc);
  for (int edgeIdx = 0; edgeIdx < 4; ++edgeIdx)
  {
    this->FarPlaneHandles[edgeIdx].Actor->GetActors(pc);
  }
  this->GetOutlineActor()->GetActors(pc);
}

//------------------------------------------------------------------------------
void vtkImplicitFrustumRepresentation::ReleaseGraphicsResources(vtkWindow* window)
{
  this->GetOutlineActor()->ReleaseGraphicsResources(window);
  this->FrustumActor->ReleaseGraphicsResources(window);
  this->NearPlaneEdgesHandle.Actor->ReleaseGraphicsResources(window);
  this->OriginHandle.Actor->ReleaseGraphicsResources(window);
  this->NearPlaneCenterHandle.Actor->ReleaseGraphicsResources(window);
  this->YawHandle.Actor->ReleaseGraphicsResources(window);
  this->PitchHandle.Actor->ReleaseGraphicsResources(window);
  this->RollHandle.Actor->ReleaseGraphicsResources(window);
  for (int edgeIdx = 0; edgeIdx < 4; ++edgeIdx)
  {
    this->FarPlaneHandles[edgeIdx].Actor->ReleaseGraphicsResources(window);
  }
}

//------------------------------------------------------------------------------
int vtkImplicitFrustumRepresentation::RenderOpaqueGeometry(vtkViewport* viewport)
{
  int count = 0;
  this->BuildRepresentation();
  count += this->GetOutlineActor()->RenderOpaqueGeometry(viewport);
  count += this->NearPlaneEdgesHandle.Actor->RenderOpaqueGeometry(viewport);
  count += this->OriginHandle.Actor->RenderOpaqueGeometry(viewport);
  count += this->NearPlaneCenterHandle.Actor->RenderOpaqueGeometry(viewport);
  count += this->YawHandle.Actor->RenderOpaqueGeometry(viewport);
  count += this->PitchHandle.Actor->RenderOpaqueGeometry(viewport);
  count += this->RollHandle.Actor->RenderOpaqueGeometry(viewport);
  for (int edgeIdx = 0; edgeIdx < 4; ++edgeIdx)
  {
    count += this->FarPlaneHandles[edgeIdx].Actor->RenderOpaqueGeometry(viewport);
  }

  if (this->DrawFrustum)
  {
    count += this->FrustumActor->RenderOpaqueGeometry(viewport);
  }

  return count;
}

//------------------------------------------------------------------------------
int vtkImplicitFrustumRepresentation::RenderTranslucentPolygonalGeometry(vtkViewport* viewport)
{
  int count = 0;
  this->BuildRepresentation();
  count += this->GetOutlineActor()->RenderTranslucentPolygonalGeometry(viewport);
  count += this->NearPlaneEdgesHandle.Actor->RenderTranslucentPolygonalGeometry(viewport);
  count += this->OriginHandle.Actor->RenderTranslucentPolygonalGeometry(viewport);
  count += this->NearPlaneCenterHandle.Actor->RenderTranslucentPolygonalGeometry(viewport);
  count += this->YawHandle.Actor->RenderTranslucentPolygonalGeometry(viewport);
  count += this->PitchHandle.Actor->RenderTranslucentPolygonalGeometry(viewport);
  count += this->RollHandle.Actor->RenderTranslucentPolygonalGeometry(viewport);
  for (int edgeIdx = 0; edgeIdx < 4; ++edgeIdx)
  {
    count += this->FarPlaneHandles[edgeIdx].Actor->RenderTranslucentPolygonalGeometry(viewport);
  }

  if (this->DrawFrustum)
  {
    count += this->FrustumActor->RenderTranslucentPolygonalGeometry(viewport);
  }

  return count;
}

//------------------------------------------------------------------------------
vtkTypeBool vtkImplicitFrustumRepresentation::HasTranslucentPolygonalGeometry()
{
  vtkTypeBool result = false;
  result |= this->GetOutlineActor()->HasTranslucentPolygonalGeometry();
  result |= this->NearPlaneEdgesHandle.Actor->HasTranslucentPolygonalGeometry();
  result |= this->OriginHandle.Actor->HasTranslucentPolygonalGeometry();
  result |= this->NearPlaneCenterHandle.Actor->HasTranslucentPolygonalGeometry();
  result |= this->RollHandle.Actor->HasTranslucentPolygonalGeometry();
  result |= this->YawHandle.Actor->HasTranslucentPolygonalGeometry();
  result |= this->PitchHandle.Actor->HasTranslucentPolygonalGeometry();
  for (int edgeIdx = 0; edgeIdx < 4; ++edgeIdx)
  {
    result |= this->FarPlaneHandles[edgeIdx].Actor->HasTranslucentPolygonalGeometry();
  }

  if (this->DrawFrustum)
  {
    result |= this->FrustumActor->HasTranslucentPolygonalGeometry();
  }

  return result;
}

//------------------------------------------------------------------------------
void vtkImplicitFrustumRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Origin: " << this->Origin << std::endl;
  os << indent << "Orientation Transform: " << this->OrientationTransform << std::endl;
  os << indent << "Edges Handles Property: " << this->EdgeHandleProperty << std::endl;
  os << indent << "Selected Edges Handles Property: " << this->SelectedEdgeHandleProperty
     << std::endl;
  os << indent << "Origin Handles Property: " << this->OriginHandleProperty << std::endl;
  os << indent << "Selected Origin Handles Property: " << this->SelectedOriginHandleProperty
     << std::endl;
  os << indent << "Frustum Property: " << this->FrustumProperty << std::endl;
  os << indent << "Along X Axis: " << (this->AlongXAxis ? "On" : "Off") << std::endl;
  os << indent << "Along Y Axis: " << (this->AlongYAxis ? "On" : "Off") << std::endl;
  os << indent << "ALong Z Axis: " << (this->AlongZAxis ? "On" : "Off") << std::endl;
  os << indent << "Draw Frustum: " << (this->DrawFrustum ? "On" : "Off") << std::endl;

  os << indent << "Representation State: ";
  switch (this->RepresentationState)
  {
    case Outside:
      os << "Outside" << std::endl;
      break;
    case Moving:
      os << "Moving" << std::endl;
      break;
    case MovingOrigin:
      os << "MovingOrigin" << std::endl;
      break;
    case TranslatingOriginOnAxis:
      os << "TranslatingOriginOnAxis" << std::endl;
      break;
    case AdjustingHorizontalAngle:
      os << "AdjustingHorizontalAngle" << std::endl;
      break;
    case AdjustingVerticalAngle:
      os << "AdjustingVerticalAngle" << std::endl;
      break;
    case AdjustingNearPlaneDistance:
      os << "AdjustingNearPlaneDistance" << std::endl;
      break;
    case AdjustingYaw:
      os << "AdjustingYaw" << std::endl;
      break;
    case AdjustingPitch:
      os << "AdjustingPitch" << std::endl;
      break;
    case AdjustingRoll:
      os << "AdjustingRoll" << std::endl;
      break;
  }
}

//------------------------------------------------------------------------------
void vtkImplicitFrustumRepresentation::HighlightOriginHandle(bool highlight)
{
  if (highlight)
  {
    this->OriginHandle.Actor->SetProperty(this->SelectedOriginHandleProperty);
    this->NearPlaneCenterHandle.Actor->SetProperty(this->SelectedOriginHandleProperty);
  }
  else
  {
    this->OriginHandle.Actor->SetProperty(this->OriginHandleProperty);
    this->NearPlaneCenterHandle.Actor->SetProperty(this->OriginHandleProperty);
  }
}

//------------------------------------------------------------------------------
void vtkImplicitFrustumRepresentation::HighlightFarPlaneVerticalHandle(bool highlight)
{
  if (highlight)
  {
    this->FarPlaneHandles[static_cast<int>(FrustumFace::Bottom)].Actor->SetProperty(
      this->SelectedEdgeHandleProperty);
    this->FarPlaneHandles[static_cast<int>(FrustumFace::Top)].Actor->SetProperty(
      this->SelectedEdgeHandleProperty);
  }
  else
  {
    this->FarPlaneHandles[static_cast<int>(FrustumFace::Bottom)].Actor->SetProperty(
      this->EdgeHandleProperty);
    this->FarPlaneHandles[static_cast<int>(FrustumFace::Top)].Actor->SetProperty(
      this->EdgeHandleProperty);
  }
}

//------------------------------------------------------------------------------
void vtkImplicitFrustumRepresentation::HighlightFarPlaneHorizontalHandle(bool highlight)
{
  if (highlight)
  {
    this->FarPlaneHandles[static_cast<int>(FrustumFace::Right)].Actor->SetProperty(
      this->SelectedEdgeHandleProperty);
    this->FarPlaneHandles[static_cast<int>(FrustumFace::Left)].Actor->SetProperty(
      this->SelectedEdgeHandleProperty);
  }
  else
  {
    this->FarPlaneHandles[static_cast<int>(FrustumFace::Right)].Actor->SetProperty(
      this->EdgeHandleProperty);
    this->FarPlaneHandles[static_cast<int>(FrustumFace::Left)].Actor->SetProperty(
      this->EdgeHandleProperty);
  }
}

//------------------------------------------------------------------------------
void vtkImplicitFrustumRepresentation::HighlightNearPlaneHandle(bool highlight)
{
  if (highlight)
  {
    this->NearPlaneEdgesHandle.Actor->SetProperty(this->SelectedEdgeHandleProperty);
  }
  else
  {
    this->NearPlaneEdgesHandle.Actor->SetProperty(this->EdgeHandleProperty);
  }
}

//------------------------------------------------------------------------------
void vtkImplicitFrustumRepresentation::HighlightRollHandle(bool highlight)
{
  if (highlight)
  {
    this->RollHandle.Actor->SetProperty(this->SelectedEdgeHandleProperty);
  }
  else
  {
    this->RollHandle.Actor->SetProperty(this->EdgeHandleProperty);
  }
}

//------------------------------------------------------------------------------
void vtkImplicitFrustumRepresentation::HighlightYawHandle(bool highlight)
{
  if (highlight)
  {
    this->YawHandle.Actor->SetProperty(this->SelectedEdgeHandleProperty);
  }
  else
  {
    this->YawHandle.Actor->SetProperty(this->EdgeHandleProperty);
  }
}

//------------------------------------------------------------------------------
void vtkImplicitFrustumRepresentation::HighlightPitchHandle(bool highlight)
{
  if (highlight)
  {
    this->PitchHandle.Actor->SetProperty(this->SelectedEdgeHandleProperty);
  }
  else
  {
    this->PitchHandle.Actor->SetProperty(this->EdgeHandleProperty);
  }
}

//------------------------------------------------------------------------------
void vtkImplicitFrustumRepresentation::TranslateOrigin(const vtkVector3d& p1, const vtkVector3d& p2)
{
  if (this->Renderer == nullptr)
  {
    return;
  }

  vtkCamera* camera = this->Renderer->GetActiveCamera();
  if (camera == nullptr)
  {
    return;
  }

  // Get the motion vector
  vtkVector3d translation(0, 0, 0);

  if (!this->IsTranslationConstrained())
  {
    translation = p2 - p1;
  }
  else
  {
    translation[this->GetTranslationAxis()] =
      p2[this->GetTranslationAxis()] - p1[this->GetTranslationAxis()];
  }

  // Translate the current origin
  vtkVector3d newOrigin = this->Origin + translation;

  // Project back onto plane orthogonal to camera
  vtkVector3d vpn;
  camera->GetViewPlaneNormal(vpn.GetData());

  vtkPlane::ProjectPoint(
    newOrigin.GetData(), this->Origin.GetData(), vpn.GetData(), newOrigin.GetData());

  this->Origin = newOrigin;
  this->UpdateFrustumTransform();
}

//------------------------------------------------------------------------------
void vtkImplicitFrustumRepresentation::TranslateOriginOnAxis(
  const vtkVector3d& p1, const vtkVector3d& p2)
{
  vtkVector3d translation = p2 - p1;

  // Add to the current point, project back down onto plane
  vtkVector3d axis = this->GetForwardAxis();

  // Project the point on the axis vector
  this->Origin = this->Origin + (axis * axis.Dot(translation));

  this->UpdateFrustumTransform();
}

//------------------------------------------------------------------------------
void vtkImplicitFrustumRepresentation::UpdateFrustumTransform()
{
  vtkNew<vtkTransform> transform;
  transform->Identity();
  transform->Translate(this->Origin.GetData());
  transform->Concatenate(this->OrientationTransform);
  transform->Inverse();

  if (this->Frustum->GetTransform() != transform)
  {
    this->Frustum->SetTransform(transform);
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkImplicitFrustumRepresentation::AdjustHorizontalAngle(
  const vtkVector3d& vtkNotUsed(previous), const vtkVector3d& current)
{
  vtkVector3d edge = current - this->Origin;
  double horizontalDistance = edge.Dot(this->GetRightAxis());
  horizontalDistance = std::fabs(horizontalDistance);
  const double length = edge.Norm();

  double angle = std::asin(horizontalDistance / length);

  this->SetHorizontalAngle(vtkMath::DegreesFromRadians(angle));
}

//------------------------------------------------------------------------------
void vtkImplicitFrustumRepresentation::AdjustVerticalAngle(
  const vtkVector3d& vtkNotUsed(p1), const vtkVector3d& current)
{
  vtkVector3d edge = current - this->Origin;
  double verticalDistance = edge.Dot(this->GetUpAxis());
  verticalDistance = std::fabs(verticalDistance);
  const double length = edge.Norm();

  double angle = std::asin(verticalDistance / length);

  this->SetVerticalAngle(vtkMath::DegreesFromRadians(angle));
}

//------------------------------------------------------------------------------
void vtkImplicitFrustumRepresentation::AdjustNearPlaneDistance(
  const vtkVector2d& eventPosition, const vtkVector3d& p1, const vtkVector3d& p2)
{
  if (eventPosition[0] == this->LastEventPosition[0] &&
    eventPosition[1] == this->LastEventPosition[1])
  {
    return;
  }

  vtkVector3d delta = p2 - p1;
  double currentDistance = this->Frustum->GetNearPlaneDistance();
  double deltaDistance = delta.Dot(this->GetForwardAxis());

  // Clamp the near plane so that it's still pickable
  double minNearPlaneDistance = 0.01 * this->Length;
  double newDistance = std::max(currentDistance + deltaDistance, minNearPlaneDistance);
  this->Frustum->SetNearPlaneDistance(newDistance);
}

//------------------------------------------------------------------------------
void vtkImplicitFrustumRepresentation::SetInteractionColor(double r, double g, double b)
{
  this->SelectedEdgeHandleProperty->SetColor(r, g, b);
  this->SelectedOriginHandleProperty->SetColor(r, g, b);
  this->SetSelectedOutlineColor(r, g, b);
}

//------------------------------------------------------------------------------
void vtkImplicitFrustumRepresentation::SetHandleColor(double r, double g, double b)
{
  this->EdgeHandleProperty->SetColor(r, g, b);
  this->OriginHandleProperty->SetColor(r, g, b);
}

//------------------------------------------------------------------------------
void vtkImplicitFrustumRepresentation::SetForegroundColor(double r, double g, double b)
{
  this->FrustumProperty->SetAmbientColor(r, g, b);
  this->SetOutlineColor(r, g, b);
}

//------------------------------------------------------------------------------
void vtkImplicitFrustumRepresentation::PlaceWidget(double bds[6])
{
  vtkVector<double, 6> bounds;
  vtkVector3d center;
  this->AdjustBounds(bds, bounds.GetData(), center.GetData());
  this->SetOutlineBounds(bds);

  for (int i = 0; i < 6; i++)
  {
    this->InitialBounds[i] = bounds[i];
  }
  this->SetWidgetBounds(bounds.GetData());

  vtkBoundingBox bbox(bounds.GetData());
  this->InitialLength = bbox.GetDiagonalLength();
  this->Length = this->InitialLength;

  this->OrientationTransform->Identity();
  if (this->AlongXAxis)
  {
    this->OrientationTransform->RotateZ(90);
  }
  else if (this->AlongZAxis)
  {
    this->OrientationTransform->RotateX(90);
  }

  this->ValidPick = true; // since we have positioned the widget successfully
  this->BuildRepresentation();
}

//------------------------------------------------------------------------------
void vtkImplicitFrustumRepresentation::SetOrigin(const vtkVector3d& xyz)
{
  if (xyz != this->Origin)
  {
    this->Origin = xyz;
    this->UpdateFrustumTransform();
  }
}

//------------------------------------------------------------------------------
void vtkImplicitFrustumRepresentation::SetOrigin(double x, double y, double z)
{
  this->SetOrigin(vtkVector3d(x, y, z));
}

//------------------------------------------------------------------------------
void vtkImplicitFrustumRepresentation::SetOrigin(double xyz[3])
{
  this->SetOrigin(vtkVector3d(xyz));
}

//------------------------------------------------------------------------------
double* vtkImplicitFrustumRepresentation::GetOrigin()
{
  return this->Origin.GetData();
}

//------------------------------------------------------------------------------
void vtkImplicitFrustumRepresentation::GetOrigin(double xyz[3]) const
{
  xyz[0] = this->Origin[0];
  xyz[1] = this->Origin[1];
  xyz[2] = this->Origin[2];
}

//----------------------------------------------------------------------------
void vtkImplicitFrustumRepresentation::SetOrientation(const vtkVector3d& xyz)
{
  vtkVector3d orientation(this->OrientationTransform->GetOrientation());
  if (orientation != xyz)
  {
    // Orientation transform is in Post Multiply mode
    // so rotation order is YXZ
    this->OrientationTransform->Identity();
    this->OrientationTransform->RotateY(xyz.GetY());
    this->OrientationTransform->RotateX(xyz.GetX());
    this->OrientationTransform->RotateZ(xyz.GetZ());
    this->UpdateFrustumTransform();
  }
}

//----------------------------------------------------------------------------
void vtkImplicitFrustumRepresentation::SetOrientation(double x, double y, double z)
{
  this->SetOrientation(vtkVector3d(x, y, z));
}

//----------------------------------------------------------------------------
void vtkImplicitFrustumRepresentation::SetOrientation(const double xyz[3])
{
  this->SetOrientation(vtkVector3d(xyz));
}

//----------------------------------------------------------------------------
double* vtkImplicitFrustumRepresentation::GetOrientation()
{
  return this->OrientationTransform->GetOrientation();
}

//----------------------------------------------------------------------------
void vtkImplicitFrustumRepresentation::GetOrientation(double& x, double& y, double& z)
{
  vtkVector3d orientation(this->OrientationTransform->GetOrientation());
  x = orientation[0];
  y = orientation[1];
  z = orientation[2];
}

//----------------------------------------------------------------------------
void vtkImplicitFrustumRepresentation::GetOrientation(double xyz[3])
{
  vtkVector3d orientation(this->OrientationTransform->GetOrientation());
  xyz[0] = orientation[0];
  xyz[1] = orientation[1];
  xyz[2] = orientation[2];
}

//------------------------------------------------------------------------------
void vtkImplicitFrustumRepresentation::Rotate(
  const vtkVector3d& prevPickPoint, const vtkVector3d& pickPoint, const vtkVector3d& axis)
{
  if (prevPickPoint == pickPoint)
  {
    return;
  }

  const vtkVector3d position(this->GetOrigin());
  const vtkVector3d centeredP1 = prevPickPoint - position;
  const vtkVector3d centeredP2 = pickPoint - position;

  vtkVector3d rotationAxis(this->OrientationTransform->TransformVector(axis.GetData()));
  double rotationAngle = vtkMath::SignedAngleBetweenVectors(
    centeredP1.GetData(), centeredP2.GetData(), rotationAxis.GetData());

  this->OrientationTransform->RotateWXYZ(
    vtkMath::DegreesFromRadians(rotationAngle), rotationAxis.GetData());

  this->UpdateFrustumTransform();
}

//------------------------------------------------------------------------------
double vtkImplicitFrustumRepresentation::GetHorizontalAngle() const
{
  return this->Frustum->GetHorizontalAngle();
}

//------------------------------------------------------------------------------
void vtkImplicitFrustumRepresentation::SetHorizontalAngle(double angle)
{
  this->Frustum->SetHorizontalAngle(angle);
}

//------------------------------------------------------------------------------
double vtkImplicitFrustumRepresentation::GetVerticalAngle() const
{
  return this->Frustum->GetVerticalAngle();
}

//------------------------------------------------------------------------------
void vtkImplicitFrustumRepresentation::SetVerticalAngle(double angle)
{
  this->Frustum->SetVerticalAngle(angle);
}

//------------------------------------------------------------------------------
double vtkImplicitFrustumRepresentation::GetNearPlaneDistance() const
{
  return this->Frustum->GetNearPlaneDistance();
}

//------------------------------------------------------------------------------
void vtkImplicitFrustumRepresentation::SetNearPlaneDistance(double distance)
{
  this->Frustum->SetNearPlaneDistance(distance);
}

//------------------------------------------------------------------------------
void vtkImplicitFrustumRepresentation::SetDrawFrustum(bool drawFrustum)
{
  if (drawFrustum == this->DrawFrustum)
  {
    return;
  }

  this->DrawFrustum = drawFrustum;
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkImplicitFrustumRepresentation::SetAlongXAxis(bool var)
{
  if (this->AlongXAxis != var)
  {
    this->AlongXAxis = var;
    this->Modified();
  }
  if (var)
  {
    this->AlongYAxisOff();
    this->AlongZAxisOff();
  }
}

//------------------------------------------------------------------------------
void vtkImplicitFrustumRepresentation::SetAlongYAxis(bool var)
{
  if (this->AlongYAxis != var)
  {
    this->AlongYAxis = var;
    this->Modified();
  }
  if (var)
  {
    this->AlongXAxisOff();
    this->AlongZAxisOff();
  }
}

//------------------------------------------------------------------------------
void vtkImplicitFrustumRepresentation::SetAlongZAxis(bool var)
{
  if (this->AlongZAxis != var)
  {
    this->AlongZAxis = var;
    this->Modified();
  }
  if (var)
  {
    this->AlongXAxisOff();
    this->AlongYAxisOff();
  }
}

//------------------------------------------------------------------------------
void vtkImplicitFrustumRepresentation::GetPolyData(vtkPolyData* pd)
{
  pd->ShallowCopy(this->FrustumPD);
}

//------------------------------------------------------------------------------
void vtkImplicitFrustumRepresentation::UpdatePlacement()
{
  this->UpdateOutline();
  this->BuildRepresentation();
}

//------------------------------------------------------------------------------
void vtkImplicitFrustumRepresentation::BuildRepresentation()
{
  if (this->Renderer == nullptr || !this->Renderer->GetRenderWindow())
  {
    return;
  }

  if (this->GetMTime() > this->BuildTime || this->Frustum->GetMTime() > this->BuildTime ||
    this->Renderer->GetRenderWindow()->GetMTime() > this->BuildTime)
  {
    vtkInformation* info = this->GetPropertyKeys();
    this->FrustumActor->SetPropertyKeys(info);
    this->NearPlaneEdgesHandle.Actor->SetPropertyKeys(info);
    this->OriginHandle.Actor->SetPropertyKeys(info);
    this->NearPlaneCenterHandle.Actor->SetPropertyKeys(info);

    for (int edgeIdx = 0; edgeIdx < 4; ++edgeIdx)
    {
      this->FarPlaneHandles[edgeIdx].Actor->SetPropertyKeys(info);
    }

    this->GetOutlineActor()->SetPropertyKeys(info);

    // Build an oriented basis - frustum is aligned to the y axis
    vtkVector3d origin(this->GetOrigin());
    vtkVector3d forwardAxis = this->GetForwardAxis();
    vtkVector3d upAxis = this->GetUpAxis();
    vtkVector3d rightAxis = this->GetRightAxis();

    this->UpdateCenterAndBounds(origin.GetData());

    double outlineBounds[6];
    this->GetOutlineBounds(outlineBounds);
    double param1 = 0;
    double param2 = 0;
    int plane1 = 0;
    int plane2 = 0;
    vtkVector3d intersection1;
    vtkVector3d intersection2;
    vtkVector3d endPoint = origin + forwardAxis * this->GetDiagonalLength();
    vtkBox::IntersectWithLine(outlineBounds, origin.GetData(), endPoint.GetData(), param1, param2,
      intersection1.GetData(), intersection2.GetData(), plane1, plane2);

    vtkVector3d distanceToOutline = intersection2 - origin;
    this->Length = distanceToOutline.Norm();

    // Set up the position handles
    vtkVector3d originHandlePosition = origin;
    vtkVector3d nearPlaneCenter = origin + forwardAxis * this->GetNearPlaneDistance();
    this->OriginHandle.Source->SetCenter(originHandlePosition.GetData());
    this->NearPlaneCenterHandle.Source->SetCenter(nearPlaneCenter.GetData());

    // Place the orientation controls
    double orientationHandlesRadius = 0.2 * this->Length;
    vtkVector3d orientationHandlesCenter = origin;

    this->RollHandle.Source->SetMajorRadiusVector(orientationHandlesRadius, 0, 0);
    this->RollHandle.Source->SetCenter(orientationHandlesCenter.GetData());
    this->RollHandle.Source->SetNormal(forwardAxis.GetData());

    vtkVector3d pitchAxis(0, -orientationHandlesRadius, 0);
    this->OrientationTransform->TransformVector(pitchAxis.GetData(), pitchAxis.GetData());

    this->PitchHandle.Source->SetCenter(orientationHandlesCenter.GetData());
    this->PitchHandle.Source->SetMajorRadiusVector(pitchAxis.GetData());
    this->PitchHandle.Source->SetNormal(rightAxis.GetData());

    vtkVector3d yawAxis(orientationHandlesRadius, 0, 0);
    this->OrientationTransform->TransformVector(yawAxis.GetData(), yawAxis.GetData());

    this->YawHandle.Source->SetCenter(orientationHandlesCenter.GetData());
    this->YawHandle.Source->SetMajorRadiusVector(yawAxis.GetData());
    this->YawHandle.Source->SetNormal(upAxis.GetData());

    // Construct frustum
    this->BuildFrustum();

    this->SizeHandles();
    this->BuildTime.Modified();
  }
}

//------------------------------------------------------------------------------
void vtkImplicitFrustumRepresentation::SizeHandles()
{
  double radius = this->SizeHandlesInPixels(1.5, this->GetOrigin());

  this->OriginHandle.Source->SetRadius(radius);
  this->NearPlaneCenterHandle.Source->SetRadius(radius);

  for (int edgeIdx = 0; edgeIdx < 4; ++edgeIdx)
  {
    this->FarPlaneHandles[edgeIdx].Tuber->SetRadius(0.25 * radius);
  }

  this->NearPlaneEdgesHandle.Tuber->SetRadius(0.25 * radius);
  this->RollHandle.Tuber->SetRadius(0.25 * radius);
  this->YawHandle.Tuber->SetRadius(0.25 * radius);
  this->PitchHandle.Tuber->SetRadius(0.25 * radius);
}

//------------------------------------------------------------------------------
void vtkImplicitFrustumRepresentation::BuildFrustum()
{
  const double height = this->Length;

  this->FrustumPD->Reset();

  // The edge between two faces of the frustum is given by the
  // the cross product between their normals
  vtkVector3d rightNormal(this->Frustum->GetRightPlane()->GetNormal());
  vtkVector3d leftNormal(this->Frustum->GetLeftPlane()->GetNormal());
  vtkVector3d bottomNormal(this->Frustum->GetBottomPlane()->GetNormal());
  vtkVector3d topNormal(this->Frustum->GetTopPlane()->GetNormal());

  std::array<vtkVector3d, 4> edgeDirections = {
    bottomNormal.Cross(leftNormal).Normalized(),  // bottom-left
    rightNormal.Cross(bottomNormal).Normalized(), // bottom-right
    topNormal.Cross(rightNormal).Normalized(),    // top-right
    leftNormal.Cross(topNormal).Normalized(),     // top-left
  };

  vtkNew<vtkIdList> nearPlanePointIndices;
  nearPlanePointIndices->Allocate(4);
  vtkNew<vtkIdList> farPlanePointIndices;
  farPlanePointIndices->Allocate(4);

  vtkNew<vtkTransform> transform;
  transform->Identity();
  transform->Translate(this->Origin.GetData());
  transform->Concatenate(this->OrientationTransform);

  // Generate frustum points
  vtkPoints* frustumPoints = this->FrustumPD->GetPoints();
  for (const vtkVector3d& direction : edgeDirections)
  {
    // Scale the vector so that y is on the far or near plane
    vtkVector3d nearPoint = direction * (this->Frustum->GetNearPlaneDistance() / direction.GetY());
    vtkVector3d farPoint = direction * (height / direction.GetY());

    // Apply frustum transform
    transform->TransformPoint(nearPoint.GetData(), nearPoint.GetData());
    transform->TransformPoint(farPoint.GetData(), farPoint.GetData());

    vtkIdType nearPointIdx = frustumPoints->InsertNextPoint(nearPoint.GetData());
    vtkIdType farPointIdx = frustumPoints->InsertNextPoint(farPoint.GetData());

    // Register point indices to the plane they belong to create handles later on
    nearPlanePointIndices->InsertNextId(nearPointIdx);
    farPlanePointIndices->InsertNextId(farPointIdx);
  }

  // Create frustum planes
  vtkCellArray* polys = this->FrustumPD->GetPolys();
  polys->InsertNextCell({ 2, 3, 5, 4 }); // Right
  polys->InsertNextCell({ 6, 7, 1, 0 }); // Left
  polys->InsertNextCell({ 4, 5, 7, 6 }); // Top
  polys->InsertNextCell({ 0, 1, 3, 2 }); // Bottom
  polys->InsertNextCell({ 0, 2, 4, 6 }); // Near

  this->FrustumPD->Modified();

  // Create edges handles
  this->NearPlaneEdgesHandle.PolyData->Reset();
  for (int edgeIdx = 0; edgeIdx < 4; ++edgeIdx)
  {
    this->FarPlaneHandles[edgeIdx].PolyData->Reset();
  }

  // Near plane handle
  vtkPoints* nearPlanePoints = this->NearPlaneEdgesHandle.PolyData->GetPoints();
  frustumPoints->GetPoints(nearPlanePointIndices, nearPlanePoints);

  vtkCellArray* nearPlaneLines = this->NearPlaneEdgesHandle.PolyData->GetLines();
  nearPlaneLines->InsertNextCell({ 0, 1 });
  nearPlaneLines->InsertNextCell({ 1, 2 });
  nearPlaneLines->InsertNextCell({ 2, 3 });
  nearPlaneLines->InsertNextCell({ 3, 0 });

  this->NearPlaneEdgesHandle.PolyData->Modified();

  // Far plane handles
  constexpr std::array<std::array<vtkIdType, 2>, 4> perPlaneIndices = { {
    { 1, 2 }, // right
    { 3, 0 }, // left
    { 0, 1 }, // top
    { 2, 3 }, // bottom
  } };

  for (int edgeIdx = 0; edgeIdx < 4; edgeIdx++)
  {
    vtkPoints* farPlaneHorizontalPoints = this->FarPlaneHandles[edgeIdx].PolyData->GetPoints();
    frustumPoints->GetPoints(farPlanePointIndices, farPlaneHorizontalPoints);

    vtkCellArray* farPlaneHorizontalLines = this->FarPlaneHandles[edgeIdx].PolyData->GetLines();
    farPlaneHorizontalLines->InsertNextCell(2, perPlaneIndices[edgeIdx].data());

    this->FarPlaneHandles[edgeIdx].PolyData->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkImplicitFrustumRepresentation::RegisterPickers()
{
  vtkPickingManager* pm = this->GetPickingManager();
  if (pm == nullptr)
  {
    return;
  }
  pm->AddPicker(this->Picker, this);
}

//------------------------------------------------------------------------------
void vtkImplicitFrustumRepresentation::GetFrustum(vtkFrustum* frustum) const
{
  if (frustum == nullptr)
  {
    return;
  }

  frustum->SetTransform(this->Frustum->GetTransform());
  frustum->SetHorizontalAngle(this->Frustum->GetHorizontalAngle());
  frustum->SetVerticalAngle(this->Frustum->GetVerticalAngle());
  frustum->SetNearPlaneDistance(this->Frustum->GetNearPlaneDistance());
}

VTK_ABI_NAMESPACE_END
