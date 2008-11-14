/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartShape.h

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

/// \file vtkQtChartShape.h
/// \date November 13, 2008

#ifndef _vtkQtChartShape_h
#define _vtkQtChartShape_h

#include "vtkQtChartExport.h"

class QPointF;
class QRectF;


class VTKQTCHART_EXPORT vtkQtChartShape
{
public:
  enum BoundingBoxCode
    {
    Left   = 0x01000000, ///< Left of bounding box.
    Top    = 0x00010000, ///< Above the bounding box.
    Right  = 0x00000100, ///< Right of bounding box.
    Bottom = 0x00000001  ///< Below the bounding box.
    };

public:
  vtkQtChartShape();
  vtkQtChartShape(int series, int index);
  vtkQtChartShape(const vtkQtChartShape &other);
  virtual ~vtkQtChartShape() {}

  vtkQtChartShape &operator=(const vtkQtChartShape &other);

  virtual void getBounds(QRectF &bounds) const = 0;
  virtual bool contains(const QPointF &point) const = 0;
  virtual bool intersects(const QRectF &area) const = 0;

  int getSeries() const {return this->Series;}
  void setSeries(int series) {this->Series = series;}

  int getIndex() const {return this->Index;}
  void setIndex(int index) {this->Index = index;}

public:
  static int getBoundingBoxCode(const QPointF &point, const QRectF &bounds);
  static int getXBoundingBoxCode(float x, const QRectF &bounds);
  static int getYBoundingBoxCode(float y, const QRectF &bounds);

private:
  int Series; ///< Stores the series.
  int Index;  ///< Stores the index.
};

#endif
