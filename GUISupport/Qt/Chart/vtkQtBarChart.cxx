/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtBarChart.cxx

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

/// \file vtkQtBarChart.cxx
/// \date February 15, 2008

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#pragma warning(disable:4512)
#endif

#include "vtkQtBarChart.h"

#include "vtkQtBarChartOptions.h"
#include "vtkQtBarChartSeriesOptions.h"
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

#include <QBrush>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QList>
#include <QPen>


class vtkQtBarChartItem : public QGraphicsItem
{
public:
  enum {Type = vtkQtChart_BarChartItemType};

public:
  vtkQtBarChartItem(QGraphicsItem *parent=0);
  virtual ~vtkQtBarChartItem() {}

  virtual int type() const {return vtkQtBarChartItem::Type;}

  virtual QRectF boundingRect() const;
  virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
      QWidget *widget=0);

  QList<QGraphicsRectItem *> Bars;
  QList<int> Highlights;
  bool IsHighlighted;
};


class vtkQtBarChartInternal
{
public:
  vtkQtBarChartInternal();
  ~vtkQtBarChartInternal() {}

  int getSeries(QGraphicsRectItem *bar) const;

  QList<vtkQtBarChartItem *> Series;
  vtkQtChartAxisCornerDomain Domain;
  vtkQtChartSeriesDomainGroup Groups;
};


//-----------------------------------------------------------------------------
vtkQtBarChartItem::vtkQtBarChartItem(QGraphicsItem *item)
  : QGraphicsItem(item, item ? item->scene() : 0), Bars(), Highlights()
{
  this->IsHighlighted = false;
}

QRectF vtkQtBarChartItem::boundingRect() const
{
  return QRect(0, 0, 0, 0);
}

void vtkQtBarChartItem::paint(QPainter *,
    const QStyleOptionGraphicsItem *, QWidget *)
{
}


//-----------------------------------------------------------------------------
vtkQtBarChartInternal::vtkQtBarChartInternal()
  : Series(), Domain(), Groups()
{
  this->Domain.setHorizontalPreferences(false, false, true);
  this->Domain.setVerticalPreferences(true, true, false);
}

int vtkQtBarChartInternal::getSeries(QGraphicsRectItem *bar) const
{
  if(bar)
    {
    vtkQtBarChartItem *series = qgraphicsitem_cast<vtkQtBarChartItem *>(
        bar->parentItem());
    if(series)
      {
      return this->Series.indexOf(series);
      }
    }

  return -1;
}


//-----------------------------------------------------------------------------
vtkQtBarChart::vtkQtBarChart()
  : vtkQtChartSeriesLayer()
{
  this->Internal = new vtkQtBarChartInternal();
  this->Options = new vtkQtBarChartOptions(this);
  this->InModelChange = false;

  // Listen for option changes.
  this->connect(this->Options, SIGNAL(axesCornerChanged()),
      this, SLOT(handleAxesCornerChange()));
  this->connect(this->Options, SIGNAL(barFractionsChanged()),
      this, SIGNAL(layoutNeeded()));
  this->connect(this->Options, SIGNAL(outlineStyleChanged()),
      this, SLOT(handleOutlineChange()));

  // Listen for selection changes.
  this->connect(this->Selection,
      SIGNAL(selectionChanged(const vtkQtChartSeriesSelection &)),
      this, SLOT(updateHighlights()));
}

vtkQtBarChart::~vtkQtBarChart()
{
  delete this->Internal;
}

void vtkQtBarChart::setChartArea(vtkQtChartArea *area)
{
  vtkQtChartSeriesLayer::setChartArea(area);
  this->reset();
}

void vtkQtBarChart::setModel(vtkQtChartSeriesModel *model)
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

void vtkQtBarChart::setOptions(const vtkQtBarChartOptions &options)
{
  // Copy the new options. The chart will collapse the layout signals.
  this->Options->setAxesCorner(options.getAxesCorner());
  this->Options->setBarGroupFraction(options.getBarGroupFraction());
  this->Options->setBarWidthFraction(options.getBarWidthFraction());
  this->Options->setBinOutlineStyle(options.getOutlineStyle());
  this->Options->getHelpFormat()->setFormat(
      options.getHelpFormat()->getFormat());
}

vtkQtBarChartSeriesOptions *vtkQtBarChart::getBarSeriesOptions(
    int series) const
{
  return qobject_cast<vtkQtBarChartSeriesOptions *>(
      this->getSeriesOptions(series));
}

void vtkQtBarChart::getLayerDomain(vtkQtChartLayerDomain &domain) const
{
  domain.mergeDomain(this->Internal->Domain, this->Options->getAxesCorner());
}

