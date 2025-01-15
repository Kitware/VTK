// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkResliceCursorLineRepresentation.h"
#include "vtkActor2D.h"
#include "vtkBoundingBox.h"
#include "vtkCamera.h"
#include "vtkCoordinate.h"
#include "vtkHandleRepresentation.h"
#include "vtkImageActor.h"
#include "vtkImageData.h"
#include "vtkImageReslice.h"
#include "vtkInteractorObserver.h"
#include "vtkLine.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPlane.h"
#include "vtkPlaneSource.h"
#include "vtkPointHandleRepresentation2D.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkProperty2D.h"
#include "vtkRenderer.h"
#include "vtkResliceCursor.h"
#include "vtkResliceCursorActor.h"
#include "vtkResliceCursorPicker.h"
#include "vtkResliceCursorPolyDataAlgorithm.h"
#include "vtkSmartPointer.h"
#include "vtkTextActor.h"
#include "vtkTextMapper.h"
#include "vtkTextProperty.h"
#include "vtkTransform.h"
#include "vtkWindow.h"

#include <array>
#include <sstream>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkResliceCursorLineRepresentation);

//------------------------------------------------------------------------------
vtkResliceCursorLineRepresentation::vtkResliceCursorLineRepresentation()
{
  this->ResliceCursorActor = vtkResliceCursorActor::New();

  this->Picker = vtkResliceCursorPicker::New();
  this->ApplyTolerance();

  this->MatrixReslice = vtkMatrix4x4::New();
  this->MatrixView = vtkMatrix4x4::New();
  this->MatrixReslicedView = vtkMatrix4x4::New();
}

//------------------------------------------------------------------------------
vtkResliceCursorLineRepresentation::~vtkResliceCursorLineRepresentation()
{
  this->ResliceCursorActor->Delete();
  this->Picker->Delete();
  this->MatrixReslice->Delete();
  this->MatrixView->Delete();
  this->MatrixReslicedView->Delete();
}

//------------------------------------------------------------------------------
vtkResliceCursor* vtkResliceCursorLineRepresentation::GetResliceCursor()
{
  return this->ResliceCursorActor->GetCursorAlgorithm()->GetResliceCursor();
}

//------------------------------------------------------------------------------
int vtkResliceCursorLineRepresentation::ComputeInteractionState(int X, int Y, int modify)
{
  this->InteractionState = vtkResliceCursorLineRepresentation::Outside;

  if (!this->Renderer)
  {
    return this->InteractionState;
  }

  vtkResliceCursor* rc = this->GetResliceCursor();
  if (!rc)
  {
    vtkErrorMacro(<< "Reslice cursor not set!");
    return this->InteractionState;
  }

  this->Modifier = modify;

  // Ensure that the axis is initialized..
  const int axis1 = this->ResliceCursorActor->GetCursorAlgorithm()->GetAxis1();
  double bounds[6];
  this->ResliceCursorActor->GetCenterlineActor(axis1)->GetBounds(bounds);
  if (bounds[1] < bounds[0])
  {
    return this->InteractionState;
  }

  // Pick
  this->Picker->SetResliceCursorAlgorithm(this->ResliceCursorActor->GetCursorAlgorithm());
  int picked = this->Picker->Pick(X, Y, 0, this->Renderer);

  const bool pickedAxis1 = this->Picker->GetPickedAxis1() != 0;
  const bool pickedAxis2 = this->Picker->GetPickedAxis2() != 0;
  const bool pickedCenter = this->Picker->GetPickedCenter() != 0;
  if (picked)
  {
    this->Picker->GetPickPosition(this->StartPickPosition);
  }

  // Now assign the interaction state

  if (pickedCenter)
  {
    this->InteractionState = vtkResliceCursorLineRepresentation::OnCenter;
  }
  else if (pickedAxis1)
  {
    this->InteractionState = vtkResliceCursorLineRepresentation::OnAxis1;
  }
  else if (pickedAxis2)
  {
    this->InteractionState = vtkResliceCursorLineRepresentation::OnAxis2;
  }

  return this->InteractionState;
}

