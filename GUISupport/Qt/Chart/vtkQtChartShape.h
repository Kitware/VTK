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
class QPolygonF;
class QRectF;


/// \class vtkQtChartShape
/// \brief
///   The vtkQtChartShape class is the base class for shapes used in
///   the chart shape locators.
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

  /// \brief
  ///   Constructs a chart shape.
  /// \param series The series index.
  /// \param index The index in the given series.
  vtkQtChartShape(int series, int index);
  vtkQtChartShape(const vtkQtChartShape &other);
  virtual ~vtkQtChartShape() {}

  vtkQtChartShape &operator=(const vtkQtChartShape &other);

  /// \brief
  ///   Gets the bounding box for the shape.
  /// \param bounds Used to return the bounding box.
  virtual void getBounds(QRectF &bounds) const = 0;

  /// \brief
  ///   Gets whether or not the shape contains the given point.
  /// \param point The position to evaluate.
  /// \return
  ///   True if the shape contains the given point.
  virtual bool contains(const QPointF &point) const = 0;

  /// \brief
  ///   Gets whether or not the shape intersects the given area.
  /// \param area The area to evaluate.
  /// \return
  ///   True if the shape intersects the given area.
  virtual bool intersects(const QRectF &area) const = 0;

  /// \brief
  ///   Sets the shape for rectangular types.
  ///
  /// This method can be used to set the shape. The default
  /// implementation does nothing. This is useful for objects that
  /// have a rectangular shape.
  ///
  /// \param rectangle The new shape.
  /// \sa vtkQtChartShape::setPolygon(const QPolygonF &)
  virtual void setRectangle(const QRectF &rectangle);

  /// \brief
  ///   Sets the shape for polygonal types.
  ///
  /// This method can be used to set the shape. The default
  /// implementation does nothing. This is useful for objects that
  /// have a polygonal shape.
  ///
  /// \param polygon The new shape.
  /// \sa vtkQtChartShape::setRectangle(const QRectF &)
  virtual void setPolygon(const QPolygonF &polygon);

  /// \brief
  ///   Gets the series number.
  /// \return
  ///   The series number.
  int getSeries() const {return this->Series;}

  /// \brief
  ///   Sets the series number.
  /// \param series The new series number.
  void setSeries(int series) {this->Series = series;}

  /// \brief
  ///   Gets the index in the series.
  /// \return
  ///   The index in the series.
  int getIndex() const {return this->Index;}

  /// \brief
  ///   Sets the index in the series.
  /// \param index The new index in the series.
  void setIndex(int index) {this->Index = index;}

public:
  /// \brief
  ///   Gets the bounding box code for the given point and rectangle.
  ///
  /// This method combines the x and y bounding box codes.
  ///
  /// \param point The position to evaluate.
  /// \param bounds The bounding box.
  static int getBoundingBoxCode(const QPointF &point, const QRectF &bounds);

  /// \brief
  ///   Gets the bounding box code for the given coordinate and rectangle.
  /// \param x The x position to evaluate.
  /// \param bounds The bounding box.
  static int getXBoundingBoxCode(float x, const QRectF &bounds);

  /// \brief
  ///   Gets the bounding box code for the given coordinate and rectangle.
  /// \param y The y position to evaluate.
  /// \param bounds The bounding box.
  static int getYBoundingBoxCode(float y, const QRectF &bounds);

private:
  int Series; ///< Stores the series.
  int Index;  ///< Stores the index.
};

#endif
