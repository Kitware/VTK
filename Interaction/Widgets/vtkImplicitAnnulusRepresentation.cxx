// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkImplicitAnnulusRepresentation.h"

#include "vtkActor.h"
#include "vtkAnnulus.h"
#include "vtkAssemblyPath.h"
#include "vtkBoundingBox.h"
#include "vtkBox.h"
#include "vtkCamera.h"
#include "vtkCell.h"
#include "vtkCellArray.h"
#include "vtkCellPicker.h"
#include "vtkConeSource.h"
#include "vtkImplicitPlaneRepresentation.h"
#include "vtkInteractorObserver.h"
#include "vtkLineSource.h"
#include "vtkLogger.h"
#include "vtkMath.h"
#include "vtkPickingManager.h"
#include "vtkPlane.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSetGet.h"
#include "vtkSphereSource.h"
#include "vtkTransform.h"
#include "vtkTubeFilter.h"
#include "vtkType.h"
#include "vtkVector.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkImplicitAnnulusRepresentation);

namespace
{
/**
 * @brief Compute the distance between the point and the axis of the annulus.
 */
double ComputeDistanceToAxis(vtkAnnulus* annulus, const vtkVector3d& point)
{
  vtkVector3d center;
  annulus->GetCenter(center.GetData());

  vtkVector3d axis;
  annulus->GetAxis(axis.GetData());

  vtkVector3d centerToPoint = point - center;

  vtkVector3d crossed = axis.Cross(centerToPoint);
  return crossed.Norm();
}
};

//------------------------------------------------------------------------------
vtkImplicitAnnulusRepresentation::vtkImplicitAnnulusRepresentation()
{
  auto initializeRadiusRepresentation = [](RadiusHandleRepresentation& representation)
  {
    vtkNew<vtkPoints> points;
    points->SetDataTypeToDouble();
    representation.PolyData->SetPoints(points);

    vtkNew<vtkCellArray> lines;
    representation.PolyData->SetLines(lines);

    representation.Tuber->SetInputData(representation.PolyData);
    representation.Tuber->SetNumberOfSides(12);

    representation.Mapper->SetInputConnection(representation.Tuber->GetOutputPort());
    representation.Actor->SetMapper(representation.Mapper);
    // The feature edges or tuber turns on scalar viz - we need it off.
    representation.Mapper->ScalarVisibilityOff();
  };

  auto initializeAxisRepresentation = [](AxisHandleRepresentation& representation)
  {
    // Create the axis
    representation.LineSource->SetResolution(1);
    representation.LineMapper->SetInputConnection(representation.LineSource->GetOutputPort());
    representation.LineActor->SetMapper(representation.LineMapper);

    // Create the axis arrow
    representation.ArrowSource->SetResolution(12);
    representation.ArrowSource->SetAngle(25.0);
    representation.ArrowMapper->SetInputConnection(representation.ArrowSource->GetOutputPort());
    representation.ArrowActor->SetMapper(representation.ArrowMapper);
  };

  this->InteractionState = InteractionStateType::Outside;

  // Handle size is in pixels for this widget
  this->HandleSize = 5.0;

  vtkNew<vtkPoints> pts;
  pts->SetDataTypeToDouble();
  this->AnnulusPD->SetPoints(pts);

  vtkNew<vtkCellArray> polys;
  this->AnnulusPD->SetPolys(polys);

  this->AnnulusMapper->SetInputData(this->AnnulusPD);
  this->AnnulusActor->SetMapper(this->AnnulusMapper);

  // Radii handles
  initializeRadiusRepresentation(this->InnerRadiusRepresentation);
  initializeRadiusRepresentation(this->OuterRadiusRepresentation);

  // Axis handles
  initializeAxisRepresentation(this->LowerAxisRepresentation);
  initializeAxisRepresentation(this->UpperAxisRepresentation);

  // Create the center handle
  this->CenterHandleSource->SetThetaResolution(16);
  this->CenterHandleSource->SetPhiResolution(16);
  this->CenterHandleMapper->SetInputConnection(this->CenterHandleSource->GetOutputPort());
  this->CenterHandleActor->SetMapper(this->CenterHandleMapper);

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
  this->Picker->AddPickList(this->LowerAxisRepresentation.LineActor);
  this->Picker->AddPickList(this->LowerAxisRepresentation.ArrowActor);
  this->Picker->AddPickList(this->UpperAxisRepresentation.LineActor);
  this->Picker->AddPickList(this->UpperAxisRepresentation.ArrowActor);
  this->Picker->AddPickList(this->CenterHandleActor);
  this->Picker->AddPickList(this->GetOutlineActor());
  this->Picker->PickFromListOn();

  this->AnnulusPicker->SetTolerance(0.005);
  this->AnnulusPicker->AddPickList(this->InnerRadiusRepresentation.Actor);
  this->AnnulusPicker->AddPickList(this->OuterRadiusRepresentation.Actor);
  this->AnnulusPicker->PickFromListOn();

  // Set up the initial properties
  // Annulus properties
  this->AnnulusProperty->SetAmbient(1.0);
  this->AnnulusProperty->SetAmbientColor(1.0, 1.0, 1.0);
  this->AnnulusProperty->SetOpacity(0.5);

  this->SelectedAnnulusProperty->SetAmbient(1.0);
  this->SelectedAnnulusProperty->SetAmbientColor(0.0, 1.0, 0.0);
  this->SelectedAnnulusProperty->SetOpacity(0.25);

  // Annulus axis properties
  this->AxisProperty->SetAmbient(1.0);
  this->AxisProperty->SetColor(1, 0, 0);
  this->AxisProperty->SetLineWidth(2);

  this->SelectedAxisProperty->SetAmbient(1.0);
  this->SelectedAxisProperty->SetColor(0, 1, 0);
  this->SelectedAxisProperty->SetLineWidth(2);

  // Center handle properties
  this->CenterHandleProperty->SetAmbient(1.0);
  this->CenterHandleProperty->SetColor(1, 0, 0);

  this->SelectedCenterHandleProperty->SetAmbient(1.0);
  this->SelectedCenterHandleProperty->SetColor(0, 1, 0);

  // Edge property
  this->RadiusHandleProperty->SetAmbient(1.0);
  this->RadiusHandleProperty->SetColor(1.0, 0.0, 0.0);
  this->SelectedRadiusHandleProperty->SetAmbient(1.0);
  this->SelectedRadiusHandleProperty->SetColor(0.0, 1.0, 0.0);

  // Pass the initial properties to the actors.
  this->AnnulusActor->SetProperty(this->AnnulusProperty);
  this->LowerAxisRepresentation.LineActor->SetProperty(this->AxisProperty);
  this->LowerAxisRepresentation.ArrowActor->SetProperty(this->AxisProperty);
  this->UpperAxisRepresentation.LineActor->SetProperty(this->AxisProperty);
  this->UpperAxisRepresentation.ArrowActor->SetProperty(this->AxisProperty);
  this->CenterHandleActor->SetProperty(this->CenterHandleProperty);
  this->AnnulusActor->SetProperty(this->AnnulusProperty);
  this->InnerRadiusRepresentation.Actor->SetProperty(this->RadiusHandleProperty);
  this->OuterRadiusRepresentation.Actor->SetProperty(this->RadiusHandleProperty);
}

