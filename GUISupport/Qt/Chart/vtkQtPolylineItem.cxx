/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtPolylineItem.cxx

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

/// \file vtkQtPolylineItem.cxx
/// \date February 1, 2008

#include "vtkQtPolylineItem.h"

#include <QPainter>
#include <QPen>
#include <QPolygonF>


vtkQtPolylineItem::vtkQtPolylineItem(QGraphicsItem *item, QGraphicsScene *graphicsScene)
  : QGraphicsItem(item, graphicsScene)
{
  this->Pen = new QPen();
  this->Polyline = new QPolygonF();
}

vtkQtPolylineItem::~vtkQtPolylineItem()
{
  delete this->Pen;
  delete this->Polyline;
}

const QPen &vtkQtPolylineItem::pen() const
{
  return *this->Pen;
}

void vtkQtPolylineItem::setPen(const QPen& p)
{
  this->prepareGeometryChange();
  *this->Pen = p;
  this->update();
}

void vtkQtPolylineItem::setPolyline(const QPolygonF& line)
{
  this->prepareGeometryChange();
  *this->Polyline = line;
  this->update();
}

const QPolygonF& vtkQtPolylineItem::polyline() const
{
  return *this->Polyline;
}

QRectF vtkQtPolylineItem::boundingRect() const
{
  QRectF area = this->Polyline->boundingRect();
  float penWidth = this->Pen->widthF() * 0.5;
  area.adjust(-penWidth, -penWidth, penWidth, penWidth);
  return area;
}

QPainterPath vtkQtPolylineItem::shape() const
{
  QPainterPath path;
  path.addPolygon(*this->Polyline);
  return path;
}

bool vtkQtPolylineItem::contains(const QPointF &point) const
{
  QRectF box;
  float penWidth = (this->Pen->widthF() * 0.5) + 1.0;
  box.setCoords(point.x() - penWidth, point.y() - penWidth,
      point.x() + penWidth, point.y() + penWidth);
  for(int i = 1; i < this->Polyline->size(); i++)
    {
    if(this->doesLineCrossBox((*this->Polyline)[i - 1], (*this->Polyline)[i],
        box))
      {
      return true;
      }
    }

  return false;
}

void vtkQtPolylineItem::paint(QPainter* p,
   const QStyleOptionGraphicsItem* /*option*/, QWidget* /*widget*/)
{
  p->setPen(*this->Pen);
  p->drawPolyline(*this->Polyline);
}

bool vtkQtPolylineItem::doesLineCrossBox(const QPointF &point1,
      const QPointF &point2, const QRectF &box) const
{
  qreal x1 = point1.x();
  qreal y1 = point1.y();
  qreal x2 = point2.x();
  qreal y2 = point2.y();

  qreal left = box.left();
  qreal right = box.right();
  qreal top = box.top();
  qreal bottom = box.bottom();

  enum { Left, Right, Top, Bottom };
  // clip the lines, after cohen-sutherland, see e.g. http://www.nondot.org/~sabre/graphpro/line6.html
  int p1 = ((x1 < left) << Left)
            | ((x1 > right) << Right)
            | ((y1 < top) << Top)
            | ((y1 > bottom) << Bottom);
  int p2 = ((x2 < left) << Left)
            | ((x2 > right) << Right)
            | ((y2 < top) << Top)
            | ((y2 > bottom) << Bottom);

  if (p1 & p2)
    {
    // completely inside
    return false;
    }

  if (p1 | p2)
    {
    qreal dx = x2 - x1;
    qreal dy = y2 - y1;

    // clip x coordinates
    if (x1 < left)
      {
      y1 += dy/dx * (left - x1);
      x1 = left;
      }
    else if (x1 > right)
      {
      y1 -= dy/dx * (x1 - right);
      x1 = right;
      }
    if (x2 < left)
      {
      y2 += dy/dx * (left - x2);
      x2 = left;
      }
    else if (x2 > right)
      {
      y2 -= dy/dx * (x2 - right);
      x2 = right;
      }

    p1 = ((y1 < top) << Top)
          | ((y1 > bottom) << Bottom);
    p2 = ((y2 < top) << Top)
          | ((y2 > bottom) << Bottom);

    if (p1 & p2)
      {
      return false;
      }

    // clip y coordinates
    if (y1 < top)
      {
      x1 += dx/dy * (top - y1);
      y1 = top;
      }
    else if (y1 > bottom)
      {
      x1 -= dx/dy * (y1 - bottom);
      y1 = bottom;
      }
    if (y2 < top)
      {
      x2 += dx/dy * (top - y2);
      y2 = top;
      }
    else if (y2 > bottom)
      {
      x2 -= dx/dy * (y2 - bottom);
      y2 = bottom;
      }

    p1 = ((x1 < left) << Left)
          | ((x1 > right) << Right);
    p2 = ((x2 < left) << Left)
          | ((x2 > right) << Right);

    if (p1 & p2)
      {
      return false;
      }

    return true;
    }

  return false;
}


