/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtStatisticalBoxChart.cxx

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

/// \file vtkQtStatisticalBoxChart.cxx
/// \date May 15, 2008

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#endif

#include "vtkQtStatisticalBoxChart.h"

#include "vtkQtChartArea.h"
#include "vtkQtChartAxisCornerDomain.h"
#include "vtkQtChartAxisDomain.h"
#include "vtkQtChartAxis.h"
#include "vtkQtChartAxisLayer.h"
#include "vtkQtChartAxisOptions.h"
#include "vtkQtChartBar.h"
#include "vtkQtChartColors.h"
#include "vtkQtChartContentsArea.h"
#include "vtkQtChartContentsSpace.h"
#include "vtkQtChartHelpFormatter.h"
#include "vtkQtChartIndexRangeList.h"
#include "vtkQtChartLayerDomain.h"
#include "vtkQtChartQuad.h"
#include "vtkQtChartSeriesDomainGroup.h"
#include "vtkQtChartSeriesDomain.h"
#include "vtkQtChartSeriesModel.h"
#include "vtkQtChartSeriesOptions.h"
#include "vtkQtChartSeriesSelection.h"
#include "vtkQtChartSeriesSelectionModel.h"
#include "vtkQtChartShapeLocator.h"
#include "vtkQtChartStyleBoolean.h"
#include "vtkQtChartStyleBrush.h"
#include "vtkQtChartStyleManager.h"
#include "vtkQtChartStyleMarker.h"
#include "vtkQtChartStyleSize.h"
#include "vtkQtPointMarker.h"
#include "vtkQtStatisticalBoxChartOptions.h"

#include <QBrush>
#include <QGraphicsScene>
#include <QList>
#include <QPen>
#include <QPointF>
#include <QPolygonF>
#include <QRectF>
#include <QStyleOptionGraphicsItem>


class vtkQtStatisticalBoxChartSeries
{
public:
  vtkQtStatisticalBoxChartSeries();
  ~vtkQtStatisticalBoxChartSeries();

  void updateSeries(int series);

public:
  QRectF Box;
  QPointF LowPoint;
  QPointF MedianPoint;
  QPointF HighPoint;
  QPolygonF Outliers;
  vtkQtPointMarker Marker;
  QList<vtkQtChartShape *> Shapes;
  QList<int> Highlights;
  bool Highlighted;
};


class vtkQtStatisticalBoxChartSeriesGroup
{
public:
  vtkQtStatisticalBoxChartSeriesGroup();
  ~vtkQtStatisticalBoxChartSeriesGroup() {}

  void sortSeries();

public:
  QList<QList<vtkQtChartShape *> > Shapes;
};


class vtkQtStatisticalBoxChartDomainGroup : public vtkQtChartSeriesDomainGroup
{
public:
  vtkQtStatisticalBoxChartDomainGroup();
  virtual ~vtkQtStatisticalBoxChartDomainGroup();

  virtual void clear();

protected:
  virtual void insertGroup(int group);
  virtual void removeGroup(int group);

public:
  QList<vtkQtStatisticalBoxChartSeriesGroup *> Tables;
};


class vtkQtStatisticalBoxChartInternal
{
public:
  vtkQtStatisticalBoxChartInternal();
  ~vtkQtStatisticalBoxChartInternal();

  void setPointQuad(vtkQtChartShape *quad, const QPointF &point,
      const QSizeF &size, float width);
  void setPointBar(vtkQtChartShape *bar, const QPointF &point,
      const QSizeF &size, float width);

  void clearSearchTree(int seriesGroup);

public:
  QList<vtkQtStatisticalBoxChartSeries *> Series;
  vtkQtChartAxisCornerDomain Domain;
  vtkQtStatisticalBoxChartDomainGroup Groups;
  vtkQtChartShapeLocator ShapeTree;
  QRectF Bounds;
  int CurrentGroup;
};


//-----------------------------------------------------------------------------
vtkQtStatisticalBoxChartSeries::vtkQtStatisticalBoxChartSeries()
  : Box(), LowPoint(), MedianPoint(), HighPoint(), Outliers(),
    Marker(QSizeF(5.0, 5.0)), Shapes(), Highlights()
{
  this->Highlighted = false;
}

vtkQtStatisticalBoxChartSeries::~vtkQtStatisticalBoxChartSeries()
{
  QList<vtkQtChartShape *>::Iterator iter = this->Shapes.begin();
  for( ; iter != this->Shapes.end(); ++iter)
    {
    delete *iter;
    }
}

void vtkQtStatisticalBoxChartSeries::updateSeries(int series)
{
  QList<vtkQtChartShape *>::Iterator iter = this->Shapes.begin();
  for( ; iter != this->Shapes.end(); ++iter)
    {
    (*iter)->setSeries(series);
    }
}


//-----------------------------------------------------------------------------
vtkQtStatisticalBoxChartSeriesGroup::vtkQtStatisticalBoxChartSeriesGroup()
  : Shapes()
{
}

void vtkQtStatisticalBoxChartSeriesGroup::sortSeries()
{
  QList<QList<vtkQtChartShape *> >::Iterator iter = this->Shapes.begin();
  for(int i = 0; iter != this->Shapes.end(); ++iter, ++i)
    {
    vtkQtChartShapeLocator::sort(*iter);
    }
}


//-----------------------------------------------------------------------------
vtkQtStatisticalBoxChartDomainGroup::vtkQtStatisticalBoxChartDomainGroup()
  : vtkQtChartSeriesDomainGroup(), Tables()
{
}

