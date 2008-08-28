/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtStackedChart.cxx

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

/// \file vtkQtStackedChart.cxx
/// \date February 27, 2008

#include "vtkQtStackedChart.h"

#include "vtkQtChartAxis.h"
#include "vtkQtChartAxisCornerDomain.h"
#include "vtkQtChartAxisDomain.h"
#include "vtkQtChartAxisLayer.h"
#include "vtkQtChartAxisOptions.h"
#include "vtkQtChartContentsArea.h"
#include "vtkQtChartContentsSpace.h"
#include "vtkQtChartHelpFormatter.h"
#include "vtkQtChartLayerDomain.h"
#include "vtkQtChartSeriesDomain.h"
#include "vtkQtChartSeriesDomainGroup.h"
#include "vtkQtChartSeriesModel.h"
#include "vtkQtChartSeriesSelection.h"
#include "vtkQtChartSeriesSelectionModel.h"
#include "vtkQtChartArea.h"
#include "vtkQtStackedChartOptions.h"
#include "vtkQtStackedChartSeriesOptions.h"

#include <QBrush>
#include <QGraphicsPolygonItem>
#include <QGraphicsScene>
#include <QList>
#include <QPen>
#include <QVector>


class vtkQtStackedChartItem
{
public:
  vtkQtStackedChartItem(QGraphicsPolygonItem *polygon=0);
  vtkQtStackedChartItem(const vtkQtStackedChartItem &other);
  ~vtkQtStackedChartItem() {}

  void setMapping(int group, int index);

  vtkQtStackedChartItem &operator=(const vtkQtStackedChartItem &other);

  QGraphicsPolygonItem *Polygon;
  int Group;
  int Index;
  bool IsHighlighted;
};


class vtkQtStackedChartDomainGroup : public vtkQtChartSeriesDomainGroup
{
public:
  vtkQtStackedChartDomainGroup();
  virtual ~vtkQtStackedChartDomainGroup() {}

  virtual void clear();

protected:
  virtual void insertGroup(int group);
  virtual void removeGroup(int group);

public:
  QList<QVector<QVector<double> > > Tables;
};


class vtkQtStackedChartInternal
{
public:
  vtkQtStackedChartInternal();
  ~vtkQtStackedChartInternal();

  int getSeries(QGraphicsPolygonItem *polygon) const;
  QPointF getMidPoint(const QPointF &point1, const QPointF &point2) const;

  QList<vtkQtStackedChartItem *> Series;
  QList<QGraphicsPolygonItem *> Highlights;
  vtkQtChartAxisCornerDomain Domain;
  vtkQtStackedChartDomainGroup Groups;
};


//-----------------------------------------------------------------------------
vtkQtStackedChartItem::vtkQtStackedChartItem(QGraphicsPolygonItem *polygon)
{
  this->Polygon = polygon;
  this->Group = -1;
  this->Index = -1;
  this->IsHighlighted = false;
}

vtkQtStackedChartItem::vtkQtStackedChartItem(
    const vtkQtStackedChartItem &other)
{
  this->Polygon = other.Polygon;
  this->Group = other.Group;
  this->Index = other.Index;
  this->IsHighlighted = other.IsHighlighted;
}

void vtkQtStackedChartItem::setMapping(int group, int index)
{
  this->Group = group;
  this->Index = index;
}

vtkQtStackedChartItem &vtkQtStackedChartItem::operator=(
    const vtkQtStackedChartItem &other)
{
  this->Polygon = other.Polygon;
  this->Group = other.Group;
  this->Index = other.Index;
  this->IsHighlighted = other.IsHighlighted;
  return *this;
}


//-----------------------------------------------------------------------------
vtkQtStackedChartDomainGroup::vtkQtStackedChartDomainGroup()
  : vtkQtChartSeriesDomainGroup(true), Tables()
{
}

void vtkQtStackedChartDomainGroup::clear()
{
  vtkQtChartSeriesDomainGroup::clear();
  this->Tables.clear();
}

void vtkQtStackedChartDomainGroup::insertGroup(int group)
{
  vtkQtChartSeriesDomainGroup::insertGroup(group);
  this->Tables.insert(group, QVector<QVector<double> >());
}

void vtkQtStackedChartDomainGroup::removeGroup(int group)
{
  vtkQtChartSeriesDomainGroup::removeGroup(group);
  this->Tables.removeAt(group);
}


//-----------------------------------------------------------------------------
vtkQtStackedChartInternal::vtkQtStackedChartInternal()
  : Series(), Highlights(), Domain(), Groups()
{
  this->Domain.setVerticalPreferences(false, true, false);
}

