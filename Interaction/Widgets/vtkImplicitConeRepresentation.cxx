// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkImplicitConeRepresentation.h"

#include "vtkActor.h"
#include "vtkAssemblyPath.h"
#include "vtkBoundingBox.h"
#include "vtkBox.h"
#include "vtkCamera.h"
#include "vtkCell.h"
#include "vtkCellArray.h"
#include "vtkCellPicker.h"
#include "vtkCone.h"
#include "vtkConeSource.h"
#include "vtkImageData.h"
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
#include "vtkType.h"
#include "vtkVector.h"

#include <algorithm>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkImplicitConeRepresentation);

//------------------------------------------------------------------------------
vtkImplicitConeRepresentation::vtkImplicitConeRepresentation()
{
  this->InteractionState = InteractionStateType::Outside;

  // This class represents a one-sided cone only
  this->Cone->IsDoubleConeOff();

  // Handle size is in pixels for this widget
  this->HandleSize = 5.0;

  vtkNew<vtkPoints> pts;
  pts->SetDataTypeToDouble();
  this->ConePD->SetPoints(pts);

  vtkNew<vtkCellArray> polys;
  this->ConePD->SetPolys(polys);

  this->ConePDMapper->SetInputData(this->ConePD);
  this->ConePDActor->SetMapper(this->ConePDMapper);

  vtkNew<vtkPoints> edgePoints;
  edgePoints->SetDataTypeToDouble();
  this->EdgesPD->SetPoints(edgePoints);

  vtkNew<vtkCellArray> edgeLines;
  this->EdgesPD->SetLines(edgeLines);

  this->EdgesTuber->SetInputData(this->EdgesPD);
  this->EdgesTuber->SetNumberOfSides(12);
  this->EdgesMapper->SetInputConnection(this->EdgesTuber->GetOutputPort());
  this->EdgesActor->SetMapper(this->EdgesMapper);
  // The feature edges or tuber turns on scalar viz - we need it off.
  this->EdgesMapper->ScalarVisibilityOff();

  // Create the axis
  this->AxisLineSource->SetResolution(1);
  this->AxisLineMapper->SetInputConnection(this->AxisLineSource->GetOutputPort());
  this->AxisLineActor->SetMapper(this->AxisLineMapper);

  // Create the axis arrow
  this->AxisArrowSource->SetResolution(12);
  this->AxisArrowSource->SetAngle(25.0);
  this->AxisArrowMapper->SetInputConnection(this->AxisArrowSource->GetOutputPort());
  this->AxisArrowActor->SetMapper(this->AxisArrowMapper);

  // Create the origin handle
  this->OriginHandleSource->SetThetaResolution(16);
  this->OriginHandleSource->SetPhiResolution(16);
  this->OriginHandleMapper->SetInputConnection(this->OriginHandleSource->GetOutputPort());
  this->OriginHandleActor->SetMapper(this->OriginHandleMapper);

  // Define the point coordinates
  std::array<double, 6> bounds = {
    -0.5,
    0.5,
    -0.5,
    0.5,
    -0.5,
    0.5,
  };

  // Initial creation of the widget, serves to initialize it
  this->PlaceWidget(bounds.data());

  // Manage the picking stuff
  this->Picker->SetTolerance(0.005);
  this->Picker->AddPickList(this->AxisLineActor);
  this->Picker->AddPickList(this->AxisArrowActor);
  this->Picker->AddPickList(this->OriginHandleActor);
  this->Picker->AddPickList(this->GetOutlineActor());
  this->Picker->PickFromListOn();

  this->ConePicker->SetTolerance(0.005);
  this->ConePicker->AddPickList(this->ConePDActor);
  this->ConePicker->AddPickList(this->EdgesActor);
  this->ConePicker->PickFromListOn();

  // Set up the initial properties
  // Cone properties
  this->ConeProperty->SetAmbient(1.0);
  this->ConeProperty->SetAmbientColor(1.0, 1.0, 1.0);
  this->ConeProperty->SetOpacity(0.5);
  this->ConePDActor->SetProperty(this->ConeProperty);

  this->SelectedConeProperty->SetAmbient(1.0);
  this->SelectedConeProperty->SetAmbientColor(0.0, 1.0, 0.0);
  this->SelectedConeProperty->SetOpacity(0.25);

  // Cone axis properties
  this->AxisProperty->SetAmbient(1.0);
  this->AxisProperty->SetColor(1, 0, 0);
  this->AxisProperty->SetLineWidth(2);

  this->SelectedAxisProperty->SetAmbient(1.0);
  this->SelectedAxisProperty->SetColor(0, 1, 0);
  this->SelectedAxisProperty->SetLineWidth(2);

  // Origin handle properties
  this->OriginHandleProperty->SetAmbient(1.0);
  this->OriginHandleProperty->SetColor(1, 0, 0);

  this->SelectedOriginHandleProperty->SetAmbient(1.0);
  this->SelectedOriginHandleProperty->SetColor(0, 1, 0);

  // Edge property
  this->EdgesProperty->SetColor(1.0, 0.0, 0.0);

  // Pass the initial properties to the actors.
  this->AxisLineActor->SetProperty(this->AxisProperty);
  this->AxisArrowActor->SetProperty(this->AxisProperty);
  this->OriginHandleActor->SetProperty(this->OriginHandleProperty);
  this->ConePDActor->SetProperty(this->ConeProperty);
  this->EdgesActor->SetProperty(this->EdgesProperty);
}

