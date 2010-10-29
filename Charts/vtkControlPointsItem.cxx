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
#include "vtkContextScene.h"
#include "vtkControlPointsItem.h"
#include "vtkIdTypeArray.h"
#include "vtkObjectFactory.h"
#include "vtkPen.h"
#include "vtkPoints2D.h"
#include "vtkSmartPointer.h"
#include "vtkTransform2D.h"

#include <cassert>

//-----------------------------------------------------------------------------
vtkControlPointsItem::vtkControlPointsItem()
{
  this->Pen->SetLineType(vtkPen::SOLID_LINE);
  this->Pen->SetWidth(1.);
  this->Pen->SetColorF(1., 1., 1.);
  this->Brush->SetColorF(0.85, 0.85, 1., 0.75);

  this->Points = vtkPoints2D::New();
  this->Selection = vtkIdTypeArray::New();
  this->SelectedPoints = vtkPoints2D::New();
  this->Callback = vtkCallbackCommand::New();
  this->Callback->SetClientData(this);
  this->Callback->SetCallback(
    vtkControlPointsItem::CallComputePoints);
  this->ScreenPointRadius = 6.f;
  this->ItemPointRadius2 = 0.f;
}

//-----------------------------------------------------------------------------
vtkControlPointsItem::~vtkControlPointsItem()
{
  if (this->Points)
    {
    this->Points->Delete();
    this->Points = 0;
    }
  if (this->SelectedPoints)
    {
    this->SelectedPoints->Delete();
    this->SelectedPoints = 0;
    }
  if (this->Callback)
    {
    this->Callback->Delete();
    this->Callback = 0;
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
  this->Points->GetBounds(bounds);
}

//-----------------------------------------------------------------------------
bool vtkControlPointsItem::Paint(vtkContext2D* painter)
{
  if (this->Points->GetNumberOfPoints())
    {
    painter->ApplyPen(this->Pen);
    painter->ApplyBrush(this->Brush);
    this->DrawPoints(painter, this->Points, this->Selection);
    }
  if (this->SelectedPoints->GetNumberOfPoints())
    {
    painter->GetPen()->SetLineType(vtkPen::SOLID_LINE);
    painter->GetPen()->SetColorF(0.87, 0.87, 1.);
    painter->GetBrush()->SetColorF(0.65, 0.65, 0.95, 0.55);
    this->DrawPoints(painter, this->SelectedPoints);
    }
  // control point size in the item coordinate system;
  double pointSize[4], transformedPointSize[4];
  pointSize[0] = 0.;
  pointSize[1] = 0.;
  pointSize[2] = 0.;
  pointSize[3] = this->ScreenPointRadius;
  painter->GetTransform()->InverseTransformPoints(pointSize, transformedPointSize, 2);
  this->ItemPointRadius2 = (transformedPointSize[2] - transformedPointSize[0])*(transformedPointSize[2] - transformedPointSize[0]) +
    + (transformedPointSize[3] - transformedPointSize[1])*(transformedPointSize[3] - transformedPointSize[1]);
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
  this->Modified();
  if (this->GetScene())
    {
    this->GetScene()->SetDirty(true);
    }
}

//-----------------------------------------------------------------------------
void vtkControlPointsItem::DrawPoints(vtkContext2D* painter, vtkPoints2D* points,
                                      vtkIdTypeArray* pointsToExclude)
{
  vtkTransform2D* sceneTransform = painter->GetTransform();
  vtkSmartPointer<vtkTransform2D> translation =
    vtkSmartPointer<vtkTransform2D>::New();

  double point[2];
  double pointInScene[2];

  const int count = points->GetNumberOfPoints();
  for (vtkIdType i = 0; i < count; ++i)
    {
    vtkIdType idx = pointsToExclude ? pointsToExclude->LookupValue(i) : -1;
    if (idx != -1)
      {
      continue;
      }
    points->GetPoint(i, point);
    sceneTransform->TransformPoints(point, pointInScene, 1);

    painter->PushMatrix();
    translation->Identity();
    translation->Translate(pointInScene[0], pointInScene[1]);
    painter->SetTransform(translation);
    painter->DrawWedge(0.f, 0.f, this->ScreenPointRadius, 0.f, 0.f, 360.f);
    painter->DrawArc(0.f, 0.f, this->ScreenPointRadius, 0.f, 360.f);
    painter->PopMatrix();
    }
}

//-----------------------------------------------------------------------------
void vtkControlPointsItem::SelectPoint(double* currentPoint)
{
  vtkIdType pointId = this->GetPointId(currentPoint);
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
  double point[2];
  this->Points->GetPoint(pointId, point);
  this->Selection->InsertNextValue(pointId);
  this->SelectedPoints->InsertNextPoint(point);
}

//-----------------------------------------------------------------------------
void vtkControlPointsItem::DeselectPoint(double* currentPoint)
{
  // make sure the point belongs to the list of points
  vtkIdType pointId = this->GetPointId(currentPoint);
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
    vtkErrorMacro(<< "Point:" << pointId << " was not selected");
    return;
    }
  this->Selection->RemoveTuple(selectionPointId);
  this->SelectedPoints->GetData()->RemoveTuple(selectionPointId);
}

//-----------------------------------------------------------------------------
void vtkControlPointsItem::DeselectAllPoints()
{
  this->Selection->SetNumberOfTuples(0);
  this->SelectedPoints->GetData()->SetNumberOfTuples(0);
}

//-----------------------------------------------------------------------------
void vtkControlPointsItem::ToggleSelectPoint(double* currentPoint)
{
  // make sure the point belongs to the list of points
  vtkIdType pointId = this->GetPointId(currentPoint);
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
vtkIdType vtkControlPointsItem::GetPointId(double* pos, double tolerance)
{
  if (tolerance == -1.)
    {
    // Add 1.5 ratio for imprecision
    tolerance = this->ItemPointRadius2* 1.5;
    }
  // make sure the point belongs to the list of points
  vtkIdType pointId = -1;
  double point[2];
  double minDist = VTK_DOUBLE_MAX;
  const int numberOfPoints = this->Points->GetNumberOfPoints();
  for(vtkIdType i = 0; i < numberOfPoints; ++i)
    {
    this->Points->GetPoint(i, point);
    double distance = (point[0] - pos[0]) * (point[0] - pos[0]) +
      (point[1] - pos[1]) * (point[1] - pos[1]);
    if (distance < tolerance)
      {
      if (distance == 0.)
        {// we found the best match ever
        return i;
        }
      else if (distance < minDist)
        {
        pointId = i;
        minDist = distance;
        }
      }
    // don't search any further if the x is already too large
    if (point[0] > pos[0] + this->ItemPointRadius2)
      {
      break;
      }
    }
  return pointId;
}