vtkQtStackedChartInternal::~vtkQtStackedChartInternal()
{
  // Clean up the remaining series items.
  QList<vtkQtStackedChartItem *>::Iterator iter = this->Series.begin();
  for( ; iter != this->Series.end(); ++iter)
    {
    // The polygons will get cleaned up with the other graphics items.
    delete *iter;
    }
}

int vtkQtStackedChartInternal::getSeries(QGraphicsPolygonItem *polygon) const
{
  QList<vtkQtStackedChartItem *>::ConstIterator iter = this->Series.begin();
  for(int i = 0; iter != this->Series.end(); ++iter, ++i)
    {
    if((*iter)->Polygon == polygon)
      {
      return i;
      }
    }

  return -1;
}

QPointF vtkQtStackedChartInternal::getMidPoint(const QPointF &point1,
    const QPointF &point2) const
{
  return QPointF(
      (point2.x() + point1.x()) * 0.5, (point2.y() + point1.y()) * 0.5);
}


//-----------------------------------------------------------------------------
vtkQtStackedChart::vtkQtStackedChart()
  : vtkQtChartSeriesLayer()
{
  this->Internal = new vtkQtStackedChartInternal();
  this->Options = new vtkQtStackedChartOptions(this);
  this->InModelChange = false;

  // Listen for option changes.
  this->connect(this->Options, SIGNAL(axesCornerChanged()),
      this, SLOT(handleAxesCornerChange()));
  this->connect(this->Options, SIGNAL(sumationChanged()),
      this, SLOT(handleSumationChange()));
  this->connect(this->Options, SIGNAL(gradientChanged()),
      this, SLOT(handleGradientChange()));

  // Listen for selection changes.
  this->connect(this->Selection,
      SIGNAL(selectionChanged(const vtkQtChartSeriesSelection &)),
      this, SLOT(updateHighlights()));
}

vtkQtStackedChart::~vtkQtStackedChart()
{
  delete this->Internal;
}

void vtkQtStackedChart::setChartArea(vtkQtChartArea *area)
{
  vtkQtChartSeriesLayer::setChartArea(area);
  this->reset();
}

