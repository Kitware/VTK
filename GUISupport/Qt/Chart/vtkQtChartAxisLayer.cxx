/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartAxisLayer.cxx

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

/// \file vtkQtChartAxisLayer.cxx
/// \date February 1, 2008

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#endif

#include "vtkQtChartAxisLayer.h"

#include "vtkQtChartAxis.h"
#include "vtkQtChartAxisCornerDomain.h"
#include "vtkQtChartAxisDomain.h"
#include "vtkQtChartAxisDomainPriority.h"
#include "vtkQtChartAxisModel.h"
#include "vtkQtChartContentsSpace.h"
#include "vtkQtChartLayerDomain.h"
#include "vtkQtChartSeriesDomain.h"
#include "vtkQtChartArea.h"

#include <QGraphicsRectItem>

#define TOO_SMALL_WIDTH 40
#define TOO_SMALL_HEIGHT 30


class vtkQtChartAxisLayerItem
{
public:
  vtkQtChartAxisLayerItem();
  ~vtkQtChartAxisLayerItem() {}

  vtkQtChartAxisLayer::AxisBehavior Behavior;
  vtkQtChartAxisDomainPriority Priority;
  bool Modified;
};


//-----------------------------------------------------------------------------
vtkQtChartAxisLayerItem::vtkQtChartAxisLayerItem()
  : Priority()
{
  this->Behavior = vtkQtChartAxisLayer::ChartSelect;
  this->Modified = true;
}


//-----------------------------------------------------------------------------
vtkQtChartAxisLayer::vtkQtChartAxisLayer()
  : vtkQtChartLayer(), LayerBounds()
{
  this->Border = new QGraphicsRectItem(this, this->scene());
  this->setupAxesCorner();
  this->RangeChanged = false;

  // Set up the axis option items.
  for(int i = 0; i < 4; i++)
    {
    this->Option[i] = new vtkQtChartAxisLayerItem();
    }

  // Set the drawing order for the border and axes.
  this->Border->setZValue(0);
  this->Axis[vtkQtChartAxis::Top]->setZValue(1);
  this->Axis[vtkQtChartAxis::Right]->setZValue(2);
  this->Axis[vtkQtChartAxis::Bottom]->setZValue(3);
  this->Axis[vtkQtChartAxis::Left]->setZValue(4);

  // Set the border color.
  this->Border->setPen(QPen(Qt::darkGray));
}

vtkQtChartAxisLayer::~vtkQtChartAxisLayer()
{
  // Clean up the axis option items.
  for(int i = 0; i < 4; i++)
    {
    delete this->Option[i];
    }
}

vtkQtChartAxis *vtkQtChartAxisLayer::getAxis(
    vtkQtChartAxis::AxisLocation location) const
{
  return this->Axis[location];
}

vtkQtChartAxis *vtkQtChartAxisLayer::getHorizontalAxis(
    vtkQtChartLayer::AxesCorner axes) const
{
  if(axes == vtkQtChartLayer::TopLeft || axes == vtkQtChartLayer::TopRight)
    {
    return this->Axis[vtkQtChartAxis::Top];
    }
  else
    {
    return this->Axis[vtkQtChartAxis::Bottom];
    }
}

vtkQtChartAxis *vtkQtChartAxisLayer::getVerticalAxis(
    vtkQtChartLayer::AxesCorner axes) const
{
  if(axes == vtkQtChartLayer::BottomLeft || axes == vtkQtChartLayer::TopLeft)
    {
    return this->Axis[vtkQtChartAxis::Left];
    }
  else
    {
    return this->Axis[vtkQtChartAxis::Right];
    }
}

vtkQtChartAxisLayer::AxisBehavior vtkQtChartAxisLayer::getAxisBehavior(
    vtkQtChartAxis::AxisLocation location) const
{
  return this->Option[location]->Behavior;
}

void vtkQtChartAxisLayer::setAxisBehavior(
    vtkQtChartAxis::AxisLocation location,
    vtkQtChartAxisLayer::AxisBehavior behavior)
{
  if(this->Option[location]->Behavior != behavior)
    {
    this->Option[location]->Behavior = behavior;
    this->Option[location]->Modified = true;
    }
}