vtkQtStatisticalBoxChartDomainGroup::~vtkQtStatisticalBoxChartDomainGroup()
{
  QList<vtkQtStatisticalBoxChartSeriesGroup *>::Iterator iter;
  for(iter = this->Tables.begin(); iter != this->Tables.end(); ++iter)
    {
    delete *iter;
    }
}

void vtkQtStatisticalBoxChartDomainGroup::clear()
{
  vtkQtChartSeriesDomainGroup::clear();
  QList<vtkQtStatisticalBoxChartSeriesGroup *>::Iterator iter;
  for(iter = this->Tables.begin(); iter != this->Tables.end(); ++iter)
    {
    delete *iter;
    }

  this->Tables.clear();
}

void vtkQtStatisticalBoxChartDomainGroup::insertGroup(int group)
{
  vtkQtChartSeriesDomainGroup::insertGroup(group);
  this->Tables.insert(group, new vtkQtStatisticalBoxChartSeriesGroup());
}

void vtkQtStatisticalBoxChartDomainGroup::removeGroup(int group)
{
  vtkQtChartSeriesDomainGroup::removeGroup(group);
  delete this->Tables.takeAt(group);
}


//-----------------------------------------------------------------------------
vtkQtStatisticalBoxChartInternal::vtkQtStatisticalBoxChartInternal()
  : Series(), Domain(), Groups(), ShapeTree(), Bounds()
{
  this->CurrentGroup = -1;
  this->Domain.setHorizontalPreferences(false, false, true);
  this->Domain.setVerticalPreferences(true, false, false);
}

vtkQtStatisticalBoxChartInternal::~vtkQtStatisticalBoxChartInternal()
{
  QList<vtkQtStatisticalBoxChartSeries *>::Iterator iter;
  for(iter = this->Series.begin(); iter != this->Series.end(); ++iter)
    {
    delete *iter;
    }
}

void vtkQtStatisticalBoxChartInternal::setPointQuad(vtkQtChartShape *quad,
    const QPointF &point, const QSizeF &size, float width)
{
  float halfPen = width * 0.5;
  float halfWidth = size.width() * 0.5;
  float halfHeight = size.height() * 0.5;
  QPolygonF polygon;
  polygon.append(QPointF(point.x() - halfWidth - halfPen, point.y()));
  polygon.append(QPointF(point.x(), point.y() - halfHeight - halfPen));
  polygon.append(QPointF(point.x() + halfWidth + halfPen, point.y()));
  polygon.append(QPointF(point.x(), point.y() + halfHeight + halfPen));
  quad->setPolygon(polygon);
}

void vtkQtStatisticalBoxChartInternal::setPointBar(vtkQtChartShape *bar,
    const QPointF &point, const QSizeF &size, float width)
{
  bar->setRectangle(QRectF(point.x() - ((size.width() + width) * 0.5),
      point.y() - ((size.height() + width) * 0.5), size.width() + width,
      size.height() + width));
}

void vtkQtStatisticalBoxChartInternal::clearSearchTree(int seriesGroup)
{
  // Clear the shape tree if this is the displayed group.
  if(seriesGroup == this->CurrentGroup)
    {
    this->ShapeTree.clear();
    this->CurrentGroup = -1;
    }
}


//-----------------------------------------------------------------------------
vtkQtStatisticalBoxChart::vtkQtStatisticalBoxChart()
  : vtkQtChartSeriesLayer(false)
{
  this->Internal = new vtkQtStatisticalBoxChartInternal();
  this->Options = new vtkQtStatisticalBoxChartOptions(this);
  this->InModelChange = false;
  this->BuildNeeded = false;

  // Listen for option changes.
  this->connect(this->Options, SIGNAL(axesCornerChanged()), this,
      SLOT(handleAxesCornerChange()));
  this->connect(this->Options, SIGNAL(boxFractionChanged()), this,
      SIGNAL(layoutNeeded()));
  this->connect(this->Options, SIGNAL(outlineStyleChanged()), this,
      SLOT(handleOutlineChange()));

  // Listen for selection changes.
  this->connect(this->Selection,
      SIGNAL(selectionChanged(const vtkQtChartSeriesSelection &)),
      this, SLOT(updateHighlights()));
  }

vtkQtStatisticalBoxChart::~vtkQtStatisticalBoxChart()
{
  delete this->Internal;
}

void vtkQtStatisticalBoxChart::setChartArea(vtkQtChartArea *area)
{
  vtkQtChartSeriesLayer::setChartArea(area);
  this->reset();
}

void vtkQtStatisticalBoxChart::setModel(vtkQtChartSeriesModel *model)
{
  if (this->Model)
    {
    // Disconnect from the previous model's signals.
    this->disconnect(this->Model, 0, this, 0);
    }

  vtkQtChartSeriesLayer::setModel(model);
  if (this->Model)
    {
    // Listen for model changes.
    this->connect(this->Model, SIGNAL(modelReset()), this, SLOT(reset()));
    this->connect(this->Model, SIGNAL(seriesAboutToBeInserted(int, int)),
        this, SLOT(prepareSeriesInsert(int, int)));
    this->connect(this->Model, SIGNAL(seriesInserted(int, int)),
        this, SLOT(insertSeries(int, int)));
    this->connect(this->Model, SIGNAL(seriesAboutToBeRemoved(int, int)),
        this, SLOT(startSeriesRemoval(int, int)));
    this->connect(this->Model, SIGNAL(seriesRemoved(int, int)),
        this, SLOT(finishSeriesRemoval(int, int)));
    }

  // Reset the view items for the new model.
  this->reset();
}

