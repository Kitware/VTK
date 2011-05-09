/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkControlPointsItem.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkBrush.h"
#include "vtkCallbackCommand.h"
#include "vtkContext2D.h"
#include "vtkContextMouseEvent.h"
#include "vtkContextScene.h"
#include "vtkControlPointsItem.h"
#include "vtkIdTypeArray.h"
#include "vtkObjectFactory.h"
#include "vtkPen.h"
#include "vtkPoints2D.h"
#include "vtkSmartPointer.h"
#include "vtkTransform2D.h"

#include <cassert>
#include <limits>

//-----------------------------------------------------------------------------
vtkControlPointsItem::vtkControlPointsItem()
{
  this->Pen->SetLineType(vtkPen::SOLID_LINE);
  this->Pen->SetWidth(1.);
  this->Pen->SetColorF(1., 1., 1.);
  this->Brush->SetColorF(0.85, 0.85, 1., 0.75);

  this->Selection = vtkIdTypeArray::New();
  this->CurrentPoint = -1;

  this->Callback = vtkCallbackCommand::New();
  this->Callback->SetClientData(this);
  this->Callback->SetCallback(
    vtkControlPointsItem::CallComputePoints);

  this->Bounds[0] = this->Bounds[2] = 0.;
  this->Bounds[1] = this->Bounds[3] = -1.;
  this->UserBounds[0] = this->UserBounds[2] = 0.;
  this->UserBounds[1] = this->UserBounds[3] = -1.;

  this->ScreenPointRadius = 6.f;
  this->Transform = vtkTransform2D::New();

  this->StrokeMode = false;
  this->SwitchPointsMode = false;
  this->MouseMoved = false;
  this->EnforceValidFunction = true;
  this->PointToDelete = -1;
  this->PointAboutToBeDeleted = false;
  this->PointToToggle = -1;
  this->PointAboutToBeToggled = false;
}

//-----------------------------------------------------------------------------
vtkControlPointsItem::~vtkControlPointsItem()
{
  if (this->Callback)
    {
    this->Callback->Delete();
    this->Callback = 0;
    }
  if (this->Transform)
    {
    this->Transform->Delete();
    this->Transform = 0;
    }
}

//-----------------------------------------------------------------------------
void vtkControlPointsItem::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
void vtkControlPointsItem::GetBounds(double bounds[4])
{
  if (this->UserBounds[0] <= this->UserBounds[1] &&
      this->UserBounds[2] <= this->UserBounds[3])
    {
    bounds[0] = this->UserBounds[0];
    bounds[1] = this->UserBounds[1];
    bounds[2] = this->UserBounds[2];
    bounds[3] = this->UserBounds[3];
    return;
    }
  if (this->Bounds[0] > this->Bounds[1] ||
      this->Bounds[2] > this->Bounds[3])
    {
    this->ComputeBounds();
    }
  bounds[0] = this->Bounds[0];
  bounds[1] = this->Bounds[1];
  bounds[2] = this->Bounds[2];
  bounds[3] = this->Bounds[3];
}

//-----------------------------------------------------------------------------
void vtkControlPointsItem::ResetBounds()
{
  this->Bounds[0] = 0.;
  this->Bounds[1] = -1.;
  this->Bounds[2] = 0.;
  this->Bounds[3] = -1.;
}

//-----------------------------------------------------------------------------
void vtkControlPointsItem::ComputeBounds()
{
  this->Bounds[0] = this->Bounds[2] =  VTK_DOUBLE_MAX;
  this->Bounds[1] = this->Bounds[3] = -VTK_DOUBLE_MAX;
  for (vtkIdType i=0; i < this->GetNumberOfPoints(); ++i)
    {
    double point[4];
    this->GetControlPoint(i, point);
    this->Bounds[0] = std::min(this->Bounds[0], point[0]);
    this->Bounds[1] = std::max(this->Bounds[1], point[0]);
    this->Bounds[2] = std::min(this->Bounds[2], point[1]);
    this->Bounds[3] = std::max(this->Bounds[3], point[1]);
    this->Modified();
    }
}