//------------------------------------------------------------------------------
vtkImplicitAnnulusRepresentation::~vtkImplicitAnnulusRepresentation() = default;

//------------------------------------------------------------------------------
int vtkImplicitAnnulusRepresentation::ComputeInteractionState(int X, int Y, int vtkNotUsed(modify))
{
  // See if anything has been selected
  vtkAssemblyPath* path = this->GetAssemblyPath(X, Y, 0., this->Picker);

  // The second picker may need to be called. This is done because the annulus
  // wraps around things that can be picked; thus the annulus is the selection
  // of last resort.
  if (path == nullptr)
  {
    this->AnnulusPicker->Pick(X, Y, 0., this->Renderer);
    path = this->AnnulusPicker->GetPath();
  }

  if (path == nullptr) // Nothing picked
  {
    this->SetRepresentationState(vtkImplicitAnnulusRepresentation::Outside);
    this->InteractionState = vtkImplicitAnnulusRepresentation::Outside;
    return this->InteractionState;
  }

  // Something picked, continue
  this->ValidPick = true;

  // Depending on the interaction state (set by the widget) we modify
  // the state of the representation based on what is picked.
  if (this->InteractionState == vtkImplicitAnnulusRepresentation::Moving)
  {
    vtkProp* prop = path->GetFirstNode()->GetViewProp();
    if (prop == this->LowerAxisRepresentation.LineActor ||
      prop == this->LowerAxisRepresentation.ArrowActor ||
      prop == this->UpperAxisRepresentation.LineActor ||
      prop == this->UpperAxisRepresentation.ArrowActor)
    {
      this->InteractionState = vtkImplicitAnnulusRepresentation::RotatingAxis;
      this->SetRepresentationState(vtkImplicitAnnulusRepresentation::RotatingAxis);
    }
    else if (prop == this->InnerRadiusRepresentation.Actor)
    {
      this->InteractionState = vtkImplicitAnnulusRepresentation::AdjustingInnerRadius;
      this->SetRepresentationState(vtkImplicitAnnulusRepresentation::AdjustingInnerRadius);
    }
    else if (prop == this->OuterRadiusRepresentation.Actor)
    {
      this->InteractionState = vtkImplicitAnnulusRepresentation::AdjustingOuterRadius;
      this->SetRepresentationState(vtkImplicitAnnulusRepresentation::AdjustingOuterRadius);
    }
    else if (prop == this->CenterHandleActor)
    {
      this->InteractionState = vtkImplicitAnnulusRepresentation::MovingCenter;
      this->SetRepresentationState(vtkImplicitAnnulusRepresentation::MovingCenter);
    }
    else
    {
      if (this->GetOutlineTranslation())
      {
        this->InteractionState = vtkImplicitAnnulusRepresentation::MovingOutline;
        this->SetRepresentationState(vtkImplicitAnnulusRepresentation::MovingOutline);
      }
      else
      {
        this->InteractionState = vtkImplicitAnnulusRepresentation::Outside;
        this->SetRepresentationState(vtkImplicitAnnulusRepresentation::Outside);
      }
    }
  }

  // We may add a condition to allow the camera to work IO scaling
  else if (this->InteractionState != vtkImplicitAnnulusRepresentation::Scaling)
  {
    this->InteractionState = vtkImplicitAnnulusRepresentation::Outside;
  }

  return this->InteractionState;
}

//------------------------------------------------------------------------------
void vtkImplicitAnnulusRepresentation::SetRepresentationState(InteractionStateType state)
{
  if (this->RepresentationState == state)
  {
    return;
  }

  state = vtkMath::ClampValue(
    state, InteractionStateType::Outside, InteractionStateType::TranslatingCenter);

  this->RepresentationState = state;
  this->Modified();

  this->HighlightAxis(false);
  this->HighlightAnnulus(false);
  this->HighlightOutline(false);
  this->HighlightCenterHandle(false);
  this->HighlightInnerRadiusHandle(false);
  this->HighlightInnerRadiusHandle(false);

  switch (state)
  {
    case vtkImplicitAnnulusRepresentation::RotatingAxis:
      this->HighlightAxis(true);
      break;

    case vtkImplicitAnnulusRepresentation::AdjustingInnerRadius:
      this->HighlightInnerRadiusHandle(true);
      break;

    case vtkImplicitAnnulusRepresentation::AdjustingOuterRadius:
      this->HighlightOuterRadiusHandle(true);
      break;

    case vtkImplicitAnnulusRepresentation::TranslatingCenter:
    case vtkImplicitAnnulusRepresentation::MovingCenter:
      this->HighlightCenterHandle(true);
      break;

    case vtkImplicitAnnulusRepresentation::MovingOutline:
      this->HighlightOutline(true);
      break;

    case vtkImplicitAnnulusRepresentation::Scaling:
      if (this->ScaleEnabled)
      {
        this->HighlightAxis(true);
        this->HighlightAnnulus(true);
        this->HighlightOutline(true);
        this->HighlightCenterHandle(true);
        this->HighlightInnerRadiusHandle(true);
        this->HighlightOuterRadiusHandle(true);
      }
      break;

    default:
      break;
  }
}

