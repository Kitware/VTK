// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAngleRepresentation2D.h"
#include "vtkCoordinate.h"
#include "vtkInteractorObserver.h"
#include "vtkLeaderActor2D.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointHandleRepresentation2D.h"
#include "vtkRenderer.h"
#include "vtkVector.h"
#include "vtkVectorOperators.h"
#include "vtkWindow.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkAngleRepresentation2D);

//------------------------------------------------------------------------------
vtkAngleRepresentation2D::vtkAngleRepresentation2D()
{
  // By default, use one of these handles
  this->HandleRepresentation = vtkPointHandleRepresentation2D::New();

  this->Ray1->GetPositionCoordinate()->SetCoordinateSystemToWorld();
  this->Ray1->GetPosition2Coordinate()->SetCoordinateSystemToWorld();
  this->Ray1->SetArrowStyleToOpen();
  this->Ray1->SetArrowPlacementToPoint2();

  this->Ray2->GetPositionCoordinate()->SetCoordinateSystemToWorld();
  this->Ray2->GetPosition2Coordinate()->SetCoordinateSystemToWorld();
  this->Ray2->SetArrowStyleToOpen();
  this->Ray2->SetArrowPlacementToPoint2();

  this->Arc->GetPositionCoordinate()->SetCoordinateSystemToWorld();
  this->Arc->GetPosition2Coordinate()->SetCoordinateSystemToWorld();
  this->Arc->SetArrowPlacementToNone();
  this->Arc->SetLabel("Angle");
  this->Arc->SetLabelFormat(this->LabelFormat);
}

//------------------------------------------------------------------------------
vtkAngleRepresentation2D::~vtkAngleRepresentation2D() = default;

//------------------------------------------------------------------------------
double vtkAngleRepresentation2D::GetAngle()
{
  return this->Arc->GetAngle();
}

//------------------------------------------------------------------------------
void vtkAngleRepresentation2D::GetPoint1WorldPosition(double pos[3])
{
  if (this->Point1Representation)
  {
    this->Point1Representation->GetWorldPosition(pos);
  }
  else
  {
    pos[0] = pos[1] = pos[2] = 0.0;
  }
}

//------------------------------------------------------------------------------
void vtkAngleRepresentation2D::GetCenterWorldPosition(double pos[3])
{
  if (this->CenterRepresentation)
  {
    this->CenterRepresentation->GetWorldPosition(pos);
  }
  else
  {
    pos[0] = pos[1] = pos[2] = 0.0;
  }
}

//------------------------------------------------------------------------------
void vtkAngleRepresentation2D::GetPoint2WorldPosition(double pos[3])
{
  if (this->Point2Representation)
  {
    this->Point2Representation->GetWorldPosition(pos);
  }
  else
  {
    pos[0] = pos[1] = pos[2] = 0.0;
  }
}

//------------------------------------------------------------------------------
void vtkAngleRepresentation2D::SetPoint1DisplayPosition(double x[3])
{
  if (!this->Point1Representation)
  {
    vtkErrorMacro("SetPoint1DisplayPosition: no point1 representation");
    return;
  }
  this->Point1Representation->SetDisplayPosition(x);
  double p[3];
  this->Point1Representation->GetWorldPosition(p);
  this->Point1Representation->SetWorldPosition(p);
  this->BuildRepresentation();
}

//------------------------------------------------------------------------------
void vtkAngleRepresentation2D::SetPoint1WorldPosition(double x[3])
{
  if (!this->Point1Representation)
  {
    vtkErrorMacro("SetPoint1DisplayPosition: no point1 representation");
    return;
  }
  this->Point1Representation->SetWorldPosition(x);
  this->BuildRepresentation();
}

//------------------------------------------------------------------------------
void vtkAngleRepresentation2D::SetCenterDisplayPosition(double x[3])
{
  if (!this->CenterRepresentation)
  {
    vtkErrorMacro("SetCenterDisplayPosition: no center representation");
    return;
  }
  this->CenterRepresentation->SetDisplayPosition(x);
  double p[3];
  this->CenterRepresentation->GetWorldPosition(p);
  this->CenterRepresentation->SetWorldPosition(p);
  this->BuildRepresentation();
}