//-----------------------------------------------------------------------------
bool vtkControlPointsItem::Paint(vtkContext2D* painter)
{
  painter->ApplyPen(this->Pen);
  painter->ApplyBrush(this->Brush);
  this->DrawUnselectedPoints(painter);

  painter->GetPen()->SetLineType(vtkPen::SOLID_LINE);
  painter->GetPen()->SetColorF(0.87, 0.87, 1.);
  painter->GetBrush()->SetColorF(0.75, 0.75, 0.95, 0.65);
  float oldPenWidth = painter->GetPen()->GetWidth();
  float oldScreenPointRadius = this->ScreenPointRadius;
  if (this->Selection->GetNumberOfTuples())
    {
    painter->GetPen()->SetWidth(oldPenWidth * 1.4);
    this->ScreenPointRadius = oldScreenPointRadius * 1.1;
    this->DrawSelectedPoints(painter);
    }
  if (this->PointToToggle != -1 && this->PointAboutToBeToggled)
    {
    painter->GetPen()->SetWidth(oldPenWidth);
    this->ScreenPointRadius = oldScreenPointRadius / 1.2;
    this->DrawPoint(painter, this->PointToToggle);
    }
  if (this->PointToDelete != -1 && this->PointAboutToBeDeleted)
    {
    painter->GetPen()->SetColorF(1., 0., 0.);
    painter->GetPen()->SetWidth(oldPenWidth * 2.);
    this->ScreenPointRadius = oldScreenPointRadius * 1.2;
    this->DrawPoint(painter, this->PointToDelete);
    painter->GetPen()->SetColorF(0.87, 0.87, 1.);
    }
  if (this->CurrentPoint != -1 &&
      (!this->PointAboutToBeDeleted || this->CurrentPoint != this->PointToDelete ) &&
      (!this->PointAboutToBeToggled || this->CurrentPoint != this->PointToToggle ))
    {
    painter->GetPen()->SetColorF(0.55, 0.55, 0.75);
    painter->GetBrush()->SetColorF(0.65, 0.65, 0.95, 0.55);
    painter->GetPen()->SetWidth(oldPenWidth * 2.);
    this->ScreenPointRadius = oldScreenPointRadius * 1.2;
    this->DrawPoint(painter, this->CurrentPoint);
    }
  this->ScreenPointRadius = oldScreenPointRadius;
  this->Transform->SetMatrix(painter->GetTransform()->GetMatrix());

  return true;
}

//-----------------------------------------------------------------------------
void vtkControlPointsItem::CallComputePoints(
  vtkObject* vtkNotUsed(sender), unsigned long vtkNotUsed(event),
  void* receiver, void* vtkNotUsed(params))
{
  vtkControlPointsItem* item =
    reinterpret_cast<vtkControlPointsItem*>(receiver);
  item->ComputePoints();
}

//-----------------------------------------------------------------------------
void vtkControlPointsItem::ComputePoints()
{
  if (this->GetNumberOfPoints() == 0)
    {
    this->Selection->SetNumberOfTuples(0);
    }

  const int selectedPointCount = this->Selection->GetNumberOfTuples();
  if (selectedPointCount)
    {
    vtkIdTypeArray* oldSelection = this->Selection;
    this->Selection = vtkIdTypeArray::New();
    for (vtkIdType i = 0; i < selectedPointCount; ++i)
      {
      assert(oldSelection->GetValue(i) < this->GetNumberOfPoints());
      this->SelectPoint(oldSelection->GetValue(i));
      }
    oldSelection->Delete();
    }

  if (this->GetScene())
    {
    this->GetScene()->SetDirty(true);
    }
  this->Modified();
}

//-----------------------------------------------------------------------------
bool vtkControlPointsItem::Hit(const vtkContextMouseEvent &mouse)
{
  double pos[2];
  pos[0] = mouse.Pos[0];
  pos[1] = mouse.Pos[1];
  bool clamped = this->ClampPos(pos);
  if (!clamped)
    {
    return true;
    }
  // maybe the cursor is over the first or last point (which could be outside
  // the bounds because of the screen point size).
  pos[0] = mouse.Pos[0];
  pos[1] = mouse.Pos[1];
  if (this->IsOverPoint(pos, 0) ||
      this->IsOverPoint(pos, this->GetNumberOfPoints() - 1))
    {
    return true;
    }
  return false;
}

//-----------------------------------------------------------------------------
bool vtkControlPointsItem::ClampPos(double pos[2])
{
  double bounds[4];
  this->GetBounds(bounds);
  bool clamped = false;
  if (pos[0] < bounds[0])
    {
    pos[0] = bounds[0];
    clamped = true;
    }
  if (pos[0] > bounds[1])
    {
    pos[0] = bounds[1];
    clamped = true;
    }
  if (pos[1] < 0.)
    {
    pos[1] = 0.;
    clamped = true;
    }
  if (pos[1] > 1.)
    {
    pos[1] = 1.;
    clamped = true;
    }
  return clamped;
}

