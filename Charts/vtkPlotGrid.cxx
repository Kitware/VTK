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

#include "vtkObjectFactory.h"

//-----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkPlotGrid, "1.1");
vtkCxxSetObjectMacro(vtkPlotGrid, XAxis, vtkAxis);
vtkCxxSetObjectMacro(vtkPlotGrid, YAxis, vtkAxis);
//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPlotGrid);

//-----------------------------------------------------------------------------
vtkPlotGrid::vtkPlotGrid()
{
  this->XAxis = NULL;
  this->YAxis = NULL;
  this->Point1[0] = 0.0;
  this->Point1[1] = 0.0;
  this->Point2[0] = 0.0;
  this->Point2[1] = 0.0;
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
  float ignored; // Values I want to ignore when getting others
  this->XAxis->GetPoint1(&this->Point1[0]);
  this->XAxis->GetPoint2(this->Point2[0], ignored);
  this->YAxis->GetPoint2(ignored, this->Point2[1]);

  // Now do some grid drawing...
  painter->GetPen()->SetWidth(1.0);

  // in x
  int xLines = this->XAxis->GetNumberOfTicks();
  float spacing = (this->Point2[0] - this->Point1[0]) / float(xLines-1);
  for (int i = 0; i < xLines; ++i)
    {
    painter->DrawLine(int(this->Point1[0] + i*spacing), this->Point1[1],
                      int(this->Point1[0] + i*spacing), this->Point2[1]);
    }

  // in y
  int yLines = this->YAxis->GetNumberOfTicks();
  spacing = (this->Point2[1] - this->Point1[1]) / float(yLines-1);
  for (int i = 0; i < yLines; ++i)
    {
    painter->DrawLine(this->Point1[0], int(this->Point1[1] + i*spacing),
                      this->Point2[0], int(this->Point1[1] + i*spacing));
    }
}

//-----------------------------------------------------------------------------
void vtkPlotGrid::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

}
