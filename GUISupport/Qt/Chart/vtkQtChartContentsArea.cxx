/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartContentsArea.cxx

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

/// \file vtkQtChartContentsArea.cxx
/// \date 2/8/2008

#include "vtkQtChartContentsArea.h"


vtkQtChartContentsArea::vtkQtChartContentsArea(QGraphicsItem *item,
    QGraphicsScene *graphicsScene)
  : QGraphicsItem(item, graphicsScene)
{
  this->XOffset = 0;
  this->YOffset = 0;
}

QRectF vtkQtChartContentsArea::boundingRect() const
{
  return QRectF(0, 0, 0, 0);
}

void vtkQtChartContentsArea::setXOffset(float offset)
{
  if(offset != this->XOffset)
    {
    this->XOffset = offset;
    this->updateMatrix();
    }
}

void vtkQtChartContentsArea::setYOffset(float offset)
{
  if(offset != this->YOffset)
    {
    this->YOffset = offset;
    this->updateMatrix();
    }
}

void vtkQtChartContentsArea::paint(QPainter *,
    const QStyleOptionGraphicsItem *, QWidget *)
{
}

void vtkQtChartContentsArea::updateMatrix()
{
  QMatrix current = this->matrix();
  current.setMatrix(current.m11(), 0, 0, current.m22(),
      this->XOffset, this->YOffset);
  this->setMatrix(current, false);
}