//------------------------------------------------------------------------------
// Record the current event position, and the center position.
void vtkResliceCursorLineRepresentation ::StartWidgetInteraction(double startEventPos[2])
{
  this->StartEventPosition[0] = startEventPos[0];
  this->StartEventPosition[1] = startEventPos[1];

  if (this->ManipulationMode == WindowLevelling)
  {
    this->InitialWindow = this->CurrentWindow;
    this->InitialLevel = this->CurrentLevel;
  }
  else
  {
    if (vtkResliceCursor* rc = this->GetResliceCursor())
    {
      rc->GetCenter(this->StartCenterPosition);
    }
  }

  this->LastEventPosition[0] = startEventPos[0];
  this->LastEventPosition[1] = startEventPos[1];
}

//------------------------------------------------------------------------------
void vtkResliceCursorLineRepresentation::WidgetInteraction(double e[2])
{
  vtkResliceCursor* rc = this->GetResliceCursor();

  if (this->ManipulationMode == WindowLevelling)
  {
    this->WindowLevel(e[0], e[1]);
    this->LastEventPosition[0] = e[0];
    this->LastEventPosition[1] = e[1];
    return;
  }

  // Depending on the state, different motions are allowed.

  if (this->InteractionState == Outside || !this->Renderer || !rc)
  {
    this->LastEventPosition[0] = e[0];
    this->LastEventPosition[1] = e[1];
    return;
  }

  if (rc->GetThickMode() &&
    this->ManipulationMode == vtkResliceCursorRepresentation::ResizeThickness)
  {

    double sf = 1.0;

    // Compute the scale factor
    const int* size = this->Renderer->GetSize();
    double dPos = e[1] - this->LastEventPosition[1];
    sf *= (1.0 + 2.0 * (dPos / size[1])); // scale factor of 2.0 is arbitrary
    sf = sf < 0.0 ? 1.0 : sf; // prevent negative thickness with huge movement outside the window

    std::array<double, 3> scale = { 1.0, 1.0, 1.0 };

    if (this->IndependentThickness)
    {
      int axis = this->GetCursorAlgorithm()->GetReslicePlaneNormal();
      axis = this->InteractionState == OnAxis1 ? this->GetCursorAlgorithm()->GetPlaneAxis1() : axis;
      axis = this->InteractionState == OnAxis2 ? this->GetCursorAlgorithm()->GetPlaneAxis2() : axis;
      scale[axis] = sf;
    }
    else
    {
      scale.fill(sf);
    }

    double thickness[3];
    rc->GetThickness(thickness);
    rc->SetThickness(thickness[0] * scale[0], thickness[1] * scale[1], thickness[2] * scale[2]);

    this->LastEventPosition[0] = e[0];
    this->LastEventPosition[1] = e[1];

    return;
  }

  // depending on the state, perform different operations
  //
  // 1. Translation

  if (this->InteractionState == OnCenter)
  {

    // Intersect with the viewing vector. We will use this point and the
    // start event point to compute an offset vector to translate the
    // center by.

    double intersectionPos[3], newCenter[3];
    this->Picker->Pick(e, intersectionPos, this->Renderer);

    // Offset the center by this vector.

    for (int i = 0; i < 3; i++)
    {
      newCenter[i] = this->StartCenterPosition[i] + intersectionPos[i] - this->StartPickPosition[i];
    }

    rc->SetCenter(newCenter);
  }

  // 2. Rotation of axis 1

  if (this->InteractionState == OnAxis1 &&
    this->ManipulationMode == vtkResliceCursorRepresentation::PanAndRotate)
  {
    this->RotateAxis(e, this->ResliceCursorActor->GetCursorAlgorithm()->GetPlaneAxis1());
  }

  // 3. Rotation of axis 2

  if (this->InteractionState == OnAxis2 &&
    this->ManipulationMode == vtkResliceCursorRepresentation::PanAndRotate)
  {
    this->RotateAxis(e, this->ResliceCursorActor->GetCursorAlgorithm()->GetPlaneAxis2());
  }

  // 4. Rotation of both axes

  if ((this->InteractionState == OnAxis2 || this->InteractionState == OnAxis1) &&
    this->ManipulationMode == vtkResliceCursorRepresentation::RotateBothAxes)
  {
    // Rotate both by the same angle
    const double angle =
      this->RotateAxis(e, this->ResliceCursorActor->GetCursorAlgorithm()->GetPlaneAxis1());
    this->RotateAxis(this->ResliceCursorActor->GetCursorAlgorithm()->GetPlaneAxis2(), angle);
  }

  // 5. Translation of axis 1
  if (this->InteractionState == OnAxis1 &&
    this->ManipulationMode == vtkResliceCursorRepresentation::TranslateSingleAxis)
  {
    this->TranslateAxis(e, this->GetResliceCursorActor()->GetCursorAlgorithm()->GetPlaneAxis1());
  }

  // 6. Translation of axis 2
  if (this->InteractionState == OnAxis2 &&
    this->ManipulationMode == vtkResliceCursorRepresentation::TranslateSingleAxis)
  {
    this->TranslateAxis(e, this->GetResliceCursorActor()->GetCursorAlgorithm()->GetPlaneAxis2());
  }

  this->LastEventPosition[0] = e[0];
  this->LastEventPosition[1] = e[1];
}