//------------------------------------------------------------------------------
void vtkImplicitAnnulusRepresentation::StartWidgetInteraction(double e[2])
{
  this->StartEventPosition[0] = e[0];
  this->StartEventPosition[1] = e[1];
  this->StartEventPosition[2] = 0.0;
  this->LastEventPosition = { e[0], e[1], 0.0 };
}

//------------------------------------------------------------------------------
void vtkImplicitAnnulusRepresentation::WidgetInteraction(double e[2])
{
  vtkCamera* camera = this->Renderer->GetActiveCamera();
  if (!camera)
  {
    return;
  }

  vtkVector3d prevPickPoint = this->GetWorldPoint(this->Picker, this->LastEventPosition.GetData());
  vtkVector3d pickPoint = this->GetWorldPoint(this->Picker, e);

  vtkVector3d annulusPickPoint = this->GetWorldPoint(this->AnnulusPicker, e);

  // Process the motion
  switch (this->InteractionState)
  {
    case InteractionStateType::MovingOutline:
      this->TranslateOutline(prevPickPoint.GetData(), pickPoint.GetData());
      break;

    case InteractionStateType::MovingCenter:
      this->TranslateCenter(prevPickPoint, pickPoint);
      break;

    case InteractionStateType::TranslatingCenter:
      this->TranslateCenterOnAxis(prevPickPoint, pickPoint);
      break;

    case InteractionStateType::AdjustingInnerRadius:
      this->AdjustInnerRadius(e[0], e[1], annulusPickPoint);
      break;

    case InteractionStateType::AdjustingOuterRadius:
      this->AdjustOuterRadius(e[0], e[1], annulusPickPoint);
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

  this->LastEventPosition = { e[0], e[1], 0.0 };

  this->BuildRepresentation();
}

//------------------------------------------------------------------------------
void vtkImplicitAnnulusRepresentation::EndWidgetInteraction(double vtkNotUsed(e)[2])
{
  this->SetRepresentationState(InteractionStateType::Outside);
}

//------------------------------------------------------------------------------
double* vtkImplicitAnnulusRepresentation::GetBounds()
{
  this->BuildRepresentation();
  this->BoundingBox->SetBounds(this->GetOutlineActor()->GetBounds());
  this->BoundingBox->AddBounds(this->AnnulusActor->GetBounds());
  this->BoundingBox->AddBounds(this->InnerRadiusRepresentation.Actor->GetBounds());
  this->BoundingBox->AddBounds(this->OuterRadiusRepresentation.Actor->GetBounds());
  this->BoundingBox->AddBounds(this->LowerAxisRepresentation.LineActor->GetBounds());
  this->BoundingBox->AddBounds(this->LowerAxisRepresentation.ArrowActor->GetBounds());
  this->BoundingBox->AddBounds(this->UpperAxisRepresentation.LineActor->GetBounds());
  this->BoundingBox->AddBounds(this->UpperAxisRepresentation.ArrowActor->GetBounds());
  this->BoundingBox->AddBounds(this->CenterHandleActor->GetBounds());

  return this->BoundingBox->GetBounds();
}

//------------------------------------------------------------------------------
void vtkImplicitAnnulusRepresentation::GetActors(vtkPropCollection* pc)
{
  this->GetOutlineActor()->GetActors(pc);
  this->AnnulusActor->GetActors(pc);
  this->InnerRadiusRepresentation.Actor->GetActors(pc);
  this->OuterRadiusRepresentation.Actor->GetActors(pc);
  this->LowerAxisRepresentation.LineActor->GetActors(pc);
  this->LowerAxisRepresentation.ArrowActor->GetActors(pc);
  this->UpperAxisRepresentation.LineActor->GetActors(pc);
  this->UpperAxisRepresentation.ArrowActor->GetActors(pc);
  this->CenterHandleActor->GetActors(pc);
}

//------------------------------------------------------------------------------
void vtkImplicitAnnulusRepresentation::ReleaseGraphicsResources(vtkWindow* w)
{
  this->GetOutlineActor()->ReleaseGraphicsResources(w);
  this->AnnulusActor->ReleaseGraphicsResources(w);
  this->InnerRadiusRepresentation.Actor->ReleaseGraphicsResources(w);
  this->OuterRadiusRepresentation.Actor->ReleaseGraphicsResources(w);
  this->LowerAxisRepresentation.LineActor->ReleaseGraphicsResources(w);
  this->LowerAxisRepresentation.ArrowActor->ReleaseGraphicsResources(w);
  this->UpperAxisRepresentation.LineActor->ReleaseGraphicsResources(w);
  this->UpperAxisRepresentation.ArrowActor->ReleaseGraphicsResources(w);
  this->CenterHandleActor->ReleaseGraphicsResources(w);
}

//------------------------------------------------------------------------------
int vtkImplicitAnnulusRepresentation::RenderOpaqueGeometry(vtkViewport* v)
{
  int count = 0;
  this->BuildRepresentation();
  count += this->GetOutlineActor()->RenderOpaqueGeometry(v);
  count += this->InnerRadiusRepresentation.Actor->RenderOpaqueGeometry(v);
  count += this->OuterRadiusRepresentation.Actor->RenderOpaqueGeometry(v);
  count += this->LowerAxisRepresentation.LineActor->RenderOpaqueGeometry(v);
  count += this->LowerAxisRepresentation.ArrowActor->RenderOpaqueGeometry(v);
  count += this->UpperAxisRepresentation.LineActor->RenderOpaqueGeometry(v);
  count += this->UpperAxisRepresentation.ArrowActor->RenderOpaqueGeometry(v);
  count += this->CenterHandleActor->RenderOpaqueGeometry(v);

  if (this->DrawAnnulus)
  {
    count += this->AnnulusActor->RenderOpaqueGeometry(v);
  }

  return count;
}

//------------------------------------------------------------------------------
int vtkImplicitAnnulusRepresentation::RenderTranslucentPolygonalGeometry(vtkViewport* v)
{
  int count = 0;
  this->BuildRepresentation();
  count += this->GetOutlineActor()->RenderTranslucentPolygonalGeometry(v);
  count += this->InnerRadiusRepresentation.Actor->RenderTranslucentPolygonalGeometry(v);
  count += this->OuterRadiusRepresentation.Actor->RenderTranslucentPolygonalGeometry(v);
  count += this->LowerAxisRepresentation.LineActor->RenderTranslucentPolygonalGeometry(v);
  count += this->LowerAxisRepresentation.ArrowActor->RenderTranslucentPolygonalGeometry(v);
  count += this->UpperAxisRepresentation.LineActor->RenderTranslucentPolygonalGeometry(v);
  count += this->UpperAxisRepresentation.ArrowActor->RenderTranslucentPolygonalGeometry(v);
  count += this->CenterHandleActor->RenderTranslucentPolygonalGeometry(v);

  if (this->DrawAnnulus)
  {
    count += this->AnnulusActor->RenderTranslucentPolygonalGeometry(v);
  }

  return count;
}

//------------------------------------------------------------------------------
vtkTypeBool vtkImplicitAnnulusRepresentation::HasTranslucentPolygonalGeometry()
{
  vtkTypeBool result = false;
  result |= this->GetOutlineActor()->HasTranslucentPolygonalGeometry();
  result |= this->InnerRadiusRepresentation.Actor->HasTranslucentPolygonalGeometry();
  result |= this->OuterRadiusRepresentation.Actor->HasTranslucentPolygonalGeometry();
  result |= this->LowerAxisRepresentation.LineActor->HasTranslucentPolygonalGeometry();
  result |= this->LowerAxisRepresentation.ArrowActor->HasTranslucentPolygonalGeometry();
  result |= this->UpperAxisRepresentation.LineActor->HasTranslucentPolygonalGeometry();
  result |= this->UpperAxisRepresentation.ArrowActor->HasTranslucentPolygonalGeometry();
  result |= this->CenterHandleActor->HasTranslucentPolygonalGeometry();

  if (this->DrawAnnulus)
  {
    result |= this->AnnulusActor->HasTranslucentPolygonalGeometry();
  }

  return result;
}

//------------------------------------------------------------------------------
void vtkImplicitAnnulusRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Resolution: " << this->Resolution << std::endl;

  os << indent << "Axis Property: " << this->AxisProperty << std::endl;
  os << indent << "Axis Property: (none)" << std::endl;
  os << indent << "Selected Axis Property: " << this->SelectedAxisProperty << std::endl;
  os << indent << "Selected Axis Property: (none)" << std::endl;
  os << indent << "Annulus Property: " << this->AnnulusProperty << std::endl;
  os << indent << "Annulus Property: (none)" << std::endl;
  os << indent << "Selected Annulus Property: " << this->SelectedAnnulusProperty << std::endl;
  os << indent << "Selected Annulus Property: (none)" << std::endl;
  os << indent << "Edges Property: " << this->RadiusHandleProperty << std::endl;
  os << indent << "Edges Property: (none)" << std::endl;

  os << indent << "Along X Axis: " << (this->AlongXAxis ? "On" : "Off") << std::endl;
  os << indent << "Along Y Axis: " << (this->AlongYAxis ? "On" : "Off") << std::endl;
  os << indent << "ALong Z Axis: " << (this->AlongZAxis ? "On" : "Off") << std::endl;

  os << indent << "Tubing: " << (this->Tubing ? "On" : "Off") << std::endl;
  os << indent << "Scale Enabled: " << (this->ScaleEnabled ? "On" : "Off") << std::endl;
  os << indent << "Draw Annulus: " << (this->DrawAnnulus ? "On" : "Off") << std::endl;
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
    case MovingCenter:
      os << "MovingCenter" << std::endl;
      break;
    case RotatingAxis:
      os << "RotatingAxis" << std::endl;
      break;
    case AdjustingInnerRadius:
      os << "AdjustingInnerRadius" << std::endl;
      break;
    case AdjustingOuterRadius:
      os << "AdjustingInnerRadius" << std::endl;
      break;
    case Scaling:
      os << "Scaling" << std::endl;
      break;
    case TranslatingCenter:
      os << "TranslatingCenter" << std::endl;
      break;
  }
}