//------------------------------------------------------------------------------
vtkImplicitConeRepresentation::~vtkImplicitConeRepresentation() = default;

//------------------------------------------------------------------------------
int vtkImplicitConeRepresentation::ComputeInteractionState(int X, int Y, int vtkNotUsed(modify))
{
  // See if anything has been selected
  vtkAssemblyPath* path = this->GetAssemblyPath(X, Y, 0., this->Picker);

  // The second picker may need to be called. This is done because the cone
  // wraps around things that can be picked; thus the cone is the selection
  // of last resort.
  if (path == nullptr)
  {
    this->ConePicker->Pick(X, Y, 0., this->Renderer);
    path = this->ConePicker->GetPath();
  }

  if (path == nullptr) // Nothing picked
  {
    this->SetRepresentationState(vtkImplicitConeRepresentation::Outside);
    this->InteractionState = vtkImplicitConeRepresentation::Outside;
    return this->InteractionState;
  }

  // Something picked, continue
  this->ValidPick = true;

  // Depending on the interaction state (set by the widget) we modify
  // the state of the representation based on what is picked.
  if (this->InteractionState == vtkImplicitConeRepresentation::Moving)
  {
    vtkProp* prop = path->GetFirstNode()->GetViewProp();
    if (prop == this->AxisLineActor || prop == this->AxisArrowActor)
    {
      this->InteractionState = vtkImplicitConeRepresentation::RotatingAxis;
      this->SetRepresentationState(vtkImplicitConeRepresentation::RotatingAxis);
    }
    else if (prop == this->ConePDActor || prop == EdgesActor)
    {
      this->InteractionState = vtkImplicitConeRepresentation::AdjustingAngle;
      this->SetRepresentationState(vtkImplicitConeRepresentation::AdjustingAngle);
    }
    else if (prop == this->OriginHandleActor)
    {
      this->InteractionState = vtkImplicitConeRepresentation::MovingOrigin;
      this->SetRepresentationState(vtkImplicitConeRepresentation::MovingOrigin);
    }
    else
    {
      if (this->GetOutlineTranslation())
      {
        this->InteractionState = vtkImplicitConeRepresentation::MovingOutline;
        this->SetRepresentationState(vtkImplicitConeRepresentation::MovingOutline);
      }
      else
      {
        this->InteractionState = vtkImplicitConeRepresentation::Outside;
        this->SetRepresentationState(vtkImplicitConeRepresentation::Outside);
      }
    }
  }

  // We may add a condition to allow the camera to work IO scaling
  else if (this->InteractionState != vtkImplicitConeRepresentation::Scaling)
  {
    this->InteractionState = vtkImplicitConeRepresentation::Outside;
  }

  return this->InteractionState;
}

//------------------------------------------------------------------------------
void vtkImplicitConeRepresentation::SetRepresentationState(InteractionStateType state)
{
  // Clamp the state
  state = std::min(
    std::max(state, InteractionStateType::Outside), InteractionStateType::TranslatingOrigin);

  if (this->RepresentationState == state)
  {
    return;
  }

  this->RepresentationState = state;
  this->Modified();

  this->HighlightAxis(false);
  this->HighlightCone(false);
  this->HighlightOutline(false);
  this->HighlightOriginHandle(false);

  switch (state)
  {
    case vtkImplicitConeRepresentation::RotatingAxis:
      this->HighlightAxis(true);
      break;

    case vtkImplicitConeRepresentation::AdjustingAngle:
      this->HighlightCone(true);
      break;

    case vtkImplicitConeRepresentation::TranslatingOrigin:
    case vtkImplicitConeRepresentation::MovingOrigin:
      this->HighlightOriginHandle(true);
      break;

    case vtkImplicitConeRepresentation::MovingOutline:
      this->HighlightOutline(true);
      break;

    case vtkImplicitConeRepresentation::Scaling:
      if (this->ScaleEnabled)
      {
        this->HighlightAxis(true);
        this->HighlightCone(true);
        this->HighlightOutline(true);
        this->HighlightOriginHandle(true);
      }
      break;

    default:
      break;
  }
}

//------------------------------------------------------------------------------
void vtkImplicitConeRepresentation::StartWidgetInteraction(double e[2])
{
  this->StartEventPosition[0] = e[0];
  this->StartEventPosition[1] = e[1];
  this->StartEventPosition[2] = 0.0;

  this->LastEventPosition[0] = e[0];
  this->LastEventPosition[1] = e[1];
  this->LastEventPosition[2] = 0.0;
}