//-----------------------------------------------------------------------------
void vtkControlPointsItem::DrawUnselectedPoints(vtkContext2D* painter)
{
  const int count = this->GetNumberOfPoints();
  for (vtkIdType i = 0; i < count; ++i)
    {
    if (i == this->CurrentPoint ||
        (i == this->PointToDelete && this->PointAboutToBeDeleted) ||
        (i == this->PointToToggle && this->PointAboutToBeToggled))
      {
      continue;
      }
    vtkIdType idx = this->Selection ? this->Selection->LookupValue(i) : -1;
    if (idx != -1)
      {
      continue;
      }
    this->DrawPoint(painter, i);
    }
}

//-----------------------------------------------------------------------------
void vtkControlPointsItem::DrawSelectedPoints(vtkContext2D* painter)
{
  const int count = this->Selection ? this->Selection->GetNumberOfTuples() : 0;
  for (vtkIdType i = 0; i < count; ++i)
    {
    vtkIdType index = this->Selection->GetValue(i);
    if (index == this->CurrentPoint ||
        (index == this->PointToDelete && this->PointAboutToBeDeleted) ||
        (index == this->PointToToggle && this->PointAboutToBeToggled))

      {
      continue;
      }
    this->DrawPoint(painter, index);
    }
}

//-----------------------------------------------------------------------------
void vtkControlPointsItem::DrawPoint(vtkContext2D* painter, vtkIdType index)
{
  double point[4];
  this->GetControlPoint(index, point);

  double pointInScene[2];
  vtkTransform2D* sceneTransform = painter->GetTransform();
  sceneTransform->TransformPoints(point, pointInScene, 1);

  vtkSmartPointer<vtkTransform2D> translation =
    vtkSmartPointer<vtkTransform2D>::New();
  translation->Translate(pointInScene[0], pointInScene[1]);

  painter->PushMatrix();
  painter->SetTransform(translation);
  painter->DrawWedge(0.f, 0.f, this->ScreenPointRadius, 0.f, 0.f, 360.f);
  painter->DrawArc(0.f, 0.f, this->ScreenPointRadius, 0.f, 360.f);
  if (index == this->PointToDelete && this->PointAboutToBeDeleted)
    {
    painter->DrawLine(-this->ScreenPointRadius, -this->ScreenPointRadius,
                      this->ScreenPointRadius, this->ScreenPointRadius);
    painter->DrawLine(-this->ScreenPointRadius, this->ScreenPointRadius,
                      this->ScreenPointRadius, -this->ScreenPointRadius);
    }
  painter->PopMatrix();
}

//-----------------------------------------------------------------------------
void vtkControlPointsItem::SelectPoint(double* currentPoint)
{
  vtkIdType pointId = this->FindPoint(currentPoint);
  if (pointId == -1)
    {
    vtkErrorMacro( << "try to select a point that doesn't exist");
    return;
    }
  this->SelectPoint(pointId);
}

//-----------------------------------------------------------------------------
void vtkControlPointsItem::SelectPoint(vtkIdType pointId)
{
  if (this->Selection->LookupValue(pointId) != -1)
    {
    return;
    }
  this->Selection->InsertNextValue(pointId);
  this->GetScene()->SetDirty(true);
}

//-----------------------------------------------------------------------------
void vtkControlPointsItem::SelectAllPoints()
{
  this->DeselectAllPoints();
  const int count = this->GetNumberOfPoints();
  for (vtkIdType i = 0; i < count; ++i)
    {
    this->SelectPoint(i);
    }
}

//-----------------------------------------------------------------------------
void vtkControlPointsItem::DeselectPoint(double* point)
{
  // make sure the point belongs to the list of points
  vtkIdType pointId = this->FindPoint(point);
  if (pointId == -1)
    {
    vtkErrorMacro( << "try to deselect a point that doesn't exist");
    return;
    }
  this->DeselectPoint(pointId);
}

