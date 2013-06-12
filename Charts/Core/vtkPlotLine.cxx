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
#include "vtkRect.h"
#include "vtkPoints2D.h"
#include "vtkIdTypeArray.h"

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

  if (this->BadPoints && this->BadPoints->GetNumberOfTuples() > 0)
    {
    // draw lines skipping bad points
    float *points = static_cast<float *>(this->Points->GetVoidPointer(0));

    vtkIdType lastGood = 0;

    for (vtkIdType i = 0; i < this->BadPoints->GetNumberOfTuples(); i++)
      {
      vtkIdType id = this->BadPoints->GetValue(i);

      // render from last good point to one before this bad point
      if (id - lastGood > 2)
        {
        painter->DrawPoly(points + 2 * (lastGood + 1), id - lastGood - 1);
        }

      lastGood = id;
      }

    // render any trailing good points
    if (this->Points->GetNumberOfPoints() - lastGood > 2)
      {
      painter->DrawPoly(points + 2 * (lastGood + 1),
                        this->Points->GetNumberOfPoints() - lastGood - 1);
      }
    }
  else
    {
    // draw lines between all points
    painter->DrawPoly(this->Points);
    }

  return this->vtkPlotPoints::Paint(painter);
}

//-----------------------------------------------------------------------------
bool vtkPlotLine::PaintLegend(vtkContext2D *painter, const vtkRectf& rect, int)
{
  painter->ApplyPen(this->Pen);
  painter->DrawLine(rect[0], rect[1]+0.5*rect[3],
                    rect[0]+rect[2], rect[1]+0.5*rect[3]);
  this->Superclass::PaintLegend(painter, rect, 0);
  return true;
}

//-----------------------------------------------------------------------------
void vtkPlotLine::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