//------------------------------------------------------------------------------
void vtkImplicitAnnulusRepresentation::HighlightAxis(bool highlight)
{
  if (highlight)
  {
    this->LowerAxisRepresentation.LineActor->SetProperty(this->SelectedAxisProperty);
    this->LowerAxisRepresentation.ArrowActor->SetProperty(this->SelectedAxisProperty);
    this->UpperAxisRepresentation.LineActor->SetProperty(this->SelectedAxisProperty);
    this->UpperAxisRepresentation.ArrowActor->SetProperty(this->SelectedAxisProperty);
  }
  else
  {
    this->LowerAxisRepresentation.LineActor->SetProperty(this->AxisProperty);
    this->LowerAxisRepresentation.ArrowActor->SetProperty(this->AxisProperty);
    this->UpperAxisRepresentation.LineActor->SetProperty(this->AxisProperty);
    this->UpperAxisRepresentation.ArrowActor->SetProperty(this->AxisProperty);
  }
}

//------------------------------------------------------------------------------
void vtkImplicitAnnulusRepresentation::HighlightCenterHandle(bool highlight)
{
  if (highlight)
  {
    this->CenterHandleActor->SetProperty(this->SelectedCenterHandleProperty);
  }
  else
  {
    this->CenterHandleActor->SetProperty(this->CenterHandleProperty);
  }
}

//------------------------------------------------------------------------------
void vtkImplicitAnnulusRepresentation::HighlightAnnulus(bool highlight)
{
  if (highlight)
  {
    this->AnnulusActor->SetProperty(this->SelectedAnnulusProperty);
    this->InnerRadiusRepresentation.Actor->SetProperty(this->SelectedAnnulusProperty);
    this->OuterRadiusRepresentation.Actor->SetProperty(this->SelectedAnnulusProperty);
  }
  else
  {
    this->AnnulusActor->SetProperty(this->AnnulusProperty);
    this->InnerRadiusRepresentation.Actor->SetProperty(this->RadiusHandleProperty);
    this->OuterRadiusRepresentation.Actor->SetProperty(this->RadiusHandleProperty);
  }
}

