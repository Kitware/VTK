/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartTitle.cxx

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

/// \file vtkQtChartTitle.cxx
/// \date 11/17/2006

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#endif

#include "vtkQtChartTitle.h"

#include <QEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QPrinter>


vtkQtChartTitle::vtkQtChartTitle(Qt::Orientation orient, QWidget *widgetParent)
  : QWidget(widgetParent), Text(), Bounds()
{
  this->Orient = orient;
  this->Align = Qt::AlignCenter;

  // Set up the default size policy.
  if(this->Orient == Qt::Horizontal)
    {
    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }
  else
    {
    this->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    }
}

void vtkQtChartTitle::setOrientation(Qt::Orientation orient)
{
  if(orient != this->Orient)
    {
    this->Orient = orient;
    if(this->Orient == Qt::Horizontal)
      {
      this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
      }
    else
      {
      this->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
      }

    this->calculateSize();
    emit this->orientationChanged();
    }
}

void vtkQtChartTitle::setText(const QString &text)
{
  if(text != this->Text)
    {
    this->Text = text;
    this->calculateSize();
    }
}

void vtkQtChartTitle::drawTitle(QPainter &painter)
{
  QRect area;
  if(this->Orient == Qt::Vertical)
    {
    // Rotate the painter if the orientation is vertical.
    painter.translate(QPoint(0, this->height() - 1));
    painter.rotate(-90.0);

    // Set up the text area.
    if(this->height() - this->Bounds.height() < 0)
      {
      // TODO: Allow the user to move the drawing origin to see the
      // hidden parts of the text.
      area.setRect(0, 0, this->Bounds.height(), this->width());
      }
    else
      {
      area.setRect(0, 0, this->height(), this->width());
      }
    }
  else
    {
    // Set up the text area.
    if(this->width() - this->Bounds.width() < 0)
      {
      area.setRect(0, 0, this->Bounds.width(), this->height());
      }
    else
      {
      area.setRect(0, 0, this->width(), this->height());
      }
    }

  // If the painter is a printer, set the font.
  painter.setFont(QFont(this->font(), painter.device()));

  // Set up the painter and draw the text.
  painter.setPen(this->palette().color(QPalette::Text));
  painter.drawText(area, this->Align, this->Text);
}

bool vtkQtChartTitle::event(QEvent *e)
{
  if(e->type() == QEvent::FontChange)
    {
    this->calculateSize();
    }

  return QWidget::event(e);
}

void vtkQtChartTitle::paintEvent(QPaintEvent *e)
{
  if(this->Text.isEmpty() || !this->Bounds.isValid() || !e->rect().isValid())
    {
    return;
    }

  QPainter painter(this);
  this->drawTitle(painter);

  e->accept();
}

void vtkQtChartTitle::calculateSize()
{
  // Use the font size and orientation to determine the size needed.
  QSize bounds;
  if(!this->Text.isEmpty())
    {
    QFontMetrics fm = this->fontMetrics();
    bounds.setWidth(fm.width(this->Text));
    bounds.setHeight(fm.height());
    if(this->Orient == Qt::Vertical)
      {
      bounds.transpose();
      }
    }

  // If the size has changed, update the layout.
  if(this->Bounds != bounds)
    {
    this->Bounds = bounds;
    this->updateGeometry();
    }
}