const vtkQtChartAxisDomainPriority &vtkQtChartAxisLayer::getAxisDomainPriority(
    vtkQtChartAxis::AxisLocation location) const
{
  return this->Option[location]->Priority;
}

void vtkQtChartAxisLayer::setAxisDomainPriority(
    vtkQtChartAxis::AxisLocation location,
    const vtkQtChartAxisDomainPriority &priority)
{
  if(this->Option[location]->Priority != priority)
    {
    this->Option[location]->Priority = priority;
    this->Option[location]->Modified = true;
    }
}

void vtkQtChartAxisLayer::layoutChart(const QRectF &area)
{
  // If any of the axes use ChartSelect layout, gather the layer
  // domain information.
  int i = 0;
  bool gatherDomains = false;
  for( ; !gatherDomains && i < 4; i++)
    {
    gatherDomains = (this->RangeChanged || this->Option[i]->Modified) &&
        this->Option[i]->Behavior == vtkQtChartAxisLayer::ChartSelect;
    }

  vtkQtChartLayerDomain layerDomain;
  if(gatherDomains)
    {
    for(int j = 0; j < this->ChartArea->getNumberOfLayers(); j++)
      {
      this->ChartArea->getLayer(j)->getLayerDomain(layerDomain);
      }
    }

  // Set up the axes in order for the axis domain priorities.
  static const vtkQtChartAxis::AxisLocation order[4] =
    {
    vtkQtChartAxis::Bottom,
    vtkQtChartAxis::Left,
    vtkQtChartAxis::Top,
    vtkQtChartAxis::Right
    };

  vtkQtChartAxis::AxisDomain axesDomain[4] =
    {
    vtkQtChartAxis::UnsupportedDomain,
    vtkQtChartAxis::UnsupportedDomain,
    vtkQtChartAxis::UnsupportedDomain,
    vtkQtChartAxis::UnsupportedDomain
    };

  for(i = 0; i < 4; i++)
    {
    if((this->RangeChanged || this->Option[order[i]]->Modified) &&
        this->Option[order[i]]->Behavior == vtkQtChartAxisLayer::ChartSelect)
      {
      // Use the chart domain and axis domain priority to determine
      // the axis domain. The domain depends on the neighboring axes.
      int previous = i - 1;
      if(previous < 0)
        {
        previous = 3;
        }

      // If the neighboring axes are not using chart-select layout,
      // set their domain type in the array.
      if(axesDomain[order[previous]] == vtkQtChartAxis::UnsupportedDomain &&
          this->Option[order[previous]]->Behavior != 
          vtkQtChartAxisLayer::ChartSelect)
        {
        axesDomain[order[previous]] = this->getAxisDomain(order[previous]);
        }

      int next = i + 1;
      if(next > 3)
        {
        next = 0;
        }

      if(axesDomain[order[next]] == vtkQtChartAxis::UnsupportedDomain &&
          this->Option[order[next]]->Behavior != 
          vtkQtChartAxisLayer::ChartSelect)
        {
        axesDomain[order[next]] = this->getAxisDomain(order[next]);
        }

      // Find the domain for the axis.
      vtkQtChartAxisDomain domain;
      this->findAxisDomain(order[i], order[previous],
          axesDomain[order[previous]], layerDomain, domain);
      this->findAxisDomain(order[i], order[next],
          axesDomain[order[next]], layerDomain, domain);

      this->Axis[order[i]]->setDataAvailable(!domain.isEmpty());
      this->Axis[order[i]]->setRangePaddingUsed(domain.isRangePaddingUsed());
      this->Axis[order[i]]->setExpansionToZeroUsed(
          domain.isExpansionToZeroUsed());
      this->Axis[order[i]]->setExtraSpaceUsed(domain.isExtraSpaceUsed());

      bool isRange = false;
      QList<QVariant> list = domain.getDomain(isRange);
      this->Axis[order[i]]->setBestFitGenerated(isRange);
      if(isRange)
        {
        this->Axis[order[i]]->setBestFitRange(list[0], list[1]);
        }
      else
        {
        // Clear the current labels from the model. Block the
        // signals from the axis while it is being modified to
        // prevent recursion.
        vtkQtChartAxisModel *axisModel = this->Axis[order[i]]->getModel();
        this->Axis[order[i]]->blockSignals(true);
        axisModel->startModifyingData();
        axisModel->removeAllLabels();
        QList<QVariant>::Iterator iter = list.begin();
        for( ; iter != list.end(); ++iter)
          {
          axisModel->addLabel(*iter);
          }

        axisModel->finishModifyingData();
        this->Axis[order[i]]->blockSignals(false);
        }
      }
    else if(this->Option[order[i]]->Modified)
      {
      this->Axis[order[i]]->setBestFitGenerated(
          this->Option[order[i]]->Behavior == vtkQtChartAxisLayer::BestFit);
      }

    this->Option[order[i]]->Modified = false;
    }

  this->RangeChanged = false;
  vtkQtChartAxis::AxisLocation left = vtkQtChartAxis::Left;
  vtkQtChartAxis::AxisLocation bottom = vtkQtChartAxis::Bottom;
  vtkQtChartAxis::AxisLocation right = vtkQtChartAxis::Right;
  vtkQtChartAxis::AxisLocation top = vtkQtChartAxis::Top;

  // Make sure there is enough vertical space. The top and bottom axes
  // know their preferred size before layout.
  QRectF bounds = area;
  float fontHeight = this->Axis[left]->getFontHeight();
  if(this->Axis[right]->getFontHeight() > fontHeight)
    {
    fontHeight = this->Axis[right]->getFontHeight();
    }

  fontHeight *= 0.5;
  float available = fontHeight;
  float space = this->Axis[top]->getPreferredSpace();
  if(space > fontHeight)
    {
    available = space;
    }

  space = this->Axis[bottom]->getPreferredSpace();
  available += space > fontHeight ? space : fontHeight;

  // Set the 'too small' flag on each of the axis objects.
  bool tooSmall = bounds.height() - available < TOO_SMALL_HEIGHT;
  for(i = 0; i < 4; i++)
    {
    this->Axis[i]->setSpaceTooSmall(tooSmall);
    }

  // Layout the left and right axes first.
  this->Axis[left]->layoutAxis(bounds);
  this->Axis[right]->layoutAxis(bounds);

  QRectF axisBounds;
  if(!tooSmall)
    {
    // Make sure there is enough horizontal space.
    available = bounds.width();
    axisBounds = this->Axis[left]->getBounds();
    available -= axisBounds.width();

    axisBounds = this->Axis[right]->getBounds();
    available -= axisBounds.width();

    tooSmall = available < TOO_SMALL_WIDTH;
    if(tooSmall)
      {
      // Set the 'too small' flag on each of the axis objects.
      // Re-layout the left and right axes.
      for(i = 0; i < 4; i++)
        {
        this->Axis[i]->setSpaceTooSmall(tooSmall);
        }

      this->Axis[left]->layoutAxis(bounds);
      this->Axis[right]->layoutAxis(bounds);
      }
    }

  // Layout the top and bottom axes. They need size from the left and
  // right axes layout.
  this->Axis[top]->layoutAxis(bounds);
  this->Axis[bottom]->layoutAxis(bounds);

  // The top and bottom axes should have the same width. The top
  // axis may need to be layed out again to account for the width
  // of the bottom axis labels.
  axisBounds = this->Axis[bottom]->getBounds();
  float bottomWidth = axisBounds.width();
  axisBounds = this->Axis[top]->getBounds();
  if(bottomWidth != axisBounds.width())
    {
    this->Axis[top]->layoutAxis(bounds);
    }

  if(!tooSmall)
    {
    // Check the horizontal space using the bounds from the top and
    // bottom axes.
    axisBounds = this->Axis[top]->getBounds();
    tooSmall = axisBounds.width() < TOO_SMALL_WIDTH;
    if(tooSmall)
      {
      // Set the 'too small' flag on each of the axis objects.
      // Re-layout all of the axes.
      for(i = 0; i < 4; i++)
        {
        this->Axis[i]->setSpaceTooSmall(tooSmall);
        }

      this->Axis[left]->layoutAxis(bounds);
      this->Axis[right]->layoutAxis(bounds);
      this->Axis[top]->layoutAxis(bounds);
      this->Axis[bottom]->layoutAxis(bounds);
      }
    else
      {
      // Adjust the size of the left and right axes. The top and bottom
      // axes may have needed more space.
      this->Axis[left]->adjustAxisLayout();
      this->Axis[right]->adjustAxisLayout();
      }
    }

  // Save the layer bounds for access.
  bounds = this->Axis[left]->getBounds();
  this->LayerBounds.setTop(bounds.top());
  this->LayerBounds.setBottom(bounds.bottom());

  bounds = this->Axis[bottom]->getBounds();
  this->LayerBounds.setLeft(bounds.left());
  this->LayerBounds.setRight(bounds.right());

  // Update the layer border. Adjust the width and height to account
  // for Qt's rect paint code.
  this->Border->setRect(this->LayerBounds);
}