//------------------------------------------------------------------------------
void vtkAngleRepresentation2D::SetCenterWorldPosition(double x[3])
{
  if (!this->CenterRepresentation)
  {
    vtkErrorMacro("SetCenterWorldPosition: no center representation");
    return;
  }
  this->CenterRepresentation->SetWorldPosition(x);
  this->BuildRepresentation();
}

//------------------------------------------------------------------------------
void vtkAngleRepresentation2D::SetPoint2DisplayPosition(double x[3])
{
  if (!this->Point2Representation)
  {
    vtkErrorMacro("SetPoint2DisplayPosition: no point2 representation");
    return;
  }
  this->Point2Representation->SetDisplayPosition(x);
  double p[3];
  this->Point2Representation->GetWorldPosition(p);
  this->Point2Representation->SetWorldPosition(p);
  this->BuildRepresentation();
}

//------------------------------------------------------------------------------
void vtkAngleRepresentation2D::SetPoint2WorldPosition(double x[3])
{
  if (!this->Point2Representation)
  {
    vtkErrorMacro("SetPoint2DisplayPosition: no point2 representation");
    return;
  }
  this->Point2Representation->SetWorldPosition(x);
  this->BuildRepresentation();
}

//------------------------------------------------------------------------------
void vtkAngleRepresentation2D::GetPoint1DisplayPosition(double pos[3])
{
  if (this->Point1Representation)
  {
    this->Point1Representation->GetDisplayPosition(pos);
    pos[2] = 0.0;
  }
  else
  {
    pos[0] = pos[1] = pos[2] = 0.0;
  }
}

//------------------------------------------------------------------------------
void vtkAngleRepresentation2D::GetCenterDisplayPosition(double pos[3])
{
  if (this->CenterRepresentation)
  {
    this->CenterRepresentation->GetDisplayPosition(pos);
    pos[2] = 0.0;
  }
  else
  {
    pos[0] = pos[1] = pos[2] = 0.0;
  }
}

//------------------------------------------------------------------------------
void vtkAngleRepresentation2D::GetPoint2DisplayPosition(double pos[3])
{
  if (this->Point2Representation)
  {
    this->Point2Representation->GetDisplayPosition(pos);
    pos[2] = 0.0;
  }
  else
  {
    pos[0] = pos[1] = pos[2] = 0.0;
  }
}

