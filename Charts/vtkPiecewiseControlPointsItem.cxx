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

#include <cassert>

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
void vtkPiecewiseControlPointsItem::SetPiecewiseFunction(vtkPiecewiseFunction* t)
{
  vtkSetObjectBodyMacro(PiecewiseFunction, vtkPiecewiseFunction, t);
  this->PiecewiseFunction->AddObserver(vtkCommand::ModifiedEvent, this->Callback);
  this->ComputePoints();
}

//-----------------------------------------------------------------------------
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
