/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtPointMarker.h

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

/// \file vtkQtPointMarker.h
/// \date February 12, 2008

#ifndef _vtkQtPointMarker_h
#define _vtkQtPointMarker_h


#include "vtkQtChartExport.h"
#include <QSizeF>
#include <QRectF>

class QPainter;


/// \class vtkQtPointMarker
/// \brief
///   The vtkQtPointMarker class is used to draw a shape at a point.
class VTKQTCHART_EXPORT vtkQtPointMarker
{
public:
  enum MarkerStyle
    {
    NoMarker = 0, ///< Nothing is drawn.
    Cross,        ///< Draws a cross.
    Plus,         ///< Draws a plus.
    Square,       ///< Draws a square.
    Circle,       ///< Draws a circle.
    Diamond,      ///< Draws a diamond.

    /// The next available style for extension classes.
    UserStyle = 32
    };

public:
  vtkQtPointMarker(const QSizeF &size, MarkerStyle style=Circle);
  virtual ~vtkQtPointMarker();

  virtual void paint(QPainter *painter);

  QSizeF getSize() const;
  void setSize(const QSizeF &size);

  MarkerStyle getStyle() const {return this->Style;}
  void setStyle(MarkerStyle style);

protected:
  QRectF Rect;

private:
  MarkerStyle Style;
};

#endif