void vtkQtStackedChart::setModel(vtkQtChartSeriesModel *model)
{
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

void vtkQtStackedChart::setOptions(const vtkQtStackedChartOptions &options)
{
  // Copy the new options. The chart will collapse the layout signals.
  this->Options->setSumNormalized(options.isSumNormalized());
  this->Options->setGradientDisplayed(options.isGradientDislpayed());
  this->Options->setAxesCorner(options.getAxesCorner());
  this->Options->getHelpFormat()->setFormat(
      options.getHelpFormat()->getFormat());
}

vtkQtStackedChartSeriesOptions *vtkQtStackedChart::getStackedSeriesOptions(
    int series) const
{
  return qobject_cast<vtkQtStackedChartSeriesOptions *>(
      this->getSeriesOptions(series));
}

void vtkQtStackedChart::getLayerDomain(vtkQtChartLayerDomain &domain) const
{
  domain.mergeDomain(this->Internal->Domain, this->Options->getAxesCorner());
}

void vtkQtStackedChart::layoutChart(const QRectF &area)
{
  // Update the position.
  this->setPos(area.topLeft());
  if(this->Internal->Series.size() == 0)
    {
    return;
    }

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

  // Get the x-axis minimum and maximum locations.
  float zero = yAxis->getZeroPixel();
  bool isRange = false;
  QList<QVariant> xDomain;
  QList<int> seriesList;
  if(seriesDomain)
    {
    seriesList = this->Internal->Groups.getGroup(seriesGroup);
    xDomain = seriesDomain->getXDomain().getDomain(isRange);
    }

  int i = 0;
  vtkQtStackedChartSeriesOptions *options = 0;
  int total = this->Model->getNumberOfSeries();
  for(int series = 0; series < total; series++)
    {
    QGraphicsPolygonItem *polygon = this->Internal->Series[series]->Polygon;
    if(polygon)
      {
      polygon->setVisible(seriesList.contains(series));
      if(polygon->isVisible())
        {
        int j = 0;
        QPolygonF points;
        for( ; j < this->Internal->Groups.Tables[seriesGroup][i].size(); j++)
          {
          points.append(QPointF(xAxis->getPixel(xDomain[j]), yAxis->getPixel(
              QVariant(this->Internal->Groups.Tables[seriesGroup][i][j]))));
          }

        if(i == 0)
          {
          j = this->Internal->Groups.Tables[seriesGroup][i].size() - 1;
          for( ; j >= 0; j--)
            {
            points.append(QPointF(xAxis->getPixel(xDomain[j]), zero));
            }
          }
        else
          {
          int k = i - 1;
          j = this->Internal->Groups.Tables[seriesGroup][k].size() - 1;
          for( ; j >= 0; j--)
            {
            points.append(QPointF(xAxis->getPixel(xDomain[j]), yAxis->getPixel(
                QVariant(this->Internal->Groups.Tables[seriesGroup][k][j]))));
            }
          }

        polygon->setPolygon(points);
        i++;

        // Set up the series gradient if needed.
        if(this->Options->isGradientDislpayed())
          {
          options = this->getStackedSeriesOptions(series);
          QRectF bounds = polygon->boundingRect();
          float center = bounds.center().x();
          QLinearGradient gradient(center, bounds.top(),
              center, bounds.bottom());
          QColor color = options->getBrush().color();
          gradient.setColorAt(0.0, color);
          gradient.setColorAt(1.0, color.dark());
          polygon->setBrush(QBrush(gradient));
          }
        }
      }
    }

  // Layout the highlights.
  this->layoutHighlights();
}

bool vtkQtStackedChart::drawItemFilter(QGraphicsItem *item, QPainter *painter)
{
  // If the item is a series polygon, clip it to the chart layer bounds.
  QGraphicsPolygonItem *polygon =
      qgraphicsitem_cast<QGraphicsPolygonItem *>(item);
  if(polygon && this->ChartArea)
    {
    QRectF bounds;
    this->ChartArea->getContentsSpace()->getChartLayerBounds(bounds);
    painter->setClipRect(bounds, Qt::IntersectClip);
    }

  return false;
}

bool vtkQtStackedChart::getHelpText(const QPointF &point, QString &text)
{
  vtkQtChartSeriesSelection selection;
  this->getPointsAt(point, selection);
  if(!selection.isEmpty())
    {
    // Use the axis options to format the data.
    vtkQtChartAxisLayer *layer = this->ChartArea->getAxisLayer();
    vtkQtChartAxisOptions *xAxis = layer->getHorizontalAxis(
        this->Options->getAxesCorner())->getOptions();
    vtkQtChartAxisOptions *yAxis = layer->getVerticalAxis(
        this->Options->getAxesCorner())->getOptions();

    // Use the x-axis domain and the table for the data values.
    const QList<vtkQtChartSeriesSelectionItem> &points = selection.getPoints();
    int series = points[0].Series;
    vtkQtStackedChartItem *item = this->Internal->Series[series];
    const vtkQtChartSeriesDomain *seriesDomain =
        this->Internal->Domain.getDomain(item->Group);
    bool isRange = false;
    int index = points[0].Points[0].first;
    QStringList args;
    args.append(xAxis->formatValue(
        seriesDomain->getXDomain().getDomain(isRange)[index]));
    args.append(yAxis->formatValue(QVariant(
        this->Internal->Groups.Tables[item->Group][item->Index][index])));
    if(item->Index > 0)
      {
      double value =
          this->Internal->Groups.Tables[item->Group][item->Index][index] -
          this->Internal->Groups.Tables[item->Group][item->Index - 1][index];
      args.append(yAxis->formatValue(QVariant(value)));
      }
    else
      {
      args.append(args[1]);
      }

    text = this->Options->getHelpFormat()->getHelpText(
        this->Model->getSeriesName(series).toString(), args);
    return true;
    }

  return false;
}

void vtkQtStackedChart::getSeriesAt(const QPointF &point,
    vtkQtChartSeriesSelection &selection) const
{
  // Get the list of items from the scene. Search the list for the
  // top series polygon.
  vtkQtChartIndexRangeList indexes;
  QList<QGraphicsItem *> list = this->scene()->items(point);
  QList<QGraphicsItem *>::Iterator iter = list.begin();
  for( ; iter != list.end(); ++iter)
    {
    // Make sure the polygon is a series.
    QGraphicsPolygonItem *polygon =
        qgraphicsitem_cast<QGraphicsPolygonItem *>(*iter);
    int series = this->Internal->getSeries(polygon);
    if(series != -1)
      {
      // Only add the top series to the selection.
      indexes.append(vtkQtChartIndexRange(series, series));
      break;
      }
    }

  selection.setSeries(indexes);
}

void vtkQtStackedChart::getPointsAt(const QPointF &point,
    vtkQtChartSeriesSelection &selection) const
{
  // Get the list of items from the scene. Search the list for the
  // top series polygon.
  QList<vtkQtChartSeriesSelectionItem> indexes;
  QList<QGraphicsItem *> list = this->scene()->items(point);
  QList<QGraphicsItem *>::Iterator iter = list.begin();
  for( ; iter != list.end(); ++iter)
    {
    // Make sure the polygon is a series.
    QGraphicsPolygonItem *polygon =
        qgraphicsitem_cast<QGraphicsPolygonItem *>(*iter);
    int series = this->Internal->getSeries(polygon);
    if(series != -1)
      {
      int index = this->findClosestIndex(polygon->polygon(),
          this->Contents->mapFromScene(point));
      if(index != -1)
        {
        vtkQtChartSeriesSelectionItem item(series);
        item.Points.append(vtkQtChartIndexRange(index, index));
        indexes.append(item);
        }

      break;
      }
    }

  selection.setPoints(indexes);
}

void vtkQtStackedChart::getSeriesIn(const QRectF &area,
    vtkQtChartSeriesSelection &selection) const
{
  // Get the list of items from the scene. Search the list for the
  // top series polygon.
  vtkQtChartIndexRangeList indexes;
  QList<QGraphicsItem *> list = this->scene()->items(area);
  QList<QGraphicsItem *>::Iterator iter = list.begin();
  for( ; iter != list.end(); ++iter)
    {
    // Make sure the polygon is a series.
    QGraphicsPolygonItem *polygon =
        qgraphicsitem_cast<QGraphicsPolygonItem *>(*iter);
    int series = this->Internal->getSeries(polygon);
    if(series != -1)
      {
      // Add the series to the selection.
      indexes.append(vtkQtChartIndexRange(series, series));
      }
    }

  selection.setSeries(indexes);
}

void vtkQtStackedChart::getPointsIn(const QRectF &,
    vtkQtChartSeriesSelection &) const
{
  // TODO
}

QRectF vtkQtStackedChart::boundingRect() const
{
  return QRectF(0,0,0,0);
}

void vtkQtStackedChart::paint(QPainter *,
    const QStyleOptionGraphicsItem *, QWidget *)
{
}

void vtkQtStackedChart::reset()
{
  // Make sure the selection model is notified of the change.
  this->InModelChange = true;
  this->Selection->beginModelReset();

  // Clean up the old view items.
  bool needsLayout = this->Internal->Series.size() > 0;
  QList<vtkQtStackedChartItem *>::Iterator iter =
      this->Internal->Series.begin();
  for( ; iter != this->Internal->Series.end(); ++iter)
    {
    if((*iter)->Polygon)
      {
      delete (*iter)->Polygon;
      }

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

vtkQtChartSeriesOptions *vtkQtStackedChart::createOptions(
    QObject *parentObject)
{
  return new vtkQtStackedChartSeriesOptions(parentObject);
}

void vtkQtStackedChart::setupOptions(vtkQtChartSeriesOptions *options)
{
  vtkQtStackedChartSeriesOptions *seriesOptions =
      qobject_cast<vtkQtStackedChartSeriesOptions *>(options);
  if(seriesOptions)
    {
    // Listen for series options changes.
    this->connect(seriesOptions, SIGNAL(visibilityChanged(bool)),
        this, SLOT(handleSeriesVisibilityChange(bool)));
    this->connect(seriesOptions, SIGNAL(penChanged(const QPen &)),
        this, SLOT(handleSeriesPenChange(const QPen &)));
    this->connect(seriesOptions, SIGNAL(brushChanged(const QBrush &)),
        this, SLOT(handleSeriesBrushChange(const QBrush &)));
    }
}

void vtkQtStackedChart::prepareSeriesInsert(int first, int last)
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

void vtkQtStackedChart::insertSeries(int first, int last)
{
  if(this->ChartArea)
    {
    // Update the series indexes stored in the domain groups.
    this->Internal->Groups.prepareInsert(first, last);

    // Add an item for each series.
    QList<int> tableGroups;
    vtkQtStackedChartSeriesOptions *options = 0;
    for( ; first <= last; first++)
      {
      // Only add a polygon if the series y-axis range is numeric.
      QGraphicsPolygonItem *polygon = 0;
      QList<QVariant> yDomain = this->Model->getSeriesRange(first, 1);
      if(yDomain.size() == 2)
        {
        QVariant::Type domain = yDomain[0].type();
        if(domain == QVariant::Int || domain == QVariant::Double)
          {
          polygon = new QGraphicsPolygonItem(this->Contents, this->scene());
          }
        }

      this->Internal->Series.insert(first, new vtkQtStackedChartItem(polygon));
      if(polygon)
        {
        // Set the drawing options for the series.
        options = this->getStackedSeriesOptions(first);
        polygon->setPen(options->getPen());
        polygon->setBrush(options->getBrush());

        if(options->isVisible())
          {
          int seriesGroup = -1;
          this->addSeriesDomain(first, &seriesGroup);
          if(seriesGroup != -1 && !tableGroups.contains(seriesGroup))
            {
            tableGroups.append(seriesGroup);
            }
          }
        }
      }

    // Fix up the z-order for the new items and any previous items.
    int total = this->Internal->Series.size() - 1;
    for( ; last >= 0; last--)
      {
      vtkQtStackedChartItem *series = this->Internal->Series[last];
      if(series->Polygon)
        {
        series->Polygon->setZValue(total - last);
        }
      }

    if(tableGroups.size() > 0)
      {
      QList<int>::Iterator iter = tableGroups.begin();
      for( ; iter != tableGroups.end(); ++iter)
        {
        this->updateItemMap(*iter);
        this->createTable(*iter);
        }

      emit this->rangeChanged();
      emit this->layoutNeeded();
      }

    // Close the event for the selection model, which will trigger a
    // selection change signal.
    this->Selection->endInsertSeries(first, last);
    this->InModelChange = false;
    }
}

void vtkQtStackedChart::startSeriesRemoval(int first, int last)
{
  if(this->ChartArea)
    {
    // Notify the selection model of the change. The selection will be
    // adjusted for the changes in this call so it can be layed out
    // when the changes are completed.
    this->InModelChange = true;
    this->Selection->beginRemoveSeries(first, last);

    // Remove each of the series items.
    vtkQtStackedChartItem *series = 0;
    for( ; last >= first; last--)
      {
      series = this->Internal->Series.takeAt(last);
      if(series->Polygon)
        {
        delete series->Polygon;
        }

      delete series;
      }

    // Fix the z-order for any subsequent items.
    int total = this->Internal->Series.size() - 1;
    for(first--; first >= 0; first--)
      {
      series = this->Internal->Series[first];
      if(series->Polygon)
        {
        series->Polygon->setZValue(total - first);
        }
      }
    }
}

void vtkQtStackedChart::finishSeriesRemoval(int first, int last)
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
        // Re-calculate the chart domain and table.
        this->updateItemMap(*iter);
        this->calculateXDomain(*iter);
        this->createTable(*iter);
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

void vtkQtStackedChart::handleAxesCornerChange()
{
  if(this->Model && this->ChartArea)
    {
    emit this->rangeChanged();
    emit this->layoutNeeded();
    }
}

void vtkQtStackedChart::handleSumationChange()
{
  if(this->Model && this->ChartArea)
    {
    for(int i = 0; i < this->Internal->Groups.getNumberOfGroups(); i++)
      {
      if(this->Options->isSumNormalized())
        {
        this->normalizeTable(i);
        this->calculateYDomain(i);
        }
      else
        {
        this->createTable(i);
        }
      }

    if(this->Internal->Groups.getNumberOfGroups() > 0)
      {
      emit this->rangeChanged();
      emit this->layoutNeeded();
      }
    }
}

void vtkQtStackedChart::handleGradientChange()
{
  if(this->Model && this->ChartArea)
    {
    if(this->Options->isGradientDislpayed())
      {
      emit this->layoutNeeded();
      }
    else
      {
      // If the gradient display is off, reset all the series brushes.
      vtkQtStackedChartSeriesOptions *options = 0;
      QList<vtkQtStackedChartItem *>::Iterator iter =
          this->Internal->Series.begin();
      for(int i = 0; iter != this->Internal->Series.end(); ++iter, ++i)
        {
        options = this->getStackedSeriesOptions(i);
        (*iter)->Polygon->setBrush(options->getBrush());
        }
      }
    }
}

void vtkQtStackedChart::handleSeriesVisibilityChange(bool visible)
{
  // Get the series index from the options index.
  vtkQtStackedChartSeriesOptions *options =
      qobject_cast<vtkQtStackedChartSeriesOptions *>(this->sender());
  int series = this->getSeriesOptionsIndex(options);
  if(series >= 0 && series < this->Internal->Series.size() &&
      this->Internal->Series[series]->Polygon)
    {
    if(visible)
      {
      int seriesGroup = -1;
      this->addSeriesDomain(series, &seriesGroup);
      if(seriesGroup != -1)
        {
        this->updateItemMap(seriesGroup);
        this->createTable(seriesGroup);
        emit this->rangeChanged();
        emit this->layoutNeeded();
        }
      }
    else
      {
      this->Internal->Series[series]->setMapping(-1, -1);
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
          this->updateItemMap(seriesGroup);
          this->calculateXDomain(seriesGroup);
          this->createTable(seriesGroup);
          }

        this->Internal->Groups.finishRemoval();
        emit this->rangeChanged();
        emit this->layoutNeeded();
        }
      }
    }
}

void vtkQtStackedChart::handleSeriesPenChange(const QPen &pen)
{
  // Get the series index from the options.
  vtkQtStackedChartSeriesOptions *options =
      qobject_cast<vtkQtStackedChartSeriesOptions *>(this->sender());
  int series = this->getSeriesOptionsIndex(options);
  if(series >= 0 && series < this->Internal->Series.size())
    {
    vtkQtStackedChartItem *item = this->Internal->Series[series];
    if(item->Polygon)
      {
      item->Polygon->setPen(pen);
      }
    }
}

void vtkQtStackedChart::handleSeriesBrushChange(const QBrush &brush)
{
  // Get the series index from the options.
  vtkQtStackedChartSeriesOptions *options =
      qobject_cast<vtkQtStackedChartSeriesOptions *>(this->sender());
  int series = this->getSeriesOptionsIndex(options);
  if(series >= 0 && series < this->Internal->Series.size())
    {
    vtkQtStackedChartItem *item = this->Internal->Series[series];
    if(item->Polygon)
      {
      if(item->IsHighlighted)
        {
        QColor color = vtkQtChartAxisOptions::lighter(brush.color());
        item->Polygon->setBrush(color);
        }
      else
        {
        item->Polygon->setBrush(brush);
        }
      }
    }
}

void vtkQtStackedChart::updateHighlights()
{
  if(!this->InModelChange && this->ChartArea)
    {
    this->layoutHighlights();
    }
}

void vtkQtStackedChart::layoutHighlights()
{
  if(this->Internal->Series.size() > 0)
    {
    // Restore the brush color for currently highlighted items.
    int i = 0;
    vtkQtStackedChartSeriesOptions *options = 0;
    QList<vtkQtStackedChartItem *>::Iterator iter = this->Internal->Series.begin();
    for( ; iter != this->Internal->Series.end(); ++iter, ++i)
      {
      if((*iter)->IsHighlighted && (*iter)->Polygon)
        {
        (*iter)->IsHighlighted = false;
        options = this->getStackedSeriesOptions(i);
        if(this->Options->isGradientDislpayed())
          {
          QRectF bounds = (*iter)->Polygon->boundingRect();
          float center = bounds.center().x();
          QLinearGradient gradient(center, bounds.top(),
              center, bounds.bottom());
          QColor color = options->getBrush().color();
          gradient.setColorAt(0.0, color);
          gradient.setColorAt(1.0, color.dark());
          (*iter)->Polygon->setBrush(QBrush(gradient));
          }
        else
          {
          (*iter)->Polygon->setBrush(options->getBrush());
          }
        }
      }

    QList<QGraphicsPolygonItem *>::Iterator highlight =
        this->Internal->Highlights.begin();
    for( ; highlight != this->Internal->Highlights.end(); ++highlight)
      {
      delete *highlight;
      }

    this->Internal->Highlights.clear();

    // Get the current selection from the selection model.
    if(!this->Selection->isSelectionEmpty())
      {
      vtkQtStackedChartItem *item = 0;
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
            options = this->getStackedSeriesOptions(i);
            item = this->Internal->Series[i];
            if(item->Polygon)
              {
              item->IsHighlighted = true;
              QColor color = vtkQtChartAxisOptions::lighter(
                  options->getBrush().color());
              item->Polygon->setBrush(color);
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
          // Add lightened polygons for the selected points.
          item = this->Internal->Series[jter->Series];
          options = this->getStackedSeriesOptions(jter->Series);
          QPolygonF seriesPoints = item->Polygon->polygon();
          int half = seriesPoints.size() / 2;
          vtkQtChartIndexRangeList::ConstIterator kter = jter->Points.begin();
          for( ; kter != jter->Points.end(); ++kter)
            {
            // Add the mid-point to the front if needed.
            QPolygonF selectedPoints;
            if(kter->first != 0)
              {
              selectedPoints.append(this->Internal->getMidPoint(
                  seriesPoints[kter->first - 1], seriesPoints[kter->first]));
              }

            // Add the selected points.
            int count = kter->second - kter->first + 1;
            selectedPoints << seriesPoints.mid(kter->first, count);

            // Add a midpoint to the end if needed. Add one for the
            // beginning of the bottom half as well.
            int bSecond = seriesPoints.size() - 1 - kter->first;
            int bFirst = bSecond - count + 1;
            if(kter->second < half - 1)
              {
              selectedPoints.append(this->Internal->getMidPoint(
                  seriesPoints[kter->second], seriesPoints[kter->second + 1]));
              selectedPoints.append(this->Internal->getMidPoint(
                  seriesPoints[bFirst - 1], seriesPoints[bFirst]));
              }

            // Add the selected points for the bottom half.
            selectedPoints << seriesPoints.mid(bFirst, count);

            // Add the final mid-point if needed.
            if(kter->first != 0)
              {
              selectedPoints.append(this->Internal->getMidPoint(
                  seriesPoints[bSecond], seriesPoints[bSecond + 1]));
              }

            // Add the highlight polygon.
            QGraphicsPolygonItem *polygon = new QGraphicsPolygonItem(
                selectedPoints, this->Contents, this->Contents->scene());
            this->Internal->Highlights.append(polygon);
            polygon->setZValue(item->Polygon->zValue() + 0.5);
            polygon->setPen(options->getPen());
            QColor color = vtkQtChartAxisOptions::lighter(
                options->getBrush().color());
            polygon->setBrush(color);
            }
          }
        }
      }
    }
}

