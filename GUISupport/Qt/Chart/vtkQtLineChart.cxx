/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtLineChart.cxx

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

/// \file vtkQtLineChart.cxx
/// \date February 14, 2008

#include "vtkQtLineChart.h"

#include "vtkQtChartAxis.h"
#include "vtkQtChartAxisCornerDomain.h"
#include "vtkQtChartAxisDomain.h"
#include "vtkQtChartAxisLayer.h"
#include "vtkQtChartAxisOptions.h"
#include "vtkQtChartContentsArea.h"
#include "vtkQtChartContentsSpace.h"
#include "vtkQtChartHelpFormatter.h"
#include "vtkQtChartLayerDomain.h"
#include "vtkQtChartPointLocator.h"
#include "vtkQtChartSeriesDomain.h"
#include "vtkQtChartSeriesDomainGroup.h"
#include "vtkQtChartSeriesModel.h"
#include "vtkQtChartSeriesSelection.h"
#include "vtkQtChartSeriesSelectionModel.h"
#include "vtkQtChartArea.h"
#include "vtkQtLineChartOptions.h"
#include "vtkQtLineChartSeriesOptions.h"
#include "vtkQtPolylineItem.h"
#include "vtkQtPointMarker.h"
#include "vtkQtSimplePointLocator.h"

#include <QGraphicsScene>
#include <QList>
#include <QPair>


class vtkQtLineChartItem : public QGraphicsItem
{
public:
  enum {Type = vtkQtChart_LineChartItemType};

public:
  vtkQtLineChartItem(QGraphicsItem *parent=0);
  virtual ~vtkQtLineChartItem();

  virtual int type() const {return vtkQtLineChartItem::Type;}
  virtual QRectF boundingRect() const;
  virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
      QWidget *widget=0);

  vtkQtPolylineItem *Polyline;
  vtkQtPointMarker *Points;
  vtkQtChartPointLocator *Locator;
};


class vtkQtLineChartInternal
{
public:
  vtkQtLineChartInternal();
  ~vtkQtLineChartInternal() {}

  int getSeries(vtkQtPolylineItem *polyline) const;
  int getSeries(vtkQtPointMarker *marker) const;

  QList<vtkQtLineChartItem *> Series;
  QList<QPair<int, vtkQtLineChartItem *> > Highlights;
  QList<vtkQtPointMarker *> LightPoints;
  vtkQtChartAxisCornerDomain Domains[4];
  vtkQtChartSeriesDomainGroup Groups[4];
  vtkQtChartPointLocator *Locator;
  vtkQtChartPointLocator *DefaultLocator;
};


//-----------------------------------------------------------------------------
vtkQtLineChartItem::vtkQtLineChartItem(QGraphicsItem *item)
  : QGraphicsItem(item, item ? item->scene() : 0)
{
  this->Polyline = new vtkQtPolylineItem(this, this->scene());
  this->Points = new vtkQtPointMarker(QSizeF(5.0, 5.0),
      vtkQtPointMarker::Circle, this, this->scene());
  this->Locator = 0;

  this->Polyline->setZValue(1);
  this->Points->setZValue(2);
}

vtkQtLineChartItem::~vtkQtLineChartItem()
{
  if(this->Locator)
    {
    delete this->Locator;
    }
}

QRectF vtkQtLineChartItem::boundingRect() const
{
  return QRect(0, 0, 0, 0);
}

void vtkQtLineChartItem::paint(QPainter *,
    const QStyleOptionGraphicsItem *, QWidget *)
{
}


//-----------------------------------------------------------------------------
vtkQtLineChartInternal::vtkQtLineChartInternal()
  : Series(), Highlights(), LightPoints()
{
  this->Locator = 0;
  this->DefaultLocator = 0;
}

int vtkQtLineChartInternal::getSeries(vtkQtPolylineItem *polyline) const
{
  if(polyline)
    {
    QGraphicsItem *item = polyline->parentItem();
    vtkQtLineChartItem *series =
        qgraphicsitem_cast<vtkQtLineChartItem *>(item);
    if(series)
      {
      return this->Series.indexOf(series);
      }
    }

  return -1;
}

int vtkQtLineChartInternal::getSeries(vtkQtPointMarker *marker) const
{
  if(marker)
    {
    QGraphicsItem *item = marker->parentItem();
    vtkQtLineChartItem *series =
        qgraphicsitem_cast<vtkQtLineChartItem *>(item);
    if(series)
      {
      return this->Series.indexOf(series);
      }
    }

  return -1;
}


