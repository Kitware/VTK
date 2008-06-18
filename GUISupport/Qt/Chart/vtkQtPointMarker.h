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
#include <QGraphicsItem>

#include "vtkQtChartGraphicsItemTypes.h" // needed for enum

class QBrush;
class QPen;
class QPolygonF;
class QSizeF;


/// \class vtkQtPointMarker
/// \brief
///   The vtkQtPointMarker class is used to draw a shape at a point.
class VTKQTCHART_EXPORT vtkQtPointMarker : public QGraphicsItem
{
public:
  enum MarkerStyle
    {
    Cross = 0, ///< Draws a cross.
    Plus,      ///< Draws a plus.
    Square,    ///< Draws a square.
    Circle,    ///< Draws a circle.
    Diamond,   ///< Draws a diamond.

    /// The next available style for extension classes.
    UserStyle = 32
    };

  enum {Type = vtkQtChart_PointMarkerType};

public:
  vtkQtPointMarker(const QSizeF &size, MarkerStyle style=Circle,
      QGraphicsItem *parent=0, QGraphicsScene *scene=0);
  virtual ~vtkQtPointMarker();

  virtual int type() const {return vtkQtPointMarker::Type;}
  virtual QRectF boundingRect() const;
  virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
      QWidget *widget=0);

  const QPolygonF &getPoints() const;
  void setPoints(const QPolygonF &points);

  QSizeF getSize() const;
  void setSize(const QSizeF &size);

  MarkerStyle getStyle() const {return this->Style;}
  void setStyle(MarkerStyle style);

  const QPen &pen() const;
  void setPen(const QPen &newPen);

  const QBrush &brush() const;
  void setBrush(const QBrush &newBrush);

  const QRectF &getClipRect() const {return this->Bounds;}
  void setClipRect(const QRectF &bounds) {this->Bounds = bounds;}

protected:
  virtual void paintMarker(QPainter *painter,
      const QStyleOptionGraphicsItem *option, QWidget *widget);

protected:
  QRectF Rect;

private:
  QRectF Bounds;
  MarkerStyle Style;
  QPolygonF *Points;
  QPen *Pen;
  QBrush *Brush;
};

#endif