//-----------------------------------------------------------------------------
void vtkControlPointsItem::DeselectPoint(vtkIdType pointId)
{
  vtkIdType selectionPointId = this->Selection->LookupValue(pointId);
  if (selectionPointId == -1)
    {
    //vtkErrorMacro(<< "Point:" << pointId << " was not selected");
    return;
    }
  this->Selection->RemoveTuple(selectionPointId);
  this->GetScene()->SetDirty(true);
}

//-----------------------------------------------------------------------------
void vtkControlPointsItem::DeselectAllPoints()
{
  this->Selection->SetNumberOfTuples(0);
}

//-----------------------------------------------------------------------------
void vtkControlPointsItem::ToggleSelectPoint(double* currentPoint)
{
  // make sure the point belongs to the list of points
  vtkIdType pointId = this->FindPoint(currentPoint);
  if (pointId == -1)
    {
    vtkErrorMacro( << "try to toggle a point that doesn't exist");
    return;
    }
  this->ToggleSelectPoint(pointId);
}

//-----------------------------------------------------------------------------
void vtkControlPointsItem::ToggleSelectPoint(vtkIdType pointId)
{
  vtkIdType selectionId = this->Selection->LookupValue(pointId);
  if (selectionId != -1)
    {
    this->DeselectPoint(pointId);
    return;
    }
  this->SelectPoint(pointId);
}

//-----------------------------------------------------------------------------
bool vtkControlPointsItem::SelectPoints(const vtkVector2f& min, const vtkVector2f& max)
{
  bool atLeast1PointSelected = false;
  const int numberOfPoints = this->GetNumberOfPoints();
  for(vtkIdType i = 0; i < numberOfPoints; ++i)
    {
    double point[4];
    this->GetControlPoint(i, point);
    if (point[0] >= min.X() && point[0] <= max.X() &&
        point[1] >= min.Y() && point[1] <= max.Y())
      {
      this->SelectPoint(i);
      atLeast1PointSelected = true;
      }
    else
      {
      this->DeselectPoint(i);
      }
    }
  return atLeast1PointSelected;
}

//-----------------------------------------------------------------------------
vtkIdType vtkControlPointsItem::GetCurrentPoint()const
{
  return this->CurrentPoint;
}
/*
//-----------------------------------------------------------------------------
vtkIdType vtkControlPointsItem::FindPoint(double* pos, double tolerance)
{
  if (tolerance < std::numeric_limits<float>::epsilon())
    {
    tolerance = std::numeric_limits<float>::epsilon();
    }
  // make sure the point belongs to the list of points
  vtkIdType pointId = -1;
  double minDist = VTK_DOUBLE_MAX;
  const int numberOfPoints = this->GetNumberOfPoints();
  for(vtkIdType i = 0; i < numberOfPoints; ++i)
    {
    double point[4];
    this->GetControlPoint(i, point);
    double distance = (point[0] - pos[0]) * (point[0] - pos[0]) +
      (point[1] - pos[1]) * (point[1] - pos[1]);
    if (distance <= tolerance)
      {
      if (distance == 0.)
        {// we found the best match ever
        return i;
        }
      else if (distance < minDist)
        {// we found something not too bad, we maybe we can find closer
        pointId = i;
        minDist = distance;
        }
      }
    // don't search any further if the x is already too large
    //if (point[0] > pos[0] + this->ItemPointRadius2)
    //  {
    //  break;
    //  }
    }
  return pointId;
}
*/

//-----------------------------------------------------------------------------
bool vtkControlPointsItem::IsOverPoint(double* pos, vtkIdType pointId)
{
  if (pointId < 0 || pointId >= this->GetNumberOfPoints())
    {
    return false;
    }

  double screenPos[2];
  this->Transform->TransformPoints(pos, screenPos, 1);

  double point[4];
  this->GetControlPoint(pointId, point);
  double screenPoint[2];
  this->Transform->TransformPoints(point, screenPoint, 1);

  double distance2 =
    (screenPoint[0] - screenPos[0]) * (screenPoint[0] - screenPos[0]) +
    (screenPoint[1] - screenPos[1]) * (screenPoint[1] - screenPos[1]);
  double tolerance = 1.3;
  double radius2 = this->ScreenPointRadius * this->ScreenPointRadius
    * tolerance * tolerance;
  return distance2 <= radius2;
}

