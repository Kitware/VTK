// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCamera3DRepresentation.h"

#include "vtkActor.h"
#include "vtkAssemblyPath.h"
#include "vtkBox.h"
#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkCameraActor.h"
#include "vtkCameraHandleSource.h"
#include "vtkCellArray.h"
#include "vtkCellPicker.h"
#include "vtkConeSource.h"
#include "vtkCubeSource.h"
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
#include "vtkSphereSource.h"
#include "vtkTransform.h"
#include "vtkTransformFilter.h"
#include "vtkVector.h"
#include "vtkWindow.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkCamera3DRepresentation);

namespace
{
double constexpr CAMERA_MINIMUM_VIEW_ANGLE = 5.0;
double constexpr CAMERA_MAXIMUM_VIEW_ANGLE = 170.0;
}

//------------------------------------------------------------------------------
vtkCamera3DRepresentation::vtkCamera3DRepresentation()
{
  this->InteractionState = vtkCamera3DRepresentation::Outside;
  this->HandleSize = 10.0;
  this->ValidPick = 1;

  this->CameraTransform->PostMultiply();
  this->FrontTransform->PostMultiply();
  this->UpTransform->PostMultiply();

  this->Camera = vtkSmartPointer<vtkCamera>::New();
  this->CameraFrustumActor->SetCamera(this->Camera);

  // Set up the initial properties
  this->CreateDefaultProperties();

  // Construct the poly data representing the Camera
  vtkNew<vtkCubeSource> cameraBox;
  vtkNew<vtkConeSource> cameraCone;
  cameraBox->SetXLength(2.0);
  cameraCone->SetCenter(1.0, 0.0, 0.0);
  cameraCone->SetDirection(-1.0, 0.0, 0.0);
  cameraCone->SetRadius(0.375);
  cameraCone->SetResolution(16);
  // Link it to a transform to manipulate it
  vtkNew<vtkTransformFilter> cameraBoxTransformFilter;
  vtkNew<vtkTransformFilter> cameraConeTransformFilter;
  cameraBoxTransformFilter->SetTransform(this->CameraTransform);
  cameraBoxTransformFilter->SetInputConnection(cameraBox->GetOutputPort());
  cameraConeTransformFilter->SetTransform(this->CameraTransform);
  cameraConeTransformFilter->SetInputConnection(cameraCone->GetOutputPort());
  vtkNew<vtkPolyDataMapper> cameraBoxMapper;
  vtkNew<vtkPolyDataMapper> cameraConeMapper;
  cameraBoxMapper->SetInputConnection(cameraBoxTransformFilter->GetOutputPort());
  cameraConeMapper->SetInputConnection(cameraConeTransformFilter->GetOutputPort());
  this->CameraBoxActor->SetMapper(cameraBoxMapper);
  this->CameraConeActor->SetMapper(cameraConeMapper);

  // Create the sphere handles
  std::array<vtkNew<vtkPolyDataMapper>, 3> handleSphereMapper;
  for (std::size_t i = 0; i < this->HandleSphereActor.size(); ++i)
  {
    this->HandleSphereGeometry[i]->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);
    this->HandleSphereGeometry[i]->SetThetaResolution(16);
    this->HandleSphereGeometry[i]->SetPhiResolution(8);
    this->HandleSphereActor[i]->SetMapper(handleSphereMapper[i]);
    this->HandlePicker->AddPickList(this->HandleSphereActor[i]);
  }
  this->HandlePicker->AddPickList(this->CameraBoxActor);
  this->HandlePicker->AddPickList(this->CameraConeActor);
  this->HandlePicker->SetTolerance(0.001);
  this->HandlePicker->PickFromListOn();

  // Link them to transforms to manipulate them
  vtkNew<vtkTransformFilter> frontTransformFilter;
  vtkNew<vtkTransformFilter> upTransformFilter;
  frontTransformFilter->SetTransform(this->FrontTransform);
  upTransformFilter->SetTransform(this->UpTransform);
  frontTransformFilter->SetInputConnection(this->HandleSphereGeometry[1]->GetOutputPort());
  upTransformFilter->SetInputConnection(this->HandleSphereGeometry[2]->GetOutputPort());

  handleSphereMapper[0]->SetInputConnection(this->HandleSphereGeometry[0]->GetOutputPort());
  handleSphereMapper[1]->SetInputConnection(frontTransformFilter->GetOutputPort());
  handleSphereMapper[2]->SetInputConnection(upTransformFilter->GetOutputPort());

  // Create the line handles
  std::array<vtkNew<vtkPolyDataMapper>, 2> handleLineMapper;
  for (std::size_t i = 0; i < this->HandleLineActor.size(); ++i)
  {
    this->HandleLineGeometry[i]->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);
    handleLineMapper[i]->SetInputConnection(this->HandleLineGeometry[i]->GetOutputPort());
    this->HandleLineActor[i]->SetMapper(handleLineMapper[i]);
  }
}

