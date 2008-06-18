/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartPointLocator.h

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

/// \file vtkQtChartPointLocator.h
/// \date May 29, 2008

#ifndef _vtkQtChartPointLocator_h
#define _vtkQtChartPointLocator_h

#include "vtkQtChartExport.h"
#include <QObject>
#include "vtkQtChartSeriesSelection.h" // needed for typedef

class QPointF;
class QPolygonF;
class QRectF;


class VTKQTCHART_EXPORT vtkQtChartPointLocator : public QObject
{
public:
  vtkQtChartPointLocator(QObject *parent=0);
  virtual ~vtkQtChartPointLocator() {}

  virtual vtkQtChartPointLocator *getNewInstance(QObject *parent=0) const = 0;

  virtual void setPoints(const QPolygonF &points) = 0;

  virtual void findPointsIn(const QRectF &area,
      vtkQtChartIndexRangeList &points) = 0;
};

#endif