void vtkQtChartAxisLayer::setChartArea(vtkQtChartArea *area)
{
  vtkQtChartLayer::setChartArea(area);
  vtkQtChartContentsSpace *zoom = 0;
  if(this->ChartArea)
    {
    zoom = this->ChartArea->getContentsSpace();
    }

  for(int i = 0; i < 4; i++)
    {
    this->Axis[i]->setContentsSpace(zoom);
    }
}

QRectF vtkQtChartAxisLayer::boundingRect() const
{
  return QRectF(0, 0, 0, 0);
}

void vtkQtChartAxisLayer::paint(QPainter *,
    const QStyleOptionGraphicsItem *, QWidget *)
{
}

void vtkQtChartAxisLayer::handleChartRangeChange()
{
  this->RangeChanged = true;
}

void vtkQtChartAxisLayer::cancelChartRangeChange()
{
  this->RangeChanged = false;
}

void vtkQtChartAxisLayer::setupAxesCorner()
{
  // Create an axis object for each location.
  vtkQtChartAxis::AxisLocation left = vtkQtChartAxis::Left;
  this->Axis[left] = new vtkQtChartAxis(left, this);
  this->Axis[left]->setObjectName("LeftAxis");
  vtkQtChartAxisModel *model = new vtkQtChartAxisModel(this);
  model->setObjectName("LeftAxisModel");
  this->Axis[left]->setModel(model);

  vtkQtChartAxis::AxisLocation bottom = vtkQtChartAxis::Bottom;
  this->Axis[bottom] = new vtkQtChartAxis(bottom, this);
  this->Axis[bottom]->setObjectName("BottomAxis");
  model = new vtkQtChartAxisModel(this);
  model->setObjectName("BottomAxisModel");
  this->Axis[bottom]->setModel(model);

  vtkQtChartAxis::AxisLocation right = vtkQtChartAxis::Right;
  this->Axis[right] = new vtkQtChartAxis(right, this);
  this->Axis[right]->setObjectName("RightAxis");
  model = new vtkQtChartAxisModel(this);
  model->setObjectName("RightAxisModel");
  this->Axis[right]->setModel(model);

  vtkQtChartAxis::AxisLocation top = vtkQtChartAxis::Top;
  this->Axis[top] = new vtkQtChartAxis(top, this);
  this->Axis[top]->setObjectName("TopAxis");
  model = new vtkQtChartAxisModel(this);
  model->setObjectName("TopAxisModel");
  this->Axis[top]->setModel(model);

  // Set up the axis neighbors and the parallel axis.
  this->Axis[left]->setNeigbors(this->Axis[bottom], this->Axis[top]);
  this->Axis[bottom]->setNeigbors(this->Axis[left], this->Axis[right]);
  this->Axis[right]->setNeigbors(this->Axis[bottom], this->Axis[top]);
  this->Axis[top]->setNeigbors(this->Axis[left], this->Axis[right]);

  this->Axis[left]->setParallelAxis(this->Axis[right]);
  this->Axis[bottom]->setParallelAxis(this->Axis[top]);
  this->Axis[right]->setParallelAxis(this->Axis[left]);
  this->Axis[top]->setParallelAxis(this->Axis[bottom]);

  // Forward the axis update signals.
  for(int i = 0; i < 4; i++)
    {
    this->connect(this->Axis[i], SIGNAL(layoutNeeded()),
        this, SIGNAL(layoutNeeded()));
    }
}