//------------------------------------------------------------------------------
void vtkImplicitConeRepresentation::WidgetInteraction(double e[2])
{
  vtkCamera* camera = this->Renderer->GetActiveCamera();
  if (!camera)
  {
    return;
  }

  // Do different things depending on state
  // Calculations everybody does
  // Compute the two points defining the motion vector
  vtkVector3d prevPickPoint = this->GetWorldPoint(this->Picker, this->LastEventPosition.GetData());
  vtkVector3d pickPoint = this->GetWorldPoint(this->Picker, e);

  vtkVector3d prevConePickPoint =
    this->GetWorldPoint(this->ConePicker, this->LastEventPosition.GetData());
  vtkVector3d pickConePoint = this->GetWorldPoint(this->ConePicker, e);

  // Process the motion
  switch (this->InteractionState)
  {
    case InteractionStateType::MovingOutline:
      this->TranslateOutline(prevPickPoint.GetData(), pickPoint.GetData());
      break;

    case InteractionStateType::MovingOrigin:
      this->TranslateOrigin(prevPickPoint, pickPoint);
      break;

    case InteractionStateType::TranslatingOrigin:
      this->TranslateOriginOnAxis(prevPickPoint, pickPoint);
      break;

    case InteractionStateType::AdjustingAngle:
      this->AdjustAngle(e[0], e[1], prevConePickPoint, pickConePoint);
      break;

    case InteractionStateType::Scaling:
      if (this->ScaleEnabled)
      {
        this->Scale(prevPickPoint, pickPoint, e[0], e[1]);
      }
      break;

    case InteractionStateType::RotatingAxis:
      vtkVector3d vpn;
      camera->GetViewPlaneNormal(vpn.GetData());
      this->Rotate(e[0], e[1], prevPickPoint, pickPoint, vpn);
      break;
  }

  this->LastEventPosition[0] = e[0];
  this->LastEventPosition[1] = e[1];
  this->LastEventPosition[2] = 0.0;

  this->BuildRepresentation();
}

//------------------------------------------------------------------------------
void vtkImplicitConeRepresentation::EndWidgetInteraction(double vtkNotUsed(e)[2])
{
  this->SetRepresentationState(InteractionStateType::Outside);
}

//------------------------------------------------------------------------------
double* vtkImplicitConeRepresentation::GetBounds()
{
  this->BuildRepresentation();
  this->BoundingBox->SetBounds(this->GetOutlineActor()->GetBounds());
  this->BoundingBox->AddBounds(this->ConePDActor->GetBounds());
  this->BoundingBox->AddBounds(this->EdgesActor->GetBounds());
  this->BoundingBox->AddBounds(this->AxisLineActor->GetBounds());
  this->BoundingBox->AddBounds(this->AxisArrowActor->GetBounds());
  this->BoundingBox->AddBounds(this->OriginHandleActor->GetBounds());

  return this->BoundingBox->GetBounds();
}

//------------------------------------------------------------------------------
void vtkImplicitConeRepresentation::GetActors(vtkPropCollection* pc)
{
  this->GetOutlineActor()->GetActors(pc);
  this->ConePDActor->GetActors(pc);
  this->EdgesActor->GetActors(pc);
  this->AxisLineActor->GetActors(pc);
  this->AxisArrowActor->GetActors(pc);
  this->OriginHandleActor->GetActors(pc);
}

//------------------------------------------------------------------------------
void vtkImplicitConeRepresentation::ReleaseGraphicsResources(vtkWindow* w)
{
  this->GetOutlineActor()->ReleaseGraphicsResources(w);
  this->ConePDActor->ReleaseGraphicsResources(w);
  this->EdgesActor->ReleaseGraphicsResources(w);
  this->AxisLineActor->ReleaseGraphicsResources(w);
  this->AxisArrowActor->ReleaseGraphicsResources(w);
  this->OriginHandleActor->ReleaseGraphicsResources(w);
}

//------------------------------------------------------------------------------
int vtkImplicitConeRepresentation::RenderOpaqueGeometry(vtkViewport* v)
{
  int count = 0;
  this->BuildRepresentation();
  count += this->GetOutlineActor()->RenderOpaqueGeometry(v);
  count += this->EdgesActor->RenderOpaqueGeometry(v);
  count += this->AxisLineActor->RenderOpaqueGeometry(v);
  count += this->AxisArrowActor->RenderOpaqueGeometry(v);
  count += this->OriginHandleActor->RenderOpaqueGeometry(v);

  if (this->DrawCone)
  {
    count += this->ConePDActor->RenderOpaqueGeometry(v);
  }

  return count;
}