void vtkQtStatisticalBoxChart::setOptions(
    const vtkQtStatisticalBoxChartOptions &options)
{
  // Copy the new options. The chart will collapse the layout signals.
  this->Options->setAxesCorner(options.getAxesCorner());
  this->Options->setBoxWidthFraction(options.getBoxWidthFraction());
  this->Options->setOutlineStyle(options.getOutlineStyle());
}

QPixmap vtkQtStatisticalBoxChart::getSeriesIcon(int series) const
{
  // Fill in the pixmap background.
  QPixmap icon(16, 16);
  icon.fill(QColor(255, 255, 255, 0));

  // Get the options for the series.
  vtkQtChartSeriesOptions *options =
      this->getSeriesOptions(series);
  if(options)
    {
    // Fill a box with the series color.
    QPainter painter(&icon);
    QPen pen(Qt::black);
    if(this->Options->getOutlineStyle() == vtkQtStatisticalBoxChartOptions::Darker)
      {
      pen = options->getBrush().color().dark();
      }
    painter.setPen(pen);
    painter.setBrush(options->getBrush());
    painter.drawRect(3, 3, 10, 10);
    }

  return icon;
}

void vtkQtStatisticalBoxChart::getLayerDomain(vtkQtChartLayerDomain &domain) const
{
  domain.mergeDomain(this->Internal->Domain, this->Options->getAxesCorner());
}

void vtkQtStatisticalBoxChart::layoutChart(const QRectF &area)
{
  // Update the position and bounds.
  this->prepareGeometryChange();
  this->Internal->Bounds.setSize(area.size());
  this->setPos(area.topLeft());
  if(this->Internal->Series.size() == 0)
    {
    return;
    }

  // Get the axis layer to get the axes and domain priority.
  vtkQtChartAxisLayer *layer = this->ChartArea->getAxisLayer();
  vtkQtChartAxis *xAxis =
      layer->getHorizontalAxis(this->Options->getAxesCorner());
  vtkQtChartAxis *yAxis =
      layer->getVerticalAxis(this->Options->getAxesCorner());

  // Use the domain to find the minimum space between bars.
  int i = 0;
  int domainIndex = -1;
  float minDistance = 0;
  bool isRange = false;
  QList<QVariant> domain;
  const vtkQtChartSeriesDomain *seriesDomain =
      this->Internal->Domain.getDomain(xAxis->getAxisDomain(),
      yAxis->getAxisDomain(), &domainIndex);
  if(seriesDomain)
    {
    domain = seriesDomain->getXDomain().getDomain(isRange);
    for( ; i < domain.size() - 1; i++)
      {
      float distance = qAbs<float>(xAxis->getPixel(domain[i + 1]) -
          xAxis->getPixel(domain[i]));
      if(i == 0 || distance < minDistance)
        {
        minDistance = distance;
        }
      }
    }

  // Use the width fractions to determine the actual bar width.
  minDistance *= this->Options->getBoxWidthFraction();
  float boxWidth = minDistance;

  // Get the list of series in the selected domain.
  QList<int> seriesList;
  if(seriesDomain)
    {
    seriesList = this->Internal->Groups.getGroup(domainIndex);
    }

  if(boxWidth < 1)
    {
    boxWidth = 1;
    }

  // Position and size the box series. Skip the series if it is
  // invisible or invalid for the domain.
  float halfWidth = boxWidth * 0.5;
  vtkQtChartSeriesOptions *options = 0;
  QList<int>::Iterator iter = seriesList.begin();
  for( ; iter != seriesList.end(); ++iter)
    {
    vtkQtStatisticalBoxChartSeries *series = this->Internal->Series[*iter];
    int total = this->Model->getNumberOfSeriesValues(*iter);
    if(total >= 5)
      {
      float px = xAxis->getPixel(this->Model->getSeriesName(*iter));
      float left = px - halfWidth;

      // Minimum: 0
      // Lower Quartile: 1
      // Median: 2
      // Upper Quartile: 3
      // Maximum: 4
      float min = yAxis->getPixel(this->Model->getSeriesValue(*iter, 0, 1));
      float lower = yAxis->getPixel(this->Model->getSeriesValue(*iter, 1, 1));
      float median = yAxis->getPixel(this->Model->getSeriesValue(*iter, 2, 1));
      float upper = yAxis->getPixel(this->Model->getSeriesValue(*iter, 3, 1));
      float max = yAxis->getPixel(this->Model->getSeriesValue(*iter, 4, 1));

      // Set the box size.
      series->Box.setRect(left, upper, boxWidth, lower - upper);

      // Set up the box for the search tree.
      options = this->getSeriesOptions(*iter);
      float penWidth = options->getPen().widthF();
      if(penWidth == 0.0)
        {
        penWidth = 1.0;
        }

      float halfPen = penWidth * 0.5;
      series->Shapes[0]->setRectangle(series->Box.adjusted(
          -halfPen, -halfPen, halfPen, halfPen));

      // Set the median point.
      series->MedianPoint.setX(px);
      series->MedianPoint.setY(median);

      // Set the low whisker point.
      series->LowPoint.setX(px);
      series->LowPoint.setY(min);

      // Set the high whisker point.
      series->HighPoint.setX(px);
      series->HighPoint.setY(max);

      // Add in the outliers.
      QPointF point;
      series->Outliers.clear();
      bool useQuad = options->getMarkerStyle() == vtkQtPointMarker::Diamond ||
          options->getMarkerStyle() == vtkQtPointMarker::Plus;
      for(int j = 5; j < total; j++)
        {
        float py = yAxis->getPixel(this->Model->getSeriesValue(*iter, j, 1));
        point = QPointF(px, py);
        series->Outliers.append(point);
        if(useQuad)
          {
          this->Internal->setPointQuad(series->Shapes[j - 4], point,
              options->getMarkerSize(), penWidth);
          }
        else
          {
          this->Internal->setPointBar(series->Shapes[j - 4], point,
              options->getMarkerSize(), penWidth);
          }
        }
      }
    }

  // Update the search tree.
  if(seriesDomain)
    {
    if(this->ChartArea->isInteractivelyResizing())
      {
      this->BuildNeeded = true;
      }
    else
      {
      this->buildShapeTree(domainIndex);
      }
    }
}

