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

#include "vtkAxis.h"
#include "vtkBrush.h"
#include "vtkCallbackCommand.h"
#include "vtkContext2D.h"
#include "vtkContextDevice2D.h"
#include "vtkContextKeyEvent.h"
#include "vtkContextMouseEvent.h"
#include "vtkContextScene.h"
#include "vtkControlPointsItem.h"
#include "vtkIdTypeArray.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPen.h"
#include "vtkPoints2D.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkTransform2D.h"
#include "vtkVectorOperators.h"

#include <algorithm>
#include <cassert>
#include <limits>

//-----------------------------------------------------------------------------
vtkControlPointsItem::vtkControlPointsItem()
{
  this->Pen->SetLineType(vtkPen::SOLID_LINE);
  this->Pen->SetWidth(2.);
  this->Pen->SetColor(140, 144, 125, 200);
  this->Brush->SetColor(125, 135, 144, 200);

  this->SelectedPointPen = vtkPen::New();
  this->SelectedPointPen->SetWidth(2.);
  // 98, 140, 178
  this->SelectedPointPen->SetColor(63, 90, 115, 200);
  //this->SelectedPointPen->SetColor(98, 140, 178, 200);
  this->SelectedPointBrush = vtkBrush::New();
  this->SelectedPointBrush->SetColor(58, 121, 178, 200);

  this->Selection = vtkIdTypeArray::New();
  this->CurrentPoint = -1;

  this->BlockUpdates = 0;
  this->StartedInteractions = 0;
  this->StartedChanges = 0;

  this->Callback = vtkCallbackCommand::New();
  this->Callback->SetClientData(this);
  this->Callback->SetCallback(
    vtkControlPointsItem::CallComputePoints);

  this->Bounds[0] = this->Bounds[2] = 0.;
  this->Bounds[1] = this->Bounds[3] = -1.;
  this->UserBounds[0] = this->UserBounds[2] = 0.;
  this->UserBounds[1] = this->UserBounds[3] = -1.;
  this->ValidBounds[0] = this->ValidBounds[2] = 0.;
  this->ValidBounds[1] = this->ValidBounds[3] = -1.;

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
  this->InvertShadow = false;
  this->EndPointsXMovable = true;
  this->EndPointsYMovable = true;
  this->EndPointsRemovable = true;
  this->ShowLabels = false;
  this->LabelFormat = NULL;
  this->SetLabelFormat("%.3f, %.3f");
}