//------------------------------------------------------------------------------
int vtkImplicitConeRepresentation::RenderTranslucentPolygonalGeometry(vtkViewport* v)
{
  int count = 0;
  this->BuildRepresentation();
  count += this->GetOutlineActor()->RenderTranslucentPolygonalGeometry(v);
  count += this->EdgesActor->RenderTranslucentPolygonalGeometry(v);
  count += this->AxisLineActor->RenderTranslucentPolygonalGeometry(v);
  count += this->AxisArrowActor->RenderTranslucentPolygonalGeometry(v);
  count += this->OriginHandleActor->RenderTranslucentPolygonalGeometry(v);

  if (this->DrawCone)
  {
    count += this->ConePDActor->RenderTranslucentPolygonalGeometry(v);
  }

  return count;
}

//------------------------------------------------------------------------------
vtkTypeBool vtkImplicitConeRepresentation::HasTranslucentPolygonalGeometry()
{
  int result = 0;
  result |= this->GetOutlineActor()->HasTranslucentPolygonalGeometry();
  result |= this->EdgesActor->HasTranslucentPolygonalGeometry();
  result |= this->AxisLineActor->HasTranslucentPolygonalGeometry();
  result |= this->AxisArrowActor->HasTranslucentPolygonalGeometry();
  result |= this->OriginHandleActor->HasTranslucentPolygonalGeometry();

  if (this->DrawCone)
  {
    result |= this->ConePDActor->HasTranslucentPolygonalGeometry();
  }

  return result;
}

//------------------------------------------------------------------------------
void vtkImplicitConeRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Resolution: " << this->Resolution << std::endl;

  if (this->AxisProperty)
  {
    os << indent << "Axis Property: " << this->AxisProperty << std::endl;
  }
  else
  {
    os << indent << "Axis Property: (none)" << std::endl;
  }
  if (this->SelectedAxisProperty)
  {
    os << indent << "Selected Axis Property: " << this->SelectedAxisProperty << std::endl;
  }
  else
  {
    os << indent << "Selected Axis Property: (none)" << std::endl;
  }

  if (this->ConeProperty)
  {
    os << indent << "Cone Property: " << this->ConeProperty << std::endl;
  }
  else
  {
    os << indent << "Cone Property: (none)" << std::endl;
  }
  if (this->SelectedConeProperty)
  {
    os << indent << "Selected Cone Property: " << this->SelectedConeProperty << std::endl;
  }
  else
  {
    os << indent << "Selected Cone Property: (none)" << std::endl;
  }

  if (this->EdgesProperty)
  {
    os << indent << "Edges Property: " << this->EdgesProperty << std::endl;
  }
  else
  {
    os << indent << "Edges Property: (none)" << std::endl;
  }

  os << indent << "Along X Axis: " << (this->AlongXAxis ? "On" : "Off") << std::endl;
  os << indent << "Along Y Axis: " << (this->AlongYAxis ? "On" : "Off") << std::endl;
  os << indent << "ALong Z Axis: " << (this->AlongZAxis ? "On" : "Off") << std::endl;

  os << indent << "Tubing: " << (this->Tubing ? "On" : "Off") << std::endl;
  os << indent << "Scale Enabled: " << (this->ScaleEnabled ? "On" : "Off") << std::endl;
  os << indent << "Draw Cone: " << (this->DrawCone ? "On" : "Off") << std::endl;
  os << indent << "Bump Distance: " << this->BumpDistance << std::endl;

  os << indent << "Representation State: ";
  switch (this->RepresentationState)
  {
    case Outside:
      os << "Outside" << std::endl;
      break;
    case Moving:
      os << "Moving" << std::endl;
      break;
    case MovingOutline:
      os << "MovingOutline" << std::endl;
      break;
    case MovingOrigin:
      os << "MovingOrigin" << std::endl;
      break;
    case RotatingAxis:
      os << "RotatingAxis" << std::endl;
      break;
    case AdjustingAngle:
      os << "AdjustingAngle" << std::endl;
      break;
    case Scaling:
      os << "Scaling" << std::endl;
      break;
    case TranslatingOrigin:
      os << "TranslatingOrigin" << std::endl;
      break;
  }
}

//------------------------------------------------------------------------------
void vtkImplicitConeRepresentation::HighlightAxis(bool highlight)
{
  if (highlight)
  {
    this->AxisLineActor->SetProperty(this->SelectedAxisProperty);
    this->AxisArrowActor->SetProperty(this->SelectedAxisProperty);
  }
  else
  {
    this->AxisLineActor->SetProperty(this->AxisProperty);
    this->AxisArrowActor->SetProperty(this->AxisProperty);
  }
}

void vtkImplicitConeRepresentation::HighlightOriginHandle(bool highlight)
{
  if (highlight)
  {
    this->OriginHandleActor->SetProperty(this->SelectedOriginHandleProperty);
  }
  else
  {
    this->OriginHandleActor->SetProperty(this->OriginHandleProperty);
  }
}