//-----------------------------------------------------------------------------
vtkQtLineChart::vtkQtLineChart()
  : vtkQtChartSeriesLayer()
{
  this->Internal = new vtkQtLineChartInternal();
  this->Options = new vtkQtLineChartOptions(this);
  this->InModelChange = false;

  // Set up the series point locator.
  this->Internal->DefaultLocator = new vtkQtSimplePointLocator(this);
  this->Internal->Locator = this->Internal->DefaultLocator;

  // Listen for selection changes.
  this->connect(this->Selection,
      SIGNAL(selectionChanged(const vtkQtChartSeriesSelection &)),
      this, SLOT(updateHighlights()));
}

vtkQtLineChart::~vtkQtLineChart()
{
  delete this->Internal;
}

void vtkQtLineChart::setChartArea(vtkQtChartArea *area)
{
  vtkQtChartSeriesLayer::setChartArea(area);
  this->reset();
}

void vtkQtLineChart::setModel(vtkQtChartSeriesModel *model)
{
  if(model == this->Model)
    {
    return;
    }

  if(this->Model)
    {
    // Disconnect from the previous model's signals.
    this->disconnect(this->Model, 0, this, 0);
    }

  vtkQtChartSeriesLayer::setModel(model);
  if(this->Model)
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

void vtkQtLineChart::setPointLocator(vtkQtChartPointLocator *locator)
{
  if(this->Internal->Locator != locator)
    {
    // Remove all the current locator instances.
    QList<vtkQtLineChartItem *>::Iterator iter =
        this->Internal->Series.begin();
    for( ; iter != this->Internal->Series.begin(); ++iter)
      {
      if((*iter)->Locator)
        {
        delete (*iter)->Locator;
        (*iter)->Locator = 0;
        }
      }

    this->Internal->Locator = locator;
    if(!this->Internal->Locator)
      {
      this->Internal->Locator = this->Internal->DefaultLocator;
      }

    // Add a new locator to each series item.
    iter = this->Internal->Series.begin();
    for( ; iter != this->Internal->Series.begin(); ++iter)
      {
      (*iter)->Locator = this->Internal->Locator->getNewInstance();
      if((*iter)->Locator)
        {
        (*iter)->Locator->setPoints((*iter)->Polyline->polyline());
        }
      }
    }
}

void vtkQtLineChart::setOptions(const vtkQtLineChartOptions &options)
{
  this->Options->getHelpFormat()->setFormat(
      options.getHelpFormat()->getFormat());
}

vtkQtLineChartSeriesOptions *vtkQtLineChart::getLineSeriesOptions(
    int series) const
{
  return qobject_cast<vtkQtLineChartSeriesOptions *>(
      this->getSeriesOptions(series));
}

void vtkQtLineChart::getLayerDomain(vtkQtChartLayerDomain &domain) const
{
  domain.mergeDomain(this->Internal->Domains[vtkQtChartLayer::BottomLeft],
      vtkQtChartLayer::BottomLeft);
  domain.mergeDomain(this->Internal->Domains[vtkQtChartLayer::BottomRight],
      vtkQtChartLayer::BottomRight);
  domain.mergeDomain(this->Internal->Domains[vtkQtChartLayer::TopLeft],
      vtkQtChartLayer::TopLeft);
  domain.mergeDomain(this->Internal->Domains[vtkQtChartLayer::TopRight],
      vtkQtChartLayer::TopRight);
}

void vtkQtLineChart::layoutChart(const QRectF &area)
{
  this->setPos(area.topLeft());
  if(this->Internal->Series.size() == 0)
    {
    return;
    }

  int i = 0;
  int domainIndex = -1;
  QList<int> seriesList[4];
  vtkQtChartAxis* xaxis = 0;
  vtkQtChartAxis* yaxis = 0;
  vtkQtChartAxisLayer* axisLayer = this->ChartArea->getAxisLayer();
  for(i = 0; i < 4; i++)
    {
    xaxis = axisLayer->getHorizontalAxis((vtkQtChartLayer::AxesCorner)i);
    yaxis = axisLayer->getVerticalAxis((vtkQtChartLayer::AxesCorner)i);
    this->Internal->Domains[i].getDomain(xaxis->getAxisDomain(),
        yaxis->getAxisDomain(), &domainIndex);
    seriesList[i] = this->Internal->Groups[i].getGroup(domainIndex);
    }

  int num = this->Internal->Series.count();
  for(i = 0; i < num; i++)
    {
    vtkQtLineChartItem *series = this->Internal->Series[i];
    vtkQtLineChartSeriesOptions *options = this->getLineSeriesOptions(i);
    vtkQtChartLayer::AxesCorner corner = options->getAxesCorner();
    series->setVisible(seriesList[corner].contains(i));

    if(!series->isVisible())
      {
      continue;
      }

    xaxis = axisLayer->getHorizontalAxis(corner);
    yaxis = axisLayer->getVerticalAxis(corner);

    int total = this->Model->getNumberOfSeriesValues(i);
    QPolygonF points(total);
    QVariant xValue, yValue;
    for(int j = 0; j < total; j++)
      {
      xValue = this->Model->getSeriesValue(i, j, 0);
      yValue = this->Model->getSeriesValue(i, j, 1);
      points[j] = QPointF(xaxis->getPixel(xValue), yaxis->getPixel(yValue));
      }

    series->Polyline->setPolyline(points);
    if(series->Points->isVisible())
      {
      series->Points->setPoints(points);
      }

    if(series->Locator)
      {
      series->Locator->setPoints(points);
      }
    }

  // Layout the highlights.
  this->layoutHighlights();
}

bool vtkQtLineChart::drawItemFilter(QGraphicsItem *item, QPainter *painter)
{
  if(this->ChartArea)
    {
    // If the item is a series polyline, clip it to the chart layer bounds.
    QRectF bounds;
    this->ChartArea->getContentsSpace()->getChartLayerBounds(bounds);
    vtkQtPolylineItem *polyline =
        qgraphicsitem_cast<vtkQtPolylineItem *>(item);
    if(polyline)
      {
      painter->setClipRect(bounds, Qt::IntersectClip);
      }
    else
      {
      // If the item is a point marker, set the clipping rectangle.
      vtkQtPointMarker *marker = qgraphicsitem_cast<vtkQtPointMarker *>(item);
      if(marker)
        {
        bounds = QRectF(this->Contents->mapFromScene(bounds.topLeft()),
            bounds.size());
        bounds.adjust(-0.5, -0.5, 0.5, 0.5);
        marker->setClipRect(bounds);
        }
      }
    }

  return false;
}

bool vtkQtLineChart::getHelpText(const QPointF &point, QString &text)
{
  vtkQtChartSeriesSelection selection;
  this->getPointsAt(point, selection);
  if(!selection.isEmpty())
    {
    // Use the axis options to format the data.
    vtkQtChartAxisOptions *xAxis = 0;
    vtkQtChartAxisOptions *yAxis = 0;
    vtkQtLineChartSeriesOptions *options = 0;
    vtkQtChartAxisLayer *layer = this->ChartArea->getAxisLayer();
    const QList<vtkQtChartSeriesSelectionItem> &points = selection.getPoints();
    QList<vtkQtChartSeriesSelectionItem>::ConstIterator iter = points.begin();
    for( ; iter != points.end(); ++iter)
      {
      // Use the axis options to format the data.
      options = this->getLineSeriesOptions(iter->Series);
      xAxis = layer->getHorizontalAxis(options->getAxesCorner())->getOptions();
      yAxis = layer->getVerticalAxis(options->getAxesCorner())->getOptions();

      vtkQtChartIndexRangeList::ConstIterator jter = iter->Points.begin();
      for( ; jter != iter->Points.end(); ++jter)
        {
        for(int i = jter->first; i <= jter->second; i++)
          {
          if(!text.isEmpty())
            {
            text.append("\n\n");
            }

          // Get the data from the model.
          QStringList args;
          args.append(xAxis->formatValue(
              this->Model->getSeriesValue(iter->Series, i, 0)));
          args.append(yAxis->formatValue(
              this->Model->getSeriesValue(iter->Series, i, 1)));
          text = this->Options->getHelpFormat()->getHelpText(
              this->Model->getSeriesName(iter->Series).toString(), args);
          }
        }
      }

    return true;
    }

  return false;
}

void vtkQtLineChart::getSeriesAt(const QPointF &point,
    vtkQtChartSeriesSelection &selection) const
{
  // Get the list of items from the scene. Search the list for series
  // polylines.
  vtkQtChartIndexRangeList indexes;
  QList<QGraphicsItem *> list = this->scene()->items(point);
  QList<QGraphicsItem *>::Iterator iter = list.begin();
  for( ; iter != list.end(); ++iter)
    {
    // Make sure the polyline is a line series.
    vtkQtPolylineItem *polyline =
        qgraphicsitem_cast<vtkQtPolylineItem *>(*iter);
    int series = this->Internal->getSeries(polyline);
    if(series != -1)
      {
      // Add the series to the selection.
      indexes.append(vtkQtChartIndexRange(series, series));
      }
    }

  selection.setSeries(indexes);
}

void vtkQtLineChart::getPointsAt(const QPointF &point,
    vtkQtChartSeriesSelection &selection) const
{
  // Get the list of items from the scene. Search the list for series
  // points.
  QList<int> visited;
  QList<vtkQtChartSeriesSelectionItem> indexes;
  QList<QGraphicsItem *> list = this->scene()->items(point);
  QList<QGraphicsItem *>::Iterator iter = list.begin();
  for( ; iter != list.end(); ++iter)
    {
    // Make sure the polyline is a line series.
    vtkQtPolylineItem *polyline =
        qgraphicsitem_cast<vtkQtPolylineItem *>(*iter);
    int series = this->Internal->getSeries(polyline);
    if(series == -1)
      {
      vtkQtPointMarker *marker = qgraphicsitem_cast<vtkQtPointMarker *>(*iter);
      series = this->Internal->getSeries(marker);
      }

    if(series != -1 && !visited.contains(series))
      {
      // Use the locator to find the list of points. Create a pick area
      // from the mouse location and the point marker size.
      if(this->Internal->Series[series]->Locator)
        {
        visited.append(series);
        vtkQtLineChartSeriesOptions *options =
            this->getLineSeriesOptions(series);
        QRectF area(point, options->getMarkerSize());
        area.translate(-area.width() * 0.5, -area.height() * 0.5);
        area.translate(-this->pos().x(), -this->pos().y());
        vtkQtChartSeriesSelectionItem item(series);
        this->Internal->Series[series]->Locator->findPointsIn(
            area, item.Points);
        if(item.Points.size() > 0)
          {
          indexes.append(item);
          }
        }
      }
    }

  selection.setPoints(indexes);
}

void vtkQtLineChart::getSeriesIn(const QRectF &area,
    vtkQtChartSeriesSelection &selection) const
{
  // Get the list of items from the scene. Search the list for series
  // polylines.
  vtkQtChartIndexRangeList indexes;
  QList<QGraphicsItem *> list = this->scene()->items(area);
  QList<QGraphicsItem *>::Iterator iter = list.begin();
  for( ; iter != list.end(); ++iter)
    {
    vtkQtPolylineItem *polyline =
        qgraphicsitem_cast<vtkQtPolylineItem *>(*iter);
    if(polyline)
      {
      // Make sure the polyline is a line series.
      int series = this->Internal->getSeries(polyline);
      if(series != -1)
        {
        // Add the series to the selection.
        indexes.append(vtkQtChartIndexRange(series, series));
        }
      }
    }

  selection.setSeries(indexes);
}

void vtkQtLineChart::getPointsIn(const QRectF &area,
    vtkQtChartSeriesSelection &selection) const
{
  // Get the list of items from the scene. Search the list for series
  // points.
  QList<int> visited;
  QRectF contentsArea = area.translated(-this->pos().x(), -this->pos().y());
  QList<vtkQtChartSeriesSelectionItem> indexes;
  QList<QGraphicsItem *> list = this->scene()->items(area);
  QList<QGraphicsItem *>::Iterator iter = list.begin();
  for( ; iter != list.end(); ++iter)
    {
    // Make sure the polyline is a line series.
    vtkQtPolylineItem *polyline =
        qgraphicsitem_cast<vtkQtPolylineItem *>(*iter);
    int series = this->Internal->getSeries(polyline);
    if(series == -1)
      {
      vtkQtPointMarker *marker = qgraphicsitem_cast<vtkQtPointMarker *>(*iter);
      series = this->Internal->getSeries(marker);
      }

    if(series != -1 && !visited.contains(series))
      {
      // Use the locator to find the list of points.
      if(this->Internal->Series[series]->Locator)
        {
        visited.append(series);
        vtkQtChartSeriesSelectionItem item(series);
        this->Internal->Series[series]->Locator->findPointsIn(
            contentsArea, item.Points);
        if(item.Points.size() > 0)
          {
          indexes.append(item);
          }
        }
      }
    }

  selection.setPoints(indexes);
}

QRectF vtkQtLineChart::boundingRect() const
{
  return QRectF(0,0,0,0);
}

void vtkQtLineChart::paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*)
{
}

