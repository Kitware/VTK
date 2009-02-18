/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartBar.h

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

/// \file vtkQtChartBar.h
/// \date November 13, 2008

#ifndef _vtkQtChartBar_h
#define _vtkQtChartBar_h

#include "vtkQtChartExport.h"
#include "vtkQtChartShape.h"


/// \class vtkQtChartBar
/// \brief
///   The vtkQtChartBar class defines a bar used by the chart bar
///   locator.
class VTKQTCHART_EXPORT vtkQtChartBar : public vtkQtChartShape
{
public:
  vtkQtChartBar();

  /// \brief
  ///   Constructs a chart bar shape.
  /// \param series The series index.
  /// \param index The index in the given series.
  vtkQtChartBar(int series, int index);
  vtkQtChartBar(const vtkQtChartBar &other);
  virtual ~vtkQtChartBar();

  vtkQtChartBar &operator=(const vtkQtChartBar &other);

  virtual void getBounds(QRectF &bounds) const;
  virtual bool contains(const QPointF &point) const;
  virtual bool intersects(const QRectF &area) const;

  /// \brief
  ///   Sets the bar shape.
  /// \param rectangle The new bar shape.
  virtual void setRectangle(const QRectF &rectangle) {this->setBar(rectangle);}

  /// \brief
  ///   Gets the bar shape.
  /// \return
  ///   The bar rectangle.
  QRectF &getBar() {return *this->Bar;}

  /// \brief
  ///   Gets the bar shape.
  /// \return
  ///   The bar rectangle.
  const QRectF &getBar() const {return *this->Bar;}

  /// \brief
  ///   Sets the bar shape.
  /// \param bar The new bar rectangle.
  void setBar(const QRectF &bar);

private:
  QRectF *Bar; ///< Stores the rectangle.
};

#endif