//------------------------------------------------------------------------------
void vtkImplicitConeRepresentation::HighlightCone(bool highlight)
{
  if (highlight)
  {
    this->ConePDActor->SetProperty(this->SelectedConeProperty);
    this->EdgesActor->SetProperty(this->SelectedConeProperty);
  }
  else
  {
    this->ConePDActor->SetProperty(this->ConeProperty);
    this->EdgesActor->SetProperty(this->EdgesProperty);
  }
}

//------------------------------------------------------------------------------
void vtkImplicitConeRepresentation::Rotate(
  double X, double Y, const vtkVector3d& p1, const vtkVector3d& p2, const vtkVector3d& vpn)
{
  vtkVector3d v = p2 - p1;         // mouse motion vector in world space
  vtkVector3d axis = vpn.Cross(v); // axis of rotation

  vtkVector3d origin(this->Cone->GetOrigin());
  vtkVector3d coneAxis(this->Cone->GetAxis());

  if (axis.Norm() == 0.0)
  {
    return;
  }
  const int* size = this->Renderer->GetSize();
  double l2 = (X - this->LastEventPosition[0]) * (X - this->LastEventPosition[0]) +
    (Y - this->LastEventPosition[1]) * (Y - this->LastEventPosition[1]);
  double theta = 360.0 * sqrt(l2 / (size[0] * size[0] + size[1] * size[1]));

  // Manipulate the transform to reflect the rotation
  vtkNew<vtkTransform> transform;
  transform->Identity();
  transform->Translate(origin.GetData());
  transform->RotateWXYZ(theta, axis.GetData());
  transform->Translate((-origin).GetData());

  // Set the new normal
  vtkVector3d aNew;
  transform->TransformNormal(coneAxis.GetData(), aNew.GetData());
  this->Cone->SetAxis(aNew.GetData());
}

//------------------------------------------------------------------------------
void vtkImplicitConeRepresentation::TranslateRepresentation(const vtkVector3d& motion)
{
  vtkVector3d coneOrigin(this->Cone->GetOrigin());
  vtkVector3d newConeOrigin = coneOrigin + motion;
  this->Cone->SetOrigin(newConeOrigin.GetData());
}

//------------------------------------------------------------------------------
// Loop through all points and translate them
void vtkImplicitConeRepresentation::TranslateOrigin(const vtkVector3d& p1, const vtkVector3d& p2)
{
  vtkCamera* camera = this->Renderer->GetActiveCamera();
  if (!camera)
  {
    return;
  }

  // Get the motion vector
  vtkVector3d v = { 0, 0, 0 };

  if (!this->IsTranslationConstrained())
  {
    v = vtkVector3d(p2) - vtkVector3d(p1);
  }
  else
  {
    v[this->GetTranslationAxis()] = p2[this->GetTranslationAxis()] - p1[this->GetTranslationAxis()];
  }

  // Translate the current origin
  vtkVector3d origin(this->Cone->GetOrigin());
  vtkVector3d newOrigin = origin + v;

  // Project back onto plane orthogonal to camera
  vtkVector3d vpn;
  camera->GetViewPlaneNormal(vpn.GetData());

  vtkPlane::ProjectPoint(newOrigin.GetData(), origin.GetData(), vpn.GetData(), newOrigin.GetData());

  this->Cone->SetOrigin(newOrigin.GetData());
}

//------------------------------------------------------------------------------
// Translate the origin on the axis
void vtkImplicitConeRepresentation::TranslateOriginOnAxis(
  const vtkVector3d& p1, const vtkVector3d& p2)
{
  // Get the motion vector
  vtkVector3d v = p2 - p1;

  // Add to the current point, project back down onto plane
  vtkVector3d origin(this->Cone->GetOrigin());
  vtkVector3d axis(this->Cone->GetAxis());
  vtkVector3d newOrigin = origin + v;

  // Normalize the axis vector
  axis.Normalize();

  // Project the point on the axis vector
  vtkVector3d u = newOrigin - origin;
  newOrigin = origin + (axis * axis.Dot(u));
  this->Cone->SetOrigin(newOrigin.GetData());
}

//------------------------------------------------------------------------------
void vtkImplicitConeRepresentation::Scale(
  const vtkVector3d& p1, const vtkVector3d& p2, double vtkNotUsed(X), double Y)
{
  // Get the motion vector
  vtkVector3d v = p2 - p1;

  vtkVector3d coneOrigin(this->Cone->GetOrigin());

  // Compute the scale factor
  double diagonal = this->GetDiagonalLength();
  if (diagonal == 0.)
  {
    return;
  }
  double sf = v.Norm() / diagonal;
  if (Y > this->LastEventPosition[1])
  {
    sf = 1.0 + sf;
  }
  else
  {
    sf = 1.0 - sf;
  }

  vtkNew<vtkTransform> transform;
  transform->Identity();
  transform->Translate(coneOrigin.GetData());
  transform->Scale(sf, sf, sf);
  transform->Translate((-coneOrigin).GetData());

  this->TransformBounds(transform);
}

