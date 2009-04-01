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

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#endif

#include "vtkQtPointMarker.h"

#include <QPainter>
#include <QPolygonF>
#include <QRectF>
#include <QSizeF>


//----------------------------------------------------------------------------
vtkQtPointMarker::vtkQtPointMarker(const QSizeF &size,
    vtkQtPointMarker::MarkerStyle style)
  : Rect(-size.width() * 0.5, -size.height() * 0.5,
    size.width(), size.height())
{
  this->Style = style;
}

vtkQtPointMarker::~vtkQtPointMarker()
{
}

void vtkQtPointMarker::paint(QPainter *painter)
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
    case vtkQtPointMarker::NoMarker:
    case vtkQtPointMarker::UserStyle:
    default:
      {
      break;
      }
    }
}

QSizeF vtkQtPointMarker::getSize() const
{
  return this->Rect.size();
}

void vtkQtPointMarker::setSize(const QSizeF &size)
{
  if(size != this->Rect.size())
    {
    this->Rect.setRect(-size.width() * 0.5, -size.height() * 0.5,
        size.width(), size.height());
    }
}

void vtkQtPointMarker::setStyle(vtkQtPointMarker::MarkerStyle style)
{
  this->Style = style;
}


