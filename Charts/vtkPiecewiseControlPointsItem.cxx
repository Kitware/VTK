/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPiecewiseControlPointsItem.cxx

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
#include "vtkIdTypeArray.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPiecewiseControlPointsItem.h"
#include "vtkObjectFactory.h"
#include "vtkPen.h"
#include "vtkPoints2D.h"

// to handle mouse.GetButton
#include "vtkContextScene.h"

#include <cassert>
#include <limits>
#include <algorithm>

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPiecewiseControlPointsItem);

//-----------------------------------------------------------------------------
vtkPiecewiseControlPointsItem::vtkPiecewiseControlPointsItem()
{
  this->PiecewiseFunction = 0;

  MouseOver = -1;
}

//-----------------------------------------------------------------------------
vtkPiecewiseControlPointsItem::~vtkPiecewiseControlPointsItem()
{
  if (this->PiecewiseFunction)
    {
    this->PiecewiseFunction->Delete();
    this->PiecewiseFunction = 0;
    }
}

//-----------------------------------------------------------------------------
void vtkPiecewiseControlPointsItem::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "PiecewiseFunction: ";
  if (this->PiecewiseFunction)
    {
    os << endl;
    this->PiecewiseFunction->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }
}

//-----------------------------------------------------------------------------
void vtkPiecewiseControlPointsItem::SetPiecewiseFunction(vtkPiecewiseFunction* t)
{
  vtkSetObjectBodyMacro(PiecewiseFunction, vtkPiecewiseFunction, t);
  this->PiecewiseFunction->AddObserver(vtkCommand::ModifiedEvent, this->Callback);
  this->ComputePoints();
}

//-----------------------------------------------------------------------------
void vtkPiecewiseControlPointsItem::ComputePoints()
{
  int size = this->PiecewiseFunction ? this->PiecewiseFunction->GetSize() : 0;
  this->Points->SetNumberOfPoints(size);
  if (!size)
    {
    this->Selection->SetNumberOfTuples(0);
    this->SelectedPoints->SetNumberOfPoints(0);
    return;
    }
  double node[4];
  for (vtkIdType i = 0; i < size; ++i)
    {
    this->PiecewiseFunction->GetNodeValue(i,node);
    this->Points->SetPoint(i, node[0], node[1]);
    }
  size = this->Selection->GetNumberOfTuples();
  if (size)
    {
    vtkIdTypeArray* oldSelection = this->Selection;
    vtkPoints2D* oldSelectedPoints = this->SelectedPoints;
    this->Selection = vtkIdTypeArray::New();
    this->SelectedPoints = vtkPoints2D::New();
    for (vtkIdType i = 0; i < size; ++i)
      {
      this->SelectPoint(oldSelection->GetValue(i));
      }
    oldSelection->Delete();
    oldSelectedPoints->Delete();
    }
  this->Superclass::ComputePoints();
}

//-----------------------------------------------------------------------------
bool vtkPiecewiseControlPointsItem::Hit(const vtkContextMouseEvent &mouse)
{
  double pos[2];
  pos[0] = mouse.Pos[0];
  pos[1] = mouse.Pos[1];
  return this->GetPointId(pos) != -1;
}

//-----------------------------------------------------------------------------
bool vtkPiecewiseControlPointsItem::MouseMoveEvent(const vtkContextMouseEvent &mouse)
{
  if (mouse.Button != vtkContextMouseEvent::LEFT_BUTTON)
    {
    return false;
    }
  if (this->MouseOver >= 0)
    {
    double currentPoint[4] = {0.0, 0.0, 0.0, 0.0};
    this->PiecewiseFunction->GetNodeValue(this->MouseOver, currentPoint);
    currentPoint[0] = mouse.Pos[0];
    currentPoint[1] = mouse.Pos[1];
    this->PiecewiseFunction->SetNodeValue(this->MouseOver, currentPoint);
    return true;
    }
  else // should only happen on a draw mode
    {
    //this->PiecewiseFunction->AddPoint(mouse.Pos[0], mouse.Pos[1]);
    }
  return false;
}

//-----------------------------------------------------------------------------
bool vtkPiecewiseControlPointsItem::MouseButtonPressEvent(const vtkContextMouseEvent &mouse)
{
  if (mouse.Button != vtkContextMouseEvent::LEFT_BUTTON)
    {
    return false;
    }
  this->ButtonPressPosition[0] = mouse.Pos[0];
  this->ButtonPressPosition[1] = mouse.Pos[1];

  double pos[2];
  pos[0] = mouse.Pos[0];
  pos[1] = mouse.Pos[1];
  this->MouseOver = this->GetPointId(pos);
  if (this->MouseOver == -1)
    {
    this->DeselectAllPoints();
    this->GetScene()->SetDirty(true);
    return false;
    }
  return true;
}

//-----------------------------------------------------------------------------
bool vtkPiecewiseControlPointsItem::MouseButtonReleaseEvent(const vtkContextMouseEvent &mouse)
{
  if (mouse.Button != vtkContextMouseEvent::LEFT_BUTTON)
    {
    return false;
    }
  int deltaX = static_cast<int>(mouse.Pos[0] - this->ButtonPressPosition[0]);
  int deltaY = static_cast<int>(mouse.Pos[1] - this->ButtonPressPosition[1]);

  double point[2];
  point[0] = mouse.Pos[0];
  point[1] = mouse.Pos[1];
  // If there is a point under the mouse, invert its state (highlight or not)
  if (this->MouseOver != -1 &&
      (deltaX * deltaX + deltaY * deltaY < this->ItemPointRadius2))
    {
    vtkIdType pointId = this->GetPointId(point);
    if (pointId != -1)
      {
      this->ToggleSelectPoint(pointId);
      this->GetScene()->SetDirty(true);
      this->MouseOver = -1;
      return true;
      }
    }
  if (this->MouseOver == -1)
    {
    // offset all the point ids
    vtkIdType nextPointId = -1;
    int size = this->Points->GetNumberOfPoints();
    for (vtkIdType i = 0; i < size; ++i)
      {
      double* point = this->Points->GetPoint(i);
      if (point[0] > mouse.Pos[0])
        {
        nextPointId = i;
        break;
        }
      }
    if (nextPointId != -1)
      {
      size = this->Selection->GetNumberOfTuples();
      for (vtkIdType i = 0; i < size; ++i)
        {
        vtkIdType pointId = this->Selection->GetValue(i);
        if (pointId > nextPointId)
          {
          this->Selection->SetValue(i, ++pointId);
          }
        }
      }
    this->PiecewiseFunction->AddPoint(mouse.Pos[0], mouse.Pos[1]);
    // TBD should the point be selected by default ?
    //this->DeselectAllPoints();
    return true;
    }
  this->MouseOver = -1;
  return false;
}