//------------------------------------------------------------------------------
void vtkImplicitConeRepresentation::AdjustAngle(
  double X, double Y, const vtkVector3d& p1, const vtkVector3d& p2)
{
  if (X == this->LastEventPosition[0] && Y == this->LastEventPosition[1])
  {
    return;
  }

  const double prevAngle = vtkMath::RadiansFromDegrees(this->Cone->GetAngle());
  const double prevCos = std::cos(prevAngle);

  vtkVector3d origin;
  this->Cone->GetOrigin(origin.GetData());

  vtkVector3d conePoint1 = p1 - origin;
  const double lenght1 = std::sqrt(conePoint1.Dot(conePoint1));
  vtkVector3d conePoint2 = p2 - origin;
  const double lenght2 = std::sqrt(conePoint2.Dot(conePoint2));

  if (lenght2 > 0)
  {
    const double newCos = std::min(prevCos * lenght1 / lenght2, 1.);
    const double newAngle = vtkMath::DegreesFromRadians(std::acos(newCos));
    this->SetAngle(newAngle);
  }
}

//------------------------------------------------------------------------------
void vtkImplicitConeRepresentation::SetInteractionColor(double r, double g, double b)
{
  this->SelectedAxisProperty->SetColor(r, g, b);
  this->SelectedConeProperty->SetAmbientColor(r, g, b);
  this->SetSelectedOutlineColor(r, g, b);
}

//------------------------------------------------------------------------------
void vtkImplicitConeRepresentation::SetHandleColor(double r, double g, double b)
{
  this->AxisProperty->SetColor(r, g, b);
}

//------------------------------------------------------------------------------
void vtkImplicitConeRepresentation::SetForegroundColor(double r, double g, double b)
{
  this->ConeProperty->SetAmbientColor(r, g, b);
  this->SetOutlineColor(r, g, b);
}

//------------------------------------------------------------------------------
void vtkImplicitConeRepresentation::PlaceWidget(double bds[6])
{
  std::array<double, 6> bounds;
  vtkVector3d origin;
  this->AdjustBounds(bds, bounds.data(), origin.GetData());
  this->SetOutlineBounds(bounds.data());

  this->AxisLineSource->SetPoint1(this->Cone->GetOrigin());
  if (this->AlongYAxis)
  {
    this->Cone->SetAxis(0, 1, 0);
    this->AxisLineSource->SetPoint2(0, 1, 0);
  }
  else if (this->AlongZAxis)
  {
    this->Cone->SetAxis(0, 0, 1);
    this->AxisLineSource->SetPoint2(0, 0, 1);
  }
  else // default or x-normal
  {
    this->Cone->SetAxis(1, 0, 0);
    this->AxisLineSource->SetPoint2(1, 0, 0);
  }

  for (int i = 0; i < 6; i++)
  {
    this->InitialBounds[i] = bounds[i];
  }
  this->SetWidgetBounds(bounds.data());

  this->InitialLength = sqrt((bounds[1] - bounds[0]) * (bounds[1] - bounds[0]) +
    (bounds[3] - bounds[2]) * (bounds[3] - bounds[2]) +
    (bounds[5] - bounds[4]) * (bounds[5] - bounds[4]));

  this->ValidPick = true; // since we have positioned the widget successfully
  this->BuildRepresentation();
}

//------------------------------------------------------------------------------
// Set the origin of the cone.
void vtkImplicitConeRepresentation::SetOrigin(double x, double y, double z)
{
  this->Cone->SetOrigin(x, y, z);
}

//------------------------------------------------------------------------------
// Set the origin of the cone. Note that the origin is clamped slightly inside
// the bounding box or the cone tends to disappear as it hits the boundary.
void vtkImplicitConeRepresentation::SetOrigin(double x[3])
{
  this->SetOrigin(x[1], x[2], x[3]);
}

//------------------------------------------------------------------------------
// Get the origin of the cone.
double* vtkImplicitConeRepresentation::GetOrigin()
{
  return this->Cone->GetOrigin();
}

//------------------------------------------------------------------------------
void vtkImplicitConeRepresentation::GetOrigin(double xyz[3])
{
  double* origin = this->Cone->GetOrigin();
  xyz[0] = origin[0];
  xyz[1] = origin[1];
  xyz[2] = origin[2];
}

//------------------------------------------------------------------------------
// Set the axis of the cone.
void vtkImplicitConeRepresentation::SetAxis(double x, double y, double z)
{
  vtkVector3d n(x, y, z);
  n.Normalize();

  vtkVector3d currentAxis(this->Cone->GetAxis());
  if (n != currentAxis)
  {
    this->Cone->SetAxis(n.GetData());
    this->Modified();
  }
}

//------------------------------------------------------------------------------
// Set the axis the cone.
void vtkImplicitConeRepresentation::SetAxis(double n[3])
{
  this->SetAxis(n[0], n[1], n[2]);
}