//------------------------------------------------------------------------------
vtkCamera3DRepresentation::~vtkCamera3DRepresentation() = default;

//------------------------------------------------------------------------------
void vtkCamera3DRepresentation::StartWidgetInteraction(double e[2])
{
  this->StartEventPosition[0] = e[0];
  this->StartEventPosition[1] = e[1];
  this->StartEventPosition[2] = 0.0;

  this->LastEventPosition[0] = e[0];
  this->LastEventPosition[1] = e[1];
  this->LastEventPosition[2] = 0.0;
}

//------------------------------------------------------------------------------
void vtkCamera3DRepresentation::WidgetInteraction(double e[2])
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
    case vtkCamera3DRepresentation::Translating:
      this->TranslateAll(prevPickPoint, pickPoint);
      break;
    case vtkCamera3DRepresentation::TranslatingPosition:
      this->TranslatePosition(prevPickPoint, pickPoint);
      break;
    case vtkCamera3DRepresentation::TranslatingNearTarget:
      this->TranslateNearTarget(prevPickPoint, pickPoint);
      break;
    case vtkCamera3DRepresentation::TranslatingTarget:
      this->TranslateTarget(prevPickPoint, pickPoint);
      break;
    case vtkCamera3DRepresentation::TranslatingUp:
      this->TranslateUp(prevPickPoint, pickPoint);
      break;
    case vtkCamera3DRepresentation::Scaling:
      this->Scale(prevPickPoint, pickPoint, static_cast<int>(e[0]), static_cast<int>(e[1]));
      break;
    case vtkCamera3DRepresentation::Outside:
    default:
      break;
  }

  this->LastEventPosition[0] = e[0];
  this->LastEventPosition[1] = e[1];
  this->LastEventPosition[2] = 0.0;
}

//------------------------------------------------------------------------------
void vtkCamera3DRepresentation::GetTranslation(const double p1[4], const double p2[4], double v[3])
{
  if (this->TranslationAxis == Axis::XAxis)
  {
    v[0] = p2[0] - p1[0];
    v[1] = 0.0;
    v[2] = 0.0;
  }

  else if (this->TranslationAxis == Axis::YAxis)
  {
    v[0] = 0.0;
    v[1] = p2[1] - p1[1];
    v[2] = 0.0;
  }

  else if (this->TranslationAxis == Axis::ZAxis)
  {
    v[0] = 0.0;
    v[1] = 0.0;
    v[2] = p2[2] - p1[2];
  }
  else
  {
    v[0] = p2[0] - p1[0];
    v[1] = p2[1] - p1[1];
    v[2] = p2[2] - p1[2];
  }
}

//------------------------------------------------------------------------------
void vtkCamera3DRepresentation::TranslateAll(const double p1[4], const double p2[4])
{
  this->TranslatePosition(p1, p2);
  this->TranslateTarget(p1, p2);
}

//------------------------------------------------------------------------------
void vtkCamera3DRepresentation::TranslatePosition(const double p1[4], const double p2[4])
{
  double translation[3] = { 0.0 };
  this->GetTranslation(p1, p2, translation);

  double position[3];
  this->Camera->GetPosition(position);
  this->Camera->SetPosition(
    position[0] + translation[0], position[1] + translation[1], position[2] + translation[2]);

  this->UpdateGeometry();
}

//------------------------------------------------------------------------------
void vtkCamera3DRepresentation::TranslateNearTarget(const double p1[4], const double p2[4])
{
  double translation[3] = { 0.0 };
  this->GetTranslation(p1, p2, translation);

  double nearTarget[3];
  this->FrontTransform->GetPosition(nearTarget);
  double distance = this->Camera->GetDistance();

  this->Camera->SetFocalPoint(
    nearTarget[0] + translation[0], nearTarget[1] + translation[1], nearTarget[2] + translation[2]);
  this->Camera->SetDistance(distance);

  this->UpdateGeometry();
}

