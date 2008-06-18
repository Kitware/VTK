/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtSimplePointLocator.cxx

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

/// \file vtkQtSimplePointLocator.cxx
/// \date May 29, 2008

#include "vtkQtSimplePointLocator.h"

#include <QPolygonF>
#include <QRectF>


vtkQtSimplePointLocator::vtkQtSimplePointLocator(QObject *parentObject)
  : vtkQtChartPointLocator(parentObject)
{
  this->Points = new QPolygonF();
}

vtkQtSimplePointLocator::~vtkQtSimplePointLocator()
{
  delete this->Points;
}

vtkQtChartPointLocator *vtkQtSimplePointLocator::getNewInstance(
    QObject *parentObject) const
{
  return new vtkQtSimplePointLocator(parentObject);
}

void vtkQtSimplePointLocator::setPoints(const QPolygonF &points)
{
  *this->Points = points;
}

void vtkQtSimplePointLocator::findPointsIn(const QRectF &area,
    vtkQtChartIndexRangeList &points)
{
  // Loop through the list of points to find the ones in the area.
  QPolygonF::Iterator iter = this->Points->begin();
  for(int index = 0; iter != this->Points->end(); ++iter, ++index)
    {
    if(area.contains(*iter))
      {
      points.append(vtkQtChartIndexRange(index, index));
      }
    }
}