//-----------------------------------------------------------------------------
vtkIdType vtkControlPointsItem::FindPoint(double* pos)
{
  double tolerance = 1.3;
  double radius2 = this->ScreenPointRadius * this->ScreenPointRadius
    * tolerance * tolerance;
  double screenPos[2];
  this->Transform->TransformPoints(pos, screenPos, 1);
  vtkIdType pointId = -1;
  double minDist = VTK_DOUBLE_MAX;
  const int numberOfPoints = this->GetNumberOfPoints();
  for(vtkIdType i = 0; i < numberOfPoints; ++i)
    {
    double point[4];
    this->GetControlPoint(i, point);
    double screenPoint[2];
    this->Transform->TransformPoints(point, screenPoint, 1);
    double distance2 =
      (screenPoint[0] - screenPos[0]) * (screenPoint[0] - screenPos[0]) +
      (screenPoint[1] - screenPos[1]) * (screenPoint[1] - screenPos[1]);
    if (distance2 <= radius2)
      {
      if (distance2 == 0.)
        {// we found the best match ever
        return i;
        }
      else if (distance2 < minDist)
        {// we found something not too bad, maybe we can find closer
        pointId = i;
        minDist = distance2;
        }
      }
    // don't search any further if the x is already too large
    if (screenPoint[0] > (screenPos[0] + this->ScreenPointRadius * tolerance))
      {
      break;
      }
    }
  return pointId;
}

//-----------------------------------------------------------------------------
vtkIdType vtkControlPointsItem::GetControlPointId(double* point)
{
  const int numberOfPoints = this->GetNumberOfPoints();
  for(vtkIdType i = 0; i < numberOfPoints; ++i)
    {
    double controlPoint[4];
    this->GetControlPoint(i, controlPoint);
    if (controlPoint[0] == point[0])
      {
      return i;
      }
    }
  return -1;
}

//-----------------------------------------------------------------------------
vtkIdType vtkControlPointsItem::AddPoint(double* newPos)
{
  // offset all the point ids
  const int pointsCount = this->GetNumberOfPoints();
  vtkIdType previousPointId = pointsCount;
  for (vtkIdType i = 0; i < pointsCount; ++i)
    {
    double point[4];
    this->GetControlPoint(i, point);
    if (point[0] >= newPos[0])
      {
      previousPointId = i - 1;
      break;
      }
    }
  if (previousPointId == pointsCount)
    {
    return previousPointId;
    }
  const int selectionCount = this->Selection->GetNumberOfTuples();
  for (vtkIdType i = 0; i < selectionCount; ++i)
    {
    vtkIdType pointId = this->Selection->GetValue(i);
    if (pointId > previousPointId)
      {
      this->Selection->SetValue(i, ++pointId);
      }
    }
  if (this->CurrentPoint != -1
      && this->CurrentPoint >= previousPointId)
    {
    this->SetCurrentPoint(this->CurrentPoint + 1);
    }
  return previousPointId + 1;
}

//-----------------------------------------------------------------------------
vtkIdType vtkControlPointsItem::RemovePoint(double* point)
{
  return this->RemovePointId(this->GetControlPointId(point));
}

//-----------------------------------------------------------------------------
vtkIdType vtkControlPointsItem::RemovePointId(vtkIdType pointId)
{
  assert(pointId != -1);

  // Useless to remove the point here as it will be removed anyway in ComputePoints
  this->DeselectPoint(pointId);

  const int selectionCount = this->Selection->GetNumberOfTuples();
  for (vtkIdType i = 0; i < selectionCount; ++i)
    {
    vtkIdType selectedPointId = this->Selection->GetValue(i);
    if (selectedPointId > pointId)
      {
      this->Selection->SetValue(i, --selectedPointId);
      }
    }

  if (this->CurrentPoint == pointId)
    {
    this->SetCurrentPoint(-1);
    }
  if (this->CurrentPoint > pointId)
    {
    this->SetCurrentPoint(this->CurrentPoint - 1);
    }
  return pointId;
}

//-----------------------------------------------------------------------------
void vtkControlPointsItem::SetCurrentPoint(vtkIdType index)
{
  if (index == this->CurrentPoint)
    {
    return;
    }
  this->CurrentPoint = index;
  this->InvokeEvent(vtkControlPointsItem::CurrentPointChangedEvent,
                    reinterpret_cast<void *>(this->CurrentPoint));
  this->GetScene()->SetDirty(true);
}

