/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartShape.cxx

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

/// \file vtkQtChartShape.cxx
/// \date November 13, 2008

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#endif

#include "vtkQtChartShape.h"

#include <QPointF>
#include <QRectF>


vtkQtChartShape::vtkQtChartShape()
{
  this->Series = -1;
  this->Index = -1;
}

vtkQtChartShape::vtkQtChartShape(int series, int index)
{
  this->Series = series;
  this->Index = index;
}

vtkQtChartShape::vtkQtChartShape(const vtkQtChartShape &other)
{
  this->Series = other.Series;
  this->Index = other.Index;
}

vtkQtChartShape &vtkQtChartShape::operator=(const vtkQtChartShape &other)
{
  this->Series = other.Series;
  this->Index = other.Index;
  return *this;
}

void vtkQtChartShape::setRectangle(const QRectF &)
{
}

void vtkQtChartShape::setPolygon(const QPolygonF &)
{
}

int vtkQtChartShape::getBoundingBoxCode(const QPointF &point,
    const QRectF &bounds)
{
  int code = vtkQtChartShape::getXBoundingBoxCode(point.x(), bounds);
  code |= vtkQtChartShape::getYBoundingBoxCode(point.y(), bounds);
  return code;
}

int vtkQtChartShape::getXBoundingBoxCode(float x, const QRectF &bounds)
{
  int code = 0;
  if(x < bounds.left())
    {
    code |= vtkQtChartShape::Left;
    }
  else if(x > bounds.right())
    {
    code |= vtkQtChartShape::Right;
    }

  return code;
}

int vtkQtChartShape::getYBoundingBoxCode(float y, const QRectF &bounds)
{
  int code = 0;
  if(y < bounds.top())
    {
    code |= vtkQtChartShape::Top;
    }
  else if(y > bounds.bottom())
    {
    code |= vtkQtChartShape::Bottom;
    }

  return code;
}


