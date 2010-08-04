/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlotLine.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPlotLine.h"

#include "vtkContext2D.h"
#include "vtkPen.h"

#include "vtkObjectFactory.h"


//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPlotLine);

//-----------------------------------------------------------------------------
vtkPlotLine::vtkPlotLine()
{
  this->MarkerStyle = vtkPlotPoints::NONE;
}

//-----------------------------------------------------------------------------
vtkPlotLine::~vtkPlotLine()
{
}

//-----------------------------------------------------------------------------
bool vtkPlotLine::Paint(vtkContext2D *painter)
{
  // This is where everything should be drawn, or dispatched to other methods.
  vtkDebugMacro(<< "Paint event called in vtkPlotLine.");

  if (!this->Visible || !this->Points)
    {
    return false;
    }

  // Draw the line between the points
  painter->ApplyPen(this->Pen);
  painter->DrawPoly(this->Points);

  return this->vtkPlotPoints::Paint(painter);
}

//-----------------------------------------------------------------------------
bool vtkPlotLine::PaintLegend(vtkContext2D *painter, float rect[4])
{
  painter->ApplyPen(this->Pen);
  painter->DrawLine(rect[0], rect[1]+0.5*rect[3],
                    rect[0]+rect[2], rect[1]+0.5*rect[3]);
  this->Superclass::PaintLegend(painter, rect);
  return true;
}

//-----------------------------------------------------------------------------
void vtkPlotLine::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