//------------------------------------------------------------------------------
double vtkResliceCursorLineRepresentation::TranslateAxis(double e[2], int axis)
{
  // Intersect with the viewing vector. We will use this point and the
  // start event point to compute an offset vector to translate the
  // center by.

  double intersectionPos[3], newCenter[3], move_value[3], currentPlaneNormal[3];
  this->Picker->Pick(e, intersectionPos, this->Renderer);

  // Offset the center by this vector.
  vtkPlane* normalPlane = this->GetResliceCursor()->GetPlane(axis);
  normalPlane->GetNormal(currentPlaneNormal);
  vtkMath::Subtract(intersectionPos, this->StartCenterPosition, move_value);
  double distance = vtkMath::Dot(currentPlaneNormal, move_value);
  newCenter[0] = this->StartCenterPosition[0] + (currentPlaneNormal[0] * distance);
  newCenter[1] = this->StartCenterPosition[1] + (currentPlaneNormal[1] * distance);
  newCenter[2] = this->StartCenterPosition[2] + (currentPlaneNormal[2] * distance);
  this->GetResliceCursor()->SetCenter(newCenter);
  return distance;
}

//------------------------------------------------------------------------------
double vtkResliceCursorLineRepresentation ::RotateAxis(double e[2], int axis)
{
  vtkResliceCursor* rc = this->GetResliceCursor();

  double center[3];
  rc->GetCenter(center);

  // Intersect with the viewing vector. We will use this point and the
  // start event point to compute the rotation angle

  double currIntersectionPos[3], lastIntersectionPos[3];
  this->DisplayToReslicePlaneIntersection(e, currIntersectionPos);
  this->DisplayToReslicePlaneIntersection(this->LastEventPosition, lastIntersectionPos);

  if (lastIntersectionPos[0] == currIntersectionPos[0] &&
    lastIntersectionPos[1] == currIntersectionPos[1] &&
    lastIntersectionPos[2] == currIntersectionPos[2])
  {
    return 0;
  }

  double lastVector[3], currVector[3];
  for (int i = 0; i < 3; i++)
  {
    lastVector[i] = lastIntersectionPos[i] - center[i];
    currVector[i] = currIntersectionPos[i] - center[i];
  }

  vtkMath::Normalize(lastVector);
  vtkMath::Normalize(currVector);

  // compute the angle between both vectors. This is the amount to
  // rotate by.
  double angle = acos(vtkMath::Dot(lastVector, currVector));
  double crossVector[3];
  vtkMath::Cross(lastVector, currVector, crossVector);

  double aboutAxis[3];
  const int rcPlaneIdx = this->ResliceCursorActor->GetCursorAlgorithm()->GetReslicePlaneNormal();
  vtkPlane* normalPlane = rc->GetPlane(rcPlaneIdx);
  normalPlane->GetNormal(aboutAxis);
  const double align = vtkMath::Dot(aboutAxis, crossVector);
  const double sign = align > 0 ? 1.0 : -1.0;
  angle *= sign;

  if (angle == 0)
  {
    return 0;
  }

  this->RotateAxis(axis, angle);

  return angle;
}

//------------------------------------------------------------------------------
// Get the plane normal to the viewing axis.
vtkResliceCursorPolyDataAlgorithm* vtkResliceCursorLineRepresentation::GetCursorAlgorithm()
{
  return this->ResliceCursorActor->GetCursorAlgorithm();
}

