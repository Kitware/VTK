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


/// \class vtkQtChartPointLocator
/// \brief
///   The vtkQtChartPointLocator class is the base class for the
///   chart point locators.
class VTKQTCHART_EXPORT vtkQtChartPointLocator : public QObject
{
public:
  /// \brief
  ///   Creates a chart point locator.
  /// \param parent The parent object.
  vtkQtChartPointLocator(QObject *parent=0);
  virtual ~vtkQtChartPointLocator() {}

  /// \brief
  ///   Creates a chart point locator.
  /// \param parent The parent object.
  /// \return
  ///   A pointer to the new chart point locator.
  virtual vtkQtChartPointLocator *getNewInstance(QObject *parent=0) const = 0;

  /// \brief
  ///   Sets the points the locator should use.
  /// \param points The points the locator should use.
  virtual void setPoints(const QPolygonF &points) = 0;

  /// \brief
  ///   Finds the points in the given rectangle.
  /// \param area The area to search.
  /// \param points Used to return the selected points.
  virtual void findPointsIn(const QRectF &area,
      vtkQtChartIndexRangeList &points) = 0;
};

#endif