bool vtkQtStatisticalBoxChart::getHelpText(const QPointF &point, QString &text)
{
  // Translate the point to contents coordinates.
  QPointF local = point;
  this->ChartArea->getContentsSpace()->translateToLayerContents(local);

  // Get the selected shapes from the tree.
  vtkQtChartIndexRangeList indexes;
  QList<vtkQtChartShape *> shapes =
      this->Internal->ShapeTree.getItemsAt(local);
  if(shapes.size() > 0)
    {
    // Use the axis options to format the data.
    vtkQtChartAxisLayer *layer = this->ChartArea->getAxisLayer();
    vtkQtChartAxisOptions *yAxis = layer->getVerticalAxis(
        this->Options->getAxesCorner())->getOptions();

    // Get the data from the model. If the index is -1, the shape is
    // for the series box.
    QStringList args;
    int series = shapes.first()->getSeries();
    int index = shapes.first()->getIndex();
    if(index == -1)
      {
      args.append(yAxis->formatValue(
          this->Model->getSeriesValue(series, 1, 1)));
      args.append(yAxis->formatValue(
          this->Model->getSeriesValue(series, 2, 1)));
      args.append(yAxis->formatValue(
          this->Model->getSeriesValue(series, 3, 1)));
      text = this->Options->getHelpFormat()->getHelpText(
          this->Model->getSeriesName(series).toString(), args);
      }
    else
      {
      args.append(yAxis->formatValue(
          this->Model->getSeriesValue(series, index + 5, 1)));
      text = this->Options->getOutlierFormat()->getHelpText(
          this->Model->getSeriesName(series).toString(), args);
      }

    return true;
    }

  return false;
}

void vtkQtStatisticalBoxChart::finishInteractiveResize()
{
  if(this->BuildNeeded)
    {
    // Get the axis layer to get the axes and domains.
    vtkQtChartAxisLayer *layer = this->ChartArea->getAxisLayer();
    vtkQtChartAxis *xAxis = layer->getHorizontalAxis(
        this->Options->getAxesCorner());
    vtkQtChartAxis *yAxis = layer->getVerticalAxis(
        this->Options->getAxesCorner());

    int seriesGroup;
    const vtkQtChartSeriesDomain *seriesDomain =
        this->Internal->Domain.getDomain(xAxis->getAxisDomain(),
        yAxis->getAxisDomain(), &seriesGroup);
    if(seriesDomain)
      {
      this->buildShapeTree(seriesGroup);
      }
    }
}

void vtkQtStatisticalBoxChart::getSeriesAt(const QPointF &point,
    vtkQtChartSeriesSelection &selection) const
{
  // Translate the point to contents coordinates.
  QPointF local = point;
  this->ChartArea->getContentsSpace()->translateToLayerContents(local);

  // Get the selected series from the tree.
  vtkQtChartIndexRangeList indexes;
  QList<vtkQtChartShape *> shapes =
      this->Internal->ShapeTree.getItemsAt(local);
  QList<vtkQtChartShape *>::Iterator iter = shapes.begin();
  for( ; iter != shapes.end(); ++iter)
    {
    // Add the series to the selection.
    int series = (*iter)->getSeries();
    indexes.addRange(series, series);
    }

  selection.setSeries(indexes);
}

void vtkQtStatisticalBoxChart::getPointsAt(const QPointF &point,
    vtkQtChartSeriesSelection &selection) const
{
  // Translate the point to contents coordinates.
  QPointF local = point;
  this->ChartArea->getContentsSpace()->translateToLayerContents(local);

  // Get the selected outliers from the tree.
  selection.clear();
  QList<vtkQtChartShape *> shapes =
      this->Internal->ShapeTree.getItemsAt(local);
  QList<vtkQtChartShape *>::Iterator iter = shapes.begin();
  for( ; iter != shapes.end(); ++iter)
    {
    int index = (*iter)->getIndex();
    if(index != -1)
      {
      selection.addPoints((*iter)->getSeries(),
          vtkQtChartIndexRangeList(index, index));
      }
    }
}

void vtkQtStatisticalBoxChart::getSeriesIn(const QRectF &area,
    vtkQtChartSeriesSelection &selection) const
{
  // Translate the rectangle to contents coordinates.
  QRectF local = area;
  this->ChartArea->getContentsSpace()->translateToLayerContents(local);

  // Get the selected series from the tree.
  vtkQtChartIndexRangeList indexes;
  QList<vtkQtChartShape *> shapes =
      this->Internal->ShapeTree.getItemsIn(local);
  QList<vtkQtChartShape *>::Iterator iter = shapes.begin();
  for( ; iter != shapes.end(); ++iter)
    {
    // Add the series to the selection.
    int series = (*iter)->getSeries();
    indexes.addRange(series, series);
    }

  selection.setSeries(indexes);
}

