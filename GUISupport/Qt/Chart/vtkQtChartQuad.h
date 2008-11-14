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

  const QPolygonF &getPoints() const;
  void setPoints(const QPolygonF &points);
  void setPoint(int index, const QPointF &point);

private:
  QPolygonF *Points; ///< Stores the four points.
};

#endif