//------------------------------------------------------------------------------
void vtkResliceCursorLineRepresentation ::RotateAxis(int axis, double angle)
{
  vtkResliceCursor* rc = this->GetResliceCursor();
  vtkPlane* planeToBeRotated = rc->GetPlane(axis);
  double* viewUp = rc->GetViewUp(axis);

  const int rcPlaneIdx = this->ResliceCursorActor->GetCursorAlgorithm()->GetReslicePlaneNormal();

  vtkPlane* normalPlane = rc->GetPlane(rcPlaneIdx);

  double vectorToBeRotated[3], aboutAxis[3], rotatedVector[3];
  planeToBeRotated->GetNormal(vectorToBeRotated);
  normalPlane->GetNormal(aboutAxis);

  this->RotateVectorAboutVector(vectorToBeRotated, aboutAxis, angle, rotatedVector);
  this->RotateVectorAboutVector(viewUp, aboutAxis, angle, viewUp);
  planeToBeRotated->SetNormal(rotatedVector);
}

//------------------------------------------------------------------------------
void vtkResliceCursorLineRepresentation ::RotateVectorAboutVector(double vectorToBeRotated[3],
  double axis[3], // vector about which we rotate
  double angle,   // angle in radians
  double o[3])
{
  vtkNew<vtkTransform> transform;
  transform->RotateWXYZ(vtkMath::DegreesFromRadians(angle), axis);
  transform->TransformVector(vectorToBeRotated, o);
}

//------------------------------------------------------------------------------
int vtkResliceCursorLineRepresentation ::DisplayToReslicePlaneIntersection(
  double displayPos[2], double intersectionPos[3])
{
  // First compute the equivalent of this display point on the focal plane
  double fp[4], tmp1[4], camPos[4], eventFPpos[4];
  this->Renderer->GetActiveCamera()->GetFocalPoint(fp);
  this->Renderer->GetActiveCamera()->GetPosition(camPos);
  fp[3] = 1.0;
  this->Renderer->SetWorldPoint(fp);
  this->Renderer->WorldToDisplay();
  this->Renderer->GetDisplayPoint(tmp1);

  tmp1[0] = displayPos[0];
  tmp1[1] = displayPos[1];
  this->Renderer->SetDisplayPoint(tmp1);
  this->Renderer->DisplayToWorld();
  this->Renderer->GetWorldPoint(eventFPpos);

  const int rcPlaneIdx = this->ResliceCursorActor->GetCursorAlgorithm()->GetReslicePlaneNormal();
  vtkPlane* normalPlane = this->GetResliceCursor()->GetPlane(rcPlaneIdx);

  double t;

  return normalPlane->IntersectWithLine(eventFPpos, camPos, t, intersectionPos);
}

//------------------------------------------------------------------------------
void vtkResliceCursorLineRepresentation::BuildRepresentation()
{
  if (this->GetMTime() > this->BuildTime ||
    this->GetResliceCursor()->GetMTime() > this->BuildTime ||
    (this->Renderer && this->Renderer->GetVTKWindow() &&
      this->Renderer->GetVTKWindow()->GetMTime() > this->BuildTime))
  {
    this->Superclass::BuildRepresentation();
    this->BuildTime.Modified();
  }

  if (this->Renderer)
  {
    const int planeOrientation = this->GetCursorAlgorithm()->GetReslicePlaneNormal();
    double* viewUp = this->GetResliceCursor()->GetViewUp(planeOrientation);
    this->Renderer->GetActiveCamera()->GetViewUp(viewUp);
  }
}

//------------------------------------------------------------------------------
void vtkResliceCursorLineRepresentation::ReleaseGraphicsResources(vtkWindow* w)
{
  this->ResliceCursorActor->ReleaseGraphicsResources(w);
  this->TexturePlaneActor->ReleaseGraphicsResources(w);
  this->ImageActor->ReleaseGraphicsResources(w);
  this->TextActor->ReleaseGraphicsResources(w);
}

//------------------------------------------------------------------------------
int vtkResliceCursorLineRepresentation::RenderOverlay(vtkViewport* viewport)
{
  int count = 0;

  if (this->TexturePlaneActor->GetVisibility() && !this->UseImageActor)
  {
    count += this->TexturePlaneActor->RenderOverlay(viewport);
  }
  if (this->ImageActor->GetVisibility() && this->UseImageActor)
  {
    count += this->ImageActor->RenderOverlay(viewport);
  }
  if (this->DisplayText && this->TextActor->GetVisibility())
  {
    count += this->TextActor->RenderOverlay(viewport);
  }

  return count;
}

//------------------------------------------------------------------------------
void vtkResliceCursorLineRepresentation::SetUserMatrix(vtkMatrix4x4* m)
{
  this->TexturePlaneActor->SetUserMatrix(m);
  this->ResliceCursorActor->SetUserMatrix(m);
}

