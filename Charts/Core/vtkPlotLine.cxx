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
#include "vtkIdTypeArray.h"
#include "vtkPen.h"
#include "vtkPoints2D.h"
#include "vtkRect.h"

#include "vtkObjectFactory.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPlotLine);

//-----------------------------------------------------------------------------
vtkPlotLine::vtkPlotLine()
{
  this->MarkerStyle = vtkPlotPoints::NONE;
  this->PolyLine = true;
}

//-----------------------------------------------------------------------------
vtkPlotLine::~vtkPlotLine() = default;

//-----------------------------------------------------------------------------
bool vtkPlotLine::Paint(vtkContext2D* painter)
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
    float* points = static_cast<float*>(this->Points->GetVoidPointer(0));
    const int pointSize = 2;
    vtkIdType lastGood = 0;
    vtkIdType bpIdx = 0;
    vtkIdType lineIncrement = this->PolyLine ? 1 : 2;
    vtkIdType nPoints = this->Points->GetNumberOfPoints();
    vtkIdType nBadPoints = this->BadPoints->GetNumberOfTuples();

    while (lastGood < nPoints)
    {
      vtkIdType id =
        bpIdx < nBadPoints ? this->BadPoints->GetValue(bpIdx) : this->Points->GetNumberOfPoints();

      // With non polyline, we discard a line if any of its points are bad
      if (!this->PolyLine && id % 2 == 1)
      {
        id--;
      }

      // render from last good point to one before this bad point
      if (id - lastGood > 1)
      {
        int start = lastGood;
        int numberOfPoints = id - start;
        if (this->PolyLine)
        {
          painter->DrawPoly(points + pointSize * start, numberOfPoints);
        }
        else
        {
          painter->DrawLines(points + pointSize * start, numberOfPoints);
        }
      }
      lastGood = id + lineIncrement;
      bpIdx++;
    }
  }
  else
  {
    // draw lines between all points
    if (this->PolyLine)
    {
      painter->DrawPoly(this->Points);
    }
    else
    {
      painter->DrawLines(this->Points);
    }
  }

  return this->vtkPlotPoints::Paint(painter);
}

//-----------------------------------------------------------------------------
bool vtkPlotLine::PaintLegend(vtkContext2D* painter, const vtkRectf& rect, int)
{
  painter->ApplyPen(this->Pen);
  painter->DrawLine(rect[0], rect[1] + 0.5 * rect[3], rect[0] + rect[2], rect[1] + 0.5 * rect[3]);
  this->Superclass::PaintLegend(painter, rect, 0);
  return true;
}

//-----------------------------------------------------------------------------
void vtkPlotLine::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
