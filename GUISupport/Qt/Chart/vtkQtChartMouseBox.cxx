/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartMouseBox.cxx

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

/// \file vtkQtChartMouseBox.cxx
/// \date March 11, 2008

#include "vtkQtChartMouseBox.h"

#include <QBrush>
#include <QPainter>
#include <QPen>


vtkQtChartMouseBox::vtkQtChartMouseBox(QGraphicsItem *item,
    QGraphicsScene *graphicsScene)
  : QGraphicsRectItem(item, graphicsScene)
{
  this->setPen(QPen(Qt::black, 1.0, Qt::DashLine));
}

void vtkQtChartMouseBox::adjustRectangle(const QPointF &current)
{
  // Determine the new area. The origin should be kept as one of the
  // corners.
  QRectF box;
  if(current.x() < 0.0)
    {
    if(current.y() < 0.0)
      {
      box.setTopLeft(current);
      box.setBottomRight(QPointF(0.0, 0.0));
      }
    else
      {
      box.setBottomLeft(current);
      box.setTopRight(QPointF(0.0, 0.0));
      }
    }
  else
    {
    if(current.y() < 0.0)
      {
      box.setTopRight(current);
      box.setBottomLeft(QPointF(0.0, 0.0));
      }
    else
      {
      box.setBottomRight(current);
      box.setTopLeft(QPointF(0.0, 0.0));
      }
    }

  this->setRect(box);
}

void vtkQtChartMouseBox::paint(QPainter *painter,
    const QStyleOptionGraphicsItem *option, QWidget *widget)
{
  painter->setRenderHint(QPainter::Antialiasing, false);
  QGraphicsRectItem::paint(painter, option, widget);
}


