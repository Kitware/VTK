/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartGridLayer.cxx

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

/// \file vtkQtChartGridLayer.cxx
/// \date February 1, 2008

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#endif

#include "vtkQtChartGridLayer.h"

#include "vtkQtChartAxis.h"
#include "vtkQtChartAxisLayer.h"
#include "vtkQtChartAxisModel.h"
#include "vtkQtChartAxisOptions.h"
#include "vtkQtChartContentsArea.h"
#include "vtkQtChartContentsSpace.h"
#include "vtkQtChartArea.h"

#include <QGraphicsLineItem>
#include <QList>
#include <QPainter>
#include <QPen>


//-----------------------------------------------------------------------------
vtkQtChartGridLayer::vtkQtChartGridLayer()
  : vtkQtChartLayer()
{
  this->Bounds = new QSizeF(0.0, 0.0);
  for(int i = 0; i < 4; i++)
    {
    this->Axis[i] = 0;
    }
}

vtkQtChartGridLayer::~vtkQtChartGridLayer()
{
  delete this->Bounds;
}

void vtkQtChartGridLayer::setChartArea(vtkQtChartArea *area)
{
  int i = 0;
  if(this->ChartArea)
    {
    this->disconnect(this->ChartArea->getContentsSpace(), 0, this, 0);
    for(i = 0; i < 4; i++)
      {
      this->disconnect(this->Axis[i]->getOptions(), 0, this, 0);
      this->Axis[i] = 0;
      }
    }

  vtkQtChartLayer::setChartArea(area);
  if(this->ChartArea)
    {
    vtkQtChartContentsSpace *space = this->ChartArea->getContentsSpace();
    this->connect(space, SIGNAL(xOffsetChanged(float)),
        this, SLOT(setXOffset(float)));
    this->connect(space, SIGNAL(yOffsetChanged(float)),
        this, SLOT(setYOffset(float)));

    vtkQtChartAxisLayer *axes = this->ChartArea->getAxisLayer();
    this->Axis[vtkQtChartAxis::Left] = axes->getAxis(vtkQtChartAxis::Left);
    this->Axis[vtkQtChartAxis::Bottom] = axes->getAxis(vtkQtChartAxis::Bottom);
    this->Axis[vtkQtChartAxis::Right] = axes->getAxis(vtkQtChartAxis::Right);
    this->Axis[vtkQtChartAxis::Top] = axes->getAxis(vtkQtChartAxis::Top);
    for(i = 0; i < 4; i++)
      {
      this->connect(this->Axis[i]->getOptions(), SIGNAL(gridChanged()),
          this, SLOT(handleGridChange()));
      }

    this->handleGridChange();
    }
}

void vtkQtChartGridLayer::layoutChart(const QRectF &area)
{
  // Update the position based on the new bounds.
  this->prepareGeometryChange();
  *(this->Bounds) = area.size();
  this->setPos(area.topLeft());
}

QRectF vtkQtChartGridLayer::boundingRect() const
{
  return QRectF(QPointF(0.0, 0.0), *this->Bounds);
}

void vtkQtChartGridLayer::paint(QPainter *painter,
    const QStyleOptionGraphicsItem *, QWidget *)
{
  this->drawAxisGrid(painter, this->Axis[vtkQtChartAxis::Top]);
  this->drawAxisGrid(painter, this->Axis[vtkQtChartAxis::Right]);
  this->drawAxisGrid(painter, this->Axis[vtkQtChartAxis::Bottom]);
  this->drawAxisGrid(painter, this->Axis[vtkQtChartAxis::Left]);
}

void vtkQtChartGridLayer::setXOffset(float)
{
  this->update();
}

void vtkQtChartGridLayer::setYOffset(float)
{
  this->update();
}

void vtkQtChartGridLayer::handleGridChange()
{
  this->update();
}

void vtkQtChartGridLayer::drawAxisGrid(QPainter *painter, vtkQtChartAxis *axis)
{
  if(axis)
    {
    vtkQtChartAxisOptions *options = axis->getOptions();
    if(options->isGridVisible())
      {
      painter->setPen(options->getGridColor());
      vtkQtChartContentsSpace *space = this->ChartArea->getContentsSpace();
      bool vertical = axis->getLocation() == vtkQtChartAxis::Left ||
          axis->getLocation() == vtkQtChartAxis::Right;
      int total = axis->getModel()->getNumberOfLabels();
      for(int i = 0; i < total; i++)
        {
        // Only draw grid lines for visible tick marks.
        if(!axis->isLabelTickVisible(i))
          {
          continue;
          }

        // Get the grid line location and adjust it for translation.
        float pixel = axis->getLabelLocation(i);
        if(vertical)
          {
          // Clip the drawing to the viewport.
          pixel -= space->getYOffset();
          if(pixel > this->Bounds->height())
            {
            continue;
            }
          else if(pixel < 0.0)
            {
            break;
            }

          // Draw the grid line.
          painter->drawLine(QPointF(0.0, pixel),
              QPointF(this->Bounds->width(), pixel));
          }
        else
          {
          // Clip the drawing to the viewport.
          pixel -= space->getXOffset();
          if(pixel < 0.0)
            {
            continue;
            }
          else if(pixel > this->Bounds->width())
            {
            break;
            }

          // Draw the grid line.
          painter->drawLine(QPointF(pixel, 0.0),
              QPointF(pixel, this->Bounds->height()));
          }
        }
      }
    }
}