void vtkQtStatisticalBoxChart::getPointsIn(const QRectF &area,
    vtkQtChartSeriesSelection &selection) const
{
  // Translate the rectangle to contents coordinates.
  QRectF local = area;
  this->ChartArea->getContentsSpace()->translateToLayerContents(local);

  // Get the selected outliers from the tree.
  selection.clear();
  QList<vtkQtChartShape *> shapes =
      this->Internal->ShapeTree.getItemsIn(local);
  QList<vtkQtChartShape *>::Iterator iter = shapes.begin();
  for( ; iter != shapes.end(); ++iter)
    {
    int index = (*iter)->getIndex();
    if(index != -1)
      {
      selection.addPoints((*iter)->getSeries(),
          vtkQtChartIndexRangeList(index, index));
      }
    }
}

QRectF vtkQtStatisticalBoxChart::boundingRect() const
{
  return this->Internal->Bounds;
}

void vtkQtStatisticalBoxChart::paint(QPainter *painter,
    const QStyleOptionGraphicsItem *option, QWidget *)
{
  if(!this->ChartArea)
    {
    return;
    }

  // Use the exposed rectangle from the option object to determine
  // which series to draw.
  vtkQtChartContentsSpace *space = this->ChartArea->getContentsSpace();

  // Get the axis layer to get the axes and domain priority.
  vtkQtChartAxisLayer *layer = this->ChartArea->getAxisLayer();
  vtkQtChartLayer::AxesCorner corner = this->Options->getAxesCorner();
  vtkQtChartAxis *xAxis = layer->getHorizontalAxis(corner);
  vtkQtChartAxis *yAxis = layer->getVerticalAxis(corner);

  int domainIndex = -1;
  const vtkQtChartSeriesDomain *seriesDomain =
      this->Internal->Domain.getDomain(xAxis->getAxisDomain(),
      yAxis->getAxisDomain(), &domainIndex);
  if(seriesDomain)
    {
    // Set up the painter clipping and offset for panning.
    painter->setClipRect(this->Internal->Bounds);
    painter->translate(-space->getXOffset(), -space->getYOffset());

    // Get the list of series in the selected domain.
    vtkQtStatisticalBoxChartSeries *series = 0;
    vtkQtChartSeriesOptions *options = 0;
    QList<int> seriesList = this->Internal->Groups.getGroup(domainIndex);
    QList<int>::Iterator iter = seriesList.begin();
    for( ; iter != seriesList.end(); ++iter)
      {
      // Set up the painter for the series.
      series = this->Internal->Series[*iter];
      options = this->getSeriesOptions(*iter);
      QColor light = vtkQtChartColors::lighter(options->getBrush().color());
      QPen seriesPen = options->getPen();
      if (this->Options->getOutlineStyle() == vtkQtStatisticalBoxChartOptions::Darker)
        {
        seriesPen.setColor(options->getBrush().color().dark());
        }
      else
        {
        seriesPen.setColor(Qt::black);
        }
      painter->setPen(seriesPen);
      if(series->Highlighted)
        {
        painter->setBrush(light);
        }
      else
        {
        painter->setBrush(options->getBrush());
        }

      QPen widePen;
      if(series->Highlighted || !series->Highlights.isEmpty())
        {
        widePen = seriesPen;
        widePen.setWidthF(widePen.widthF() + 3.0);
        }

      // First, draw the wisker lines.
      painter->drawLine(series->HighPoint, series->LowPoint);
      painter->drawLine(QPointF(series->Box.left(), series->HighPoint.y()),
          QPointF(series->Box.right(), series->HighPoint.y()));
      painter->drawLine(QPointF(series->Box.left(), series->LowPoint.y()),
          QPointF(series->Box.right(), series->LowPoint.y()));

      // Next, draw the box on top of the wiskers.
      painter->drawRect(series->Box);

      // Then, draw the median line.
      painter->drawLine(QPointF(series->Box.left(), series->MedianPoint.y()),
          QPointF(series->Box.right(), series->MedianPoint.y()));

      // Finally, draw the outlier points.
      QPolygonF::Iterator point = series->Outliers.begin();
      for(int j = 0; point != series->Outliers.end(); ++point, ++j)
        {
        // Translate the painter to the point.
        painter->save();
        painter->translate(*point);

        if(!series->Highlighted && series->Highlights.contains(j))
          {
          painter->setPen(widePen);
          series->Marker.paint(painter);

          painter->setPen(seriesPen);
          painter->setBrush(light);
          }

        series->Marker.paint(painter);

        // Restore the painter for the next point.
        painter->restore();
        }
      }
    }
}

void vtkQtStatisticalBoxChart::reset()
{
  // Make sure the selection model is notified of the change.
  this->InModelChange = true;
  this->Selection->beginModelReset();

  // Clean up the old view items.
  bool needsLayout = this->Internal->Series.size() > 0;
  QList<vtkQtStatisticalBoxChartSeries *>::Iterator iter =
      this->Internal->Series.begin();
  for( ; iter != this->Internal->Series.end(); ++iter)
    {
    delete *iter;
    }

  this->Internal->Series.clear();
  this->Internal->Domain.clear();
  this->Internal->Groups.clear();

  // Add items for the new model.
  if(this->Model && this->ChartArea)
    {
    int total = this->Model->getNumberOfSeries();
    if(total > 0)
      {
      if(needsLayout)
        {
        needsLayout = false;
        emit this->rangeChanged();
        }

      this->insertSeries(0, total - 1);
      }
    }

  if(needsLayout)
    {
    emit this->rangeChanged();
    emit this->layoutNeeded();
    }

  // Notify the slection model that the reset is complete, which may
  // generate a selection changed signal.
  this->Selection->endModelReset();
  this->InModelChange = false;
}