//-----------------------------------------------------------------------------
bool vtkControlPointsItem::MouseButtonPressEvent(const vtkContextMouseEvent &mouse)
{
  this->MouseMoved = false;
  this->PointToToggle = -1;
  this->PointToDelete = -1;
  double pos[2];
  pos[0] = mouse.Pos[0];
  pos[1] = mouse.Pos[1];
  vtkIdType pointUnderMouse = this->FindPoint(pos);

  if (mouse.Button == vtkContextMouseEvent::LEFT_BUTTON)
    {
    if (pointUnderMouse != -1)
      {
      this->SetCurrentPoint(pointUnderMouse);
      return true;
      }
    else if (pointUnderMouse == -1
             && this->Selection->GetNumberOfTuples() <= 1
             && !this->StrokeMode)
      {
      this->ClampPos(pos);
      vtkIdType addedPoint = this->AddPoint(pos);
      this->SetCurrentPoint(addedPoint);
      return true;
      }
    else
      {
      this->SetCurrentPoint(-1);
      }
    return true;
    }

  if (mouse.Button == vtkContextMouseEvent::RIGHT_BUTTON
      && pointUnderMouse != -1)
    {
    this->PointToToggle = pointUnderMouse;
    this->PointAboutToBeToggled = true;
    this->GetScene()->SetDirty(true);
    return true;
    }

  if (mouse.Button == vtkContextMouseEvent::MIDDLE_BUTTON
      && pointUnderMouse != -1)
    {
    this->PointToDelete = pointUnderMouse;
    this->PointAboutToBeDeleted = true;
    this->GetScene()->SetDirty(true);
    return true;
    }

  return false;
}

//-----------------------------------------------------------------------------
bool vtkControlPointsItem::MouseDoubleClickEvent(const vtkContextMouseEvent &mouse)
{
  if (mouse.Button == vtkContextMouseEvent::RIGHT_BUTTON)
    {
    if (this->Selection->GetNumberOfTuples())
      {
      this->DeselectAllPoints();
      }
    else
      {
      this->SelectAllPoints();
      }
    return true;
    }
  bool res = this->MouseButtonPressEvent(mouse);
  if (mouse.Button == vtkContextMouseEvent::LEFT_BUTTON
     && this->CurrentPoint != -1)
    {
    this->InvokeEvent(vtkControlPointsItem::CurrentPointEditEvent,
                      reinterpret_cast<void *>(this->CurrentPoint));
    }
  return res;
}

//-----------------------------------------------------------------------------
bool vtkControlPointsItem::MouseMoveEvent(const vtkContextMouseEvent &mouse)
{
  if (mouse.Button == vtkContextMouseEvent::LEFT_BUTTON)
    {
    if (this->StrokeMode)
      {
      this->Stroke(mouse.Pos);
      }
    else if (this->CurrentPoint == -1 && this->Selection->GetNumberOfTuples() > 1)
      {
      this->MovePoints(mouse.Pos[0] - mouse.LastPos[0], mouse.Pos[1] - mouse.LastPos[1]);
      }
    else if (this->CurrentPoint != -1)
      {
      this->MoveCurrentPoint(mouse.Pos);
      }
    }
  if (mouse.Button == vtkContextMouseEvent::RIGHT_BUTTON)
    {
    if (this->PointToToggle == -1)
      {
      return false;
      }
    double pos[2];
    pos[0] = mouse.Pos[0];
    pos[1] = mouse.Pos[1];
    vtkIdType pointUnderCursor = this->FindPoint(pos);
    if ((pointUnderCursor == this->PointToToggle) != this->PointAboutToBeToggled)
      {
      this->PointAboutToBeToggled = !this->PointAboutToBeToggled;
      this->GetScene()->SetDirty(true);
      }
    }
  this->MouseMoved = true;
  if (mouse.Button == vtkContextMouseEvent::MIDDLE_BUTTON)
    {
    if (this->PointToDelete == -1)
      {
      // allow chart ruber band to work
      return false;
      }
    double pos[2];
    pos[0] = mouse.Pos[0];
    pos[1] = mouse.Pos[1];
    vtkIdType pointUnderCursor = this->FindPoint(pos);
    if ((pointUnderCursor == this->PointToDelete) != this->PointAboutToBeDeleted)
      {
      this->PointAboutToBeDeleted = !this->PointAboutToBeDeleted;
      this->GetScene()->SetDirty(true);
      }
    return true;
    }

  if (mouse.Button == vtkContextMouseEvent::RIGHT_BUTTON
      && this->CurrentPoint == -1)
    {
    return false;
    }
  return true;
}