//------------------------------------------------------------------------------
// Get the axis of the cone.
double* vtkImplicitConeRepresentation::GetAxis()
{
  return this->Cone->GetAxis();
}

//------------------------------------------------------------------------------
void vtkImplicitConeRepresentation::GetAxis(double xyz[3])
{
  this->Cone->GetAxis(xyz);
}

//------------------------------------------------------------------------------
// Set the cone angle. Angle must be a positive number.
void vtkImplicitConeRepresentation::SetAngle(double angle)
{
  this->Cone->SetAngle(angle);
}

//------------------------------------------------------------------------------
// Get the angle the cone.
double vtkImplicitConeRepresentation::GetAngle()
{
  return this->Cone->GetAngle();
}

//------------------------------------------------------------------------------
void vtkImplicitConeRepresentation::SetDrawCone(bool drawCone)
{
  if (drawCone == this->DrawCone)
  {
    return;
  }

  this->Modified();
  this->DrawCone = drawCone;
}

//------------------------------------------------------------------------------
void vtkImplicitConeRepresentation::SetAlongXAxis(bool var)
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
void vtkImplicitConeRepresentation::SetAlongYAxis(bool var)
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
void vtkImplicitConeRepresentation::SetAlongZAxis(bool var)
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
void vtkImplicitConeRepresentation::GetPolyData(vtkPolyData* pd)
{
  pd->ShallowCopy(this->ConePD);
}

//------------------------------------------------------------------------------
void vtkImplicitConeRepresentation::UpdatePlacement()
{
  this->BuildRepresentation();
  this->UpdateOutline();
}

//------------------------------------------------------------------------------
void vtkImplicitConeRepresentation::BumpCone(int dir, double factor)
{
  // Compute the distance
  double d = this->InitialLength * this->BumpDistance * factor;

  // Push the cone
  this->PushCone((dir > 0 ? d : -d));
}

//------------------------------------------------------------------------------
void vtkImplicitConeRepresentation::PushCone(double d)
{
  vtkCamera* camera = this->Renderer->GetActiveCamera();
  if (!camera)
  {
    return;
  }

  vtkVector3d vpn, origin;
  camera->GetViewPlaneNormal(vpn.GetData());
  this->Cone->GetOrigin(origin.GetData());

  origin += (d * vpn);

  this->Cone->SetOrigin(origin.GetData());

  this->BuildRepresentation();
}

//------------------------------------------------------------------------------
void vtkImplicitConeRepresentation::BuildRepresentation()
{
  if (!this->Renderer || !this->Renderer->GetRenderWindow())
  {
    return;
  }

  if (this->GetMTime() > this->BuildTime || this->Cone->GetMTime() > this->BuildTime ||
    this->Renderer->GetRenderWindow()->GetMTime() > this->BuildTime)
  {

    vtkInformation* info = this->GetPropertyKeys();
    this->GetOutlineActor()->SetPropertyKeys(info);
    this->ConePDActor->SetPropertyKeys(info);
    this->EdgesActor->SetPropertyKeys(info);
    this->AxisLineActor->SetPropertyKeys(info);
    this->AxisArrowActor->SetPropertyKeys(info);
    this->OriginHandleActor->SetPropertyKeys(info);

    vtkVector3d origin(this->Cone->GetOrigin());
    vtkVector3d axis(this->Cone->GetAxis());

    this->UpdateCenterAndBounds(origin.GetData());

    // Update the adjusted origin
    this->Cone->SetOrigin(origin.GetData());

    // Setup the cone axis
    double d = this->GetDiagonalLength();

    vtkVector3d p2 = origin + (axis * (0.30 * d));

    this->AxisLineSource->SetPoint1(origin.GetData());
    this->AxisLineSource->SetPoint2(p2.GetData());
    this->AxisArrowSource->SetCenter(p2.GetData());
    this->AxisArrowSource->SetDirection(axis.GetData());

    // Set up the position handle
    this->OriginHandleSource->SetCenter(origin.GetData());

    // Control the look of the edges
    if (this->Tubing)
    {
      this->EdgesMapper->SetInputConnection(this->EdgesTuber->GetOutputPort());
    }
    else
    {
      this->EdgesMapper->SetInputData(this->EdgesPD);
    }

    // Construct intersected cone
    this->BuildCone();

    this->SizeHandles();
    this->BuildTime.Modified();
  }
}

//------------------------------------------------------------------------------
void vtkImplicitConeRepresentation::SizeHandles()
{
  double radius =
    this->vtkWidgetRepresentation::SizeHandlesInPixels(1.5, this->OriginHandleSource->GetCenter());

  this->AxisArrowSource->SetHeight(2.0 * radius);
  this->AxisArrowSource->SetRadius(radius);

  this->OriginHandleSource->SetRadius(radius);

  this->EdgesTuber->SetRadius(0.25 * radius);
}