void vtkQtStatisticalBoxChart::prepareSeriesInsert(int first, int last)
{
  if(this->ChartArea)
    {
    // Notify the selection model of the change. The selection will be
    // adjusted for the changes in this call so it can be layed out
    // when the changes are completed.
    this->InModelChange = true;
    this->Selection->beginInsertSeries(first, last);
    }
}

void vtkQtStatisticalBoxChart::insertSeries(int first, int last)
{
  if(this->ChartArea)
    {
    // Update the series indexes stored in the domain groups.
    this->Internal->Groups.prepareInsert(first, last);

    int i = first;
    QList<int> groups;
    bool signalDomain = false;
    vtkQtStatisticalBoxChartSeries *series = 0;
    vtkQtChartSeriesOptions *options = 0;
    for( ; i <= last; i++)
      {
      // Add an item for each series.
      series = new vtkQtStatisticalBoxChartSeries();
      this->Internal->Series.insert(i, series);

      // Get the series options.
      options = this->getSeriesOptions(i);
      this->setupOptions(options);

      // Set the drawing options for the point marker.
      series->Marker.setSize(options->getMarkerSize());
      series->Marker.setStyle(options->getMarkerStyle());

      // Add shape items for the series.
      series->Shapes.append(new vtkQtChartBar(i, -1));
      bool useQuad = options->getMarkerStyle() == vtkQtPointMarker::Diamond ||
          options->getMarkerStyle() == vtkQtPointMarker::Plus;
      int outliers = this->Model->getNumberOfSeriesValues(i) - 5;
      for(int j = 0; j < outliers; j++)
        {
        if(useQuad)
          {
          series->Shapes.append(new vtkQtChartQuad(i, j));
          }
        else
          {
          series->Shapes.append(new vtkQtChartBar(i, j));
          }
        }

      // Add the series domains to the chart domains.
      if(options->isVisible())
        {
        int seriesGroup = -1;
        if(this->addSeriesDomain(i, seriesGroup))
          {
          signalDomain = true;
          }

        // Keep track of the series groups that need new shape tables.
        if(!groups.contains(seriesGroup))
          {
          groups.append(seriesGroup);
          }
        }
      }

    this->Internal->Groups.finishInsert();

    // Fix the series indexes in the search lists.
    for(i = last + 1; i < this->Internal->Series.size(); i++)
      {
      this->Internal->Series[i]->updateSeries(i);
      }

    // Create the search table for the modified domains.
    QList<int>::Iterator iter = groups.begin();
    for( ; iter != groups.end(); ++iter)
      {
      this->createShapeTable(*iter);
      }

    if(signalDomain)
      {
      emit this->rangeChanged();
      }

    emit this->layoutNeeded();

    // Close the event for the selection model, which will trigger a
    // selection change signal.
    this->Selection->endInsertSeries(first, last);
    this->InModelChange = false;
    }
}

void vtkQtStatisticalBoxChart::startSeriesRemoval(int first, int last)
{
  if(this->ChartArea)
    {
    // Notify the selection model of the change. The selection will be
    // adjusted for the changes in this call so it can be layed out
    // when the changes are completed.
    this->InModelChange = true;
    this->Selection->beginRemoveSeries(first, last);

    // Remove each of the series items.
    for( ; last >= first; last--)
      {
      vtkQtChartSeriesOptions *options = this->getSeriesOptions(last);
      this->cleanupOptions(options);
      delete this->Internal->Series.takeAt(last);
      }

    // Fix the series indexes in the search lists.
    for( ; first < this->Internal->Series.size(); first++)
      {
      this->Internal->Series[first]->updateSeries(first);
      }
    }
}

void vtkQtStatisticalBoxChart::finishSeriesRemoval(int first, int last)
{
  if(this->ChartArea)
    {
    // Find which groups need to be re-calculated
    QList<int> groups;
    QList<int>::Iterator iter;
    for(int i = first; i <= last; i++)
      {
      int index = this->Internal->Groups.removeSeries(i);
      if(index != -1)
        {
        // Add the group indexes in reverse order.
        bool doAdd = true;
        for(iter = groups.begin(); iter != groups.end(); ++iter)
          {
          if(index > *iter)
            {
            doAdd = false;
            groups.insert(iter, index);
            break;
            }
          else if(index == *iter)
            {
            doAdd = false;
            break;
            }
          }

        if(doAdd)
          {
          groups.append(index);
          }
        }
      }

    for(iter = groups.begin(); iter != groups.end(); ++iter)
      {
      if(this->Internal->Groups.getNumberOfSeries(*iter) == 0)
        {
        // Remove the empty domain.
        this->Internal->Domain.removeDomain(*iter);
        }
      else
        {
        // Re-calculate the chart domain.
        this->calculateDomain(*iter);
        this->createShapeTable(*iter);
        }
      }

    // Fix the stored indexes in the domain groups.
    this->Internal->Groups.finishRemoval(first, last);
    if(groups.size() > 0)
      {
      emit this->rangeChanged();
      emit this->layoutNeeded();
      }

    // Close the event for the selection model, which will trigger a
    // selection change signal.
    this->Selection->endRemoveSeries(first, last);
    this->InModelChange = false;
    }
}

