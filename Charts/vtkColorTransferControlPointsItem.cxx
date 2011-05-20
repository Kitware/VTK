/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkColorTransferControlPointsItem.cxx

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
#include "vtkColorTransferFunction.h"
#include "vtkColorTransferControlPointsItem.h"
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
vtkStandardNewMacro(vtkColorTransferControlPointsItem);

//-----------------------------------------------------------------------------
vtkColorTransferControlPointsItem::vtkColorTransferControlPointsItem()
{
  this->ColorTransferFunction = 0;
  this->ColorFill = false;
}

//-----------------------------------------------------------------------------
vtkColorTransferControlPointsItem::~vtkColorTransferControlPointsItem()
{
  if (this->ColorTransferFunction)
    {
    this->ColorTransferFunction->Delete();
    this->ColorTransferFunction = 0;
    }
}

//-----------------------------------------------------------------------------
void vtkColorTransferControlPointsItem::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ColorTransferFunction: ";
  if (this->ColorTransferFunction)
    {
    os << endl;
    this->ColorTransferFunction->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }
}

//-----------------------------------------------------------------------------
unsigned long int vtkColorTransferControlPointsItem::GetControlPointsMTime()
{
  if (this->ColorTransferFunction)
    {
    return this->ColorTransferFunction->GetMTime();
    }
  return this->GetMTime();
}

//-----------------------------------------------------------------------------
void vtkColorTransferControlPointsItem::SetColorTransferFunction(vtkColorTransferFunction* t)
{
  vtkSetObjectBodyMacro(ColorTransferFunction, vtkColorTransferFunction, t);
  if (this->ColorTransferFunction)
    {
    this->ColorTransferFunction->AddObserver(vtkCommand::ModifiedEvent, this->Callback);
    }
  this->ResetBounds();
  this->ComputePoints();
}

//-----------------------------------------------------------------------------
void vtkColorTransferControlPointsItem::DrawPoint(vtkContext2D* painter, vtkIdType index)
{
  if (this->ColorFill)
    {
    double xrgbms[6];
    this->ColorTransferFunction->GetNodeValue(index, xrgbms);
    painter->GetBrush()->SetColorF(xrgbms[1], xrgbms[2], xrgbms[3], 0.55);
    }
  this->vtkControlPointsItem::DrawPoint(painter, index);
}

//-----------------------------------------------------------------------------
int vtkColorTransferControlPointsItem::GetNumberOfPoints()const
{
  return this->ColorTransferFunction ? this->ColorTransferFunction->GetSize() : 0;
}

//-----------------------------------------------------------------------------
void vtkColorTransferControlPointsItem::GetControlPoint(vtkIdType index, double* pos)
{
  double xrgbms[6];
  this->ColorTransferFunction->GetNodeValue(index, xrgbms);
  pos[0] = xrgbms[0];
  pos[1] = 0.5;
  pos[2] = xrgbms[4];
  pos[3] = xrgbms[5];
}

//-----------------------------------------------------------------------------
void vtkColorTransferControlPointsItem::SetControlPoint(vtkIdType index, double* newPos)
{
  double xrgbms[6];
  this->ColorTransferFunction->GetNodeValue(index, xrgbms);
  if (newPos[0] != xrgbms[0] ||
      newPos[2] != xrgbms[1] ||
      newPos[3] != xrgbms[2])
    {
    xrgbms[0] = newPos[0];
    xrgbms[4] = newPos[2];
    xrgbms[5] = newPos[3];
    this->ColorTransferFunction->SetNodeValue(index, xrgbms);
    }
}

//-----------------------------------------------------------------------------
void vtkColorTransferControlPointsItem::EditPoint(float tX, float tY)
{
  if (!this->ColorTransferFunction)
    {
    return;
    }
  double xrgbms[6];
  this->ColorTransferFunction->GetNodeValue(this->CurrentPoint, xrgbms);
  xrgbms[4] += tX;
  xrgbms[5] += tY;
  this->ColorTransferFunction->SetNodeValue(this->CurrentPoint, xrgbms);
  if (this->CurrentPoint > 0)
    {
    this->ColorTransferFunction->GetNodeValue(this->CurrentPoint - 1, xrgbms);
    xrgbms[4] += tX;
    xrgbms[5] += tY;
    this->ColorTransferFunction->SetNodeValue(this->CurrentPoint - 1, xrgbms);
    }
}

//-----------------------------------------------------------------------------
vtkIdType vtkColorTransferControlPointsItem::AddPoint(double* newPos)
{
  if (!this->ColorTransferFunction)
    {
    return -1;
    }
#ifndef NDEBUG
  vtkIdType expectedPoint =
#endif
    this->vtkControlPointsItem::AddPoint(newPos);
  double rgb[3] = {0., 0., 0.};
  this->ColorTransferFunction->GetColor(newPos[0], rgb);
  vtkIdType addedPoint =
    this->ColorTransferFunction->AddRGBPoint(newPos[0], rgb[0], rgb[1], rgb[2]);
  assert(addedPoint == expectedPoint);
  return addedPoint;
}

//-----------------------------------------------------------------------------
vtkIdType vtkColorTransferControlPointsItem::RemovePoint(double* currentPoint)
{
  if (!this->ColorTransferFunction)
    {
    return -1;
    }
#ifndef NDEBUG
  vtkIdType expectedPoint =
#endif
    this->vtkControlPointsItem::RemovePoint(currentPoint);
  vtkIdType removedPoint =
    this->ColorTransferFunction->RemovePoint(currentPoint[0]);
  assert(removedPoint == expectedPoint);
  return removedPoint;
}
