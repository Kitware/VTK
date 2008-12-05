/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PointMarkerItems.cxx

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

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#endif

#include "QTestApp.h"

#include "vtkQtPointMarker.h"
#include <QPainter>
#include <QPolygonF>
#include <QSizeF>
#include <QWidget>


class PointMarkerWidget : public QWidget
{
public:
  PointMarkerWidget(QWidget *parent=0);
  virtual ~PointMarkerWidget() {}

  virtual QSize sizeHint() const;

protected:
  virtual void paintEvent(QPaintEvent *e);

private:
  QPolygonF Points;
  vtkQtPointMarker Marker;
  QList<vtkQtPointMarker::MarkerStyle> Styles;
  QList<QPen> Pens;
};


PointMarkerWidget::PointMarkerWidget(QWidget *widgetParent)
  : QWidget(widgetParent), Points(), Marker(QSizeF(10.0, 10.0)), Styles(),
    Pens()
{
  this->Points.append(QPointF(0.0, 0.0));
  this->Styles.append(vtkQtPointMarker::Cross);
  this->Pens.append(QPen(QBrush(QColor("red")), 1));

  this->Points.append(QPointF(50.0, 50.0));
  this->Styles.append(vtkQtPointMarker::Plus);
  this->Pens.append(QPen(QBrush(QColor("green")), 2));

  this->Points.append(QPointF(100.0, 0.0));
  this->Styles.append(vtkQtPointMarker::Square);
  this->Pens.append(QPen(QBrush(QColor("blue")), 3));

  this->Points.append(QPointF(100.0, 100.0));
  this->Styles.append(vtkQtPointMarker::Circle);
  this->Pens.append(QPen(QBrush(QColor("orange")), 4));

  this->Points.append(QPointF(0.0, 100.0));
  this->Styles.append(vtkQtPointMarker::Diamond);
  this->Pens.append(QPen(QBrush(QColor("purple")), 5));

  // Set the background color to white.
  this->setBackgroundRole(QPalette::Base);
  this->setAutoFillBackground(true);
}

QSize PointMarkerWidget::sizeHint() const
{
  QSizeF hint = this->Points.boundingRect().size();
  hint.setWidth(hint.width() + this->Marker.getSize().width() + 10.0);
  hint.setHeight(hint.height() + this->Marker.getSize().height() + 10.0);
  return hint.toSize();
}

void PointMarkerWidget::paintEvent(QPaintEvent *)
{
  // Set up the painter. Offset the painter for the margin and the
  // marker size.
  QPainter painter(this);
  painter.translate((this->Marker.getSize().width() * 0.5) + 5.0,
    (this->Marker.getSize().height() * 0.5) + 5.0);

  // Draw in a border.
  painter.drawRect(this->Points.boundingRect());

  // Draw all the points using the point marker.
  QPolygonF::Iterator point = this->Points.begin();
  QList<vtkQtPointMarker::MarkerStyle>::Iterator iter = this->Styles.begin();
  QList<QPen>::Iterator pen = this->Pens.begin();
  for( ; point != this->Points.end(); ++point)
    {
    // Set the style.
    if(iter != this->Styles.end())
      {
      this->Marker.setStyle(*iter);
      ++iter;
      }

    // Set the pen.
    if(pen != this->Pens.end())
      {
      painter.setPen(*pen);
      ++pen;
      }

    // Draw the point.
    painter.save();
    painter.translate(*point);
    this->Marker.paint(&painter);
    painter.restore();
    }
}


int PointMarkerItems(int argc, char* argv[])
{
  QTestApp app(argc, argv);

  PointMarkerWidget widget;
  widget.resize(widget.sizeHint());
  widget.show();

  int status = app.exec();

  return status; 
}

