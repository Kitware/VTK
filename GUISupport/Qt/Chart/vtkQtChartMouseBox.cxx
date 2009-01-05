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

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#endif

#include "vtkQtChartMouseBox.h"

#include <QBrush>
#include <QGraphicsView>
#include <QPainter>
#include <QPen>


vtkQtChartMouseBox::vtkQtChartMouseBox(QGraphicsView *view)
  : QObject(view)
{
  this->View = view;
  this->Last = new QPointF();
  this->Box = new QRectF();
  this->Showing = false;
}

vtkQtChartMouseBox::~vtkQtChartMouseBox()
{
  delete this->Last;
  delete this->Box;
}

void vtkQtChartMouseBox::setVisible(bool visible)
{
  if(this->Showing != visible)
    {
    this->Showing = visible;
    emit this->updateNeeded(*this->Box);
    }
}

const QPointF &vtkQtChartMouseBox::getStartingPosition() const
{
  return *this->Last;
}

void vtkQtChartMouseBox::setStartingPosition(const QPoint &start)
{
  *this->Last = this->View->mapToScene(start);
}

void vtkQtChartMouseBox::adjustRectangle(const QPoint &current)
{
  // Map the point to scene coordinates.
  QPointF point = this->View->mapToScene(current);

  // Determine the new area. The mouse down point should be kept as
  // one of the corners.
  QRectF old = *this->Box;
  if(point.x() < this->Last->x())
    {
    if(point.y() < this->Last->y())
      {
      this->Box->setTopLeft(point);
      this->Box->setBottomRight(*this->Last);
      }
    else
      {
      this->Box->setBottomLeft(point);
      this->Box->setTopRight(*this->Last);
      }
    }
  else
    {
    if(point.y() < this->Last->y())
      {
      this->Box->setTopRight(point);
      this->Box->setBottomLeft(*this->Last);
      }
    else
      {
      this->Box->setBottomRight(point);
      this->Box->setTopLeft(*this->Last);
      }
    }

  emit this->updateNeeded(this->Box->unite(old));
}

const QRectF &vtkQtChartMouseBox::getRectangle() const
{
  return *this->Box;
}