//------------------------------------------------------------------------------
void vtkCamera3DRepresentation::TranslateTarget(const double p1[4], const double p2[4])
{
  double translation[3] = { 0.0 };
  this->GetTranslation(p1, p2, translation);

  double target[3];
  this->Camera->GetFocalPoint(target);
  this->Camera->SetFocalPoint(
    target[0] + translation[0], target[1] + translation[1], target[2] + translation[2]);

  this->UpdateGeometry();
}

//------------------------------------------------------------------------------
void vtkCamera3DRepresentation::TranslateUp(const double p1[4], const double p2[4])
{
  double translation[3] = { 0.0 };
  this->GetTranslation(p1, p2, translation);

  double position[3], upPosition[3];
  this->Camera->GetPosition(position);
  this->UpTransform->GetPosition(upPosition);
  double newUp[3] = { upPosition[0] + translation[0] - position[0],
    upPosition[1] + translation[1] - position[1], upPosition[2] + translation[2] - position[2] };
  vtkMath::Normalize(newUp);
  this->Camera->SetViewUp(newUp);

  this->UpdateGeometry();
}

//------------------------------------------------------------------------------
void vtkCamera3DRepresentation::Scale(
  const double* vtkNotUsed(p1), const double* vtkNotUsed(p2), int vtkNotUsed(X), int Y)
{
  double viewAngle = this->Camera->GetViewAngle();
  double factor = 1.0;
  if (Y > this->LastEventPosition[1] && viewAngle > CAMERA_MINIMUM_VIEW_ANGLE)
  {
    factor = 1.03;
  }
  else if (Y < this->LastEventPosition[1] && viewAngle < CAMERA_MAXIMUM_VIEW_ANGLE)
  {
    factor = 0.97;
  }

  if (factor != 1.0)
  {
    this->Camera->Zoom(factor);
    this->UpdateGeometry();
  }
}

//------------------------------------------------------------------------------
void vtkCamera3DRepresentation::CreateDefaultProperties()
{
  this->HandleProperty->SetColor(1, 1, 1);
  this->SelectedHandleProperty->SetColor(0, 1, 0);
}

//------------------------------------------------------------------------------
void vtkCamera3DRepresentation::PlaceWidget(double bds[6])
{
  if (!this->Camera)
  {
    return;
  }

  double bounds[6], center[3];
  this->AdjustBounds(bds, bounds, center);

  for (int i = 0; i < 6; ++i)
  {
    this->InitialBounds[i] = bounds[i];
  }
  this->InitialLength = sqrt((bounds[1] - bounds[0]) * (bounds[1] - bounds[0]) +
    (bounds[3] - bounds[2]) * (bounds[3] - bounds[2]) +
    (bounds[5] - bounds[4]) * (bounds[5] - bounds[4]));

  double viewAngle = vtkMath::RadiansFromDegrees(this->Camera->GetViewAngle());
  double xLength = bounds[1] - bounds[0];
  double zLength = bounds[5] - bounds[4];
  double distance = std::max(xLength, zLength) / std::tan(viewAngle);

  this->Camera->SetFocalPoint(center);
  if (xLength < zLength)
  {
    this->Camera->SetPosition(center[0] + distance + (xLength / 2.0), center[1], center[2]);
  }
  else
  {
    this->Camera->SetPosition(center[0], center[1], center[2] + distance + (zLength / 2.0));
  }

  this->UpdateGeometry();
}

//------------------------------------------------------------------------------
void vtkCamera3DRepresentation::SetCamera(vtkCamera* camera)
{
  if (this->Camera != camera)
  {
    this->Camera = camera;
    this->CameraFrustumActor->SetCamera(camera);
    this->Modified();
  }
}

//------------------------------------------------------------------------------
vtkCamera* vtkCamera3DRepresentation::GetCamera()
{
  return this->Camera.Get();
}