//------------------------------------------------------------------------------
void vtkResliceCursorLineRepresentation::SetTolerance(int t)
{
  this->Superclass::SetTolerance(t);
  this->ApplyTolerance();
}

//------------------------------------------------------------------------------
void vtkResliceCursorLineRepresentation::ApplyTolerance()
{
  // Tolerance is clamped to 100 in superclass. Picker expects tolerance values
  // between 0.0 and 1.0 (fraction of the window size)
  // dividing by 200.0 to allow specifying tolerance smaller than 0.01
  this->Picker->SetTolerance(this->Tolerance / 200.0);
}

//------------------------------------------------------------------------------
int vtkResliceCursorLineRepresentation ::RenderOpaqueGeometry(vtkViewport* viewport)
{
  this->BuildRepresentation();

  // Render all the actors.

  int count = 0;
  if (this->TexturePlaneActor->GetVisibility() && !this->UseImageActor)
  {
    count += this->TexturePlaneActor->RenderOpaqueGeometry(viewport);
  }
  if (this->ImageActor->GetVisibility() && this->UseImageActor)
  {
    count += this->ImageActor->RenderOpaqueGeometry(viewport);
  }
  count += this->ResliceCursorActor->RenderOpaqueGeometry(viewport);
  if (this->DisplayText && this->TextActor->GetVisibility())
  {
    count += this->TextActor->RenderOpaqueGeometry(viewport);
  }

  return count;
}

//------------------------------------------------------------------------------
// Get the bounds for this Actor as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
double* vtkResliceCursorLineRepresentation::GetBounds()
{
  vtkMath::UninitializeBounds(this->InitialBounds);

  if (vtkResliceCursor* r = this->GetResliceCursor())
  {
    r->GetImage()->GetBounds(this->InitialBounds);
  }

  // vtkBoundingBox *bb = new vtkBoundingBox();
  // bb->AddBounds(this->ResliceCursorActor->GetBounds());
  // bb->AddBounds(this->TexturePlaneActor->GetBounds());
  // bb->GetBounds(bounds);
  // delete bb;

  return this->InitialBounds;
}

//------------------------------------------------------------------------------
int vtkResliceCursorLineRepresentation::RenderTranslucentPolygonalGeometry(vtkViewport* viewport)
{
  int count = 0;
  if (this->TexturePlaneActor->GetVisibility() && !this->UseImageActor)
  {
    count += this->TexturePlaneActor->RenderTranslucentPolygonalGeometry(viewport);
  }

  if (this->ImageActor->GetVisibility() && this->UseImageActor)
  {
    count += this->ImageActor->RenderTranslucentPolygonalGeometry(viewport);
  }

  count += this->ResliceCursorActor->RenderTranslucentPolygonalGeometry(viewport);

  return count;
}

//------------------------------------------------------------------------------
vtkTypeBool vtkResliceCursorLineRepresentation::HasTranslucentPolygonalGeometry()
{
  return (this->ResliceCursorActor->HasTranslucentPolygonalGeometry() ||
           (this->ImageActor->HasTranslucentPolygonalGeometry() && this->UseImageActor) ||
           (this->TexturePlaneActor->HasTranslucentPolygonalGeometry() && !this->UseImageActor))
    ? 1
    : 0;
}

//------------------------------------------------------------------------------
void vtkResliceCursorLineRepresentation::Highlight(int) {}

//------------------------------------------------------------------------------
void vtkResliceCursorLineRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  // Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ResliceCursorActor: " << this->ResliceCursorActor << "\n";
  if (this->ResliceCursorActor)
  {
    this->ResliceCursorActor->PrintSelf(os, indent);
  }

  os << indent << "Picker: " << this->Picker << "\n";
  if (this->Picker)
  {
    this->Picker->PrintSelf(os, indent);
  }

  os << indent << "MatrixReslicedView: " << this->MatrixReslicedView << "\n";
  if (this->MatrixReslicedView)
  {
    this->MatrixReslicedView->PrintSelf(os, indent);
  }

  os << indent << "MatrixView: " << this->MatrixView << "\n";
  if (this->MatrixView)
  {
    this->MatrixView->PrintSelf(os, indent);
  }

  os << indent << "MatrixReslice: " << this->MatrixReslice << "\n";
  if (this->MatrixReslice)
  {
    this->MatrixReslice->PrintSelf(os, indent);
  }

  // this->StartPickPosition;
  // this->StartCenterPosition;
}
VTK_ABI_NAMESPACE_END