void vtkQtStackedChart::addSeriesDomain(int series, int *seriesGroup)
{
  QList<QVariant> xDomain;
  QList<QVariant> yDomain = this->Model->getSeriesRange(series, 1);
  int points = this->Model->getNumberOfSeriesValues(series);
  for(int j = 0; j < points; j++)
    {
    xDomain.append(this->Model->getSeriesValue(series, j, 0));
    }

  // The y-axis domain is needed to separate the series groups.
  vtkQtChartSeriesDomain seriesDomain;
  seriesDomain.getXDomain().setDomain(xDomain);
  seriesDomain.getYDomain().setRange(yDomain);
  this->Internal->Domain.mergeDomain(seriesDomain, seriesGroup);

  // Add the series index to the domain group.
  this->Internal->Groups.insertSeries(series, *seriesGroup);
}

void vtkQtStackedChart::updateItemMap(int seriesGroup)
{
  QList<int> groupSeries = this->Internal->Groups.getGroup(seriesGroup);
  QList<int>::Iterator iter = groupSeries.begin();
  for(int i = 0; iter != groupSeries.end(); ++iter, ++i)
    {
    this->Internal->Series[*iter]->setMapping(seriesGroup, i);
    }
}

void vtkQtStackedChart::createTable(int seriesGroup)
{
  // Clear the group table and the associated y-axis domain.
  this->Internal->Groups.Tables[seriesGroup].clear();
  vtkQtChartSeriesDomain *seriesDomain =
      this->Internal->Domain.getDomain(seriesGroup);
  seriesDomain->getYDomain().clear();

  // Get the x-axis domain.
  bool isRange = false;
  QList<QVariant> xDomain = seriesDomain->getXDomain().getDomain(isRange);
  if(xDomain.size() > 0)
    {
    // Get the list of series for the group.
    QList<int> seriesList = this->Internal->Groups.getGroup(seriesGroup);
    QList<int>::Iterator iter = seriesList.begin();
    for(int i = 0; iter != seriesList.end(); ++iter, ++i)
      {
      // Add a table row for each of the series.
      int k = 0;
      QVariant xValue, yValue;
      this->Internal->Groups.Tables[seriesGroup].append(
          QVector<double>(xDomain.size(), 0.0));
      int points = this->Model->getNumberOfSeriesValues(*iter);
      for(int j = 0; j < points; j++, k++)
        {
        // Find the matching x-axis value in the domain.
        xValue = this->Model->getSeriesValue(*iter, j, 0);
        while(k < xDomain.size() && xValue != xDomain[k])
          {
          if(i > 0)
            {
            this->Internal->Groups.Tables[seriesGroup][i][k] =
                this->Internal->Groups.Tables[seriesGroup][i - 1][k];
            }

          k++;
          }

        if(k >= xDomain.size())
          {
          break;
          }

        // Get the y-axis value.
        yValue = this->Model->getSeriesValue(*iter, j, 1);
        this->Internal->Groups.Tables[seriesGroup][i][k] = yValue.toDouble();

        // Stack the series by adding the previous series value.
        if(i > 0)
          {
          this->Internal->Groups.Tables[seriesGroup][i][k] +=
              this->Internal->Groups.Tables[seriesGroup][i - 1][k];
          }
        }

      // Fill in any remaining table columns.
      if(i > 0)
        {
        for( ; k < xDomain.size(); k++)
          {
          this->Internal->Groups.Tables[seriesGroup][i][k] =
              this->Internal->Groups.Tables[seriesGroup][i - 1][k];
          }
        }
      }

    // Normalize the table if the user requested it.
    if(this->Options->isSumNormalized())
      {
      this->normalizeTable(seriesGroup);
      }

    this->calculateYDomain(seriesGroup);
    }
}

