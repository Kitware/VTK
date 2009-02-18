/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartQuad.h

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

/// \file vtkQtChartQuad.h
/// \date November 13, 2008

#ifndef _vtkQtChartQuad_h
#define _vtkQtChartQuad_h

#include "vtkQtChartExport.h"
#include "vtkQtChartShape.h"

class QPolygonF;


/// \class vtkQtChartQuad
/// \brief
///   The vtkQtChartQuad class defines a quad used by the chart shape
///   locator.
class VTKQTCHART_EXPORT vtkQtChartQuad : public vtkQtChartShape
{
public:
  vtkQtChartQuad();

  /// \brief
  ///   Creates a quad instance.
  /// \param series The chart series.
  /// \param index The chart series index.
  vtkQtChartQuad(int series, int index);
  vtkQtChartQuad(const vtkQtChartQuad &other);
  virtual ~vtkQtChartQuad();

  vtkQtChartQuad &operator=(const vtkQtChartQuad &other);

  virtual void getBounds(QRectF &bounds) const;
  virtual bool contains(const QPointF &point) const;
  virtual bool intersects(const QRectF &area) const;

  /// \brief
  ///   Sets the quad shape.
  ///
  /// The polygon should be a list of four points. The points should
  /// form a convex, clock-wise loop.
  ///
  /// \param polygon The list of points to define the quad.
  virtual void setPolygon(const QPolygonF &polygon) {this->setPoints(polygon);}

  /// \brief
  ///   Gets the list of quad points.
  /// \return
  ///   The list of quad points.
  const QPolygonF &getPoints() const;

  /// \brief
  ///   Sets the quad shape.
  /// \param points The list of points to define the quad.
  void setPoints(const QPolygonF &points);

  /// \brief
  ///   Sets the point for the given index.
  /// \param index The index of the quad point.
  /// \param point The new point.
  void setPoint(int index, const QPointF &point);

private:
  QPolygonF *Points; ///< Stores the four points.
};

#endif
