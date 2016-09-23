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
  this->PolyLine = true;
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
    const int pointSize = 2;
    vtkIdType lastGood = 0;

    for (vtkIdType i = 0; i < this->BadPoints->GetNumberOfTuples(); i++)
    {
      vtkIdType id = this->BadPoints->GetValue(i);

      // render from last good point to one before this bad point
      if (id - lastGood > 2)
      {
        int start = lastGood + 1;
        int numberOfPoints = id - start;
        if (!this->PolyLine)
        {
          // Start at the next point if the last bad point was the first point
          // of a line segment.
          if (start % 2 == 0)
          {
            ++start;
            --numberOfPoints;
          }
          // Stops at the previous point if the next bad point is the second point
          // of a line segment.
          if (numberOfPoints % 2 == 1)
          {
            --numberOfPoints;
          }
        }
        if (this->PolyLine)
        {
          painter->DrawPoly(points + pointSize * start, numberOfPoints);
        }
        else if (start < this->Points->GetNumberOfPoints() && numberOfPoints > 0)
        {
          painter->DrawLines(points + pointSize * start, numberOfPoints);
        }
      }

      lastGood = id;
    }

    // render any trailing good points
    if (this->Points->GetNumberOfPoints() - lastGood > 2)
    {
      int start = lastGood + 1;
      int numberOfPoints = this->Points->GetNumberOfPoints() - start;
      if (!this->PolyLine)
      {
        // Start at the next point if the last bad point was the first point
        // of a line segment.
        if (start % 2 == 0)
        {
          ++start;
          --numberOfPoints;
        }
        // Stops at the previous point if the next bad point is the second point
        // of a line segment.
        if (numberOfPoints % 2 == 1)
        {
          --numberOfPoints;
        }
      }
      if (this->PolyLine)
      {
        painter->DrawPoly(points + pointSize * start, numberOfPoints);
      }
      else if (start < this->Points->GetNumberOfPoints() && numberOfPoints > 0)
      {
        painter->DrawLines(points + pointSize * start, numberOfPoints);
      }
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