void vtkQtStackedChart::normalizeTable(int seriesGroup)
{
  if(this->Internal->Groups.Tables[seriesGroup].size() == 0)
    {
    return;
    }

  int last = this->Internal->Groups.Tables[seriesGroup].size() - 1;
  int count = this->Internal->Groups.Tables[seriesGroup][0].size();
  for(int j = 0; j < count; j++)
    {
    double total = this->Internal->Groups.Tables[seriesGroup][last][j];
    if(total > 0)
      {
      int i = 0;
      for( ; i < this->Internal->Groups.Tables[seriesGroup].size(); i++)
        {
        double fraction =
            this->Internal->Groups.Tables[seriesGroup][i][j] / total;
        this->Internal->Groups.Tables[seriesGroup][i][j] = 100.0 * fraction;
        }
      }
    }
}

void vtkQtStackedChart::calculateXDomain(int seriesGroup)
{
  vtkQtChartSeriesDomain *seriesDomain =
      this->Internal->Domain.getDomain(seriesGroup);
  seriesDomain->getXDomain().clear();

  // Get the list of series in the group and merge the domains.
  QList<int> seriesList = this->Internal->Groups.getGroup(seriesGroup);
  QList<int>::Iterator iter = seriesList.begin();
  for( ; iter != seriesList.end(); ++iter)
    {
    QList<QVariant> xDomain;
    int points = this->Model->getNumberOfSeriesValues(*iter);
    for(int j = 0; j < points; j++)
      {
      xDomain.append(this->Model->getSeriesValue(*iter, j, 0));
      }

    seriesDomain->getXDomain().mergeDomain(xDomain);
    }
}

