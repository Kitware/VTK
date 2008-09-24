/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartScene.cxx

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

/// \file vtkQtChartScene.cxx
/// \date March 12, 2008

#include "vtkQtChartScene.h"

#include "vtkQtChartLayer.h"
#include "vtkQtChartMouseBox.h"
#include <QGraphicsItem>
#include <QPainter>
#include <QStyleOptionGraphicsItem>


vtkQtChartScene::vtkQtChartScene(QObject *parentObject)
  : QGraphicsScene(parentObject)
{
  this->Box = 0;
}

void vtkQtChartScene::drawItems(QPainter *painter, int numItems,
    QGraphicsItem **itemList, const QStyleOptionGraphicsItem *options,
    QWidget *widget)
{
  for(int i = 0; i < numItems; ++i)
    {
    QGraphicsItem *item = itemList[i];

    // See if the item is the child of a chart layer.
    vtkQtChartLayer *layer = 0;
    QGraphicsItem *parentItem = item->parentItem();
    if(parentItem)
      {
      QGraphicsItem *parentParent = parentItem->parentItem();
      while(parentParent != 0)
        {
        parentItem = parentParent;
        parentParent = parentParent->parentItem();
        }

      layer = qgraphicsitem_cast<vtkQtChartLayer *>(parentItem);
      }

    painter->save();
    if(!layer || !layer->drawItemFilter(item, painter))
      {
      painter->setMatrix(item->sceneMatrix(), true);
      item->paint(painter, &options[i], widget);
      }

    painter->restore();
    }
}

void vtkQtChartScene::drawForeground(QPainter *painter, const QRectF &)
{
  // Draw the mouse box on to of the scene.
  if(this->Box && this->Box->isVisible())
    {
    painter->setPen(QPen(Qt::black, 1.0, Qt::DashLine));
    painter->setRenderHint(QPainter::Antialiasing, false);
    painter->drawRect(this->Box->getRectangle());
    }
}


