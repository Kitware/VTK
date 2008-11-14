/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartQuad.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

/// \file vtkQtChartQuad.cxx
/// \date November 13, 2008

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#endif

#include "vtkQtChartQuad.h"

#include <QPolygonF>


vtkQtChartQuad::vtkQtChartQuad()
  : vtkQtChartShape()
{
  this->Points = new QPolygonF(4);
}

vtkQtChartQuad::vtkQtChartQuad(int series, int index)
  : vtkQtChartShape(series, index)
{
  this->Points = new QPolygonF(4);
}

vtkQtChartQuad::vtkQtChartQuad(const vtkQtChartQuad &other)
  : vtkQtChartShape(other)
{
  this->Points = new QPolygonF(*other.Points);
}

vtkQtChartQuad::~vtkQtChartQuad()
{
  delete this->Points;
}

vtkQtChartQuad &vtkQtChartQuad::operator=(const vtkQtChartQuad &other)
{
  vtkQtChartShape::operator=(other);
  *this->Points = *other.Points;
  return *this;
}

void vtkQtChartQuad::getBounds(QRectF &bounds) const
{
  bounds = this->Points->boundingRect();
}

bool vtkQtChartQuad::contains(const QPointF &point) const
{
  // Use the line segments to form a convex loop. Use the loop to
  // determine if the point is inside the quad.
  for(int i = 0; i < 4; i++)
    {
    // If the following expression is less than zero, the point is
    // outside the loop:
    //   (y - y0)*(x1 - x0) - (x - x0)*(y1 - y0)
    int j = i == 3 ? 0 : i + 1;
    float xDiff = (*this->Points)[j].x() - (*this->Points)[i].x();
    float yDiff = (*this->Points)[j].y() - (*this->Points)[i].y();
    if(((point.y() - (*this->Points)[i].y()) * xDiff) -
        ((point.x() - (*this->Points)[i].x()) * yDiff) < 0.0)
      {
      return false;
      }
    }

  return true;
}

bool vtkQtChartQuad::intersects(const QRectF &area) const
{
  // Get the bounding box code for each of the quad points. If any of
  // the quad points are in the area, return true.
  int i = 0;
  int code[4] = {0, 0, 0, 0};
  for( ; i < 4; i++)
    {
    code[i] = vtkQtChartShape::getBoundingBoxCode((*this->Points)[i], area);
    if(code[i] == 0)
      {
      return true;
      }
    }

  // See if all the points are to one side of the area.
  if((code[0] & code[1] & code[2] & code[3]) != 0)
    {
    return false;
    }

  // Check for 4-corner case.
  int corner[4] =
    {
    vtkQtChartShape::Top | vtkQtChartShape::Left,
    vtkQtChartShape::Top | vtkQtChartShape::Right,
    vtkQtChartShape::Bottom | vtkQtChartShape::Right,
    vtkQtChartShape::Bottom | vtkQtChartShape::Left
    };

  for(i = 0; i < 4; i++)
    {
    if(code[i] == corner[0])
      {
      i++;
      bool found = true;
      for(int j = 1; found && j < 4; j++, i++)
        {
        found = code[i % 4] == corner[j];
        }

      if(found)
        {
        return true;
        }
      }
    }

  // See if the diagonals guarantee an intersection.
  int leftRight = vtkQtChartShape::Left | vtkQtChartShape::Right;
  int topBottom = vtkQtChartShape::Top | vtkQtChartShape::Bottom;
  int combined = code[0] | code[2];
  if(combined == leftRight || combined == topBottom)
    {
    return true;
    }

  combined = code[1] | code[3];
  if(combined == leftRight || combined == topBottom)
    {
    return true;
    }

  // See if any of the line segments cross the area.
  for(i = 0; i < 4; i++)
    {
    int j = i == 3 ? 0 : i + 1;
    if((code[i] & code[j]) != 0)
      {
      // Line segment is completely on one side of the area.
      continue;
      }

    // See if the combined code guarantees an intersection.
    combined = code[i] | code[j];
    if(combined == leftRight || combined == topBottom)
      {
      return true;
      }

    // Calculate if the line intersects the area. First, clip the x
    // coordinates.
    float x1 = (*this->Points)[i].x();
    float y1 = (*this->Points)[i].y();
    float x2 = (*this->Points)[j].x();
    float y2 = (*this->Points)[j].y();
    float dx = x2 - x1;
    float dy = y2 - y1;
    if(x1 < area.left())
      {
      y1 += (dy * (area.left() - x1)) / dx;
      x1 = area.left();
      }
    else if(x1 > area.right())
      {
      y1 -= (dy * (x1 - area.right())) / dx;
      x1 = area.right();
      }

    if(x2 < area.left())
      {
      y2 += (dy * (area.left() - x2)) / dx;
      x2 = area.left();
      }
    else if(x2 > area.right())
      {
      y2 -= (dy * (x2 - area.right())) / dx;
      x2 = area.right();
      }

    // Check the new y codes.
    int code1 = vtkQtChartShape::getYBoundingBoxCode(y1, area);
    int code2 = vtkQtChartShape::getYBoundingBoxCode(y2, area);
    if((code1 & code2) != 0)
      {
      continue;
      }

    // Next, clip the y coordinates.
    if(y1 < area.top())
      {
      x1 += (dx * (area.top() - y1)) / dy;
      y1 = area.top();
      }
    else if(y1 < area.bottom())
      {
      x1 -= (dx * (y1 - area.bottom())) / dy;
      y1 = area.bottom();
      }

    if(y2 < area.top())
      {
      x2 += (dx * (area.top() - y2)) / dy;
      y2 = area.top();
      }
    else if(y2 < area.bottom())
      {
      x2 -= (dx * (y2 - area.bottom())) / dy;
      y2 = area.bottom();
      }

    // Check the new x codes.
    code1 = vtkQtChartShape::getXBoundingBoxCode(x1, area);
    code2 = vtkQtChartShape::getXBoundingBoxCode(x2, area);
    if((code1 & code2) != 0)
      {
      continue;
      }

    return true;
    }

  return false;
}

const QPolygonF &vtkQtChartQuad::getPoints() const
{
  return *this->Points;
}

void vtkQtChartQuad::setPoints(const QPolygonF &points)
{
  if(points.size() == 4)
    {
    *this->Points = points;
    }
}

void vtkQtChartQuad::setPoint(int index, const QPointF &point)
{
  if(index >= 0 && index < 4)
    {
    (*this->Points)[index] = point;
    }
}