void vtkQtStackedChart::calculateYDomain(int seriesGroup)
{
  vtkQtChartSeriesDomain *seriesDomain =
      this->Internal->Domain.getDomain(seriesGroup);
  seriesDomain->getYDomain().clear();

  // Use the first and last rows of the table to determine the minimum
  // and maximum respectively.
  if(this->Internal->Groups.Tables[seriesGroup].size() > 0)
    {
    double minimum = 0;
    double maximum = 0;
    QVector<double>::Iterator iter =
        this->Internal->Groups.Tables[seriesGroup][0].begin();
    QVector<double>::Iterator rowEnd =
        this->Internal->Groups.Tables[seriesGroup][0].end();
    QVector<double>::Iterator jter =
        this->Internal->Groups.Tables[seriesGroup].last().begin();
    if(iter != rowEnd)
      {
      minimum = *iter;
      maximum = *jter;
      ++iter;
      ++jter;
      }

    for( ; iter != rowEnd; ++iter, ++jter)
      {
      if(*iter < minimum)
        {
        minimum = *iter;
        }

      if(*jter > maximum)
        {
        maximum = *jter;
        }
      }

    QList<QVariant> yDomain;
    yDomain.append(QVariant(minimum));
    yDomain.append(QVariant(maximum));
    seriesDomain->getYDomain().setRange(yDomain);
    }
}

int vtkQtStackedChart::findClosestIndex(const QPolygonF &polygon,
    const QPointF &point) const
{
  // Only search the first half of the polygon, since the second half
  // of the points in the stacked chart series polygon do not add new
  // x-coordinates.
  QPolygonF::ConstIterator iter = polygon.begin();
  int half = polygon.size() / 2;
  for(int i = 0; i < half && iter != polygon.end(); ++iter, ++i)
    {
    if(point.x() <= iter->x())
      {
      if(i == 0)
        {
        return i;
        }

      float distance = (iter->x() - polygon[i - 1].x()) * 0.5;
      return iter->x() - point.x() < distance ? i : i - 1;
      }
    }

  return half - 1;
}