void vtkQtBarChart::layoutChart(const QRectF &area)
{
  // Update the position.
  this->setPos(area.topLeft());
  if(this->Internal->Series.size() == 0)
    {
    return;
    }

  // Get the axis layer to get the axes and domain priority.
  vtkQtChartAxisLayer *layer = this->ChartArea->getAxisLayer();
  vtkQtChartAxis *xAxis = layer->getHorizontalAxis(this->Options->getAxesCorner());
  vtkQtChartAxis *yAxis = layer->getVerticalAxis(this->Options->getAxesCorner());

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
  minDistance *= this->Options->getBarGroupFraction();
  float barWidth = minDistance;

  // Get the list of series in the selected domain.
  QList<int> seriesList;
  if(seriesDomain)
    {
    seriesList = this->Internal->Groups.getGroup(domainIndex);
    }

  if(seriesList.size() > 0)
    {
    barWidth = this->Options->getBarWidthFraction();
    barWidth = (minDistance * barWidth) / (seriesList.size() - 1 + barWidth);
    }

  if(barWidth < 1)
    {
    barWidth = 1;
    }

  // Position and size the bar series. Skip the series if it is
  // invisible or invalid for the domain.
  float halfDistance = minDistance * 0.5;
  float base = yAxis->getZeroPixel();
  for(i = 0; i < this->Model->getNumberOfSeries(); i++)
    {
    vtkQtBarChartItem *series = this->Internal->Series[i];
    series->setVisible(seriesList.contains(i));
    if(!series->isVisible())
      {
      continue;
      }

    float xOffset = ((float)i *
        (barWidth / this->Options->getBarWidthFraction())) - halfDistance;
    for(int j = 0; j < this->Model->getNumberOfSeriesValues(i); j++)
      {
      float px = xAxis->getPixel(this->Model->getSeriesValue(i, j, 0));
      float py = yAxis->getPixel(this->Model->getSeriesValue(i, j, 1));
      if(py < base)
        {
        series->Bars[j]->setPos(px + xOffset, py);
        series->Bars[j]->setRect(0, 0, barWidth, base - py);
        }
      else
        {
        series->Bars[j]->setPos(px + xOffset, base);
        series->Bars[j]->setRect(0, 0, barWidth, py - base);
        }
      }
    }

  // Layout the highlights.
  this->layoutHighlights();
}

bool vtkQtBarChart::drawItemFilter(QGraphicsItem *item, QPainter *painter)
{
  // If the item is a series bar, clip it to the chart layer bounds.
  QGraphicsRectItem *bar = qgraphicsitem_cast<QGraphicsRectItem *>(item);
  if(bar && this->ChartArea)
    {
    QRectF bounds;
    this->ChartArea->getContentsSpace()->getChartLayerBounds(bounds);
    painter->setClipRect(bounds, Qt::IntersectClip);
    }

  return false;
}