void vtkQtLineChart::reset()
{
  // Clean up the current highlights. Make sure the selection model
  // is notified of the change.
  this->InModelChange = true;
  this->Selection->beginModelReset();
  QList<QPair<int, vtkQtLineChartItem *> >::Iterator iter =
      this->Internal->Highlights.begin();
  for( ; iter != this->Internal->Highlights.end(); ++iter)
    {
    delete iter->second;
    }

  this->Internal->Highlights.clear();

  // Clean up the current polyline items.
  bool needsLayout = this->Internal->Series.size() > 0;
  QList<vtkQtLineChartItem *>::Iterator jter = this->Internal->Series.begin();
  for( ; jter != this->Internal->Series.end(); ++jter)
    {
    delete *jter;
    }

  this->Internal->Series.clear();
  for(int i = 0; i < 4; i++)
    {
    this->Internal->Domains[i].clear();
    this->Internal->Groups[i].clear();
    }

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

vtkQtChartSeriesOptions *vtkQtLineChart::createOptions(QObject *parentObject)
{
  return new vtkQtLineChartSeriesOptions(parentObject);
}

void vtkQtLineChart::setupOptions(vtkQtChartSeriesOptions *options)
{
  vtkQtLineChartSeriesOptions *seriesOptions =
      qobject_cast<vtkQtLineChartSeriesOptions *>(options);
  if(seriesOptions)
    {
    // Listen for series option changes.
    this->connect(seriesOptions, SIGNAL(visibilityChanged(bool)),
        this, SLOT(handleSeriesVisibilityChange(bool)));
    this->connect(seriesOptions, SIGNAL(axesCornerChanged(int, int)),
        this, SLOT(handleSeriesAxesCornerChange(int, int)));
    this->connect(seriesOptions, SIGNAL(pointVisibilityChanged(bool)),
        this, SLOT(handleSeriesPointVisibilityChange(bool)));
    this->connect(seriesOptions, SIGNAL(pointMarkerChanged()),
        this, SLOT(handleSeriesPointMarkerChange()));
    this->connect(seriesOptions, SIGNAL(penChanged(const QPen &)),
        this, SLOT(handleSeriesPenChange(const QPen &)));
    this->connect(seriesOptions, SIGNAL(brushChanged(const QBrush &)),
        this, SLOT(handleSeriesBrushChange(const QBrush &)));
    }
}

void vtkQtLineChart::prepareSeriesInsert(int first, int last)
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

void vtkQtLineChart::insertSeries(int first, int last)
{
  if(this->ChartArea)
    {
    for(int j = 0; j < 4; j++)
      {
      this->Internal->Groups[j].prepareInsert(first, last);
      }

    int i = first;
    bool signalDomain = false;
    for( ; first <= last; first++)
      {
      vtkQtLineChartItem *item = new vtkQtLineChartItem(this->Contents);
      this->Internal->Series.insert(first, item);

      // Set the series drawing options.
      vtkQtLineChartSeriesOptions *options = this->getLineSeriesOptions(first);
      item->Polyline->setPen(options->getPen());
      item->Points->setVisible(options->arePointsVisible());
      item->Points->setStyle(options->getMarkerStyle());
      item->Points->setSize(options->getMarkerSize());
      item->Points->setPen(options->getPen());
      item->Points->setBrush(options->getBrush());

      // Add the point locator to the series.
      if(this->Internal->Locator)
        {
        item->Locator = this->Internal->Locator->getNewInstance();
        }

      // Add the series domains to the chart domains.
      if(options->isVisible())
        {
        if(this->addSeriesDomain(first, options->getAxesCorner()))
          {
          signalDomain = true;
          }
        }
      }

    // Fix up the z-order for the new items and any subsequent items.
    for( ; i < this->Internal->Series.size(); i++)
      {
      this->Internal->Series[i]->setZValue(i);
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

void vtkQtLineChart::startSeriesRemoval(int first, int last)
{
  if(this->ChartArea)
    {
    // Notify the selection model of the change. The selection will be
    // adjusted for the changes in this call so it can be layed out
    // when the changes are completed.
    this->InModelChange = true;
    this->Selection->beginRemoveSeries(first, last);

    // Find which domain groups need to be re-calculated
    int i = first;
    QList<int> groups[4];
    QList<int>::Iterator iter;
    for( ; i <= last; i++)
      {
      vtkQtLineChartSeriesOptions *options = this->getLineSeriesOptions(i);
      vtkQtChartLayer::AxesCorner corner = options->getAxesCorner();
      int index = this->Internal->Groups[corner].removeSeries(i);
      if(index != -1)
        {
        // Add the group indexes in reverse order.
        bool doAdd = true;
        iter = groups[corner].begin();
        for( ; iter != groups[corner].end(); ++iter)
          {
          if(index > *iter)
            {
            doAdd = false;
            groups[corner].insert(iter, index);
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
          groups[corner].append(index);
          }
        }
      }

    for(i = 0; i < 4; i++)
      {
      for(iter = groups[i].begin(); iter != groups[i].end(); ++iter)
        {
        if(this->Internal->Groups[i].getNumberOfSeries(*iter) == 0)
          {
          // Remove the empty domain.
          this->Internal->Domains[i].removeDomain(*iter);
          }
        else
          {
          // Re-calculate the chart domain.
          this->calculateDomain(*iter, (vtkQtChartLayer::AxesCorner)i);
          }
        }

      // Fix the stored indexes in the domain groups.
      this->Internal->Groups[i].finishRemoval(first, last);
      }

    for( ; last >= first; last--)
      {
      delete this->Internal->Series.takeAt(last);
      }

    // Fix the z-order for any subsequent items.
    for( ; first < this->Internal->Series.size(); first++)
      {
      this->Internal->Series[first]->setZValue(first);
      }
    }
}

void vtkQtLineChart::finishSeriesRemoval(int first, int last)
{
  if(this->ChartArea)
    {
    emit this->rangeChanged();
    emit this->layoutNeeded();

    // Close the event for the selection model, which will trigger a
    // selection change signal.
    this->Selection->endRemoveSeries(first, last);
    this->InModelChange = false;
    }
}

void vtkQtLineChart::handleSeriesVisibilityChange(bool visible)
{
  // Get the series index from the options index.
  vtkQtLineChartSeriesOptions *options =
      qobject_cast<vtkQtLineChartSeriesOptions *>(this->sender());
  int series = this->getSeriesOptionsIndex(options);
  if(series >= 0 && series < this->Internal->Series.size())
    {
    vtkQtChartLayer::AxesCorner corner = options->getAxesCorner();
    if(visible)
      {
      // If the series is going to be visible, add to the domain.
      if(this->addSeriesDomain(series, corner))
        {
        emit this->rangeChanged();
        }

      emit this->layoutNeeded();
      }
    else
      {
      int seriesGroup = this->Internal->Groups[corner].removeSeries(series);
      if(seriesGroup != -1)
        {
        // If the group is empty, remove the domain.
        if(this->Internal->Groups[corner].getNumberOfSeries(seriesGroup) == 0)
          {
          this->Internal->Domains[corner].removeDomain(seriesGroup);
          }
        else
          {
          // Re-calculate the domain.
          this->calculateDomain(seriesGroup, corner);
          }

        this->Internal->Groups[corner].finishRemoval();
        emit this->rangeChanged();
        emit this->layoutNeeded();
        }
      }
    }
}

void vtkQtLineChart::handleSeriesAxesCornerChange(int corner, int previous)
{
  vtkQtLineChartSeriesOptions *options =
      qobject_cast<vtkQtLineChartSeriesOptions *>(this->sender());
  int series = this->getSeriesOptionsIndex(options);
  if(series >= 0 && series < this->Internal->Series.size())
    {
    // Remove the series from the previous group.
    int seriesGroup = this->Internal->Groups[previous].removeSeries(series);
    if(this->Internal->Groups[previous].getNumberOfSeries(seriesGroup) == 0)
      {
      // If the group is empty, remove the domain.
      this->Internal->Domains[previous].removeDomain(seriesGroup);
      }
    else
      {
      // Re-calculate the domain.
      this->calculateDomain(seriesGroup, (vtkQtChartLayer::AxesCorner)previous);
      }

    this->Internal->Groups[previous].finishRemoval();

    // Add the series to the new group.
    this->addSeriesDomain(series, (vtkQtChartLayer::AxesCorner)corner);

    emit this->rangeChanged();
    emit this->layoutNeeded();
    }
}

void vtkQtLineChart::handleSeriesPointVisibilityChange(bool visible)
{
  vtkQtLineChartSeriesOptions *options =
      qobject_cast<vtkQtLineChartSeriesOptions *>(this->sender());
  int series = this->getSeriesOptionsIndex(options);
  if(series >= 0 && series < this->Internal->Series.size())
    {
    this->Internal->Series[series]->Points->setVisible(visible);
    }
}

void vtkQtLineChart::handleSeriesPointMarkerChange()
{
  vtkQtLineChartSeriesOptions *options =
      qobject_cast<vtkQtLineChartSeriesOptions *>(this->sender());
  int series = this->getSeriesOptionsIndex(options);
  if(series >= 0 && series < this->Internal->Series.size())
    {
    this->Internal->Series[series]->Points->setStyle(
        options->getMarkerStyle());
    this->Internal->Series[series]->Points->setSize(options->getMarkerSize());
    }
}

void vtkQtLineChart::handleSeriesPenChange(const QPen &pen)
{
  vtkQtLineChartSeriesOptions *options =
      qobject_cast<vtkQtLineChartSeriesOptions *>(this->sender());
  int series = this->getSeriesOptionsIndex(options);
  if(series >= 0 && series < this->Internal->Series.size())
    {
    vtkQtLineChartItem *item = this->Internal->Series[series];
    item->Polyline->setPen(pen);
    item->Points->setPen(pen);
    }
}

void vtkQtLineChart::handleSeriesBrushChange(const QBrush &brush)
{
  vtkQtLineChartSeriesOptions *options =
      qobject_cast<vtkQtLineChartSeriesOptions *>(this->sender());
  int series = this->getSeriesOptionsIndex(options);
  if(series >= 0 && series < this->Internal->Series.size())
    {
    this->Internal->Series[series]->Points->setBrush(brush);
    }
}

void vtkQtLineChart::updateHighlights()
{
  if(!this->InModelChange && this->ChartArea)
    {
    this->layoutHighlights();
    }
}

void vtkQtLineChart::layoutHighlights()
{
  if(this->Internal->Series.size() > 0)
    {
    // Clear the current set of highlights. Restore the series pen at
    // the same time.
    vtkQtLineChartItem *item = 0;
    vtkQtLineChartSeriesOptions *options = 0;
    QList<QPair<int, vtkQtLineChartItem *> >::Iterator iter =
        this->Internal->Highlights.begin();
    for( ; iter != this->Internal->Highlights.end(); ++iter)
      {
      item = this->Internal->Series[iter->first];
      options = this->getLineSeriesOptions(iter->first);
      if(iter->second->Polyline->isVisible())
        {
        item->Polyline->setPen(options->getPen());
        item->Points->setPen(options->getPen());
        }

      delete iter->second;
      }

    this->Internal->Highlights.clear();

    QList<vtkQtPointMarker *>::Iterator lightPoint =
        this->Internal->LightPoints.begin();
    for( ; lightPoint != this->Internal->LightPoints.end(); ++lightPoint)
      {
      delete *lightPoint;
      }

    this->Internal->LightPoints.clear();

    // Get the current selection from the selection model.
    if(!this->Selection->isSelectionEmpty())
      {
      int i = 0;
      vtkQtLineChartItem *highlight = 0;
      const vtkQtChartSeriesSelection &current =
          this->Selection->getSelection();
      if(current.getType() == vtkQtChartSeriesSelection::SeriesSelection)
        {
        const vtkQtChartIndexRangeList &series = current.getSeries();
        vtkQtChartIndexRangeList::ConstIterator jter = series.begin();
        for( ; jter != series.end(); ++jter)
          {
          for(i = jter->first; i <= jter->second; i++)
            {
            // Lighten the series pen color.
            item = this->Internal->Series[i];
            options = this->getLineSeriesOptions(i);
            QPen linePen = options->getPen();
            linePen.setColor(vtkQtChartAxisOptions::lighter(linePen.color()));
            item->Polyline->setPen(linePen);
            item->Points->setPen(linePen);

            // Create a highlight item to place behind the series.
            highlight = new vtkQtLineChartItem(this->Contents);
            this->Internal->Highlights.append(
                QPair<int, vtkQtLineChartItem *>(i, highlight));
            highlight->setZValue(item->zValue() - 0.5);
            linePen = options->getPen();
            linePen.setWidthF(linePen.widthF() + 4.0);
            highlight->Polyline->setPen(linePen);
            highlight->Polyline->setPolyline(item->Polyline->polyline());
            highlight->Points->setVisible(item->Points->isVisible());
            if(highlight->Points->isVisible())
              {
              highlight->Points->setPen(linePen);
              highlight->Points->setBrush(linePen.color());
              highlight->Points->setStyle(item->Points->getStyle());
              highlight->Points->setSize(item->Points->getSize());
              highlight->Points->setPoints(item->Points->getPoints());
              }
            }
          }
        }
      else if(current.getType() == vtkQtChartSeriesSelection::PointSelection)
        {
        const QList<vtkQtChartSeriesSelectionItem> &points =
            current.getPoints();
        QList<vtkQtChartSeriesSelectionItem>::ConstIterator jter;
        for(jter = points.begin(); jter != points.end(); ++jter)
          {
          // Lighten the series points. Get the list of selected points.
          item = this->Internal->Series[jter->Series];
          options = this->getLineSeriesOptions(jter->Series);
          QPolygonF selectedPoints;
          QPolygonF seriesPoints = item->Polyline->polyline();
          vtkQtChartIndexRangeList::ConstIterator kter = jter->Points.begin();
          for( ; kter != jter->Points.end(); ++kter)
            {
            selectedPoints << seriesPoints.mid(kter->first,
                kter->second - kter->first + 1);
            }

          // Add lightened points in front of the series.
          vtkQtPointMarker *lightPoints = new vtkQtPointMarker(
              item->Points->getSize(), item->Points->getStyle(),
              this->Contents, this->Contents->scene());
          this->Internal->LightPoints.append(lightPoints);
          lightPoints->setZValue(item->zValue() + 0.25);
          QPen linePen = options->getPen();
          linePen.setColor(vtkQtChartAxisOptions::lighter(linePen.color()));
          lightPoints->setPen(linePen);
          lightPoints->setBrush(item->Points->brush());
          lightPoints->setPoints(selectedPoints);

          // Create a highlight item to place behind the series.
          highlight = new vtkQtLineChartItem(this->Contents);
          this->Internal->Highlights.append(
              QPair<int, vtkQtLineChartItem *>(jter->Series, highlight));
          highlight->setZValue(item->zValue() - 0.5);
          linePen = options->getPen();
          linePen.setWidthF(linePen.widthF() + 4.0);
          highlight->Polyline->setVisible(false);
          highlight->Points->setPen(linePen);
          highlight->Points->setBrush(linePen.color());
          highlight->Points->setStyle(item->Points->getStyle());
          highlight->Points->setSize(item->Points->getSize());
          highlight->Points->setPoints(selectedPoints);
          }
        }
      }
    }
}

bool vtkQtLineChart::addSeriesDomain(int series,
    vtkQtChartLayer::AxesCorner corner)
{
  QList<QVariant> xDomain = this->Model->getSeriesRange(series, 0);
  QList<QVariant> yDomain = this->Model->getSeriesRange(series, 1);
  bool xIsList = xDomain.isEmpty();
  bool yIsList = yDomain.isEmpty();
  if(xIsList || yIsList)
    {
    int points = this->Model->getNumberOfSeriesValues(series);
    for(int j = 0; j < points; j++)
      {
      if(xIsList)
        {
        xDomain.append(this->Model->getSeriesValue(series, j, 0));
        }

      if(yIsList)
        {
        yDomain.append(this->Model->getSeriesValue(series, j, 1));
        }
      }
    }

  vtkQtChartSeriesDomain domain;
  if(xIsList)
    {
    domain.getXDomain().setDomain(xDomain);
    }
  else
    {
    domain.getXDomain().setRange(xDomain);
    }

  if(yIsList)
    {
    domain.getYDomain().setDomain(yDomain);
    }
  else
    {
    domain.getYDomain().setRange(yDomain);
    }

  int seriesGroup = -1;
  bool changed = this->Internal->Domains[corner].mergeDomain(domain,
      &seriesGroup);

  // Add the series index to the domain group.
  this->Internal->Groups[corner].insertSeries(series, seriesGroup);
  return changed;
}

void vtkQtLineChart::calculateDomain(int seriesGroup,
    vtkQtChartLayer::AxesCorner corner)
{
  // Clear the current domain information.
  vtkQtChartSeriesDomain *domain =
      this->Internal->Domains[corner].getDomain(seriesGroup);
  domain->getXDomain().clear();
  domain->getYDomain().clear();

  // Get the list of series in the group.
  QList<int> list = this->Internal->Groups[corner].getGroup(seriesGroup);
  for(QList<int>::Iterator iter = list.begin(); iter != list.end(); ++iter)
    {
    vtkQtLineChartSeriesOptions *options = this->getLineSeriesOptions(*iter);
    if(options && !options->isVisible())
      {
      continue;
      }

    QList<QVariant> xDomain = this->Model->getSeriesRange(*iter, 0);
    QList<QVariant> yDomain = this->Model->getSeriesRange(*iter, 1);
    bool xIsList = xDomain.isEmpty();
    bool yIsList = yDomain.isEmpty();
    if(xIsList || yIsList)
      {
      int points = this->Model->getNumberOfSeriesValues(*iter);
      for(int j = 0; j < points; j++)
        {
        if(xIsList)
          {
          xDomain.append(this->Model->getSeriesValue(*iter, j, 0));
          }

        if(yIsList)
          {
          yDomain.append(this->Model->getSeriesValue(*iter, j, 1));
          }
        }
      }

    if(xIsList)
      {
      domain->getXDomain().mergeDomain(xDomain);
      }
    else
      {
      domain->getXDomain().mergeRange(xDomain);
      }

    if(yIsList)
      {
      domain->getYDomain().mergeDomain(yDomain);
      }
    else
      {
      domain->getYDomain().mergeRange(yDomain);
      }
    }
}


