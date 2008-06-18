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
#include <QPen>


class vtkQtChartGridLayerItem : public QGraphicsItem
{
public:
  enum {Type = vtkQtChart_GridLayerItemType};

public:
  vtkQtChartGridLayerItem(QGraphicsItem *parent=0);
  virtual ~vtkQtChartGridLayerItem() {}

  virtual int type() const {return vtkQtChartGridLayerItem::Type;}

  virtual QRectF boundingRect() const;
  virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
      QWidget *widget=0);

  void setPen(const QPen &pen);
  void removeLines();

  QList<QGraphicsLineItem *> Lines;
};


//-----------------------------------------------------------------------------
vtkQtChartGridLayerItem::vtkQtChartGridLayerItem(QGraphicsItem *item)
  : QGraphicsItem(item, item->scene()), Lines()
{
}

QRectF vtkQtChartGridLayerItem::boundingRect() const
{
  return QRectF(0, 0, 0, 0);
}

void vtkQtChartGridLayerItem::paint(QPainter *,
    const QStyleOptionGraphicsItem *, QWidget *)
{
}

void vtkQtChartGridLayerItem::setPen(const QPen &pen)
{
  QList<QGraphicsLineItem *>::Iterator iter = this->Lines.begin();
  for( ; iter != this->Lines.end(); ++iter)
    {
    (*iter)->setPen(pen);
    }
}

void vtkQtChartGridLayerItem::removeLines()
{
  QList<QGraphicsLineItem *>::Iterator iter = this->Lines.begin();
  for( ; iter != this->Lines.end(); ++iter)
    {
    delete *iter;
    }

  this->Lines.clear();
}


//-----------------------------------------------------------------------------
vtkQtChartGridLayer::vtkQtChartGridLayer()
  : vtkQtChartLayer()
{
  this->Contents = new vtkQtChartContentsArea(this, this->scene());
  for(int i = 0; i < 4; i++)
    {
    this->Item[i] = new vtkQtChartGridLayerItem(this->Contents);
    this->Axis[i] = 0;
    }

  // Set the z-order for the grid lines.
  this->Item[vtkQtChartAxis::Top]->setZValue(0);
  this->Item[vtkQtChartAxis::Right]->setZValue(1);
  this->Item[vtkQtChartAxis::Bottom]->setZValue(2);
  this->Item[vtkQtChartAxis::Left]->setZValue(3);
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
  this->setPos(area.topLeft());

  // Set up the grid lines.
  this->layoutVerticalGrid(this->Item[vtkQtChartAxis::Top],
      this->Axis[vtkQtChartAxis::Top], area.height());
  this->layoutVerticalGrid(this->Item[vtkQtChartAxis::Bottom],
      this->Axis[vtkQtChartAxis::Bottom], area.height());
  this->layoutHorizontalGrid(this->Item[vtkQtChartAxis::Left],
      this->Axis[vtkQtChartAxis::Left], area.width());
  this->layoutHorizontalGrid(this->Item[vtkQtChartAxis::Right],
      this->Axis[vtkQtChartAxis::Right], area.width());
}

bool vtkQtChartGridLayer::drawItemFilter(QGraphicsItem *item,
    QPainter *painter)
{
  // If the item is a grid line, clip it to the chart layer bounds.
  QGraphicsLineItem *line = qgraphicsitem_cast<QGraphicsLineItem *>(item);
  if(line && this->ChartArea)
    {
    QRectF bounds;
    this->ChartArea->getContentsSpace()->getChartLayerBounds(bounds);
    painter->setClipRect(bounds, Qt::IntersectClip);
    }

  return false;
}

QRectF vtkQtChartGridLayer::boundingRect() const
{
  return QRectF(0, 0, 0, 0);
}

void vtkQtChartGridLayer::paint(QPainter *,
    const QStyleOptionGraphicsItem *, QWidget *)
{
}

void vtkQtChartGridLayer::setXOffset(float xOffset)
{
  this->Item[vtkQtChartAxis::Top]->setPos(-xOffset, 0.0);
  this->Item[vtkQtChartAxis::Bottom]->setPos(-xOffset, 0.0);
}

void vtkQtChartGridLayer::setYOffset(float yOffset)
{
  this->Item[vtkQtChartAxis::Left]->setPos(0.0, -yOffset);
  this->Item[vtkQtChartAxis::Right]->setPos(0.0, -yOffset);
}

void vtkQtChartGridLayer::handleGridChange()
{
  for(int i = 0; i < 4; i++)
    {
    if(this->Axis[i])
      {
      vtkQtChartAxisOptions *options = this->Axis[i]->getOptions();
      this->Item[i]->setVisible(options->isGridVisible());
      this->Item[i]->setPen(QPen(options->getGridColor()));
      }
    }
}

void vtkQtChartGridLayer::layoutVerticalGrid(vtkQtChartGridLayerItem *item,
    vtkQtChartAxis *axis, float height)
{
  // Clear out the current grid lines.
  item->removeLines();

  // Add new lines for the axis tick marks.
  int total = axis->getModel()->getNumberOfLabels();
  for(int i = 0; i < total; i++)
    {
    // Only add grid lines for visible tick marks.
    if(!axis->isLabelTickVisible(i))
      {
      continue;
      }

    float pixel = axis->getLabelLocation(i);
    item->Lines.append(new QGraphicsLineItem(
        pixel, 0, pixel, height, item, item->scene()));
    }

  // Set the grid color for the new lines.
  item->setPen(QPen(axis->getOptions()->getGridColor()));
}

void vtkQtChartGridLayer::layoutHorizontalGrid(vtkQtChartGridLayerItem *item,
    vtkQtChartAxis *axis, float width)
{
  // Clear out the current grid lines.
  item->removeLines();

  // Add new lines for the axis tick marks.
  int total = axis->getModel()->getNumberOfLabels();
  for(int i = 0; i < total; i++)
    {
    // Only add grid lines for visible tick marks.
    if(!axis->isLabelTickVisible(i))
      {
      continue;
      }

    float pixel = axis->getLabelLocation(i);
    item->Lines.append(new QGraphicsLineItem(
        0, pixel, width, pixel, item, item->scene()));
    }

  // Set the grid color for the new lines.
  item->setPen(QPen(axis->getOptions()->getGridColor()));
}