//------------------------------------------------------------------------------
// Create the cone polydata. Basically build an oriented cone of
// specified resolution. Clamp cone facets by performing
// intersection tests.
void vtkImplicitConeRepresentation::BuildCone()
{
  const double angle = this->Cone->GetAngle();
  const vtkVector3d axis(this->Cone->GetAxis());
  const vtkVector3d origin(this->Cone->GetOrigin());
  const double height = this->GetDiagonalLength();
  const double deltaRadiusAngle = 360. / this->Resolution;
  const vtkVector3d xAxis(1., 0., 0.);
  const vtkVector3d yAxis(0., 1., 0.);

  // Generate cone polydata
  this->ConePD->Reset();

  vtkPoints* conePoints = this->ConePD->GetPoints();
  conePoints->SetNumberOfPoints(this->Resolution + 1);

  // Cone origin point
  conePoints->InsertPoint(0, origin.GetData());

  // Cone base points
  vtkVector3d cross = xAxis.Cross(axis);
  double crossNorm = cross.Norm();
  double dot = xAxis.Dot(axis);
  double xAxisToConeAxisAngle = vtkMath::DegreesFromRadians(std::atan2(crossNorm, dot));

  vtkNew<vtkTransform> toXAlignedCone;
  toXAlignedCone->Identity();
  toXAlignedCone->PostMultiply();
  toXAlignedCone->Translate(height, 0, 0);
  toXAlignedCone->RotateWXYZ(angle, yAxis.GetData());

  vtkNew<vtkTransform> toWidgetBasis;
  toWidgetBasis->Identity();
  toWidgetBasis->Translate(origin.GetData());
  toWidgetBasis->RotateWXYZ(xAxisToConeAxisAngle, cross.GetData());

  for (vtkIdType pointId = 1; pointId <= this->Resolution + 1; ++pointId)
  {
    toXAlignedCone->RotateWXYZ(deltaRadiusAngle, xAxis.GetData());

    vtkVector3d point(0., 0., 0.);
    toXAlignedCone->TransformPoint(point.GetData(), point.GetData());
    toWidgetBasis->TransformPoint(point.GetData(), point.GetData());
    conePoints->InsertPoint(pointId, point.GetData());
  }

  // Cone polys
  vtkCellArray* polys = this->ConePD->GetPolys();
  for (vtkIdType i = 2; i < conePoints->GetNumberOfPoints(); ++i)
  {
    polys->InsertNextCell({ 0, i - 1, i });
  }

  // Clamp cone points to the bounding box
  double bounds[6];
  this->GetOutlineBounds(bounds);
  const vtkBoundingBox bbox(bounds);
  vtkVector3d boundsCenter;
  bbox.GetCenter(boundsCenter.GetData());

  // Move the origin slightly towards the BB center to avoid numerical errors in intersection
  const vtkVector3d adjustedOrigin = origin + (boundsCenter - origin) * 0.0001;

  for (vtkIdType pointIdx = 1; pointIdx < conePoints->GetNumberOfPoints(); ++pointIdx)
  {
    vtkVector3d point;
    conePoints->GetPoint(pointIdx, point.GetData());

    int plane1, plane2;
    vtkVector3d x2;
    double t1, t2;
    if (vtkBox::IntersectWithLine(bounds, adjustedOrigin.GetData(), point.GetData(), t1, t2,
          nullptr, x2.GetData(), plane1, plane2))
    {
      conePoints->SetPoint(pointIdx, x2.GetData());
    }
  }
  this->ConePD->Modified();

  // Create the edges polydata manually (feature edge extractor generates an unnecessary edge)
  this->EdgesPD->Reset();

  // Copy all points except the cone origin
  vtkPoints* edgePoints = this->EdgesPD->GetPoints();
  edgePoints->InsertPoints(0, conePoints->GetNumberOfPoints() - 1, 1, conePoints);

  vtkCellArray* edgeLines = this->EdgesPD->GetLines();
  edgeLines->SetNumberOfCells(conePoints->GetNumberOfPoints() - 2);
  for (vtkIdType pointIdx = 1; pointIdx < edgePoints->GetNumberOfPoints(); ++pointIdx)
  {
    edgeLines->InsertNextCell({ pointIdx - 1, pointIdx });
  }
  this->EdgesPD->Modified();
}

//------------------------------------------------------------------------------
void vtkImplicitConeRepresentation::RegisterPickers()
{
  vtkPickingManager* pm = this->GetPickingManager();
  if (!pm)
  {
    return;
  }
  pm->AddPicker(this->Picker, this);
}

//------------------------------------------------------------------------------
void vtkImplicitConeRepresentation::GetCone(vtkCone* cone) const
{
  if (cone == nullptr)
  {
    return;
  }

  // This class represents a one-sided cone
  cone->IsDoubleConeOff();
  cone->SetAxis(this->Cone->GetAxis());
  cone->SetAngle(this->Cone->GetAngle());
  cone->SetOrigin(this->Cone->GetOrigin());
  cone->SetTransform(this->Cone->GetTransform());
}

VTK_ABI_NAMESPACE_END
