/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtSimplePointLocator.h

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

/// \file vtkQtSimplePointLocator.h
/// \date May 29, 2008

#ifndef _vtkQtSimplePointLocator_h
#define _vtkQtSimplePointLocator_h

#include "vtkQtChartExport.h"
#include "vtkQtChartPointLocator.h"


/// \class vtkQtSimplePointLocator
/// \brief
///   The vtkQtSimplePointLocator class locates points by searching
///   the point list sequentially
class VTKQTCHART_EXPORT vtkQtSimplePointLocator : public vtkQtChartPointLocator
{
public:
  vtkQtSimplePointLocator(QObject *parent=0);
  virtual ~vtkQtSimplePointLocator();

  virtual vtkQtChartPointLocator *getNewInstance(QObject *parent=0) const;

  virtual void setPoints(const QPolygonF &points);

  /// \brief
  ///   Finds the points in the given rectangle.
  ///
  /// The points are located by searching the list of points
  /// sequentially.
  ///
  /// \param area The area to search.
  /// \param points Used to return the selected points.
  virtual void findPointsIn(const QRectF &area,
      vtkQtChartIndexRangeList &points);

private:
  QPolygonF *Points; ///< Stores the list of points.
};

#endif