//-----------------------------------------------------------------------------
void vtkControlPointsItem::MoveCurrentPoint(const vtkVector2f& newPos)
{
  vtkIdType movedPoint = this->MovePoint(this->CurrentPoint, newPos);
  // If the moved point was not CurrentPoint then make it current.
  this->SetCurrentPoint(movedPoint);
}

//-----------------------------------------------------------------------------
vtkIdType vtkControlPointsItem::MovePoint(vtkIdType point, const vtkVector2f& newPos)
{
  if (point == -1)
    {
    return point;
    }

  // Make sure the new point is inside the boundaries of the function
  double boundedPos[2];
  boundedPos[0] = newPos[0];
  boundedPos[1] = newPos[1];
  this->ClampPos(boundedPos);

  if (!this->SwitchPointsMode)
    {
    // Stop mode.
    // You can't move a point past another point.
    if (point > 0)
      {
      double previousPoint[4] = {0.0, 0.0, 0.0, 0.0};
      this->GetControlPoint(point - 1, previousPoint);
      boundedPos[0] = std::max(previousPoint[0], boundedPos[0]);
      }
    if (point < this->GetNumberOfPoints() - 1)
      {
      double nextPoint[4] = {0.0, 0.0, 0.0, 0.0};
      this->GetControlPoint(point + 1, nextPoint);
      boundedPos[0] = std::min(boundedPos[0], nextPoint[0]);
      }
    }
  else
    {
    // Switch mode.
    // Moving a point to the right of the next one, makes it current.
    // and moving a point to the left of the previous one makes it current.
    if (point > 0)
      {
      double previousPoint[4] = {0.0, 0.0, 0.0, 0.0};
      this->GetControlPoint(point - 1, previousPoint);
      while (boundedPos[0] < previousPoint[0])
        {
        point = point - 1;
        if (point == 0)
          {
          break;
          }
        // maybe the move is that important that it went over multiple points
        this->GetControlPoint(point - 1, previousPoint);
        }
      }
    if (point < this->GetNumberOfPoints() - 1)
      {
      double nextPoint[4] = {0.0, 0.0, 0.0, 0.0};
      this->GetControlPoint(point + 1, nextPoint);
      while (boundedPos[0] > nextPoint[0])
        {
        point = point + 1;
        if (point == this->GetNumberOfPoints() - 1)
          {
          break;
          }
        this->GetControlPoint(point + 1, nextPoint);
        }
      }
    }
  double currentPoint[4] = {0.0, 0.0, 0.0, 0.0};
  this->GetControlPoint(point, currentPoint);
  currentPoint[0] = boundedPos[0];
  currentPoint[1] = boundedPos[1];
  this->SetControlPoint(point, currentPoint);
  return point;
}

//-----------------------------------------------------------------------------
void vtkControlPointsItem::MovePoints(float tX, float tY)
{
  // don't support 'switch' mode yet
  //vtkIdTypeArray* addedSelection = vtkIdTypeArray::New();
  bool oldSwitchPoints = this->SwitchPointsMode;
  this->SwitchPointsMode = false;
  // end "don't support 'switch' mode yet"
  const int count = this->Selection->GetNumberOfTuples();
  int start = tX > 0 ? 0 : count - 1;
  int end = tX > 0 ? count : -1;
  int step = tX > 0 ? 1 : -1;
  for (vtkIdType i = start; i != end; i+=step)
    {
    vtkIdType point = this->Selection->GetValue(i);
    double currentPoint[4] = {0.0, 0.0, 0.0, 0.0};
    this->GetControlPoint(point, currentPoint);
    vtkVector2f newPos(currentPoint[0] + tX, currentPoint[1] + tY);
    //vtkIdType newIdx = this->MovePoint(point, newPos);
    // don't support 'switch' mode yet
    //if (newIdx != point)
    //  {
    //  int next = (newIdx > point) ? 1 : -1;
    //  for (int i = point + next; i != newIdx; i+=next)
    //    {
    //    addedSelection->InsertNextValue(i);
    //    }
    //  addedSelection->InsertNextValue(newIdx);
    //  }
    // end "don't support 'switch' mode yet"
    }
  // don't support 'switch' mode yet
  //this->SelectPoints(addedSelection);
  this->SwitchPointsMode = oldSwitchPoints;
  // end "don't support 'switch' mode yet"
}

