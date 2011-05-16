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
#include "vtkContextScene.h"

// to handle mouse.GetButton
#include "vtkContextMouseEvent.h"

#include <cassert>
#include <limits>
#include <algorithm>

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPiecewiseControlPointsItem);

//-----------------------------------------------------------------------------
vtkPiecewiseControlPointsItem::vtkPiecewiseControlPointsItem()
{
  this->PiecewiseFunction = 0;
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
unsigned long int vtkPiecewiseControlPointsItem::GetControlPointsMTime()
{
  if (this->PiecewiseFunction)
    {
    return this->PiecewiseFunction->GetMTime();
    }
  return this->GetMTime();
}

//-----------------------------------------------------------------------------
void vtkPiecewiseControlPointsItem::SetPiecewiseFunction(vtkPiecewiseFunction* t)
{
  vtkSetObjectBodyMacro(PiecewiseFunction, vtkPiecewiseFunction, t);
  if (this->PiecewiseFunction)
    {
    this->PiecewiseFunction->AddObserver(vtkCommand::ModifiedEvent, this->Callback);
    }
  this->ResetBounds();
  this->ComputePoints();
}

//-----------------------------------------------------------------------------
int vtkPiecewiseControlPointsItem::GetNumberOfPoints()const
{
  return this->PiecewiseFunction ? this->PiecewiseFunction->GetSize() : 0;
}

//-----------------------------------------------------------------------------
void vtkPiecewiseControlPointsItem::GetControlPoint(vtkIdType index, double* pos)
{
  this->PiecewiseFunction->GetNodeValue(index, pos);
}

//-----------------------------------------------------------------------------
void vtkPiecewiseControlPointsItem::SetControlPoint(vtkIdType index, double* newPos)
{
  double oldPos[4];
  this->PiecewiseFunction->GetNodeValue(index, oldPos);
  if (newPos[0] != oldPos[0] || newPos[1] != oldPos[1] ||
      newPos[2] != oldPos[2] || newPos[2] != oldPos[2])
    {
    this->PiecewiseFunction->SetNodeValue(index, newPos);
    }
}

//-----------------------------------------------------------------------------
void vtkPiecewiseControlPointsItem::EditPoint(float tX, float tY)
{
  if (!this->PiecewiseFunction)
    {
    return;
    }
  double xvms[4];
  this->PiecewiseFunction->GetNodeValue(this->CurrentPoint, xvms);
  xvms[2] += tX;
  xvms[3] += tY;
  this->PiecewiseFunction->SetNodeValue(this->CurrentPoint, xvms);
  if (this->CurrentPoint > 0)
    {
    this->PiecewiseFunction->GetNodeValue(this->CurrentPoint - 1, xvms);
    xvms[2] += tX;
    xvms[3] += tY;
    this->PiecewiseFunction->SetNodeValue(this->CurrentPoint - 1, xvms);
    }
}

//-----------------------------------------------------------------------------
vtkIdType vtkPiecewiseControlPointsItem::AddPoint(double* newPos)
{
  if (!this->PiecewiseFunction)
    {
    return -1;
    }
#ifndef NDEBUG
  vtkIdType expectedPoint =
#endif
    this->vtkControlPointsItem::AddPoint(newPos);
  vtkIdType addedPoint = this->PiecewiseFunction->AddPoint(newPos[0], newPos[1]);
  assert(addedPoint == expectedPoint);
  return addedPoint;
}

//-----------------------------------------------------------------------------
vtkIdType vtkPiecewiseControlPointsItem::RemovePoint(double* currentPoint)
{
  if (!this->PiecewiseFunction)
    {
    return -1;
    }
#ifndef NDEBUG
  vtkIdType expectedPoint =
#endif
    this->vtkControlPointsItem::RemovePoint(currentPoint);
  vtkIdType removedPoint =
    this->PiecewiseFunction->RemovePoint(currentPoint[0]);
  assert(removedPoint == expectedPoint);
  return removedPoint;
}
