/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtPolylineItem.h

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

/// \file vtkQtPolylineItem.h
/// \date February 6, 2008

#ifndef _vtkQtPolylineItem_h
#define _vtkQtPolylineItem_h

#include "vtkQtChartExport.h"
#include <QGraphicsItem>

#include "vtkQtChartGraphicsItemTypes.h" // needed for enum

class QPen;
class QPolygonF;


class VTKQTCHART_EXPORT vtkQtPolylineItem : public QGraphicsItem
{
public:
  enum {Type = vtkQtChart_PolylineItemType};

public:
  vtkQtPolylineItem(QGraphicsItem *parent=0, QGraphicsScene *scene=0);
  virtual ~vtkQtPolylineItem();

  const QPen& pen() const;
  void setPen(const QPen& p);

  void setPolyline(const QPolygonF& line);
  const QPolygonF& polyline() const;

  virtual int type() const {return vtkQtPolylineItem::Type;}
  virtual QRectF boundingRect() const;
  virtual QPainterPath shape() const;
  virtual bool contains(const QPointF &point) const;

  virtual void paint(QPainter* p, const QStyleOptionGraphicsItem* option,
      QWidget* widget);

private:
  bool doesLineCrossBox(const QPointF &point1, const QPointF &point2,
      const QRectF &box) const;

protected:
  QPen* Pen;
  QPolygonF* Polyline;
};

#endif