//-----------------------------------------------------------------------------
void vtkControlPointsItem::Stroke(const vtkVector2f& newPos)
{
  double pos[2];
  pos[0] = newPos[0];
  pos[1] = newPos[1];
  this->ClampPos(pos);

  // last point
  if (this->CurrentPoint != -1)
    {
    vtkIdType lastPointId = this->CurrentPoint;
    double lastPoint[4] = {0.0, 0.0, 0.0, 0.0};
    this->GetControlPoint(lastPointId, lastPoint);
    const double pointSpacing = 1.15;
    float oldScreenPointRadius = this->ScreenPointRadius;
    this->ScreenPointRadius *= pointSpacing;
    // ignore the stroke if it is too close from the last point
    if (this->FindPoint(pos) == lastPointId)
      {
      this->ScreenPointRadius = oldScreenPointRadius;
      return;
      }
    this->ScreenPointRadius = oldScreenPointRadius;
    // For the first case or when the new pos share the same X (but not the
    // same y) then we just have to modify the last point
    if (!this->MouseMoved || lastPoint[0] == pos[0])
      {
      lastPoint[0] = pos[0];
      lastPoint[1] = pos[1];
      this->SetControlPoint(this->CurrentPoint, lastPoint);
      return;
      }
    assert(lastPoint[0] != pos[0]);
    // CurrentPoint != -1 && MouseMoved
    // Starting from the last point, we search points (forward or backward) to see
    // if there are points that can be removed.
    int count = this->GetNumberOfPoints();
    if (pos[0] > lastPoint[0] && lastPointId < count - 1)
      {
      // search if there are points between pos and lastPoint
      double point[4] = {0.0, 0.0, 0.0, 0.0};
      this->GetControlPoint(lastPointId + 1, point);
      while (pos[0] >= point[0])
        {
        this->RemovePoint(point);
        count = this->GetNumberOfPoints();
        if (lastPointId == count - 1)
           {
           break;
           }
        this->GetControlPoint(lastPointId + 1, point);
        }
      }
    else if (pos[0] < lastPoint[0] && lastPointId > 0)
      {
      // search if there are points between pos and lastPoint
      double point[4] = {0.0, 0.0, 0.0, 0.0};
      this->GetControlPoint(lastPointId - 1, point);
      while (pos[0] <= point[0])
        {
        this->RemovePoint(point);
        --lastPointId;
        if (lastPointId == 0)
           {
           break;
           }
        this->GetControlPoint(lastPointId - 1, point);
        }
      }
    }
#ifndef NDEBUG
  const int oldNumberOfPoints = this->GetNumberOfPoints();
#endif
  vtkIdType addedPoint = this->AddPoint(pos);
  this->SetCurrentPoint(addedPoint);
  assert(oldNumberOfPoints + 1 == this->GetNumberOfPoints());
}

//-----------------------------------------------------------------------------
void vtkControlPointsItem::EditPoint(float vtkNotUsed(tX), float vtkNotUsed(tY))
{
}

//-----------------------------------------------------------------------------
bool vtkControlPointsItem::MouseButtonReleaseEvent(const vtkContextMouseEvent &mouse)
{
  if (mouse.Button == vtkContextMouseEvent::LEFT_BUTTON)
    {
    return true;
    }
  if (mouse.Button == vtkContextMouseEvent::RIGHT_BUTTON
      && this->PointToToggle != -1)
    {
    if (this->PointAboutToBeToggled)
      {
      this->ToggleSelectPoint(this->PointToToggle);
      this->PointToToggle = -1;
      this->PointAboutToBeToggled = false;
      }
    return true;
    }
  if (mouse.Button == vtkContextMouseEvent::MIDDLE_BUTTON
      && this->PointToDelete != -1)
    {
    if (this->PointAboutToBeDeleted)
      {
      // If EnforceValidFunction is true, we don't want less than 2 points
      if (!this->EnforceValidFunction || this->GetNumberOfPoints() > 2)
        {
        double point[4];
        this->GetControlPoint(this->PointToDelete, point);
        this->RemovePoint(point);
        this->PointToDelete = -1;
        this->PointAboutToBeDeleted = false;
        }
      else
        {
        this->PointToDelete = -1;
        this->PointAboutToBeDeleted = false;
        this->GetScene()->SetDirty(true);
        }
      }
    return true;
    }
  return false;
}
