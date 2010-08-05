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
  MouseButtonPressed = -1;
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
// good or not...?
void vtkPiecewiseControlPointsItem::ComputePoints()
{
  const int size = this->PiecewiseFunction ? this->PiecewiseFunction->GetSize() : 0;
  this->Points->SetNumberOfPoints(size);
  if (!size)
    {
    return;
    }
  double node[4];
  for (int i = 0; i < size; ++i)
    {
    this->PiecewiseFunction->GetNodeValue(i,node);
    this->Points->SetPoint(i, node[0], node[1]);
    }
}

//-----------------------------------------------------------------------------
bool vtkPiecewiseControlPointsItem::Hit(const vtkContextMouseEvent &mouse)
{
  if(this->MouseOver >= 0)
    {
    return true;
    }

  return false;
}

//-----------------------------------------------------------------------------
bool vtkPiecewiseControlPointsItem::MouseEnterEvent(const vtkContextMouseEvent &mouse)
{
  // Not efficient enough there must be a better way ...
  // Get the index of the current point
  double x = mouse.Pos[0];
  int numberOfNodes = this->PiecewiseFunction->GetSize();
  double val[4];
  double closeVal = std::numeric_limits<double>::max();
  double closeID;
  double diff;

  for(int i=0; i< numberOfNodes; ++i)
    {
    this->PiecewiseFunction->GetNodeValue(i, val);
    diff = abs(val[0]-x);
    if(diff < closeVal)
      {
      closeVal = diff;
      this->MouseOver = i;
      }
    }

  return true;
}

//-----------------------------------------------------------------------------
bool vtkPiecewiseControlPointsItem::MouseMoveEvent(const vtkContextMouseEvent &mouse)
{
  if(this->MouseButtonPressed >= 0)
    {
    double currentPoint[4] = {0.0};
    this->PiecewiseFunction->GetNodeValue(this->MouseOver, currentPoint);
    this->PiecewiseFunction->RemovePoint(currentPoint[0]);
    this->PiecewiseFunction->AddPoint(mouse.Pos[0], mouse.Pos[1]);
    // Update this->Highlight
    //this->PiecewiseFunction->Update();
    //this->Update();
    return true;
    }

  return false;
}

//-----------------------------------------------------------------------------
bool vtkPiecewiseControlPointsItem::MouseLeaveEvent(const vtkContextMouseEvent &mouse)
{
  this->MouseOver = -1;

  return true;
}

//-----------------------------------------------------------------------------
bool vtkPiecewiseControlPointsItem::MouseButtonPressEvent(const vtkContextMouseEvent &mouse)
{
  this->LastPosition[0] = mouse.Pos[0];
  this->LastPosition[1] = mouse.Pos[1];
  this->MouseButtonPressed = mouse.Button;

  return true;
}

//-----------------------------------------------------------------------------
bool vtkPiecewiseControlPointsItem::MouseButtonReleaseEvent(const vtkContextMouseEvent &mouse)
{
  int deltaX = static_cast<int>(mouse.Pos[0] - this->LastPosition[0]);
  int deltaY = static_cast<int>(mouse.Pos[1] - this->LastPosition[1]);

  // If there is a point under the mouse, invert its state (highlight or not)
  if((this->MouseOver >= 0) && (deltaX == 0) && (deltaY == 0))
    {
    double currentPoint[4] = {0.0};
    // Get the coordinates of the current point
    this->PiecewiseFunction->GetNodeValue(this->MouseOver, currentPoint);
    HighlightCurrentPoint(currentPoint);
    }

  this->MouseButtonPressed = -1;
  return true;
}