//------------------------------------------------------------------------------
int vtkCamera3DRepresentation::ComputeInteractionState(int X, int Y, int vtkNotUsed(modify))
{
  if (!this->Renderer || !this->Renderer->IsInViewport(X, Y))
  {
    return this->InteractionState = vtkCamera3DRepresentation::Outside;
  }

  this->CurrentHandle = nullptr;
  vtkAssemblyPath* path = this->GetAssemblyPath(X, Y, 0., this->HandlePicker);

  if (path != nullptr)
  {
    this->ValidPick = 1;
    if (this->TranslatingAll)
    {
      this->CurrentHandle = static_cast<vtkProp*>(this->CameraBoxActor);
      return this->InteractionState = vtkCamera3DRepresentation::Translating;
    }
    else
    {
      this->CurrentHandle = path->GetFirstNode()->GetViewProp();
      if (this->CurrentHandle == this->CameraBoxActor)
      {
        return this->InteractionState = vtkCamera3DRepresentation::TranslatingPosition;
      }
      else if (this->CurrentHandle == this->CameraConeActor)
      {
        return this->InteractionState = vtkCamera3DRepresentation::Scaling;
      }
      else if (this->CurrentHandle == this->HandleSphereActor[0])
      {
        return this->InteractionState = vtkCamera3DRepresentation::TranslatingTarget;
      }
      else if (this->CurrentHandle == this->HandleSphereActor[1])
      {
        return this->InteractionState = vtkCamera3DRepresentation::TranslatingNearTarget;
      }
      else if (this->CurrentHandle == this->HandleSphereActor[2])
      {
        return this->InteractionState = vtkCamera3DRepresentation::TranslatingUp;
      }
    }
  }

  return this->InteractionState = vtkCamera3DRepresentation::Outside;
}