//-----------------------------------------------------------------------------
vtkControlPointsItem::~vtkControlPointsItem()
{
  this->SetLabelFormat(NULL);
  if (this->Callback)
  {
    this->Callback->Delete();
    this->Callback = 0;
  }
  if (this->SelectedPointPen)
  {
    this->SelectedPointPen->Delete();
    this->SelectedPointPen = 0;
  }
  if (this->SelectedPointBrush)
  {
    this->SelectedPointBrush->Delete();
    this->SelectedPointBrush = 0;
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

  os << indent << "EndPointsXMovable: " << this->EndPointsXMovable << endl;
  os << indent << "EndPointsYMovable: " << this->EndPointsYMovable << endl;
  os << indent << "EndPointsRemovable: " << this->EndPointsRemovable << endl;
  os << indent << "ShowLabels: " << this->ShowLabels << endl;
}

//-----------------------------------------------------------------------------
void vtkControlPointsItem::GetBounds(double bounds[4])
{
  // valid user bounds ? use them
  if (this->UserBounds[0] <= this->UserBounds[1] &&
      this->UserBounds[2] <= this->UserBounds[3])
  {
    bounds[0] = this->UserBounds[0];
    bounds[1] = this->UserBounds[1];
    bounds[2] = this->UserBounds[2];
    bounds[3] = this->UserBounds[3];
    return;
  }
  // invalid bounds ? compute them
  if (!(this->Bounds[0] <= this->Bounds[1] &&
        this->Bounds[2] > this->Bounds[3]))
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
  double oldBounds[4];
  oldBounds[0] = this->Bounds[0];
  oldBounds[1] = this->Bounds[1];
  oldBounds[2] = this->Bounds[2];
  oldBounds[3] = this->Bounds[3];

  this->ComputeBounds(this->Bounds);

  if (this->Bounds[0] != oldBounds[0] ||
      this->Bounds[1] != oldBounds[1] ||
      this->Bounds[2] != oldBounds[2] ||
      this->Bounds[3] != oldBounds[3])
  {
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
void vtkControlPointsItem::ComputeBounds( double* bounds)
{
  bounds[0] = bounds[2] =  VTK_DOUBLE_MAX;
  bounds[1] = bounds[3] = -VTK_DOUBLE_MAX;
  for (vtkIdType i=0; i < this->GetNumberOfPoints(); ++i)
  {
    double point[4];
    this->GetControlPoint(i, point);
    bounds[0] = std::min(bounds[0], point[0]);
    bounds[1] = std::max(bounds[1], point[0]);
    bounds[2] = std::min(bounds[2], point[1]);
    bounds[3] = std::max(bounds[3], point[1]);
  }
}

//-----------------------------------------------------------------------------
bool vtkControlPointsItem::Paint(vtkContext2D* painter)
{
  painter->GetDevice()->EnableClipping(false);
  painter->ApplyPen(this->Pen);
  painter->ApplyBrush(this->Brush);
  this->InvertShadow = false;
  this->DrawUnselectedPoints(painter);

  painter->GetPen()->SetLineType(vtkPen::SOLID_LINE);
  //painter->GetPen()->SetColorF(0.87, 0.87, 1.);
  //painter->GetBrush()->SetColorF(0.75, 0.75, 0.95, 0.65);
  //float oldPenWidth = painter->GetPen()->GetWidth();
  painter->ApplyPen(this->SelectedPointPen);
  painter->ApplyBrush(this->SelectedPointBrush);
  this->InvertShadow = true;
  float oldScreenPointRadius = this->ScreenPointRadius;
  if (this->Selection && this->Selection->GetNumberOfTuples())
  {
    //painter->GetPen()->SetWidth(oldPenWidth * 1.4);
    //this->ScreenPointRadius = oldScreenPointRadius * 1.1;
    this->DrawSelectedPoints(painter);
  }
  this->ScreenPointRadius = oldScreenPointRadius;
  this->Transform->SetMatrix(painter->GetTransform()->GetMatrix());

  painter->GetDevice()->EnableClipping(true);
  this->PaintChildren(painter);
  return true;
}

//-----------------------------------------------------------------------------
void vtkControlPointsItem::StartChanges()
{
  ++this->StartedChanges;
  if (this->StartedChanges == 1)
  {
    this->InvokeEvent(vtkCommand::StartEvent);
    this->emitEvent(vtkCommand::StartEvent);
  }
}

//-----------------------------------------------------------------------------
void vtkControlPointsItem::EndChanges()
{
  --this->StartedChanges;
  assert(this->StartedChanges >=0);
  if (this->StartedChanges == 0)
  {
    this->emitEvent(vtkCommand::EndEvent);
    this->InvokeEvent(vtkCommand::EndEvent);
  }
}

//-----------------------------------------------------------------------------
void vtkControlPointsItem::StartInteraction()
{
  ++this->StartedInteractions;
  this->emitEvent(vtkCommand::StartInteractionEvent);
}

//-----------------------------------------------------------------------------
void vtkControlPointsItem::StartInteractionIfNotStarted()
{
  if (this->GetInteractionsCount() == 0)
  {
    this->StartInteraction();
  }
}

//-----------------------------------------------------------------------------
void vtkControlPointsItem::Interaction()
{
  assert(this->StartedInteractions > 0);
  this->emitEvent(vtkCommand::InteractionEvent);
}

//-----------------------------------------------------------------------------
void vtkControlPointsItem::EndInteraction()
{
  --this->StartedInteractions;
  assert(this->StartedInteractions >= 0);
  this->emitEvent(vtkCommand::EndInteractionEvent);
}

//-----------------------------------------------------------------------------
int vtkControlPointsItem::GetInteractionsCount()const
{
  return this->StartedInteractions;
}

//-----------------------------------------------------------------------------
//void vtkControlPointsItem::emitEvent(unsigned long event, void* params);

//-----------------------------------------------------------------------------
void vtkControlPointsItem::CallComputePoints(
  vtkObject* vtkNotUsed(sender), unsigned long event,
  void* receiver, void* vtkNotUsed(params))
{
  vtkControlPointsItem* item =
    reinterpret_cast<vtkControlPointsItem*>(receiver);
  switch(event)
  {
    case vtkCommand::StartEvent:
      ++item->BlockUpdates;
      break;
    case vtkCommand::EndEvent:
      --item->BlockUpdates;
      if (item->BlockUpdates == 0)
      {
        item->ComputePoints();
      }
      break;
    case vtkCommand::ModifiedEvent:
      item->ComputePoints();
      break;
    default:
      break;
  }
}

//-----------------------------------------------------------------------------
void vtkControlPointsItem::ComputePoints()
{
  if (this->BlockUpdates > 0)
  {
    return;
  }

  if (this->Selection && this->GetNumberOfPoints() == 0)
  {
    this->Selection->SetNumberOfTuples(0);
  }

  const int selectedPointCount = this->Selection ?
        this->Selection->GetNumberOfTuples() : 0;
  if (selectedPointCount)
  {
    vtkIdTypeArray* oldSelection = this->Selection;
    this->Selection = vtkIdTypeArray::New();
    for (vtkIdType i = 0; i < selectedPointCount; ++i)
    {
      if (oldSelection->GetValue(i) < this->GetNumberOfPoints())
      {
        this->SelectPoint(oldSelection->GetValue(i));
      }
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
void vtkControlPointsItem::TransformScreenToData(const vtkVector2f& in, vtkVector2f& out)
{
  out = in;

  // inverse shift/scale from screen space.
  const vtkRectd& ss = this->ShiftScale;
  out.SetX(static_cast<float>((out.GetX() / ss[2]) - ss[0]));
  out.SetY(static_cast<float>((out.GetY() / ss[3]) - ss[1]));

  if (this->UsingLogScale())
  {
    // using log scale.
    double bounds[4];
    this->ComputeBounds(bounds);

    double posX = in.GetX();
    double normVal = (posX - bounds[0])/(bounds[1] - bounds[0]);
    double lval = log10(bounds[0]) + normVal*(log10(bounds[1]) - log10(bounds[0]));
    posX = pow(10.0, lval);
    out.SetX(posX);
  }
}

//-----------------------------------------------------------------------------
void vtkControlPointsItem::TransformDataToScreen(const vtkVector2f& in, vtkVector2f& out)
{
  out = in;
  if (this->UsingLogScale())
  {
    double bounds[4];
    this->ComputeBounds(bounds);

    double posX = in.GetX();
    double lnormVal = (log10(posX) - log10(bounds[0])) /
                      (log10(bounds[1]) - log10(bounds[0]));
    posX = bounds[0] + lnormVal * (bounds[1] - bounds[0]);
    out.SetX(posX);
  }

  // now, shift/scale to screen space.
  const vtkRectd& ss = this->ShiftScale;
  out.SetX(static_cast<float>((out.GetX() + ss[0]) * ss[2]));
  out.SetY(static_cast<float>((out.GetY() + ss[1]) * ss[3]));
}

//-----------------------------------------------------------------------------
bool vtkControlPointsItem::Hit(const vtkContextMouseEvent &mouse)
{
  vtkVector2f vpos = mouse.GetPos();
  this->TransformScreenToData(vpos, vpos);
  double pos[2];
  pos[0] = vpos.GetX();
  pos[1] = vpos.GetY();
  double bounds[4];
  this->GetBounds(bounds);
  bool clamped = this->ClampPos(pos, bounds);
  if (!clamped)
  {
    return true;
  }
  // maybe the cursor is over the first or last point (which could be outside
  // the bounds because of the screen point size).
  pos[0] = vpos.GetX();
  pos[1] = vpos.GetY();
  for (int i = 0; i < this->GetNumberOfPoints(); ++i)
  {
    if (this->IsOverPoint(pos, i))
    {
      return true;
    }
  }
  return false;
}

//-----------------------------------------------------------------------------
bool vtkControlPointsItem::ClampPos(double pos[2], double bounds[4])
{
  if (bounds[1] < bounds[0] || bounds[3] < bounds[2])
  {
    // bounds are not valid. Don't clamp.
    return false;
  }
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
bool vtkControlPointsItem::ClampValidPos(double pos[2])
{
  double validBounds[4];
  this->GetValidBounds(validBounds);
  if (validBounds[0] > this->ValidBounds[1] ||
      validBounds[2] > this->ValidBounds[3])
  {
    double bounds[4];
    this->GetBounds(bounds);
    return this->ClampPos(pos, bounds);
  }
  return this->ClampPos(pos, validBounds);
}

//-----------------------------------------------------------------------------
void vtkControlPointsItem::DrawUnselectedPoints(vtkContext2D* painter)
{
  const int count = this->GetNumberOfPoints();
  for (vtkIdType i = 0; i < count; ++i)
  {
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
    assert(index != -1);
    this->DrawPoint(painter, index);
  }
}

//-----------------------------------------------------------------------------
void vtkControlPointsItem::DrawPoint(vtkContext2D* painter, vtkIdType index)
{
  assert(index != -1);
  double point[4];
  this->GetControlPoint(index, point);

  vtkVector2f vpoint(point[0], point[1]);
  this->TransformDataToScreen(vpoint, vpoint);
  point[0] = vpoint.GetX();
  point[1] = vpoint.GetY();

  double pointInScene[2];
  vtkTransform2D* sceneTransform = painter->GetTransform();
  sceneTransform->TransformPoints(point, pointInScene, 1);

  vtkSmartPointer<vtkTransform2D> translation =
    vtkSmartPointer<vtkTransform2D>::New();
  translation->Translate(pointInScene[0], pointInScene[1]);

  painter->PushMatrix();
  painter->SetTransform(translation);

  unsigned char brushOpacity = painter->GetBrush()->GetOpacity();
  unsigned char penColor[3];
  painter->GetPen()->GetColor(penColor);
  unsigned char penOpacity = painter->GetPen()->GetOpacity();
  //float width = painter->GetPen()->GetWidth();

  float radius = this->ScreenPointRadius;
  bool invertShadow = this->InvertShadow;
  unsigned char color[3] = {penColor[0], penColor[1], penColor[2]};

  if (this->PointToToggle == index && this->PointAboutToBeToggled)
  {
    invertShadow = !invertShadow;
  }
  if (this->PointToDelete == index && this->PointAboutToBeDeleted)
  {
    invertShadow = !invertShadow;
    color[0] = 255;
    color[1] = 0;
    color[2] = 0;
  }
  if (this->CurrentPoint == index)
  {
    radius = this->ScreenPointRadius * 1.3;
    color[0] = 255;
    color[1] = 0;
    color[2] = 255;
  }

  painter->GetPen()->SetColor(color);
  painter->DrawArc(0.f, 0.f, radius, 0.f, 360.f);

  painter->GetBrush()->SetOpacity(0);
  //painter->GetPen()->SetWidth(2.0f);

  unsigned char lightPenColor[4];
  lightPenColor[0] = std::min(color[0] + 100, 255);
  lightPenColor[1] = std::min(color[1] + 100, 255);
  lightPenColor[2] = std::min(color[2] + 100, 255);
  lightPenColor[3] = penOpacity;

  unsigned char darkPenColor[4];
  darkPenColor[0] = std::max(color[0] - 50, 0);
  darkPenColor[1] = std::max(color[1] - 50, 0);
  darkPenColor[2] = std::max(color[2] - 50, 0);
  darkPenColor[3] = penOpacity;

  painter->GetPen()->SetColor(invertShadow ? lightPenColor : darkPenColor);
  painter->DrawArc(0.f, 0.f, radius - 1.0, 200.f, 380.f);
  painter->GetPen()->SetColor(invertShadow ? darkPenColor : lightPenColor);
  painter->DrawArc(0.f, 0.f, radius - 1.0, 20.f, 200.f);

  painter->GetPen()->SetColor(color);
  if (this->PointToDelete == index && this->PointAboutToBeDeleted)
  {
    painter->DrawLine(-radius, -radius, radius, radius);
    painter->DrawLine(-radius, radius, radius, -radius);
  }

  painter->GetPen()->SetColor(penColor);

  if (this->ShowLabels && (/*index == 0 ||
                           index == this->GetNumberOfPoints()-1 || */
                           this->GetCurrentPoint() == index))
  {
    translation->Translate(0, radius+5);
    painter->SetTransform(translation);
    vtkStdString label = this->GetControlPointLabel(index);

    vtkVector2f bounds[2];
    painter->ComputeStringBounds(label, bounds[0].GetData());
    if (bounds[1].GetX() != 0.0f && bounds[1].GetY() != 0.0f)
    {
      float scale[2];
      float position[2];
      painter->GetTransform()->GetScale(scale);
      painter->GetTransform()->GetPosition(position);

      double brushColor[4];
      painter->GetBrush()->GetColorF(brushColor);
      painter->GetBrush()->SetColorF(1, 1, 1, 1);
      painter->GetBrush()->SetOpacityF(0.75);
      painter->GetPen()->SetOpacity(0);
      bounds[0] = vtkVector2f(-5/scale[0], -3/scale[1]);
      bounds[1] = vtkVector2f(bounds[1].GetX()+10/scale[0],
        bounds[1].GetY()+10/scale[1]);

      // Pull the tooltip back in if it will go off the edge of the screen.
      float maxX = (this->Scene->GetViewWidth() - position[0])/scale[0];
      if (bounds[0].GetX() >= maxX - bounds[1].GetX())
      {
        bounds[0].SetX(maxX - bounds[1].GetX());
      }
      // Pull the tooltip down in if it will go off the edge of the screen.
      float maxY = (this->Scene->GetViewHeight() - position[1])/scale[1];
      if (bounds[0].GetY() >= maxY - bounds[1].GetY())
      {
        bounds[0].SetY(maxY - bounds[1].GetY());
      }
      painter->DrawRect(bounds[0].GetX(), bounds[0].GetY(), bounds[1].GetX(), bounds[1].GetY());
      painter->DrawString(bounds[0].GetX()+5/scale[0], bounds[0].GetY()+3/scale[1], label);
      painter->GetBrush()->SetColorF(brushColor);
    }
  }

  painter->GetPen()->SetOpacity(penOpacity);
  //painter->GetPen()->SetWidth(width);
  painter->GetBrush()->SetOpacity(brushOpacity);

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
  if (!this->Selection || this->Selection->LookupValue(pointId) != -1)
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
  vtkIdType selectionPointId = this->Selection
      ? this->Selection->LookupValue(pointId) : -1;
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
  if (this->GetNumberOfSelectedPoints() == 0)
  {
    return;
  }
  this->Selection->SetNumberOfTuples(0);
  this->GetScene()->SetDirty(true);
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
  vtkIdType selectionId = this->Selection
      ? this->Selection->LookupValue(pointId) : -1;
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
    if (point[0] >= min.GetX() && point[0] <= max.GetX() &&
        point[1] >= min.GetY() && point[1] <= max.GetY())
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
vtkIdType vtkControlPointsItem::GetNumberOfSelectedPoints()const
{
  return this->Selection ? this->Selection->GetNumberOfTuples() : 0;
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
vtkIdType vtkControlPointsItem::FindPoint(double* _pos)
{
  vtkVector2f vpos(_pos[0], _pos[1]);
  this->TransformDataToScreen(vpos, vpos);
  double pos[2] = {vpos.GetX(), vpos.GetY()};

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
    vtkVector2f vpos1(point[0], point[1]);
    this->TransformDataToScreen(vpos1, vpos1);
    point[0] = vpos1.GetX();
    point[1] = vpos1.GetY();

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
void vtkControlPointsItem
::GetControlPointsIds(vtkIdTypeArray* points, bool excludeFirstAndLast)const
{
  assert(points != 0);
  int numberOfPoints = this->GetNumberOfPoints();
  if (excludeFirstAndLast)
  {
    numberOfPoints -= 2;
  }
  numberOfPoints = std::max(numberOfPoints, 0);
  points->SetNumberOfTuples(numberOfPoints);
  vtkIdType pointId = excludeFirstAndLast ? 1 : 0;
  for(vtkIdType i = 0; i < numberOfPoints; ++i)
  {
    points->SetValue(i, pointId++);
  }
}
//-----------------------------------------------------------------------------
void vtkControlPointsItem::AddPointId(vtkIdType addedPointId)
{
  assert(addedPointId != -1);
  // offset all the point ids
  const int selectionCount = this->GetNumberOfSelectedPoints();
  for (vtkIdType i = 0; i < selectionCount; ++i)
  {
    vtkIdType pointId = this->Selection->GetValue(i);
    if (pointId >= addedPointId)
    {
      this->Selection->SetValue(i, ++pointId);
    }
  }
  if (this->CurrentPoint >= addedPointId)
  {
    this->SetCurrentPoint(this->CurrentPoint + 1);
  }
}

//-----------------------------------------------------------------------------
vtkIdType vtkControlPointsItem::RemovePoint(double* point)
{
  return this->RemovePointId(this->GetControlPointId(point));
}

//-----------------------------------------------------------------------------
vtkIdType vtkControlPointsItem::RemovePoint(vtkIdType pointId)
{
  double point[4];
  this->GetControlPoint(pointId, point);
  return this->RemovePoint(point);
}

//-----------------------------------------------------------------------------
vtkIdType vtkControlPointsItem::RemovePointId(vtkIdType pointId)
{
  if(!this->IsPointRemovable(pointId))
  {
    return pointId;
  }

  this->StartChanges();

  assert(pointId != -1);
  // Useless to remove the point here as it will be removed anyway in ComputePoints
  this->DeselectPoint(pointId);

  const vtkIdType selectionCount = this->GetNumberOfSelectedPoints();
  for (vtkIdType i = 0; i < selectionCount; ++i)
  {
    vtkIdType selectedPointId = this->Selection->GetValue(i);
    if (selectedPointId > pointId)
    {
      this->Selection->SetValue(i, --selectedPointId);
    }
  }

  if (this->CurrentPoint > pointId ||
      this->CurrentPoint == this->GetNumberOfPoints() - 1)
  {
    this->SetCurrentPoint(this->CurrentPoint - 1);
  }

  this->EndChanges();
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

  vtkVector2f vpos = mouse.GetPos();
  this->TransformScreenToData(vpos, vpos);
  double pos[2];
  pos[0] = vpos.GetX();
  pos[1] = vpos.GetY();
  vtkIdType pointUnderMouse = this->FindPoint(pos);

  if (mouse.GetButton() == vtkContextMouseEvent::LEFT_BUTTON)
  {
    if (pointUnderMouse != -1)
    {
      this->SetCurrentPoint(pointUnderMouse);
      return true;
    }
    else if (pointUnderMouse == -1
             && this->GetNumberOfSelectedPoints() <= 1
             && !this->StrokeMode)
    {
      this->ClampValidPos(pos);
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

  if (mouse.GetButton() == vtkContextMouseEvent::RIGHT_BUTTON
      && pointUnderMouse != -1)
  {
    this->PointToToggle = pointUnderMouse;
    this->PointAboutToBeToggled = true;
    this->GetScene()->SetDirty(true);
    return true;
  }

  if (mouse.GetButton() == vtkContextMouseEvent::MIDDLE_BUTTON
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
  if (mouse.GetButton() == vtkContextMouseEvent::RIGHT_BUTTON)
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
  if (mouse.GetButton() == vtkContextMouseEvent::LEFT_BUTTON
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
  vtkVector2f mousePos = mouse.GetPos();
  this->TransformScreenToData(mousePos, mousePos);

  if (mouse.GetButton() == vtkContextMouseEvent::LEFT_BUTTON)
  {
    if (this->StrokeMode)
    {
      this->StartInteractionIfNotStarted();

      this->Stroke(mousePos);

      this->Interaction();
    }
    else if (this->CurrentPoint == -1 && this->GetNumberOfSelectedPoints() > 1)
    {
      vtkVector2f deltaPos = mouse.GetPos() - mouse.GetLastPos();
      if(this->IsEndPointPicked())
      {
        if(!this->GetEndPointsMovable())
        {
          return false;
        }
        else if(this->GetEndPointsXMovable())
        {
          deltaPos.SetY(0);
        }
        else if(this->GetEndPointsYMovable())
        {
          deltaPos.SetX(0);
        }
      }

      this->StartInteractionIfNotStarted();

      if (vtkIdTypeArray* points = this->GetSelection())
      {
        // must stay valid after each individual point move
        points->Register(this);
        this->MovePoints(deltaPos, points);
        points->UnRegister(this);
      }

      this->Interaction();
    }
    else if (this->CurrentPoint != -1)
    {
      vtkVector2f curPos(mousePos);
      if(this->IsEndPointPicked())
      {
        double currentPoint[4] = {0.0, 0.0, 0.0, 0.0};
        this->GetControlPoint(this->CurrentPoint, currentPoint);
        if(!this->GetEndPointsMovable())
        {
          return false;
        }
        else if(this->GetEndPointsXMovable())
        {
          curPos.SetY(currentPoint[1]);
        }
        else if(this->GetEndPointsYMovable())
        {
          curPos.SetX(currentPoint[0]);
        }
      }
      this->StartInteractionIfNotStarted();

      this->SetCurrentPointPos(curPos);

      this->Interaction();
    }
  }
  if (mouse.GetButton() == vtkContextMouseEvent::RIGHT_BUTTON)
  {
    if (this->PointToToggle == -1)
    {
      return false;
    }
    double pos[2];
    pos[0] = mousePos[0];
    pos[1] = mousePos[1];
    vtkIdType pointUnderCursor = this->FindPoint(pos);
    if ((pointUnderCursor == this->PointToToggle) != this->PointAboutToBeToggled)
    {
      this->PointAboutToBeToggled = !this->PointAboutToBeToggled;
      this->GetScene()->SetDirty(true);
    }
  }
  this->MouseMoved = true;
  if (mouse.GetButton() == vtkContextMouseEvent::MIDDLE_BUTTON)
  {
    if (this->PointToDelete == -1)
    {
      // allow chart ruber band to work
      return false;
    }
    double pos[2];
    pos[0] = mousePos[0];
    pos[1] = mousePos[1];
    vtkIdType pointUnderCursor = this->FindPoint(pos);
    if ((pointUnderCursor == this->PointToDelete) != this->PointAboutToBeDeleted)
    {
      this->PointAboutToBeDeleted = !this->PointAboutToBeDeleted;
      this->GetScene()->SetDirty(true);
    }
    return true;
  }

  if (mouse.GetButton() == vtkContextMouseEvent::RIGHT_BUTTON
      && this->CurrentPoint == -1)
  {
    return false;
  }
  if (mouse.GetButton() == vtkContextMouseEvent::NO_BUTTON)
  {
    return false;
  }
  return true;
}

//-----------------------------------------------------------------------------
void vtkControlPointsItem::SetCurrentPointPos(const vtkVector2f& newPos)
{
  vtkIdType movedPoint = this->SetPointPos(this->CurrentPoint, newPos);
  // If the moved point was not CurrentPoint then make it current.
  this->SetCurrentPoint(movedPoint);
}

//-----------------------------------------------------------------------------
vtkIdType vtkControlPointsItem::SetPointPos(vtkIdType point, const vtkVector2f& newPos)
{
  if (point == -1)
  {
    return point;
  }

  // Make sure the new point is inside the boundaries of the function
  double boundedPos[2];
  boundedPos[0] = newPos[0];
  boundedPos[1] = newPos[1];
  this->ClampValidPos(boundedPos);

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

  // SetControlPoint will call StartChanges/EndChanges correctly, so we don't
  // need to call it here.
  this->SetControlPoint(point, currentPoint);
  return point;
}

//-----------------------------------------------------------------------------
void vtkControlPointsItem::MoveCurrentPoint(const vtkVector2f& translation)
{
  this->MovePoint(this->CurrentPoint, translation);
}

//-----------------------------------------------------------------------------
vtkIdType vtkControlPointsItem::MovePoint(vtkIdType pointId, const vtkVector2f& translation)
{
  double point[4];
  this->GetControlPoint(pointId, point);
  return this->SetPointPos(pointId, vtkVector2f(point[0] + translation.GetX(), point[1] + translation.GetY()));
}

//-----------------------------------------------------------------------------
void vtkControlPointsItem::MovePoints(const vtkVector2f& translation, vtkIdTypeArray* pointIds)
{
  assert(pointIds);
  this->StartChanges();
  // don't support 'switch' mode yet
  //vtkIdTypeArray* addedSelection = vtkIdTypeArray::New();
  bool oldSwitchPoints = this->SwitchPointsMode;
  this->SwitchPointsMode = false;
  // end "don't support 'switch' mode yet"
  const int count = pointIds->GetNumberOfTuples();
  float tX = translation.GetX();
  float tY = translation.GetY();
  int start = tX < 0.f ? 0 : count - 1;
  int end = tX < 0.f ? count : -1;
  int step = tX < 0.f ? 1 : -1;
  for (vtkIdType i = start; i != end; i+=step)
  {
    vtkIdType pointId = pointIds->GetValue(i);
    double currentPoint[4] = {0.0, 0.0, 0.0, 0.0};
    this->GetControlPoint(pointId, currentPoint);
    vtkVector2f newPos(currentPoint[0] + tX, currentPoint[1] + tY);
    //vtkIdType newIdx =
      this->SetPointPos(pointId, newPos);
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
  this->EndChanges();
}

//-----------------------------------------------------------------------------
void vtkControlPointsItem::MovePoints(const vtkVector2f& translation,
                                      bool dontMoveFirstAndLast)
{
  vtkNew<vtkIdTypeArray> points;
  this->GetControlPointsIds(points.GetPointer(), dontMoveFirstAndLast);
  this->MovePoints(translation, points.GetPointer());
}

//-----------------------------------------------------------------------------
void vtkControlPointsItem::SpreadPoints(float factor, vtkIdTypeArray* pointIds)
{
  assert(pointIds);
  if (pointIds->GetNumberOfTuples() == 0)
  {
    return;
  }
  this->StartChanges();

  double min[2], max[2], center[2];
  double point[4];
  vtkIdType minPointId = pointIds->GetValue(0);
  this->GetControlPoint(minPointId, point);
  min[0] = point[0];
  min[1] = point[1];
  vtkIdType maxPointId = pointIds->GetValue(pointIds->GetNumberOfTuples() - 1);
  this->GetControlPoint(maxPointId, point);
  max[0] = point[0];
  max[1] = point[1];
  center[0] = (min[0] + max[0]) / 2.;
  center[1] = (min[1] + max[1]) / 2.;

  // Left part
  vtkIdType start = 0;
  vtkIdType end = pointIds->GetNumberOfTuples();
  vtkIdType step = 1;
  vtkIdType median = -1; // not needed when factor >= 0
  if (factor < 0.f)
  {
    // search median
    for (vtkIdType i = 0; i < end; ++i)
    {
      vtkIdType pointId = pointIds->GetValue(i);
      this->GetControlPoint(pointId, point);
      if (point[0] > center[0])
      {
        median = i;
        break;
      }
    }
    if (median == -1)
    {
      median = pointIds->GetNumberOfTuples() - 1;
    }
    start = median - 1;
    end = -1;
    step = -1;
  }
  vtkIdType i;
  for (i = start; i != end; i+=step)
  {
    vtkIdType pointId = pointIds->GetValue(i);
    this->GetControlPoint(pointId, point);
    if (point[0] > center[0] ||
        (i != start && point[0] == center[0]))
    {
      break;
    }
    double tX = -factor;
    tX *= (min[0] != center[0]) ?
      (center[0] - point[0]) / (center[0] - min[0]) :
      fabs(point[0]) / 100.;
    vtkVector2f newPos(std::min(point[0] + tX, center[0]), point[1]);
    this->SetPointPos(pointId, newPos);
  }
  // Right part
  start = pointIds->GetNumberOfTuples() - 1;
  end = i - 1;
  step = -1;
  if (factor < 0.f)
  {
    start = median;
    end = pointIds->GetNumberOfTuples();
    step = 1;
  }
  for (i = start; i != end; i+=step)
  {
    vtkIdType pointId = pointIds->GetValue(i);
    this->GetControlPoint(pointId, point);
    assert(point[0] >= center[0]);
    double tX = factor;
    tX *= (max[0] != center[0]) ?
      (point[0] - center[0]) / (max[0] - center[0]) :
      fabs(point[0]) / 100.;
    vtkVector2f newPos(std::max(point[0] + tX, center[0]), point[1]);
    this->SetPointPos(pointId, newPos);
  }
  this->EndChanges();
}

//-----------------------------------------------------------------------------
void vtkControlPointsItem::SpreadPoints(float factor, bool dontSpreadFirstAndLast)
{
  vtkNew<vtkIdTypeArray> points;
  this->GetControlPointsIds(points.GetPointer(), dontSpreadFirstAndLast);
  this->SpreadPoints(factor, points.GetPointer());
}

//-----------------------------------------------------------------------------
vtkVector2f vtkControlPointsItem::GetCenterOfMass(vtkIdTypeArray* pointIDs)const
{
  double average[4] = {0., 0., 0., 0.};
  const vtkIdType pointCount = pointIDs->GetNumberOfTuples();
  for (vtkIdType i = 0; i < pointCount; ++i)
  {
    double point[4];
    this->GetControlPoint(pointIDs->GetValue(i), point);
    average[0] += point[0]; // x
    average[1] += point[1]; // y
    average[2] += point[2]; // midpoint
    average[3] += point[3]; // sharpness
  }
  average[0] /= pointCount; // x
  average[1] /= pointCount; // y
  average[2] /= pointCount; // midpoint
  average[3] /= pointCount; // sharpness
  return vtkVector2f(average[0], average[1]);
}

//-----------------------------------------------------------------------------
void vtkControlPointsItem::Stroke(const vtkVector2f& newPos)
{
  double pos[2];
  pos[0] = newPos[0];
  pos[1] = newPos[1];
  this->ClampValidPos(pos);

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
  if (this->GetInteractionsCount())
  {
    this->EndInteraction();
  }
  if (mouse.GetButton() == vtkContextMouseEvent::LEFT_BUTTON)
  {
    return true;
  }
  if (mouse.GetButton() == vtkContextMouseEvent::RIGHT_BUTTON
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
  if (mouse.GetButton() == vtkContextMouseEvent::MIDDLE_BUTTON
      && this->PointToDelete != -1)
  {
    if (this->PointAboutToBeDeleted)
    {
      // If EnforceValidFunction is true, we don't want less than 2 points
      if (this->IsPointRemovable(this->PointToDelete))
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

//-----------------------------------------------------------------------------
bool vtkControlPointsItem::KeyPressEvent(const vtkContextKeyEvent &key)
{
  bool move = key.GetInteractor()->GetAltKey() != 0 ||
              key.GetInteractor()->GetKeySym() == vtkStdString("plus") ||
              key.GetInteractor()->GetKeySym() == vtkStdString("minus");
  bool select = !move && key.GetInteractor()->GetShiftKey() != 0;
  bool control = key.GetInteractor()->GetControlKey() != 0;
  bool current = !select && !move && !control;
  if (current)
  {
    if (key.GetInteractor()->GetKeySym() == vtkStdString("Right") ||
        key.GetInteractor()->GetKeySym() == vtkStdString("Up"))
    {
      this->SetCurrentPoint(
        std::min(this->GetNumberOfPoints() -1, this->GetCurrentPoint() + 1));
    }
    else if (key.GetInteractor()->GetKeySym() == vtkStdString("Left") ||
             key.GetInteractor()->GetKeySym() == vtkStdString("Down"))
    {
      this->SetCurrentPoint(
        std::max(0, static_cast<int>(this->GetCurrentPoint()) - 1));
    }
    else if (key.GetInteractor()->GetKeySym() == vtkStdString("End"))
    {
      this->SetCurrentPoint(this->GetNumberOfPoints() - 1);
    }
    else if (key.GetInteractor()->GetKeySym() == vtkStdString("Home"))
    {
      this->SetCurrentPoint(0);
    }
  }
  else if (select)
  {
    if (key.GetInteractor()->GetKeySym() == std::string("Right") ||
        key.GetInteractor()->GetKeySym() == std::string("Up"))
    {
      this->SelectPoint(this->CurrentPoint);
      this->SetCurrentPoint(
        std::min(this->GetNumberOfPoints() -1, this->GetCurrentPoint() + 1));
      this->SelectPoint(this->CurrentPoint);
    }
    else if (key.GetInteractor()->GetKeySym() == std::string("Left") ||
             key.GetInteractor()->GetKeySym() == std::string("Down"))
    {
      this->SelectPoint(this->CurrentPoint);
      this->SetCurrentPoint(
        std::max(0, static_cast<int>(this->GetCurrentPoint()) - 1));
      this->SelectPoint(this->CurrentPoint);
    }
    else if (key.GetInteractor()->GetKeySym() == vtkStdString("End"))
    {
      vtkIdType newCurrentPointId = this->GetNumberOfPoints() - 1;
      for (vtkIdType pointId = this->CurrentPoint; pointId < newCurrentPointId; ++pointId)
      {
        this->SelectPoint(pointId);
      }
      this->SelectPoint(newCurrentPointId);
      this->SetCurrentPoint(newCurrentPointId);
    }
    else if (key.GetInteractor()->GetKeySym() == vtkStdString("Home"))
    {
      vtkIdType newCurrentPointId = 0;
      for (vtkIdType pointId = this->CurrentPoint; pointId > newCurrentPointId; --pointId)
      {
        this->SelectPoint(pointId);
      }
      this->SelectPoint(newCurrentPointId);
      this->SetCurrentPoint(newCurrentPointId);
    }
  }
  else if (move)
  {
    vtkVector2f translate(0,0);
    if (key.GetInteractor()->GetKeySym() == std::string("Up"))
    {
      translate = translate + vtkVector2f(0., 1.);
    }
    if (key.GetInteractor()->GetKeySym() == std::string("Down"))
    {
      translate = translate + vtkVector2f(0., -1.);
    }
    if (key.GetInteractor()->GetKeySym() == std::string("Right"))
    {
      translate = translate + vtkVector2f(1., 0.);
    }
    if (key.GetInteractor()->GetKeySym() == std::string("Left"))
    {
      translate = translate + vtkVector2f(-1., 0.);
    }
    if (translate.GetX() != 0.f || translate.GetY() != 0.f)
    {
      double bounds[4];
      this->GetBounds(bounds);
      float step = control ? 0.001 : 0.01;
      translate.SetX( translate.GetX() * (bounds[1] - bounds[0]) * step);
      translate.SetY( translate.GetY() * (bounds[3] - bounds[2]) * step);
      if (this->GetNumberOfSelectedPoints())
      {
        this->StartInteractionIfNotStarted();

        if (vtkIdTypeArray* points = this->GetSelection())
        {
          points->Register(this); // must stay valid after each individual move
          this->MovePoints(translate, points);
          points->UnRegister(this);
        }

        this->Interaction();
      }
      else
      {
        this->StartInteractionIfNotStarted();

        this->MoveCurrentPoint(translate);

        this->Interaction();
      }
    }
    else if (key.GetInteractor()->GetKeySym() == std::string("plus"))
    {
      this->StartInteractionIfNotStarted();

      if (vtkIdTypeArray* pointIds = this->GetSelection())
      {
        pointIds->Register(this); // must stay valid after each individual move
        this->SpreadPoints(1., pointIds);
        pointIds->UnRegister(this);
      }

      this->Interaction();
    }
    else if (key.GetInteractor()->GetKeySym() == std::string("minus"))
    {
      this->StartInteractionIfNotStarted();

      if (vtkIdTypeArray* pointIds = this->GetSelection())
      {
        pointIds->Register(this); // must stay valid after each individual move
        this->SpreadPoints(-1., pointIds);
        pointIds->UnRegister(this);
      }

      this->Interaction();
    }
  }
  else if (control)
  {
    if (key.GetInteractor()->GetKeySym() == std::string("a"))
    {
      this->SelectAllPoints();
    }
  }
  if (key.GetInteractor()->GetKeySym() == vtkStdString("space"))
  {
    this->ToggleSelectPoint(this->GetCurrentPoint());
  }
  else if (key.GetInteractor()->GetKeySym() == vtkStdString("Escape"))
  {
    this->DeselectAllPoints();
  }
  return this->Superclass::KeyPressEvent(key);
}

//-----------------------------------------------------------------------------
bool vtkControlPointsItem::KeyReleaseEvent(const vtkContextKeyEvent &key)
{
  if (key.GetInteractor()->GetKeySym() == std::string("Delete") ||
      key.GetInteractor()->GetKeySym() == std::string("BackSpace"))
  {
    vtkIdType removedPoint = this->RemovePoint(this->GetCurrentPoint());
    if (key.GetInteractor()->GetKeySym() == std::string("BackSpace"))
    {
      this->SetCurrentPoint(removedPoint > 0 ? removedPoint - 1 : 0);
    }
    return true;
  }
  return this->Superclass::KeyPressEvent(key);
}

//-----------------------------------------------------------------------------
bool vtkControlPointsItem::GetEndPointsMovable()
{
  return (this->GetEndPointsXMovable() || this->GetEndPointsYMovable());
}

//-----------------------------------------------------------------------------
bool vtkControlPointsItem::IsEndPointPicked()
{
  int numPts = this->GetNumberOfPoints();
  if(numPts<=0)
  {
    return false;
  }
  if(this->CurrentPoint==0 || this->CurrentPoint==numPts-1)
  {
    return true;
  }
  vtkIdTypeArray* selection = this->GetSelection();
  if(selection && selection->GetNumberOfTuples()>0)
  {
    vtkIdType pid;
    for (vtkIdType i = 0; i < selection->GetNumberOfTuples(); ++i)
    {
      pid=selection->GetValue(i);
      if(pid==0 || pid==numPts-1)
      {
        return true;
      }
    }
  }
  return false;
}

//-----------------------------------------------------------------------------
bool vtkControlPointsItem::IsPointRemovable(vtkIdType pointId)
{
  vtkIdType numPts = this->GetNumberOfPoints();
  if (this->EnforceValidFunction && numPts<= 2)
  {
    return false;
  }
  if(pointId != -1 && !this->GetEndPointsRemovable() &&
    (pointId==0 || pointId==numPts-1))
  {
    return false;
  }
  return true;
}

//-----------------------------------------------------------------------------
vtkStdString vtkControlPointsItem::GetControlPointLabel(vtkIdType pointId)
{
  vtkStdString result;
  if (this->LabelFormat)
  {
    char *buffer = new char[1024];
    double point[4];
    this->GetControlPoint(pointId, point);
    sprintf(buffer, this->LabelFormat, point[0], point[1], point[2], point[3]);
    result = buffer;
    delete []buffer;
  }
  return result;
}