void vtkQtStatisticalBoxChart::handleAxesCornerChange()
{
  if(this->Model && this->ChartArea)
    {
    emit this->rangeChanged();
    emit this->layoutNeeded();
    }
}

void vtkQtStatisticalBoxChart::handleOutlineChange()
{
  // Change the bar outline.
  if (this->Model && this->ChartArea)
    {
    int total = this->Model->getNumberOfSeries();
    emit this->modelSeriesChanged(0, total-1);
    this->update();
    }
}

void vtkQtStatisticalBoxChart::handleOptionsChanged(
  vtkQtChartSeriesOptions* options,
  int ltype, const QVariant& newvalue, const QVariant& oldvalue)
{
  if (ltype == vtkQtChartSeriesOptions::VISIBLE)
    {
    bool visible = options->isVisible();
    // visibility has changed.
    this->handleSeriesVisibilityChange(options, visible);
    }

  if (ltype == vtkQtChartSeriesOptions::MARKER_STYLE)
    {
    this->handleSeriesPointMarkerChanged(options);
    }
  // TODO: Update the series rectangle.

  this->vtkQtChartSeriesLayer::handleOptionsChanged(options, ltype, newvalue,
    oldvalue);
}

void vtkQtStatisticalBoxChart::handleSeriesVisibilityChange(
  vtkQtChartSeriesOptions* options, bool visible)
{
  // Get the series index from the options index.
  int series = this->getSeriesOptionsIndex(options);
  if(series >= 0 && series < this->Internal->Series.size())
    {
    if(visible)
      {
      // If the series is going to be visible, add to the domain.
      int seriesGroup = -1;
      bool signalDomain = this->addSeriesDomain(series, seriesGroup);
      this->Internal->Groups.finishInsert();
      this->createShapeTable(seriesGroup);
      if(signalDomain)
        {
        emit this->rangeChanged();
        }

      emit this->layoutNeeded();
      }
    else
      {
      int seriesGroup = this->Internal->Groups.removeSeries(series);
      if(seriesGroup != -1)
        {
        // If the group is empty, remove the domain.
        if(this->Internal->Groups.getNumberOfSeries(seriesGroup) == 0)
          {
          this->Internal->Domain.removeDomain(seriesGroup);
          }
        else
          {
          // Re-calculate the domain.
          this->calculateDomain(seriesGroup);
          this->createShapeTable(seriesGroup);
          }

        this->Internal->Groups.finishRemoval();
        emit this->rangeChanged();
        emit this->layoutNeeded();
        }
      }
    }
}

void vtkQtStatisticalBoxChart::handleSeriesPointMarkerChanged(
  vtkQtChartSeriesOptions* options)
{
  // Get the series index from the options index.
  int series = this->getSeriesOptionsIndex(options);
  if(series >= 0 && series < this->Internal->Series.size())
    {
    vtkQtStatisticalBoxChartSeries *item = this->Internal->Series[series];
    vtkQtPointMarker::MarkerStyle oldStyle = item->Marker.getStyle();
    vtkQtPointMarker::MarkerStyle newStyle = options->getMarkerStyle();
    item->Marker.setStyle(newStyle);
    item->Marker.setSize(options->getMarkerSize());

    // See if the search points need to be changed. If the shapes are
    // the same or there are no points, no change is needed.
    bool useQuads = newStyle == vtkQtPointMarker::Diamond ||
        newStyle == vtkQtPointMarker::Plus;
    bool hasQuads = oldStyle == vtkQtPointMarker::Diamond ||
        oldStyle == vtkQtPointMarker::Plus;
    if(useQuads != hasQuads && item->Shapes.size() > 1)
      {
      // Clear the search tree and table before deleting shapes.
      int seriesGroup = this->Internal->Groups.findGroup(series);
      if(seriesGroup == this->Internal->CurrentGroup)
        {
        this->Internal->ShapeTree.clear();
        this->Internal->CurrentGroup = -1;
        }

      this->Internal->Groups.Tables[seriesGroup]->Shapes.clear();

      // Replace the old shapes with the new ones.
      QList<vtkQtChartShape *>::Iterator iter = item->Shapes.begin();
      ++iter;
      for(int i = 0; iter != item->Shapes.end(); ++iter, ++i)
        {
        delete *iter;
        if(useQuads)
          {
          *iter = new vtkQtChartQuad(series, i);
          }
        else
          {
          *iter = new vtkQtChartBar(series, i);
          }
        }

      // Build a new table for the series group.
      this->createShapeTable(seriesGroup);
      }

    emit this->layoutNeeded();
    }
}

void vtkQtStatisticalBoxChart::updateHighlights()
{
  if(!this->InModelChange && this->ChartArea)
    {
    // Remove the current selection.
    QList<vtkQtStatisticalBoxChartSeries *>::Iterator iter =
        this->Internal->Series.begin();
    for( ; iter != this->Internal->Series.end(); ++iter)
      {
      (*iter)->Highlighted = false;
      (*iter)->Highlights.clear();
      }

    // Get the current selection from the selection model.
    if(!this->Selection->isSelectionEmpty())
      {
      const vtkQtChartSeriesSelection &current =
          this->Selection->getSelection();
      if(current.getType() == vtkQtChartSeriesSelection::SeriesSelection)
        {
        const vtkQtChartIndexRangeList &series = current.getSeries();
        vtkQtChartIndexRange *range = series.getFirst();
        while(range)
          {
          for(int i = range->getFirst(); i <= range->getSecond(); i++)
            {
            this->Internal->Series[i]->Highlighted = true;
            }

          range = series.getNext(range);
          }
        }
      else if(current.getType() == vtkQtChartSeriesSelection::PointSelection)
        {
        const QMap<int, vtkQtChartIndexRangeList> &points =
            current.getPoints();
        vtkQtStatisticalBoxChartSeries *series = 0;
        QMap<int, vtkQtChartIndexRangeList>::ConstIterator jter;
        for(jter = points.begin(); jter != points.end(); ++jter)
          {
          series = this->Internal->Series[jter.key()];
          vtkQtChartIndexRange *range = jter->getFirst();
          while(range)
            {
            for(int i = range->getFirst(); i <= range->getSecond(); i++)
              {
              series->Highlights.append(i);
              }

            range = jter->getNext(range);
            }
          }
        }
      }

    // TODO: Repaint the modified area.
    this->update();
    }
}

