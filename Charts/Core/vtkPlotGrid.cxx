/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlotGrid.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPlotGrid.h"

#include "vtkContext2D.h"
#include "vtkPoints2D.h"
#include "vtkPen.h"
#include "vtkAxis.h"
#include "vtkFloatArray.h"
#include "vtkVector.h"

#include "vtkObjectFactory.h"

//-----------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkPlotGrid, XAxis, vtkAxis);
vtkCxxSetObjectMacro(vtkPlotGrid, YAxis, vtkAxis);
//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPlotGrid);

//-----------------------------------------------------------------------------
vtkPlotGrid::vtkPlotGrid()
{
  this->XAxis = NULL;
  this->YAxis = NULL;
}

//-----------------------------------------------------------------------------
vtkPlotGrid::~vtkPlotGrid()
{
  this->SetXAxis(NULL);
  this->SetYAxis(NULL);
}

//-----------------------------------------------------------------------------
bool vtkPlotGrid::Paint(vtkContext2D *painter)
{
  if (!this->XAxis || !this->YAxis)
    {
    // Need axes to define where our grid lines should be drawn
    vtkDebugMacro(<<"No axes set and so grid lines cannot be drawn.");
    return false;
    }

  vtkVector2f x1, x2, y1, y2;
  this->XAxis->GetPoint1(x1.GetData());
  this->XAxis->GetPoint2(x2.GetData());
  this->YAxis->GetPoint1(y1.GetData());
  this->YAxis->GetPoint2(y2.GetData());

  // in x
  if (this->XAxis->GetVisible() && this->XAxis->GetGridVisible())
    {
    vtkFloatArray *xLines = this->XAxis->GetTickScenePositions();
    painter->ApplyPen(this->XAxis->GetGridPen());
    float *xPositions = xLines->GetPointer(0);
    for (int i = 0; i < xLines->GetNumberOfTuples(); ++i)
      {
      painter->DrawLine(xPositions[i], y1.GetY(), xPositions[i], y2.GetY());
      }
    }

  // in y
  if (this->YAxis->GetVisible() && this->YAxis->GetGridVisible())
    {
    vtkFloatArray *yLines = this->YAxis->GetTickScenePositions();
    painter->ApplyPen(this->YAxis->GetGridPen());
    float *yPositions = yLines->GetPointer(0);
    for (int i = 0; i < yLines->GetNumberOfTuples(); ++i)
      {
      painter->DrawLine(x1.GetX(), yPositions[i], x2.GetX(), yPositions[i]);
      }
    }

  return true;
}

//-----------------------------------------------------------------------------
void vtkPlotGrid::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
