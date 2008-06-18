/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtPointMarker.cxx

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

/// \file vtkQtPointMarker.cxx
/// \date February 12, 2008

#include "vtkQtPointMarker.h"

#include <QBrush>
#include <QPainter>
#include <QPen>
#include <QPolygonF>
#include <QRectF>
#include <QSizeF>


//----------------------------------------------------------------------------
vtkQtPointMarker::vtkQtPointMarker(const QSizeF &size,
    vtkQtPointMarker::MarkerStyle style, QGraphicsItem *item,
    QGraphicsScene *graphicsScene)
  : QGraphicsItem(item, graphicsScene), Rect(-size.width() * 0.5,
    -size.height() * 0.5, size.width(), size.height()), Bounds()
{
  this->Style = style;
  this->Points = new QPolygonF();
  this->Pen = new QPen(Qt::black);
  this->Brush = new QBrush(Qt::white);
}

vtkQtPointMarker::~vtkQtPointMarker()
{
  delete this->Points;
  delete this->Pen;
  delete this->Brush;
}

QRectF vtkQtPointMarker::boundingRect() const
{
  if(this->Points->isEmpty())
    {
    return QRectF();
    }

  QRectF bounds = this->Points->boundingRect();
  float width = (this->Rect.width() + this->Pen->widthF()) * 0.5;
  float height = (this->Rect.height() + this->Pen->widthF()) * 0.5;
  bounds.adjust(-width, -height, width, height);
  return bounds;
}

void vtkQtPointMarker::paint(QPainter *painter,
    const QStyleOptionGraphicsItem *option, QWidget *widget)
{
  // Set up the painter pen and brush.
  painter->setPen(this->pen());
  painter->setBrush(this->brush());

  QPolygonF::Iterator iter = this->Points->begin();
  for( ; iter != this->Points->end(); ++iter)
    {
    if(this->Bounds.isValid() && !this->Bounds.contains(*iter))
      {
      continue;
      }

    // Transform the painter to the next point.
    painter->save();
    painter->translate(*iter);

    // Draw the appropriate marker shape.
    this->paintMarker(painter, option, widget);
    painter->restore();
    }
}

const QPolygonF &vtkQtPointMarker::getPoints() const
{
  return *this->Points;
}

void vtkQtPointMarker::setPoints(const QPolygonF &points)
{
  this->prepareGeometryChange();
  *this->Points = points;
  this->update();
}

QSizeF vtkQtPointMarker::getSize() const
{
  return this->Rect.size();
}

void vtkQtPointMarker::setSize(const QSizeF &size)
{
  if(size != this->Rect.size())
    {
    this->prepareGeometryChange();
    this->Rect.setRect(-size.width() * 0.5, -size.height() * 0.5,
        size.width(), size.height());
    this->update();
    }
}

void vtkQtPointMarker::setStyle(vtkQtPointMarker::MarkerStyle style)
{
  if(this->Style != style)
    {
    this->Style = style;
    this->update();
    }
}

const QPen &vtkQtPointMarker::pen() const
{
  return *this->Pen;
}

void vtkQtPointMarker::setPen(const QPen &newPen)
{
  if(newPen.widthF() != this->Pen->widthF())
    {
    this->prepareGeometryChange();
    }

  *this->Pen = newPen;
  this->update();
}

const QBrush &vtkQtPointMarker::brush() const
{
  return *this->Brush;
}

void vtkQtPointMarker::setBrush(const QBrush &newBrush)
{
  *this->Brush = newBrush;
  this->update();
}

void vtkQtPointMarker::paintMarker(QPainter *painter,
    const QStyleOptionGraphicsItem *, QWidget *)
{
  // Draw the appropriate marker shape.
  switch(this->Style)
    {
    case vtkQtPointMarker::Cross:
      {
      painter->drawLine(this->Rect.topLeft(), this->Rect.bottomRight());
      painter->drawLine(this->Rect.topRight(), this->Rect.bottomLeft());
      break;
      }
    case vtkQtPointMarker::Plus:
      {
      painter->drawLine(QPointF(0, this->Rect.top()),
          QPointF(0, this->Rect.bottom()));
      painter->drawLine(QPointF(this->Rect.left(), 0),
          QPointF(this->Rect.right(), 0));
      break;
      }
    case vtkQtPointMarker::Square:
      {
      painter->drawRect(this->Rect);
      break;
      }
    case vtkQtPointMarker::Circle:
      {
      painter->drawEllipse(this->Rect);
      break;
      }
    case vtkQtPointMarker::Diamond:
      {
      // Set up the diamond polygon to fit in the given size.
      QPolygonF diamond;
      float halfHeight = this->Rect.height() / 2.0;
      float halfWidth = this->Rect.width() / 2.0;
      diamond.append(QPointF(0, -halfHeight));
      diamond.append(QPointF(halfWidth, 0));
      diamond.append(QPointF(0, halfHeight));
      diamond.append(QPointF(-halfWidth, 0));
      diamond.append(QPointF(0, -halfHeight));
      painter->drawPolygon(diamond);
      break;
      }
    case vtkQtPointMarker::UserStyle:
      {
      break;
      }
    }
}