//------------------------------------------------------------------------------
void vtkCamera3DRepresentation::SetFrustumVisibility(bool visible)
{
  if (this->FrustumVisibility != visible)
  {
    this->FrustumVisibility = visible;
    this->CameraFrustumActor->SetVisibility(visible);
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkCamera3DRepresentation::SetSecondaryHandlesVisibility(bool visible)
{
  if (this->SecondaryHandlesVisibility != visible)
  {
    for (const auto& sphereActor : this->HandleSphereActor)
    {
      sphereActor->SetVisibility(visible);
    }
    for (const auto& lineActor : this->HandleLineActor)
    {
      lineActor->SetVisibility(visible);
    }
    this->SecondaryHandlesVisibility = visible;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkCamera3DRepresentation::SetInteractionState(int state)
{
  // Clamp to allowable values
  state = (state < vtkCamera3DRepresentation::Outside
      ? vtkCamera3DRepresentation::Outside
      : (state > vtkCamera3DRepresentation::Scaling ? vtkCamera3DRepresentation::Scaling : state));

  this->InteractionState = state;
  if (state != vtkCamera3DRepresentation::Outside)
  {
    this->HighlightHandle(this->CurrentHandle);
  }
  else
  {
    this->HighlightHandle(nullptr);
  }
}

//------------------------------------------------------------------------------
double* vtkCamera3DRepresentation::GetBounds()
{
  this->BuildRepresentation();
  this->BoundingBox->SetBounds(this->CameraBoxActor->GetBounds());
  this->BoundingBox->AddBounds(this->CameraConeActor->GetBounds());
  if (this->CameraFrustumActor->GetVisibility())
  {
    this->BoundingBox->AddBounds(this->CameraFrustumActor->GetBounds());
  }
  return this->BoundingBox->GetBounds();
}

//------------------------------------------------------------------------------
void vtkCamera3DRepresentation::BuildRepresentation()
{
  // Rebuild only if necessary
  if (this->GetMTime() > this->BuildTime || this->Camera->GetMTime() > this->BuildTime ||
    (this->Renderer && this->Renderer->GetVTKWindow() &&
      (this->Renderer->GetVTKWindow()->GetMTime() > this->BuildTime ||
        this->Renderer->GetActiveCamera()->GetMTime() > this->BuildTime)))
  {
    this->UpdateGeometry();
    this->BuildTime.Modified();
  }
}

//------------------------------------------------------------------------------
void vtkCamera3DRepresentation::ReleaseGraphicsResources(vtkWindow* w)
{
  this->CameraBoxActor->ReleaseGraphicsResources(w);
  this->CameraConeActor->ReleaseGraphicsResources(w);
  this->CameraFrustumActor->ReleaseGraphicsResources(w);

  for (const auto& sphereActor : this->HandleSphereActor)
  {
    sphereActor->ReleaseGraphicsResources(w);
  }
  for (const auto& lineActor : this->HandleLineActor)
  {
    lineActor->ReleaseGraphicsResources(w);
  }
}

//------------------------------------------------------------------------------
int vtkCamera3DRepresentation::RenderOpaqueGeometry(vtkViewport* v)
{
  this->BuildRepresentation();

  int count = this->CameraBoxActor->RenderOpaqueGeometry(v);
  count += this->CameraConeActor->RenderOpaqueGeometry(v);
  count += this->CameraFrustumActor->RenderOpaqueGeometry(v);

  if (this->SecondaryHandlesVisibility)
  {
    for (const auto& sphereActor : this->HandleSphereActor)
    {
      count += sphereActor->RenderOpaqueGeometry(v);
    }
    for (const auto& lineActor : this->HandleLineActor)
    {
      count += lineActor->RenderOpaqueGeometry(v);
    }
  }

  return count;
}

//------------------------------------------------------------------------------
int vtkCamera3DRepresentation::RenderTranslucentPolygonalGeometry(vtkViewport* v)
{
  this->BuildRepresentation();

  int count = this->CameraBoxActor->RenderTranslucentPolygonalGeometry(v);
  count += this->CameraConeActor->RenderTranslucentPolygonalGeometry(v);
  count += this->CameraFrustumActor->RenderTranslucentPolygonalGeometry(v);

  if (this->SecondaryHandlesVisibility)
  {
    for (const auto& sphereActor : this->HandleSphereActor)
    {
      count += sphereActor->RenderTranslucentPolygonalGeometry(v);
    }
    for (const auto& lineActor : this->HandleLineActor)
    {
      count += lineActor->RenderTranslucentPolygonalGeometry(v);
    }
  }

  return count;
}

//------------------------------------------------------------------------------
vtkTypeBool vtkCamera3DRepresentation::HasTranslucentPolygonalGeometry()
{
  this->BuildRepresentation();

  int result = this->CameraBoxActor->HasTranslucentPolygonalGeometry();
  result |= this->CameraConeActor->HasTranslucentPolygonalGeometry();
  result |= this->CameraFrustumActor->HasTranslucentPolygonalGeometry();

  for (const auto& sphereActor : this->HandleSphereActor)
  {
    result |= sphereActor->HasTranslucentPolygonalGeometry();
  }
  for (const auto& lineActor : this->HandleLineActor)
  {
    result |= lineActor->HasTranslucentPolygonalGeometry();
  }

  return result;
}

//------------------------------------------------------------------------------
void vtkCamera3DRepresentation::UpdateGeometry()
{
  // Get needed information
  double position[3], direction[3], target[3];
  this->Camera->GetPosition(position);
  this->Camera->GetDirectionOfProjection(direction);
  this->Camera->GetFocalPoint(target);

  vtkMatrix4x4* matrix = this->Camera->GetModelViewTransformMatrix();
  double up[3];
  up[0] = matrix->GetElement(1, 0);
  up[1] = matrix->GetElement(1, 1);
  up[2] = matrix->GetElement(1, 2);

  // Scaling
  double radius = this->vtkWidgetRepresentation::SizeHandlesInPixels(1.0, position);
  double size = this->vtkWidgetRepresentation::SizeHandlesInPixels(4, position);
  for (const auto& sphere : this->HandleSphereGeometry)
  {
    sphere->SetRadius(radius);
  }

  // Transform applied to camera up representation
  double baseVector[3] = { 1, 0, 0 };
  double rotationAngle = vtkMath::AngleBetweenVectors(baseVector, up);
  double rotationAxis[3];
  vtkMath::Cross(baseVector, up, rotationAxis);
  vtkMath::Normalize(rotationAxis);

  this->UpTransform->Identity();
  this->UpTransform->Translate(this->UpHandleDistance * size, 0, 0);
  this->UpTransform->RotateWXYZ(vtkMath::DegreesFromRadians(rotationAngle), rotationAxis);
  this->UpTransform->Translate(position);

  // Transform applied to camera near target representation
  rotationAngle = vtkMath::AngleBetweenVectors(baseVector, direction);
  vtkMath::Cross(baseVector, direction, rotationAxis);
  vtkMath::Normalize(rotationAxis);

  this->FrontTransform->Identity();
  this->FrontTransform->Translate(this->FrontHandleDistance * size, 0, 0);
  this->FrontTransform->RotateWXYZ(vtkMath::DegreesFromRadians(rotationAngle), rotationAxis);
  this->FrontTransform->Translate(position);

  // Transform applied to camera object representation
  this->CameraTransform->Identity();
  this->CameraTransform->Scale(size, size, size);
  this->CameraTransform->RotateWXYZ(vtkMath::DegreesFromRadians(rotationAngle), rotationAxis);

  double baseVectorUp[3] = { 0.0, 1.0, 0.0 };
  double* transformedVectorUp = this->CameraTransform->TransformDoubleVector(baseVectorUp);
  rotationAngle = vtkMath::AngleBetweenVectors(transformedVectorUp, up);
  vtkMath::Cross(transformedVectorUp, up, rotationAxis);
  vtkMath::Normalize(rotationAxis);

  this->CameraTransform->RotateWXYZ(vtkMath::DegreesFromRadians(rotationAngle), rotationAxis);
  this->CameraTransform->Translate(position);

  // Place target
  this->HandleSphereGeometry[0]->SetCenter(target);

  // Place lines
  this->HandleLineGeometry[0]->SetPoint1(position);
  this->HandleLineGeometry[1]->SetPoint1(position);
  this->HandleLineGeometry[0]->SetPoint2(this->FrontTransform->GetPosition());
  this->HandleLineGeometry[1]->SetPoint2(this->UpTransform->GetPosition());
}

//------------------------------------------------------------------------------
void vtkCamera3DRepresentation::HighlightHandle(vtkProp* prop)
{
  // first unhighlight anything picked
  if (vtkActor* actor = vtkActor::SafeDownCast(this->CurrentHandle))
  {
    actor->SetProperty(this->HandleProperty);
  }

  this->CurrentHandle = prop;

  if (vtkActor* actor = vtkActor::SafeDownCast(this->CurrentHandle))
  {
    actor->SetProperty(this->SelectedHandleProperty);
  }
}

//------------------------------------------------------------------------------
void vtkCamera3DRepresentation::RegisterPickers()
{
  vtkPickingManager* pm = this->GetPickingManager();
  if (!pm)
  {
    return;
  }
  pm->AddPicker(this->HandlePicker, this);
}

//------------------------------------------------------------------------------
void vtkCamera3DRepresentation::GetActors(vtkPropCollection* pc)
{
  if (pc != nullptr && this->GetVisibility())
  {
    this->CameraBoxActor->GetActors(pc);
    this->CameraConeActor->GetActors(pc);
    this->CameraFrustumActor->GetActors(pc);
    for (const auto& sphereActor : this->HandleSphereActor)
    {
      sphereActor->GetActors(pc);
    }
    for (const auto& lineActor : this->HandleLineActor)
    {
      lineActor->GetActors(pc);
    }
  }
  this->Superclass::GetActors(pc);
}

//------------------------------------------------------------------------------
void vtkCamera3DRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Initial Bounds: "
     << "(" << this->InitialBounds[0] << "," << this->InitialBounds[1] << ") "
     << "(" << this->InitialBounds[2] << "," << this->InitialBounds[3] << ") "
     << "(" << this->InitialBounds[4] << "," << this->InitialBounds[5] << ")\n";
  double* bounds = this->BoundingBox->GetBounds();
  os << indent << "Bounding Box: "
     << "(" << bounds[0] << "," << bounds[1] << ") "
     << "(" << bounds[2] << "," << bounds[3] << ") "
     << "(" << bounds[4] << "," << bounds[5] << ")\n";
  os << indent << "Translating All Enabled: " << (this->TranslatingAll ? "On\n" : "Off\n");
  os << indent << "Translation Axis: " << this->TranslationAxis << "\n";
  os << indent << "Front Handle Distance: " << this->FrontHandleDistance << "\n";
  os << indent << "Up Handle Distance: " << this->UpHandleDistance << "\n";
  os << indent << "Frustum Visibility: " << (this->FrustumVisibility ? "On\n" : "Off\n");
  os << indent
     << "Secondary Handles Visibility: " << (this->SecondaryHandlesVisibility ? "On\n" : "Off\n");
  os << indent << "Camera:\n";
  this->Camera->PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