//------------------------------------------------------------------------------
void vtkImplicitAnnulusRepresentation::HighlightInnerRadiusHandle(bool highlight)
{
  if (highlight)
  {
    this->InnerRadiusRepresentation.Actor->SetProperty(this->SelectedRadiusHandleProperty);
  }
  else
  {
    this->InnerRadiusRepresentation.Actor->SetProperty(this->RadiusHandleProperty);
  }
}

//------------------------------------------------------------------------------
void vtkImplicitAnnulusRepresentation::HighlightOuterRadiusHandle(bool highlight)
{
  if (highlight)
  {
    this->OuterRadiusRepresentation.Actor->SetProperty(this->SelectedRadiusHandleProperty);
  }
  else
  {
    this->OuterRadiusRepresentation.Actor->SetProperty(this->RadiusHandleProperty);
  }
}

//------------------------------------------------------------------------------
void vtkImplicitAnnulusRepresentation::Rotate(
  double X, double Y, const vtkVector3d& p1, const vtkVector3d& p2, const vtkVector3d& vpn)
{
  vtkVector3d v = p2 - p1;         // mouse motion vector in world space
  vtkVector3d axis = vpn.Cross(v); // axis of rotation

  vtkVector3d annulusCenter(this->Annulus->GetCenter());
  vtkVector3d annulusAxis(this->Annulus->GetAxis());

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
  transform->Translate(annulusCenter.GetData());
  transform->RotateWXYZ(theta, axis.GetData());
  transform->Translate((-annulusCenter).GetData());

  // Set the new normal
  vtkVector3d aNew;
  transform->TransformNormal(annulusAxis.GetData(), aNew.GetData());
  this->Annulus->SetAxis(aNew.GetData());
}

//------------------------------------------------------------------------------
void vtkImplicitAnnulusRepresentation::TranslateRepresentation(const vtkVector3d& motion)
{
  // Translate the annulus
  vtkVector3d annulusCenter(this->Annulus->GetCenter());
  vtkVector3d newAnnulusCenter = annulusCenter + motion;
  this->Annulus->SetCenter(newAnnulusCenter.GetData());
}

//------------------------------------------------------------------------------
void vtkImplicitAnnulusRepresentation::TranslateCenter(const vtkVector3d& p1, const vtkVector3d& p2)
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
    v = p2 - p1;
  }
  else
  {
    v[this->GetTranslationAxis()] = p2[this->GetTranslationAxis()] - p1[this->GetTranslationAxis()];
  }

  // Translate the current center
  vtkVector3d center(this->Annulus->GetCenter());
  vtkVector3d newCenter = center + v;

  // Project back onto plane orthogonal to camera
  vtkVector3d vpn;
  camera->GetViewPlaneNormal(vpn.GetData());

  vtkPlane::ProjectPoint(newCenter.GetData(), center.GetData(), vpn.GetData(), newCenter.GetData());

  this->Annulus->SetCenter(newCenter.GetData());
}

//------------------------------------------------------------------------------
void vtkImplicitAnnulusRepresentation::TranslateCenterOnAxis(
  const vtkVector3d& p1, const vtkVector3d& p2)
{
  // Get the motion vector
  vtkVector3d v = p2 - p1;

  // Add to the current point, project back down onto plane
  vtkVector3d center(this->Annulus->GetCenter());
  vtkVector3d axis(this->Annulus->GetAxis());
  vtkVector3d newCenter = center + v;

  // Normalize the axis vector
  axis.Normalize();

  // Project the point on the axis vector
  vtkVector3d u = newCenter - center;
  newCenter = center + (axis * axis.Dot(u));
  this->Annulus->SetCenter(newCenter.GetData());
}

//------------------------------------------------------------------------------
void vtkImplicitAnnulusRepresentation::Scale(
  const vtkVector3d& p1, const vtkVector3d& p2, double vtkNotUsed(X), double Y)
{
  // Get the motion vector
  vtkVector3d v = p2 - p1;

  vtkVector3d annulusCenter(this->Annulus->GetCenter());

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
  transform->Translate(annulusCenter.GetData());
  transform->Scale(sf, sf, sf);
  transform->Translate((-annulusCenter).GetData());

  this->TransformBounds(transform);
}

//------------------------------------------------------------------------------
void vtkImplicitAnnulusRepresentation::AdjustInnerRadius(
  double x, double y, const vtkVector3d& point)
{
  if (x == this->LastEventPosition[0] && y == this->LastEventPosition[1])
  {
    return;
  }

  double radius = ::ComputeDistanceToAxis(this->Annulus, point);

  this->SetInnerRadius(radius);
}

//------------------------------------------------------------------------------
void vtkImplicitAnnulusRepresentation::AdjustOuterRadius(
  double x, double y, const vtkVector3d& point)
{
  if (x == this->LastEventPosition[0] && y == this->LastEventPosition[1])
  {
    return;
  }

  double radius = ::ComputeDistanceToAxis(this->Annulus, point);
  this->SetOuterRadius(radius);
}

//------------------------------------------------------------------------------
void vtkImplicitAnnulusRepresentation::SetInteractionColor(double r, double g, double b)
{
  this->SelectedAxisProperty->SetColor(r, g, b);
  this->SelectedAnnulusProperty->SetAmbientColor(r, g, b);
  this->SetSelectedOutlineColor(r, g, b);
}

//------------------------------------------------------------------------------
void vtkImplicitAnnulusRepresentation::SetHandleColor(double r, double g, double b)
{
  this->AxisProperty->SetColor(r, g, b);
}

//------------------------------------------------------------------------------
void vtkImplicitAnnulusRepresentation::SetForegroundColor(double r, double g, double b)
{
  this->AnnulusProperty->SetAmbientColor(r, g, b);
  this->SetOutlineColor(r, g, b);
}