vtkQtChartAxis::AxisDomain vtkQtChartAxisLayer::getAxisDomain(
    vtkQtChartAxis::AxisLocation location) const
{
  if(this->Option[location]->Behavior == vtkQtChartAxisLayer::FixedInterval)
    {
    // Use the axis model to determine the domain.
    QVariant label;
    this->Axis[location]->getModel()->getLabel(0, label);
    return vtkQtChartAxisDomain::getAxisDomain(label.type());
    }
  else if(this->Option[location]->Behavior == vtkQtChartAxisLayer::BestFit)
    {
    // Use the best-fit range to determine the domain.
    QVariant minimum, maximum;
    this->Axis[location]->getBestFitRange(minimum, maximum);
    return vtkQtChartAxisDomain::getAxisDomain(minimum.type());
    }

  return vtkQtChartAxis::UnsupportedDomain;
}

vtkQtChartLayer::AxesCorner vtkQtChartAxisLayer::getCorner(
    vtkQtChartAxis::AxisLocation first,
    vtkQtChartAxis::AxisLocation second) const
{
  if(first == vtkQtChartAxis::Bottom)
    {
    if(second == vtkQtChartAxis::Left)
      {
      return vtkQtChartLayer::BottomLeft;
      }
    else
      {
      return vtkQtChartLayer::BottomRight;
      }
    }
  else if(first == vtkQtChartAxis::Left)
    {
    if(second == vtkQtChartAxis::Bottom)
      {
      return vtkQtChartLayer::BottomLeft;
      }
    else
      {
      return vtkQtChartLayer::TopLeft;
      }
    }
  else if(first == vtkQtChartAxis::Top)
    {
    if(second == vtkQtChartAxis::Left)
      {
      return vtkQtChartLayer::TopLeft;
      }
    else
      {
      return vtkQtChartLayer::TopRight;
      }
    }
  else
    {
    if(second == vtkQtChartAxis::Bottom)
      {
      return vtkQtChartLayer::BottomRight;
      }
    else
      {
      return vtkQtChartLayer::TopRight;
      }
    }
}