bool vtkQtBarChart::getHelpText(const QPointF &point, QString &text)
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

    // Get the data from the model.
    const QList<vtkQtChartSeriesSelectionItem> &points = selection.getPoints();
    QList<vtkQtChartSeriesSelectionItem>::ConstIterator iter = points.begin();
    for( ; iter != points.end(); ++iter)
      {
      vtkQtChartIndexRangeList::ConstIterator jter = iter->Points.begin();
      for( ; jter != iter->Points.end(); ++jter)
        {
        for(int i = jter->first; i <= jter->second; i++)
          {
          if(!text.isEmpty())
            {
            text.append("\n\n");
            }

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

void vtkQtBarChart::getSeriesAt(const QPointF &point,
    vtkQtChartSeriesSelection &selection) const
{
  // Get the list of items from the scene. Search the list for series
  // rectangles.
  vtkQtChartIndexRangeList indexes;
  QList<QGraphicsItem *> list = this->scene()->items(point);
  QList<QGraphicsItem *>::Iterator iter = list.begin();
  for( ; iter != list.end(); ++iter)
    {
    // Make sure the rectangle is in a bar series.
    QGraphicsRectItem *bar = qgraphicsitem_cast<QGraphicsRectItem *>(*iter);
    int series = this->Internal->getSeries(bar);
    if(series != -1)
      {
      // Add the series to the selection.
      indexes.append(vtkQtChartIndexRange(series, series));
      }
    }

  selection.setSeries(indexes);
}

void vtkQtBarChart::getPointsAt(const QPointF &point,
    vtkQtChartSeriesSelection &selection) const
{
  // Get the list of items from the scene. Search the list for series
  // rectangles.
  QList<vtkQtChartSeriesSelectionItem> indexes;
  QList<QGraphicsItem *> list = this->scene()->items(point);
  QList<QGraphicsItem *>::Iterator iter = list.begin();
  for( ; iter != list.end(); ++iter)
    {
    // Make sure the rectangle is in a bar series.
    QGraphicsRectItem *bar = qgraphicsitem_cast<QGraphicsRectItem *>(*iter);
    int series = this->Internal->getSeries(bar);
    if(series != -1)
      {
      // Add the bar to the selection.
      vtkQtChartSeriesSelectionItem item(series);
      int index = this->Internal->Series[series]->Bars.indexOf(bar);
      item.Points.append(vtkQtChartIndexRange(index, index));
      indexes.append(item);
      }
    }

  selection.setPoints(indexes);
}

void vtkQtBarChart::getSeriesIn(const QRectF &area,
    vtkQtChartSeriesSelection &selection) const
{
  // Get the list of items from the scene. Search the list for series
  // rectangles.
  vtkQtChartIndexRangeList indexes;
  QList<QGraphicsItem *> list = this->scene()->items(area);
  QList<QGraphicsItem *>::Iterator iter = list.begin();
  for( ; iter != list.end(); ++iter)
    {
    // Make sure the rectangle is in a bar series.
    QGraphicsRectItem *bar = qgraphicsitem_cast<QGraphicsRectItem *>(*iter);
    int series = this->Internal->getSeries(bar);
    if(series != -1)
      {
      // Add the series to the selection.
      indexes.append(vtkQtChartIndexRange(series, series));
      }
    }

  selection.setSeries(indexes);
}

void vtkQtBarChart::getPointsIn(const QRectF &area,
    vtkQtChartSeriesSelection &selection) const
{
  // Get the list of items from the scene. Search the list for series
  // rectangles.
  QList<vtkQtChartSeriesSelectionItem> indexes;
  QList<QGraphicsItem *> list = this->scene()->items(area);
  QList<QGraphicsItem *>::Iterator iter = list.begin();
  for( ; iter != list.end(); ++iter)
    {
    // Make sure the rectangle is in a bar series.
    QGraphicsRectItem *bar = qgraphicsitem_cast<QGraphicsRectItem *>(*iter);
    int series = this->Internal->getSeries(bar);
    if(series != -1)
      {
      // Add the bar to the selection.
      vtkQtChartSeriesSelectionItem item(series);
      int index = this->Internal->Series[series]->Bars.indexOf(bar);
      item.Points.append(vtkQtChartIndexRange(index, index));
      indexes.append(item);
      }
    }

  selection.setPoints(indexes);
}

QRectF vtkQtBarChart::boundingRect() const
{
  return QRectF(0, 0, 0, 0);
}

void vtkQtBarChart::paint(QPainter *,
    const QStyleOptionGraphicsItem *, QWidget *)
{
}

void vtkQtBarChart::reset()
{
  // Make sure the selection model is notified of the change.
  this->InModelChange = true;
  this->Selection->beginModelReset();

  // Clean up the old view items.
  bool needsLayout = this->Internal->Series.size() > 0;
  QList<vtkQtBarChartItem *>::Iterator iter = this->Internal->Series.begin();
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

vtkQtChartSeriesOptions *vtkQtBarChart::createOptions(QObject *parentObject)
{
  return new vtkQtBarChartSeriesOptions(parentObject);
}

void vtkQtBarChart::setupOptions(vtkQtChartSeriesOptions *options)
{
  vtkQtBarChartSeriesOptions *seriesOptions =
      qobject_cast<vtkQtBarChartSeriesOptions *>(options);
  if(seriesOptions)
    {
    // Finish setting up the series options.
    if(this->Options->getOutlineStyle() == vtkQtBarChartOptions::Darker)
      {
      seriesOptions->setPen(seriesOptions->getBrush().color().dark());
      }
    else
      {
      seriesOptions->setPen(QColor(Qt::black));
      }

    // Listen for series options changes.
    this->connect(seriesOptions, SIGNAL(visibilityChanged(bool)),
        this, SLOT(handleSeriesVisibilityChange(bool)));
    this->connect(seriesOptions, SIGNAL(penChanged(const QPen &)),
        this, SLOT(handleSeriesPenChange(const QPen &)));
    this->connect(seriesOptions, SIGNAL(brushChanged(const QBrush &)),
        this, SLOT(handleSeriesBrushChange(const QBrush &)));
    }
}

void vtkQtBarChart::prepareSeriesInsert(int first, int last)
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

void vtkQtBarChart::insertSeries(int first, int last)
{
  if(this->ChartArea)
    {
    // Update the series indexes stored in the domain groups.
    this->Internal->Groups.prepareInsert(first, last);

    int i = first;
    bool signalDomain = false;
    for( ; first <= last; first++)
      {
      // Add an item for each series.
      vtkQtBarChartItem *series = new vtkQtBarChartItem(this->Contents);
      this->Internal->Series.insert(first, series);

      // Get the series options.
      vtkQtBarChartSeriesOptions *options = this->getBarSeriesOptions(first);

      // Add bars to the series for each series point. Get the x-axis
      // values at the same time for the x-axis domain.
      QGraphicsRectItem *bar = 0;
      int total = this->Model->getNumberOfSeriesValues(first);
      for(int j = 0; j < total; j++)
        {
        bar = new QGraphicsRectItem(series, series->scene());
        series->Bars.append(bar);

        // Set the drawing options for the series.
        bar->setPen(options->getPen());
        bar->setBrush(options->getBrush());
        }

      // Add the series domains to the chart domains.
      if(options->isVisible())
        {
        if(this->addSeriesDomain(first))
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

void vtkQtBarChart::startSeriesRemoval(int first, int last)
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
      delete this->Internal->Series.takeAt(last);
      }

    // Fix the z-order for any subsequent items.
    for( ; first < this->Internal->Series.size(); first++)
      {
      this->Internal->Series[first]->setZValue(first);
      }
    }
}

void vtkQtBarChart::finishSeriesRemoval(int first, int last)
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

void vtkQtBarChart::handleAxesCornerChange()
{
  if(this->Model && this->ChartArea)
    {
    emit this->rangeChanged();
    emit this->layoutNeeded();
    }
}

void vtkQtBarChart::handleOutlineChange()
{
  // Change the bar outline.
  if(this->Model && this->ChartArea)
    {
    QPen blackPen(Qt::black);
    vtkQtBarChartSeriesOptions *options = 0;
    int total = this->Model->getNumberOfSeries();
    for(int i = 0; i < total; i++)
      {
      options = this->getBarSeriesOptions(i);
      if(this->Options->getOutlineStyle() == vtkQtBarChartOptions::Darker)
        {
        options->setPen(options->getBrush().color().dark());
        }
      else
        {
        options->setPen(blackPen);
        }
      }
    }
}

void vtkQtBarChart::handleSeriesVisibilityChange(bool visible)
{
  // Get the series index from the options index.
  vtkQtBarChartSeriesOptions *options =
      qobject_cast<vtkQtBarChartSeriesOptions *>(this->sender());
  int series = this->getSeriesOptionsIndex(options);
  if(series >= 0 && series < this->Internal->Series.size())
    {
    if(visible)
      {
      // If the series is going to be visible, add to the domain.
      if(this->addSeriesDomain(series))
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
          }

        this->Internal->Groups.finishRemoval();
        emit this->rangeChanged();
        emit this->layoutNeeded();
        }
      }
    }
}

void vtkQtBarChart::handleSeriesPenChange(const QPen &pen)
{
  // Get the series index from the options index.
  vtkQtBarChartSeriesOptions *options =
      qobject_cast<vtkQtBarChartSeriesOptions *>(this->sender());
  int series = this->getSeriesOptionsIndex(options);
  if(series >= 0 && series < this->Internal->Series.size())
    {
    vtkQtBarChartItem *item = this->Internal->Series[series];
    QList<QGraphicsRectItem *>::Iterator iter = item->Bars.begin();
    for( ; iter != item->Bars.end(); ++iter)
      {
      (*iter)->setPen(pen);
      }
    }
}

void vtkQtBarChart::handleSeriesBrushChange(const QBrush &brush)
{
  // Get the series index from the options index.
  vtkQtBarChartSeriesOptions *options =
      qobject_cast<vtkQtBarChartSeriesOptions *>(this->sender());
  int series = this->getSeriesOptionsIndex(options);
  if(series >= 0 && series < this->Internal->Series.size())
    {
    QColor color = vtkQtChartAxisOptions::lighter(brush.color());
    vtkQtBarChartItem *item = this->Internal->Series[series];
    QList<QGraphicsRectItem *>::Iterator iter = item->Bars.begin();
    for(int i = 0; iter != item->Bars.end(); ++iter, ++i)
      {
      if(item->IsHighlighted || item->Highlights.contains(i))
        {
        (*iter)->setBrush(color);
        }
      else
        {
        (*iter)->setBrush(brush);
        }
      }
    }
}

void vtkQtBarChart::updateHighlights()
{
  if(!this->InModelChange && this->ChartArea)
    {
    this->layoutHighlights();
    }
}

void vtkQtBarChart::layoutHighlights()
{
  if(this->Internal->Series.size() > 0)
    {
    // Restore the bar color for currently highlighted items.
    int i = 0;
    vtkQtBarChartSeriesOptions *options = 0;
    QList<vtkQtBarChartItem *>::Iterator iter = this->Internal->Series.begin();
    for( ; iter != this->Internal->Series.end(); ++iter, ++i)
      {
      if((*iter)->IsHighlighted)
        {
        (*iter)->IsHighlighted = false;
        options = this->getBarSeriesOptions(i);
        QList<QGraphicsRectItem *>::Iterator jter = (*iter)->Bars.begin();
        for( ; jter != (*iter)->Bars.end(); ++jter)
          {
          (*jter)->setBrush(options->getBrush());
          }
        }
      else if((*iter)->Highlights.size() > 0)
        {
        options = this->getBarSeriesOptions(i);
        QList<int>::Iterator jter = (*iter)->Highlights.begin();
        for( ; jter != (*iter)->Highlights.end(); ++jter)
          {
          (*iter)->Bars[*jter]->setBrush(options->getBrush());
          }

        (*iter)->Highlights.clear();
        }
      }

    // Get the current selection from the selection model.
    if(!this->Selection->isSelectionEmpty())
      {
      vtkQtBarChartItem *item = 0;
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
            options = this->getBarSeriesOptions(i);
            item = this->Internal->Series[i];
            item->IsHighlighted = true;
            QColor color = vtkQtChartAxisOptions::lighter(
                options->getBrush().color());
            QList<QGraphicsRectItem *>::Iterator kter = item->Bars.begin();
            for( ; kter != item->Bars.end(); ++kter)
              {
              (*kter)->setBrush(color);
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
          options = this->getBarSeriesOptions(jter->Series);
          item = this->Internal->Series[jter->Series];
          QColor color = vtkQtChartAxisOptions::lighter(
              options->getBrush().color());
          vtkQtChartIndexRangeList::ConstIterator kter = jter->Points.begin();
          for( ; kter != jter->Points.end(); ++kter)
            {
            for(i = kter->first; i <= kter->second; i++)
              {
              item->Highlights.append(i);
              item->Bars[i]->setBrush(color);
              }
            }
          }
        }
      }
    }
}

bool vtkQtBarChart::addSeriesDomain(int series)
{
  QList<QVariant> xDomain;
  QList<QVariant> yDomain = this->Model->getSeriesRange(series, 1);
  bool yIsList = yDomain.isEmpty();
  int points = this->Model->getNumberOfSeriesValues(series);
  for(int j = 0; j < points; j++)
    {
    xDomain.append(this->Model->getSeriesValue(series, j, 0));
    if(yIsList)
      {
      yDomain.append(this->Model->getSeriesValue(series, j, 1));
      }
    }

  vtkQtChartSeriesDomain seriesDomain;
  seriesDomain.getXDomain().setDomain(xDomain);
  if(yIsList)
    {
    seriesDomain.getYDomain().setDomain(yDomain);
    }
  else
    {
    seriesDomain.getYDomain().setRange(yDomain);
    }

  int seriesGroup = -1;
  bool changed = this->Internal->Domain.mergeDomain(seriesDomain, &seriesGroup);

  // Add the series index to the domain group.
  this->Internal->Groups.insertSeries(series, seriesGroup);
  return changed;
}

void vtkQtBarChart::calculateDomain(int seriesGroup)
{
  // Clear the current domain information.
  vtkQtChartSeriesDomain *domain =
      this->Internal->Domain.getDomain(seriesGroup);
  domain->getXDomain().clear();
  domain->getYDomain().clear();

  // Get the list of series in the group.
  QList<int> list = this->Internal->Groups.getGroup(seriesGroup);
  for(QList<int>::Iterator iter = list.begin(); iter != list.end(); ++iter)
    {
    vtkQtBarChartSeriesOptions *options = this->getBarSeriesOptions(*iter);
    if(options && !options->isVisible())
      {
      continue;
      }

    QList<QVariant> xDomain;
    QList<QVariant> yDomain = this->Model->getSeriesRange(*iter, 1);
    bool yIsList = yDomain.isEmpty();
    int points = this->Model->getNumberOfSeriesValues(*iter);
    for(int j = 0; j < points; j++)
      {
      xDomain.append(this->Model->getSeriesValue(*iter, j, 0));
      if(yIsList)
        {
        yDomain.append(this->Model->getSeriesValue(*iter, j, 1));
        }
      }

    domain->getXDomain().mergeDomain(xDomain);
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