//------------------------------------------------------------------------------
void vtkImplicitAnnulusRepresentation::PlaceWidget(double bds[6])
{
  vtkVector<double, 6> bounds;
  vtkVector3d center;
  this->AdjustBounds(bds, bounds.GetData(), center.GetData());
  this->SetOutlineBounds(bounds.GetData());

  this->LowerAxisRepresentation.LineSource->SetPoint1(this->Annulus->GetCenter());
  this->UpperAxisRepresentation.LineSource->SetPoint1(this->Annulus->GetCenter());
  if (this->AlongYAxis)
  {
    this->Annulus->SetAxis(0, 1, 0);
    this->LowerAxisRepresentation.LineSource->SetPoint2(0, 1, 0);
    this->UpperAxisRepresentation.LineSource->SetPoint2(0, 1, 0);
  }
  else if (this->AlongZAxis)
  {
    this->Annulus->SetAxis(0, 0, 1);
    this->LowerAxisRepresentation.LineSource->SetPoint2(0, 0, 1);
    this->UpperAxisRepresentation.LineSource->SetPoint2(0, 0, 1);
  }
  else // default or x-normal
  {
    this->Annulus->SetAxis(1, 0, 0);
    this->LowerAxisRepresentation.LineSource->SetPoint2(1, 0, 0);
    this->UpperAxisRepresentation.LineSource->SetPoint2(1, 0, 0);
  }

  for (int i = 0; i < 6; i++)
  {
    this->InitialBounds[i] = bounds[i];
  }
  this->SetWidgetBounds(bounds.GetData());

  this->InitialLength = sqrt((bounds[1] - bounds[0]) * (bounds[1] - bounds[0]) +
    (bounds[3] - bounds[2]) * (bounds[3] - bounds[2]) +
    (bounds[5] - bounds[4]) * (bounds[5] - bounds[4]));

  this->ValidPick = true; // since we have positioned the widget successfully
  this->BuildRepresentation();
}

//------------------------------------------------------------------------------
void vtkImplicitAnnulusRepresentation::SetCenter(double x, double y, double z)
{
  this->Annulus->SetCenter(x, y, z);
}

//------------------------------------------------------------------------------
void vtkImplicitAnnulusRepresentation::SetCenter(double x[3])
{
  this->SetCenter(x[1], x[2], x[3]);
}

//------------------------------------------------------------------------------
double* vtkImplicitAnnulusRepresentation::GetCenter() const
{
  return this->Annulus->GetCenter();
}

//------------------------------------------------------------------------------
void vtkImplicitAnnulusRepresentation::GetCenter(double xyz[3]) const
{
  this->Annulus->GetCenter(xyz);
}