void vtkQtChartAxisLayer::findAxisDomain(vtkQtChartAxis::AxisLocation axis,
    vtkQtChartAxis::AxisLocation neighbor,
    vtkQtChartAxis::AxisDomain neighborDomain,
    const vtkQtChartLayerDomain &layerDomain,
    vtkQtChartAxisDomain &axisDomain) const
{
  const vtkQtChartAxisCornerDomain *corner = layerDomain.getDomain(
      this->getCorner(axis, neighbor));
  if(!corner)
    {
    return;
    }

  vtkQtChartAxisDomain seriesDomain;
  const vtkQtChartSeriesDomain *series = 0;
  if(axis == vtkQtChartAxis::Bottom || axis == vtkQtChartAxis::Top)
    {
    if(neighborDomain == vtkQtChartAxis::UnsupportedDomain)
      {
      series = corner->getDomain(this->Option[axis]->Priority,
          this->Option[neighbor]->Priority);
      }
    else
      {
      series = corner->getDomain(this->Option[axis]->Priority, neighborDomain);
      }

    if(series)
      {
      seriesDomain = series->getXDomain();
      }
    }
  else
    {
    if(neighborDomain == vtkQtChartAxis::UnsupportedDomain)
      {
      series = corner->getDomain(this->Option[neighbor]->Priority,
          this->Option[axis]->Priority);
      }
    else
      {
      series = corner->getDomain(neighborDomain, this->Option[axis]->Priority);
      }

    if(series)
      {
      seriesDomain = series->getYDomain();
      }
    }

  if(!seriesDomain.isEmpty())
    {
    if(axisDomain.isEmpty())
      {
      axisDomain = seriesDomain;
      }
    else
      {
      // Get the priority index for each of the domains.
      int index1 = this->Option[axis]->Priority.getOrder().indexOf(
          axisDomain.getDomainType());
      int index2 = this->Option[axis]->Priority.getOrder().indexOf(
          seriesDomain.getDomainType());
      if(index2 < index1)
        {
        axisDomain = seriesDomain;
        }
      else if(index1 == index2)
        {
        axisDomain.mergeDomain(seriesDomain);
        }
      }
    }
}