//------------------------------------------------------------------------------
void vtkAngleRepresentation2D::BuildRepresentation()
{
  if (this->Point1Representation == nullptr || this->CenterRepresentation == nullptr ||
    this->Point2Representation == nullptr)
  {
    // for now, return. Could create defaults here.
    return;
  }

  if (!(this->GetMTime() > this->BuildTime ||
        this->Point1Representation->GetMTime() > this->BuildTime ||
        this->CenterRepresentation->GetMTime() > this->BuildTime ||
        this->Point2Representation->GetMTime() > this->BuildTime ||
        (this->Renderer && this->Renderer->GetVTKWindow() &&
          this->Renderer->GetVTKWindow()->GetMTime() > this->BuildTime)))
  {
    return;
  }

  this->Superclass::BuildRepresentation();

  // Local coordinate values
  vtkVector3d p1w, p2w, cw;
  this->GetPoint1WorldPosition(p1w.GetData());
  this->GetCenterWorldPosition(cw.GetData());
  this->GetPoint2WorldPosition(p2w.GetData());

  // Update the rays
  this->Ray1->GetPositionCoordinate()->SetValue(cw.GetData());
  this->Ray1->GetPosition2Coordinate()->SetValue(p1w.GetData());
  this->Ray2->GetPositionCoordinate()->SetValue(cw.GetData());
  this->Ray2->GetPosition2Coordinate()->SetValue(p2w.GetData());

  // Compute the angle.
  // NOTE: There is some concern that there may be fluctuations in the angle
  // value as the camera moves, etc. This calculation may have to be dampened.

  vtkVector3d vector1 = p1w - cw;
  vtkVector3d vector2 = p2w - cw;
  const double normV1 = vector1.Normalize();
  const double normV2 = vector2.Normalize();
  const double angle = vtkMath::DegreesFromRadians(std::acos(vector1.Dot(vector2))) * this->Scale;

  // Construct label
  const int size_s = std::snprintf(nullptr, 0, this->LabelFormat, angle) + 1;
  if (size_s <= 0)
  {
    this->ArcVisibility = 0;
    vtkWarningMacro("Couldn't format label.");
    return;
  }
  char* string = new char[size_s];
  std::snprintf(string, size_s, this->LabelFormat, angle);
  this->Arc->SetLabel(string);
  delete[] string;

  // Place the label and place the arc
  vtkVector3d p1d, p2d, cd;
  this->GetPoint1DisplayPosition(p1d.GetData());
  this->GetCenterDisplayPosition(cd.GetData());
  this->GetPoint2DisplayPosition(p2d.GetData());
  const double l1 = (cd - p1d).Norm();
  const double l2 = (cd - p2d).Norm();

  // If too small (pixel-wise) or no renderer get out
  if (l1 <= 5.0 || l2 <= 5.0 || this->Renderer == nullptr)
  {
    this->ArcVisibility = 0;
    return;
  }

  // Place the end points for the arc away from the tip of the two rays
  this->ArcVisibility = 1;
  this->Arc->SetLabelFormat(this->LabelFormat);
  constexpr double rayPosition = 0.80;
  double t1, t2, radius;
  if (l1 < l2)
  {
    radius = rayPosition * l1;
    t1 = rayPosition;
    t2 = (l1 / l2) * rayPosition;
  }
  else
  {
    radius = rayPosition * l2;
    t1 = (l2 / l1) * rayPosition;
    t2 = rayPosition;
  }

  const vtkVector3d ray1 = p1d - cd;
  const vtkVector3d ray2 = p2d - cd;
  const vtkVector3d a1 = cd + t1 * ray1;
  const vtkVector3d a2 = cd + t2 * ray2;

  vtkVector3d w1, w2;
  if (this->Force3DArcPlacement)
  {
    const double dist = std::min(normV1, normV2);
    w1 = cw + 0.5 * dist * vector1;
    w2 = cw + 0.5 * dist * vector2;
  }
  else
  {
    double w1b[4], w2b[4];
    vtkInteractorObserver::ComputeDisplayToWorld(this->Renderer, a1[0], a1[1], a1[2], w1b);
    vtkInteractorObserver::ComputeDisplayToWorld(this->Renderer, a2[0], a2[1], a2[2], w2b);
    w1 = { w1b[0] / w1b[3], w1b[1] / w1b[3], w1b[2] / w1b[3] };
    w2 = { w2b[0] / w2b[3], w2b[1] / w2b[3], w2b[2] / w2b[3] };
  }
  this->Arc->GetPositionCoordinate()->SetValue(w1.GetData());
  this->Arc->GetPosition2Coordinate()->SetValue(w2.GetData());

  const double length = (a1 - a2).Norm();
  if (length <= 0.0)
  {
    this->Arc->SetRadius(0.0);
  }
  else
  {
    const vtkVector3d cross = ray1.Cross(ray2);
    // equivalent to dot against (0,0,1)
    if (cross[2] > 0.0)
    {
      this->Arc->SetRadius(-radius / length);
    }
    else
    {
      this->Arc->SetRadius(radius / length);
    }
  }

  this->BuildTime.Modified();
}

//------------------------------------------------------------------------------
void vtkAngleRepresentation2D::ReleaseGraphicsResources(vtkWindow* w)
{
  this->Ray1->ReleaseGraphicsResources(w);
  this->Ray2->ReleaseGraphicsResources(w);
  this->Arc->ReleaseGraphicsResources(w);
}

//------------------------------------------------------------------------------
int vtkAngleRepresentation2D::RenderOverlay(vtkViewport* v)
{
  this->BuildRepresentation();

  int count = 0;
  if (this->Ray1Visibility)
  {
    count += this->Ray1->RenderOverlay(v);
  }
  if (this->Ray2Visibility)
  {
    count += this->Ray2->RenderOverlay(v);
  }
  if (this->ArcVisibility)
  {
    count += this->Arc->RenderOverlay(v);
  }

  return count;
}

//------------------------------------------------------------------------------
void vtkAngleRepresentation2D::PrintSelf(ostream& os, vtkIndent indent)
{
  // Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Force3DArcPlacement: " << this->Force3DArcPlacement << std::endl;
  os << indent << "Ray1: ";
  this->Ray1->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Ray2: ";
  this->Ray2->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Arc: ";
  this->Arc->PrintSelf(os, indent.GetNextIndent());
}
VTK_ABI_NAMESPACE_END