//------------------------------------------------------------------------------
void vtkImplicitAnnulusRepresentation::SetAxis(double x, double y, double z)
{
  vtkVector3d n(x, y, z);
  n.Normalize();

  vtkVector3d currentAxis(this->Annulus->GetAxis());
  if (n != currentAxis)
  {
    this->Annulus->SetAxis(n.GetData());
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkImplicitAnnulusRepresentation::SetAxis(double n[3])
{
  this->SetAxis(n[0], n[1], n[2]);
}

//------------------------------------------------------------------------------
double* vtkImplicitAnnulusRepresentation::GetAxis() const
{
  return this->Annulus->GetAxis();
}

//------------------------------------------------------------------------------
void vtkImplicitAnnulusRepresentation::GetAxis(double xyz[3]) const
{
  this->Annulus->GetAxis(xyz);
}

//------------------------------------------------------------------------------
void vtkImplicitAnnulusRepresentation::SetInnerRadius(double radius)
{
  this->Annulus->SetInnerRadius(radius);
}

//------------------------------------------------------------------------------
double vtkImplicitAnnulusRepresentation::GetInnerRadius() const
{
  return this->Annulus->GetInnerRadius();
}

//------------------------------------------------------------------------------
void vtkImplicitAnnulusRepresentation::SetOuterRadius(double radius)
{
  this->Annulus->SetOuterRadius(radius);
}

//------------------------------------------------------------------------------
double vtkImplicitAnnulusRepresentation::GetOuterRadius() const
{
  return this->Annulus->GetOuterRadius();
}

//------------------------------------------------------------------------------
void vtkImplicitAnnulusRepresentation::SetDrawAnnulus(bool drawAnnulus)
{
  if (drawAnnulus == this->DrawAnnulus)
  {
    return;
  }

  this->Modified();
  this->DrawAnnulus = drawAnnulus;
}

//------------------------------------------------------------------------------
void vtkImplicitAnnulusRepresentation::SetAlongXAxis(bool var)
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
void vtkImplicitAnnulusRepresentation::SetAlongYAxis(bool var)
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
void vtkImplicitAnnulusRepresentation::SetAlongZAxis(bool var)
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
void vtkImplicitAnnulusRepresentation::GetPolyData(vtkPolyData* pd)
{
  pd->ShallowCopy(this->AnnulusPD);
}

//------------------------------------------------------------------------------
void vtkImplicitAnnulusRepresentation::UpdatePlacement()
{
  this->BuildRepresentation();
  this->UpdateOutline();
}

//------------------------------------------------------------------------------
void vtkImplicitAnnulusRepresentation::BumpAnnulus(int dir, double factor)
{
  // Compute the distance
  double d = this->InitialLength * this->BumpDistance * factor;

  // Push the annulus
  this->PushAnnulus((dir > 0 ? d : -d));
}

//------------------------------------------------------------------------------
void vtkImplicitAnnulusRepresentation::PushAnnulus(double d)
{
  vtkCamera* camera = this->Renderer->GetActiveCamera();
  if (!camera)
  {
    return;
  }

  vtkVector3d vpn, center;
  camera->GetViewPlaneNormal(vpn.GetData());
  this->Annulus->GetCenter(center.GetData());

  center += (d * vpn);

  this->Annulus->SetCenter(center.GetData());

  this->BuildRepresentation();
}

//------------------------------------------------------------------------------
void vtkImplicitAnnulusRepresentation::BuildRepresentation()
{
  if (!this->Renderer || !this->Renderer->GetRenderWindow())
  {
    return;
  }

  if (this->GetMTime() > this->BuildTime || this->Annulus->GetMTime() > this->BuildTime ||
    this->Renderer->GetRenderWindow()->GetMTime() > this->BuildTime)
  {

    vtkInformation* info = this->GetPropertyKeys();
    this->GetOutlineActor()->SetPropertyKeys(info);
    this->AnnulusActor->SetPropertyKeys(info);
    this->InnerRadiusRepresentation.Actor->SetPropertyKeys(info);
    this->OuterRadiusRepresentation.Actor->SetPropertyKeys(info);
    this->LowerAxisRepresentation.LineActor->SetPropertyKeys(info);
    this->LowerAxisRepresentation.ArrowActor->SetPropertyKeys(info);
    this->UpperAxisRepresentation.LineActor->SetPropertyKeys(info);
    this->UpperAxisRepresentation.ArrowActor->SetPropertyKeys(info);
    this->CenterHandleActor->SetPropertyKeys(info);

    vtkVector3d center(this->Annulus->GetCenter());
    vtkVector3d axis(this->Annulus->GetAxis());

    this->UpdateCenterAndBounds(center.GetData());

    // Update the adjusted center
    this->Annulus->SetCenter(center.GetData());

    // Setup the annulus axis
    double d = this->GetDiagonalLength();
    vtkVector3d widgetAxisVector = axis * 0.3 * d;

    vtkVector3d p2 = center + widgetAxisVector;
    this->LowerAxisRepresentation.LineSource->SetPoint1(center.GetData());
    this->LowerAxisRepresentation.LineSource->SetPoint2(p2.GetData());
    this->LowerAxisRepresentation.ArrowSource->SetCenter(p2.GetData());
    this->LowerAxisRepresentation.ArrowSource->SetDirection(axis.GetData());

    p2 = center - widgetAxisVector;
    this->UpperAxisRepresentation.LineSource->SetPoint1(center.GetData());
    this->UpperAxisRepresentation.LineSource->SetPoint2(p2.GetData());
    this->UpperAxisRepresentation.ArrowSource->SetCenter(p2.GetData());
    this->UpperAxisRepresentation.ArrowSource->SetDirection(axis.GetData());

    // Set up the position handle
    this->CenterHandleSource->SetCenter(center.GetData());

    // Control the look of the edges
    if (this->Tubing)
    {
      this->InnerRadiusRepresentation.Mapper->SetInputConnection(
        this->InnerRadiusRepresentation.Tuber->GetOutputPort());
      this->OuterRadiusRepresentation.Mapper->SetInputConnection(
        this->OuterRadiusRepresentation.Tuber->GetOutputPort());
    }
    else
    {
      this->InnerRadiusRepresentation.Mapper->SetInputData(
        this->InnerRadiusRepresentation.PolyData);
      this->OuterRadiusRepresentation.Mapper->SetInputData(
        this->OuterRadiusRepresentation.PolyData);
    }

    // Construct intersected annulus
    this->BuildAnnulus();

    this->SizeHandles();
    this->BuildTime.Modified();
  }
}

//------------------------------------------------------------------------------
void vtkImplicitAnnulusRepresentation::SizeHandles()
{
  double radius =
    this->vtkWidgetRepresentation::SizeHandlesInPixels(1.5, this->CenterHandleSource->GetCenter());

  this->LowerAxisRepresentation.ArrowSource->SetHeight(2.0 * radius);
  this->LowerAxisRepresentation.ArrowSource->SetRadius(radius);
  this->UpperAxisRepresentation.ArrowSource->SetHeight(2.0 * radius);
  this->UpperAxisRepresentation.ArrowSource->SetRadius(radius);

  this->CenterHandleSource->SetRadius(radius);

  this->InnerRadiusRepresentation.Tuber->SetRadius(0.25 * radius);
  this->OuterRadiusRepresentation.Tuber->SetRadius(0.25 * radius);
}

//------------------------------------------------------------------------------
// Create the annulus polydata. Basically build an oriented annulus of
// specified resolution. Clamp annulus facets by performing
// intersection tests.
void vtkImplicitAnnulusRepresentation::BuildAnnulus()
{
  const vtkVector3d axis(this->Annulus->GetAxis());
  const vtkVector3d center(this->Annulus->GetCenter());
  const double height = this->GetDiagonalLength();
  const double deltaRadiusAngle = 360. / this->Resolution;
  const vtkVector3d yAxis(0., 1., 0.);

  // Generate annulus polydata
  this->AnnulusPD->Reset();

  const vtkIdType numberOfPointsPerCylinderSide = this->Resolution;
  const vtkIdType numberOfPointsPerCylinder = numberOfPointsPerCylinderSide * 2.;

  struct CylinderInfo
  {
    vtkIdType StartOffset;
    vtkIdType TopOffset;
    vtkIdType BottomOffset;
    double Radius;
    // Edges are identified by the index of the associated side points
    std::vector<bool> EdgeInsideBoundingBox;
  };

  CylinderInfo inner;
  inner.StartOffset = 0;
  inner.TopOffset = inner.StartOffset;
  inner.BottomOffset = inner.TopOffset + this->Resolution;
  inner.Radius = this->Annulus->GetInnerRadius();
  inner.EdgeInsideBoundingBox.resize(numberOfPointsPerCylinderSide);

  CylinderInfo outer;
  outer.StartOffset = this->Resolution * 2.;
  outer.TopOffset = outer.StartOffset;
  outer.BottomOffset = outer.StartOffset + this->Resolution;
  outer.Radius = this->Annulus->GetOuterRadius();
  outer.EdgeInsideBoundingBox.resize(numberOfPointsPerCylinderSide);

  if (inner.Radius > outer.Radius)
  {
    vtkLog(TRACE, "Inner radius is greater than the outer one. Clamping.");
    inner.Radius = outer.Radius;
  }

  // Annulus base points
  vtkVector3d cross = yAxis.Cross(axis);
  double crossNorm = cross.Norm();
  double dot = yAxis.Dot(axis);
  double yAxisToAnnulusAxisAngle = vtkMath::DegreesFromRadians(std::atan2(crossNorm, dot));

  vtkNew<vtkTransform> toYAlignedAnnulus;
  toYAlignedAnnulus->Identity();
  toYAlignedAnnulus->PostMultiply();

  vtkNew<vtkTransform> toWidgetBasis;
  toWidgetBasis->Identity();
  toWidgetBasis->Translate(center.GetData());
  toWidgetBasis->RotateWXYZ(yAxisToAnnulusAxisAngle, cross.GetData());

  // Create annulus points
  vtkPoints* annulusPoints = this->AnnulusPD->GetPoints();
  annulusPoints->SetNumberOfPoints(numberOfPointsPerCylinder * 2.);

  auto createPoints = [&](vtkIdType idx, const CylinderInfo& cylinder)
  {
    vtkVector3d topPoint(cylinder.Radius, height, 0.);
    toYAlignedAnnulus->TransformPoint(topPoint.GetData(), topPoint.GetData());
    toWidgetBasis->TransformPoint(topPoint.GetData(), topPoint.GetData());
    annulusPoints->InsertPoint(idx + cylinder.TopOffset, topPoint.GetData());

    vtkVector3d bottomPoint(cylinder.Radius, -height, 0.);
    toYAlignedAnnulus->TransformPoint(bottomPoint.GetData(), bottomPoint.GetData());
    toWidgetBasis->TransformPoint(bottomPoint.GetData(), bottomPoint.GetData());
    annulusPoints->InsertPoint(idx + cylinder.BottomOffset, bottomPoint.GetData());
  };

  for (vtkIdType pointId = 0; pointId < this->Resolution; ++pointId)
  {
    toYAlignedAnnulus->RotateWXYZ(deltaRadiusAngle, yAxis.GetData());

    createPoints(pointId, inner);
    createPoints(pointId, outer);
  }

  // Clamp annulus points to the bounding box
  double bounds[6];
  this->GetOutlineBounds(bounds);
  const vtkBoundingBox bbox(bounds);
  vtkVector3d boundsCenter;
  bbox.GetCenter(boundsCenter.GetData());

  auto clampPointsToBoundingBox = [&](CylinderInfo& cylinder)
  {
    for (vtkIdType pointIdx = 0; pointIdx < numberOfPointsPerCylinderSide; ++pointIdx)
    {
      vtkVector3d bottomPoint, topPoint;
      annulusPoints->GetPoint(cylinder.BottomOffset + pointIdx, bottomPoint.GetData());
      annulusPoints->GetPoint(cylinder.TopOffset + pointIdx, topPoint.GetData());

      int plane1, plane2;
      vtkVector3d x1, x2;
      double t1, t2;

      bool intersect = vtkBox::IntersectWithLine(bounds, bottomPoint.GetData(), topPoint.GetData(),
        t1, t2, x1.GetData(), x2.GetData(), plane1, plane2);

      cylinder.EdgeInsideBoundingBox[pointIdx] = intersect;

      if (intersect)
      {
        annulusPoints->SetPoint(cylinder.BottomOffset + pointIdx, x1.GetData());
        annulusPoints->SetPoint(cylinder.TopOffset + pointIdx, x2.GetData());
      }
    }
  };

  clampPointsToBoundingBox(inner);
  clampPointsToBoundingBox(outer);

  // Create annulus polys
  vtkCellArray* polys = this->AnnulusPD->GetPolys();

  auto buildCylinderPolys = [&](const CylinderInfo& cylinder, vtkPolyData* edgesPD)
  {
    // Copy cylinder points to edge polydata
    edgesPD->Reset();
    vtkPoints* edgePoints = edgesPD->GetPoints();
    edgePoints->InsertPoints(0, numberOfPointsPerCylinder, cylinder.StartOffset, annulusPoints);

    vtkCellArray* edgeLines = edgesPD->GetLines();

    for (vtkIdType i = 0; i < numberOfPointsPerCylinderSide - 1; ++i)
    {
      if (cylinder.EdgeInsideBoundingBox[i] && cylinder.EdgeInsideBoundingBox[i + 1])
      {
        polys->InsertNextCell(4);
        polys->InsertCellPoint(i + cylinder.TopOffset);
        polys->InsertCellPoint(i + cylinder.TopOffset + 1);
        polys->InsertCellPoint(i + cylinder.BottomOffset + 1);
        polys->InsertCellPoint(i + cylinder.BottomOffset);

        edgeLines->InsertNextCell({ i, i + 1 });
        edgeLines->InsertNextCell(
          { numberOfPointsPerCylinderSide + i, numberOfPointsPerCylinderSide + i + 1 });
      }
    }

    if (cylinder.EdgeInsideBoundingBox.back() && cylinder.EdgeInsideBoundingBox.front())
    {
      // Last cell must loop back to 0
      polys->InsertNextCell(4);
      polys->InsertCellPoint(cylinder.TopOffset + numberOfPointsPerCylinderSide - 1);
      polys->InsertCellPoint(cylinder.TopOffset);
      polys->InsertCellPoint(cylinder.BottomOffset);
      polys->InsertCellPoint(cylinder.BottomOffset + numberOfPointsPerCylinderSide - 1);

      edgeLines->InsertNextCell({ numberOfPointsPerCylinderSide - 1, 0 });
      edgeLines->InsertNextCell(
        { (numberOfPointsPerCylinderSide * 2) - 1, numberOfPointsPerCylinderSide });
    }
  };

  buildCylinderPolys(inner, this->InnerRadiusRepresentation.PolyData);
  buildCylinderPolys(outer, this->OuterRadiusRepresentation.PolyData);

  this->AnnulusPD->Modified();
  this->InnerRadiusRepresentation.PolyData->Modified();
  this->OuterRadiusRepresentation.PolyData->Modified();
}

//------------------------------------------------------------------------------
void vtkImplicitAnnulusRepresentation::RegisterPickers()
{
  vtkPickingManager* pm = this->GetPickingManager();
  if (!pm)
  {
    return;
  }
  pm->AddPicker(this->Picker, this);
}

//------------------------------------------------------------------------------
void vtkImplicitAnnulusRepresentation::GetAnnulus(vtkAnnulus* annulus) const
{
  if (annulus == nullptr)
  {
    return;
  }

  // This class represents a one-sided annulus
  annulus->SetAxis(this->Annulus->GetAxis());
  annulus->SetInnerRadius(this->Annulus->GetInnerRadius());
  annulus->SetOuterRadius(this->Annulus->GetOuterRadius());
  annulus->SetCenter(this->Annulus->GetCenter());
  annulus->SetTransform(this->Annulus->GetTransform());
}

VTK_ABI_NAMESPACE_END
