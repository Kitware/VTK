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


class VTKQTCHART_EXPORT vtkQtChartBar : public vtkQtChartShape
{
public:
  vtkQtChartBar();
  vtkQtChartBar(int series, int index);
  vtkQtChartBar(const vtkQtChartBar &other);
  virtual ~vtkQtChartBar();

  vtkQtChartBar &operator=(const vtkQtChartBar &other);

  virtual void getBounds(QRectF &bounds) const;
  virtual bool contains(const QPointF &point) const;
  virtual bool intersects(const QRectF &area) const;

  virtual void setRectangle(const QRectF &rectangle) {this->setBar(rectangle);}

  QRectF &getBar() {return *this->Bar;}
  const QRectF &getBar() const {return *this->Bar;}
  void setBar(const QRectF &bar);

private:
  QRectF *Bar; ///< Stores the rectangle.
};

#endif
