/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositeControlPointsItem.cxx

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
#include "vtkCompositeControlPointsItem.h"
#include "vtkObjectFactory.h"
#include "vtkPen.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPoints2D.h"
#include "vtkContextScene.h"

// to handle mouse.GetButton
#include "vtkContextMouseEvent.h"

#include <cassert>
#include <limits>
#include <algorithm>

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkCompositeControlPointsItem);

//-----------------------------------------------------------------------------
vtkCompositeControlPointsItem::vtkCompositeControlPointsItem()
{
  this->OpacityFunction = 0;
  this->Updating = false;
  this->ColorFill = true;
}

//-----------------------------------------------------------------------------
vtkCompositeControlPointsItem::~vtkCompositeControlPointsItem()
{
  if (this->OpacityFunction)
    {
    this->OpacityFunction->Delete();
    this->OpacityFunction = 0;
    }
}

//-----------------------------------------------------------------------------
void vtkCompositeControlPointsItem::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "OpacityFunction: ";
  if (this->OpacityFunction)
    {
    os << endl;
    this->OpacityFunction->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }
}

//-----------------------------------------------------------------------------
unsigned long int vtkCompositeControlPointsItem::GetControlPointsMTime()
{
  if (this->OpacityFunction)
    {
    return this->OpacityFunction->GetMTime();
    }
  return this->Superclass::GetControlPointsMTime();
}

//-----------------------------------------------------------------------------
void vtkCompositeControlPointsItem::SetOpacityFunction(vtkPiecewiseFunction* function)
{
  vtkSetObjectBodyMacro(OpacityFunction, vtkPiecewiseFunction, function);
  this->MergeColorTransferFunction();
  if (this->OpacityFunction)
    {
    this->OpacityFunction->AddObserver(vtkCommand::ModifiedEvent, this->Callback);
    }
  this->ResetBounds();
  this->ComputePoints();
}

//-----------------------------------------------------------------------------
void vtkCompositeControlPointsItem::SetColorTransferFunction(vtkColorTransferFunction* c)
{
  // We need to set the color transfer function here (before
  // Superclass::SetPiecewiseFunction) to be able to have a valid
  // color transfer function for MergeColorTransferFunction().
  vtkSetObjectBodyMacro(ColorTransferFunction, vtkColorTransferFunction, c);
  this->MergeColorTransferFunction();
  this->Superclass::SetColorTransferFunction(c);
}
//-----------------------------------------------------------------------------
void vtkCompositeControlPointsItem::ComputePoints()
{
  if (this->Updating)
    {
    return;
    }
  this->Superclass::ComputePoints();
}

//-----------------------------------------------------------------------------
void vtkCompositeControlPointsItem::SetControlPoint(vtkIdType index, double* newPos)
{
  this->Superclass::SetControlPoint(index, newPos);
  this->OpacityFunction->SetNodeValue(index, newPos);
}

//-----------------------------------------------------------------------------
void vtkCompositeControlPointsItem::GetControlPoint(vtkIdType index, double* pos)
{
  if (!this->OpacityFunction)
    {
    this->Superclass::GetControlPoint(index, pos);
    return;
    }
  this->OpacityFunction->GetNodeValue(index, pos);
}
//-----------------------------------------------------------------------------
void vtkCompositeControlPointsItem::EditPoint(float tX, float tY)
{
  this->Superclass::EditPoint(tX, tY);
  if (!this->OpacityFunction)
    {
    return;
    }
  double xvms[4];
  this->OpacityFunction->GetNodeValue(this->CurrentPoint, xvms);
  xvms[2] += tX;
  xvms[3] += tY;
  this->OpacityFunction->SetNodeValue(this->CurrentPoint, xvms);
  if (this->CurrentPoint > 0)
    {
    this->OpacityFunction->GetNodeValue(this->CurrentPoint - 1, xvms);
    xvms[2] += tX;
    xvms[3] += tY;
    this->ColorTransferFunction->SetNodeValue(this->CurrentPoint - 1, xvms);
    }
}

//-----------------------------------------------------------------------------
vtkIdType vtkCompositeControlPointsItem::AddPoint(double* newPos)
{
  bool oldUpdating = this->Updating;
  this->Updating = true;
  vtkIdType addedPoint = this->Superclass::AddPoint(newPos);
  this->Updating = oldUpdating;
  if (!this->OpacityFunction)
    {
    return addedPoint;
    }
  return this->OpacityFunction->AddPoint(newPos[0], newPos[1]);
}

//-----------------------------------------------------------------------------
vtkIdType vtkCompositeControlPointsItem::RemovePoint(double* currentPoint)
{
  bool oldUpdating = this->Updating;
  this->Updating = true;
  vtkIdType removedPoint = this->Superclass::RemovePoint(currentPoint);
  this->Updating = oldUpdating;
  if (!this->OpacityFunction)
    {
    return removedPoint;
    }
  return this->OpacityFunction->RemovePoint(currentPoint[0]);
}

//-----------------------------------------------------------------------------
void vtkCompositeControlPointsItem::MergeColorTransferFunction()
{
  if (!this->ColorTransferFunction || !this->OpacityFunction)
    {
    return;
    }
  // Naive implementation that does the work but can be a bit slow
  // Copy OpacityFunction points into the ColorTransferFunction
  const int piecewiseFunctionCount = this->OpacityFunction->GetSize();
  for (int i = 0; i < piecewiseFunctionCount; ++i)
    {
    double piecewisePoint[4];
    this->OpacityFunction->GetNodeValue(i, piecewisePoint);
    double rgb[3];
    this->ColorTransferFunction->GetColor(piecewisePoint[0], rgb);
    // note that we might loose the midpoint/sharpness of the point if any
    this->ColorTransferFunction->RemovePoint(piecewisePoint[0]);
    this->ColorTransferFunction->AddRGBPoint(piecewisePoint[0], rgb[0], rgb[1], rgb[2], piecewisePoint[2], piecewisePoint[3]);
    }
  // Copy ColorTransferFunction points into the OpacityFunction
  const int colorFunctionCount = this->ColorTransferFunction->GetSize();
  for (int i = 0; i < colorFunctionCount; ++i)
    {
    double xrgbms[6];
    this->ColorTransferFunction->GetNodeValue(i, xrgbms);
    double value = this->OpacityFunction->GetValue(xrgbms[0]);
    // note that we might loose the midpoint/sharpness of the point if any
    this->OpacityFunction->RemovePoint(xrgbms[0]);
    this->OpacityFunction->AddPoint(xrgbms[0], value, xrgbms[4], xrgbms[5]);
    }
}
