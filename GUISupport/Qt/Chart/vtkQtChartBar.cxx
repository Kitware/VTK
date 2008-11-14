/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartBar.cxx

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

/// \file vtkQtChartBar.cxx
/// \date November 13, 2008

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#endif

#include "vtkQtChartBar.h"

#include <QPointF>
#include <QRectF>


vtkQtChartBar::vtkQtChartBar()
  : vtkQtChartShape()
{
  this->Bar = new QRectF();
}

vtkQtChartBar::vtkQtChartBar(int series, int index)
  : vtkQtChartShape(series, index)
{
  this->Bar = new QRectF();
}

vtkQtChartBar::vtkQtChartBar(const vtkQtChartBar &other)
  : vtkQtChartShape(other)
{
  this->Bar = new QRectF(*other.Bar);
}

vtkQtChartBar::~vtkQtChartBar()
{
  delete this->Bar;
}

vtkQtChartBar &vtkQtChartBar::operator=(const vtkQtChartBar &other)
{
  vtkQtChartShape::operator=(other);
  *this->Bar = *other.Bar;
  return *this;
}

void vtkQtChartBar::getBounds(QRectF &bounds) const
{
  bounds = *this->Bar;
}

bool vtkQtChartBar::contains(const QPointF &point) const
{
  return this->Bar->contains(point);
}

bool vtkQtChartBar::intersects(const QRectF &area) const
{
  // QRectF's intersects method misses when width or height is zero.
  return qMax<float>(this->Bar->left(), area.left()) <= 
      qMin<float>(this->Bar->right(), area.right()) &&
      qMax<float>(this->Bar->top(), area.top()) <= 
      qMin<float>(this->Bar->bottom(), area.bottom());
}

void vtkQtChartBar::setBar(const QRectF &bar)
{
  *this->Bar = bar;
}