bool vtkQtStatisticalBoxChart::addSeriesDomain(int series, int &seriesGroup)
{
  QList<QVariant> xDomain;
  xDomain.append(this->Model->getSeriesName(series));
  vtkQtChartSeriesDomain seriesDomain;
  seriesDomain.getXDomain().setDomain(xDomain);

  QList<QVariant> yDomain = this->Model->getSeriesRange(series, 1);
  if(yDomain.isEmpty())
    {
    int points = this->Model->getNumberOfSeriesValues(series);
    for(int j = 0; j < points; j++)
      {
      yDomain.append(this->Model->getSeriesValue(series, j, 1));
      }

    vtkQtChartAxisDomain::sort(yDomain);
    seriesDomain.getYDomain().setDomain(yDomain);
    }
  else
    {
    seriesDomain.getYDomain().setRange(yDomain);
    }

  bool changed = this->Internal->Domain.mergeDomain(seriesDomain, &seriesGroup);

  // Add the series index to the domain group.
  this->Internal->Groups.insertSeries(series, seriesGroup);
  return changed;
}

void vtkQtStatisticalBoxChart::calculateDomain(int seriesGroup)
{
  // Clear the current domain information.
  vtkQtChartSeriesDomain *domain =
      this->Internal->Domain.getDomain(seriesGroup);
  domain->getXDomain().clear();
  domain->getYDomain().clear();

  // Get the list of series in the group.
  vtkQtChartSeriesOptions *options = 0;
  QList<int> list = this->Internal->Groups.getGroup(seriesGroup);
  for(QList<int>::Iterator iter = list.begin(); iter != list.end(); ++iter)
    {
    options = this->getSeriesOptions(*iter);
    if(options && !options->isVisible())
      { 
      continue;
      }

    QList<QVariant> xDomain;
    xDomain.append(this->Model->getSeriesName(*iter));
    domain->getXDomain().mergeDomain(xDomain);

    QList<QVariant> yDomain = this->Model->getSeriesRange(*iter, 1);
    if(yDomain.isEmpty())
      {
      int points = this->Model->getNumberOfSeriesValues(*iter);
      for(int j = 0; j < points; j++)
        {
        yDomain.append(this->Model->getSeriesValue(*iter, j, 1));
        }

      vtkQtChartAxisDomain::sort(yDomain);
      domain->getYDomain().mergeDomain(yDomain);
      }
    else
      {
      domain->getYDomain().mergeRange(yDomain);
      }
    }
}

void vtkQtStatisticalBoxChart::createShapeTable(int seriesGroup)
{
  // Clear the shape tree if this is the displayed group.
  if(seriesGroup == this->Internal->CurrentGroup)
    {
    this->Internal->ShapeTree.clear();
    this->Internal->CurrentGroup = -1;
    }

  // Clear the current table.
  vtkQtStatisticalBoxChartSeriesGroup *agroup =
      this->Internal->Groups.Tables[seriesGroup];
  agroup->Shapes.clear();

  // Add the shapes to the table for the series in the group.
  QList<int> seriesList = this->Internal->Groups.getGroup(seriesGroup);
  QList<int>::Iterator iter = seriesList.begin();
  for( ; iter != seriesList.end(); ++iter)
    {
    agroup->Shapes.append(this->Internal->Series[*iter]->Shapes);
    }
}

void vtkQtStatisticalBoxChart::buildShapeTree(int seriesGroup)
{
  this->BuildNeeded = false;
  if(seriesGroup == this->Internal->CurrentGroup)
    {
    this->Internal->ShapeTree.update();
    }
  else
    {
    this->Internal->CurrentGroup = seriesGroup;
    vtkQtStatisticalBoxChartSeriesGroup *agroup =
        this->Internal->Groups.Tables[seriesGroup];

    // Sort the modified series lists.
    agroup->sortSeries();

    // Build the search tree from the table.
    this->Internal->ShapeTree.build(agroup->Shapes);
    }
}


void vtkQtStatisticalBoxChart::setupOptions(vtkQtChartSeriesOptions *options)
{
  this->vtkQtChartSeriesLayer::setupOptions(options);
  if (!this->ChartArea || !options)
    {
    return;
    }
  // Ensure the defaults for the options are set correctly.
  vtkQtChartStyleManager *manager = this->ChartArea->getStyleManager();
  int styleindex = manager->getStyleIndex (this, options);

  vtkQtChartStyleMarker *styleMarker = qobject_cast<vtkQtChartStyleMarker *>(
    manager->getGenerator("Marker Style"));
  options->setDefaultOption(vtkQtChartSeriesOptions::MARKER_STYLE,
    styleMarker? styleMarker->getStyleMarker(styleindex) :
    vtkQtPointMarker::Circle);
}

