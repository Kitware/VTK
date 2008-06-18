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

#include "vtkQtStatisticalBoxChart.h"

#include "vtkQtStatisticalBoxChartOptions.h"
#include "vtkQtStatisticalBoxChartSeriesOptions.h"
#include "vtkQtChartAxis.h"
#include "vtkQtChartAxisCornerDomain.h"
#include "vtkQtChartAxisDomain.h"
#include "vtkQtChartAxisLayer.h"
#include "vtkQtChartAxisOptions.h"
#include "vtkQtChartContentsArea.h"
#include "vtkQtChartContentsSpace.h"
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

class vtkQtStatisticalBoxChartItem : public QGraphicsItem
{
public:
  enum {Type = vtkQtChart_StatisticalBoxChartItemType};

public:
  vtkQtStatisticalBoxChartItem(QGraphicsItem *parent=0);
  virtual ~vtkQtStatisticalBoxChartItem() {}

  virtual int type() const {return vtkQtStatisticalBoxChartItem::Type;}
  virtual QRectF boundingRect() const;
  virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
      QWidget *widget=0);

  QGraphicsRectItem* Box;
  QGraphicsLineItem* WhiskerLowPoint;
  QGraphicsLineItem* WhiskerLowLine;
  QGraphicsLineItem* WhiskerHighPoint;
  QGraphicsLineItem* WhiskerHighLine;
  QGraphicsLineItem* MedianPoint;

  QList<QGraphicsEllipseItem *> Outliers;

  QList<int> Highlights;
  bool IsHighlighted;
};

class vtkQtStatisticalBoxChartInternal
{
public:
  vtkQtStatisticalBoxChartInternal();

  ~vtkQtStatisticalBoxChartInternal() {};

  int getSeries(QGraphicsRectItem *bar) const;

  QList<vtkQtStatisticalBoxChartItem *> Series;
  vtkQtChartAxisCornerDomain Domain;
  vtkQtChartSeriesDomainGroup Groups;
};

//-----------------------------------------------------------------------------
vtkQtStatisticalBoxChartItem::vtkQtStatisticalBoxChartItem(QGraphicsItem *item) :
  QGraphicsItem(item, item ? item->scene() : 0), Outliers(),
      Highlights()
{
  this->IsHighlighted = false;
  this->Box = 0;
  this->WhiskerLowPoint = 0;
  this->WhiskerLowLine = 0;
  this->WhiskerHighPoint = 0;
  this->WhiskerHighLine = 0;
  this->MedianPoint = 0;
}

QRectF vtkQtStatisticalBoxChartItem::boundingRect() const
{
  return QRect(0, 0, 0, 0);
}

void vtkQtStatisticalBoxChartItem::paint(QPainter *,
    const QStyleOptionGraphicsItem *, QWidget *)
{
}

//-----------------------------------------------------------------------------
vtkQtStatisticalBoxChartInternal::vtkQtStatisticalBoxChartInternal() :
  Series(), Domain(), Groups()
{
  this->Domain.setHorizontalPreferences(false, false, true);
  this->Domain.setVerticalPreferences(true, true, false);
}

int vtkQtStatisticalBoxChartInternal::getSeries(QGraphicsRectItem *bar) const
{
  if (bar)
    {
    vtkQtStatisticalBoxChartItem *series =
        qgraphicsitem_cast<vtkQtStatisticalBoxChartItem *>(bar->parentItem());
    if (series)
      {
      return this->Series.indexOf(series);
      }
    }

  return -1;
}

//-----------------------------------------------------------------------------
vtkQtStatisticalBoxChart::vtkQtStatisticalBoxChart() :
  vtkQtChartSeriesLayer()
{
  this->Internal = new vtkQtStatisticalBoxChartInternal();
  this->Options = new vtkQtStatisticalBoxChartOptions(this);
  this->InModelChange = false;

  // Listen for option changes.
  this->connect(this->Options, SIGNAL(axesCornerChanged()), this,
      SLOT(handleAxesCornerChange()));
  this->connect(this->Options, SIGNAL(barFractionsChanged()), this,
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
  this->Options->setBarGroupFraction(options.getBarGroupFraction());
  this->Options->setBarWidthFraction(options.getBarWidthFraction());
  this->Options->setBinOutlineStyle(options.getOutlineStyle());
}

vtkQtStatisticalBoxChartSeriesOptions *vtkQtStatisticalBoxChart::getBarSeriesOptions(
    int series) const
{
  return qobject_cast<vtkQtStatisticalBoxChartSeriesOptions *>(this->getSeriesOptions(series));
}

void vtkQtStatisticalBoxChart::getLayerDomain(vtkQtChartLayerDomain &domain) const
{
  domain.mergeDomain(this->Internal->Domain, this->Options->getAxesCorner());
}

void vtkQtStatisticalBoxChart::layoutChart(const QRectF &area)
{

  // Update the position.
  this->setPos(area.topLeft());
  if (this->Internal->Series.size() == 0)
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
  if (seriesDomain)
    {
    domain = seriesDomain->getXDomain().getDomain(isRange);
    for (; i < domain.size() - 1; i++)
      {
      float distance = qAbs<float>(xAxis->getPixel(domain[i + 1])
          - xAxis->getPixel(domain[i]));
      if (i == 0 || distance < minDistance)
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
  if (seriesDomain)
    {
    seriesList = this->Internal->Groups.getGroup(domainIndex);
    }

  if (seriesList.size() > 0)
    {
    barWidth = this->Options->getBarWidthFraction();
    barWidth = (minDistance * barWidth) / (seriesList.size() - 1 + barWidth);
    }

  if (barWidth < 1)
    {
    barWidth = 1;
    }

  // Position and size the bar series. Skip the series if it is
  // invisible or invalid for the domain.
  float halfDistance = minDistance * 0.5;
  //  float base = yAxis->getZeroPixel();
  for (i = 0; i < this->Model->getNumberOfSeries(); i++)
    {

    vtkQtStatisticalBoxChartItem *series = this->Internal->Series[i];
    series->setVisible(seriesList.contains(i));
    if (!series->isVisible())
      {
      continue;
      }

    float xOffset = ((float)i * (barWidth
        / this->Options->getBarWidthFraction())) - halfDistance;

    if (this->Model->getNumberOfSeriesValues(i) >= 5)
      {

      QVariant min = this->Model->getSeriesValue(i, 0, 1);
      QVariant lowerQuartile = this->Model->getSeriesValue(i, 1, 1);
      QVariant median = this->Model->getSeriesValue(i, 2, 1);
      QVariant upperQuartile = this->Model->getSeriesValue(i, 3, 1);
      QVariant max = this->Model->getSeriesValue(i, 4, 1);

      QList<QVariant> outliers;
      for (int j=5; j<this->Model->getNumberOfSeriesValues(i); j++)
        {
        outliers.append(this->Model->getSeriesValue(i, j, 1));
        }

      float py1 = yAxis->getPixel(lowerQuartile);
      float py2 = yAxis->getPixel(upperQuartile);

      float px = xAxis->getPixel(this->Model->getSeriesValue(i, 0, 0));

      series->Box->setPos(px + xOffset, py1);
      series->Box->setRect(0, 0, barWidth, py2-py1);

      // Add in median
      float medX1 = px+xOffset;
      float medX2 = px+xOffset+barWidth;
      float medY1 = yAxis->getPixel(median);
      float medY2 = yAxis->getPixel(median);
      series->MedianPoint->setLine(medX1, medY1, medX2, medY2);

      // Add in lower whisker point
      float lPtX1 = px+xOffset;
      float lPtX2 = px+xOffset+barWidth;
      float lPtY1 = yAxis->getPixel(min);
      float lPtY2 = yAxis->getPixel(min);
      series->WhiskerLowPoint->setLine(lPtX1, lPtY1, lPtX2, lPtY2);

      // Add in lower whisker line
      float lLnX1 = px+xOffset+barWidth/2.0;
      float lLnX2 = px+xOffset+barWidth/2.0;
      float lLnY1 = yAxis->getPixel(min);
      float lLnY2 = yAxis->getPixel(lowerQuartile);
      series->WhiskerLowLine->setLine(lLnX1, lLnY1, lLnX2, lLnY2);

      // Add in high whisker point
      float hPtX1 = px+xOffset;
      float hPtX2 = px+xOffset+barWidth;
      float hPtY1 = yAxis->getPixel(max);
      float hPtY2 = yAxis->getPixel(max);
      series->WhiskerHighPoint->setLine(hPtX1, hPtY1, hPtX2, hPtY2);

      // Add in lower whisker line
      float hLnX1 = px+xOffset+barWidth/2.0;
      float hLnX2 = px+xOffset+barWidth/2.0;
      float hLnY1 = yAxis->getPixel(upperQuartile);
      float hLnY2 = yAxis->getPixel(max);
      series->WhiskerHighLine->setLine(hLnX1, hLnY1, hLnX2, hLnY2);

      // Add in outliers
      for (int j=0; j<outliers.size(); j++)
        {
        float outlierPy1 = yAxis->getPixel(outliers[j])-5;
        float outlierPy2 = yAxis->getPixel(outliers[j])+5;

        float outlierPx = xAxis->getPixel(this->Model->getSeriesValue(i, 0, 0))
            +barWidth/2.0 - 5.0;
        series->Outliers[j]->setPos(outlierPx + xOffset, outlierPy1);
        series->Outliers[j]->setRect(0, 0, 10, outlierPy2-outlierPy1);
        }

      }
    else
      {
      }

    }

  // Layout the highlights.
  this->layoutHighlights();
}

bool vtkQtStatisticalBoxChart::drawItemFilter(QGraphicsItem *item,
    QPainter *painter)
{
  // If the item is a series bar, clip it to the chart layer bounds.
  QGraphicsRectItem *bar = qgraphicsitem_cast<QGraphicsRectItem *>(item);
  if (bar && this->ChartArea)
    {
    QRectF bounds;
    this->ChartArea->getContentsSpace()->getChartLayerBounds(bounds);
    painter->setClipRect(bounds, Qt::IntersectClip);
    }

  return false;
}

bool vtkQtStatisticalBoxChart::getHelpText(const QPointF &, QString &)
{
  // TODO
  return false;
}

QRectF vtkQtStatisticalBoxChart::boundingRect() const
{
  return QRectF(0, 0, 0, 0);
}

void vtkQtStatisticalBoxChart::paint(QPainter *,
    const QStyleOptionGraphicsItem *, QWidget *)
{
}

void vtkQtStatisticalBoxChart::reset()
{
  // Make sure the selection model is notified of the change.
  this->InModelChange = true;
  this->Selection->beginModelReset();

  // Clean up the old view items.
  bool needsLayout = this->Internal->Series.size() > 0;
  QList<vtkQtStatisticalBoxChartItem *>::Iterator iter =
      this->Internal->Series.begin();
  for (; iter != this->Internal->Series.end(); ++iter)
    {
    delete *iter;
    }

  this->Internal->Series.clear();
  this->Internal->Domain.clear();
  this->Internal->Groups.clear();

  // Add items for the new model.
  if (this->Model && this->ChartArea)
    {
    int total = this->Model->getNumberOfSeries();
    if (total > 0)
      {
      if (needsLayout)
        {
        needsLayout = false;
        emit this->rangeChanged();
        }

      this->insertSeries(0, total - 1);
      }
    }

  if (needsLayout)
    {
    emit this->rangeChanged();
    emit this->layoutNeeded();
    }

  // Notify the slection model that the reset is complete, which may
  // generate a selection changed signal.
  this->Selection->endModelReset();
  this->InModelChange = false;
}

vtkQtChartSeriesOptions *vtkQtStatisticalBoxChart::createOptions(
    QObject *parentObject)
{
  return new vtkQtStatisticalBoxChartSeriesOptions(parentObject);
}

void vtkQtStatisticalBoxChart::setupOptions(vtkQtChartSeriesOptions *options)
{
  vtkQtStatisticalBoxChartSeriesOptions *seriesOptions = qobject_cast<
      vtkQtStatisticalBoxChartSeriesOptions *>(options);
  if (seriesOptions)
    {
    // Finish setting up the series options.
    if (this->Options->getOutlineStyle()
        == vtkQtStatisticalBoxChartOptions::Darker)
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

   }
}

void vtkQtStatisticalBoxChart::prepareSeriesInsert(int first, int last)
{
  if (this->ChartArea)
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
  if (this->ChartArea)
    {
    // Update the series indexes stored in the domain groups.
    this->Internal->Groups.prepareInsert(first, last);

    int i = first;
    bool signalDomain = false;
    for (; first <= last; first++)
      {
      // Add an item for each series.
      vtkQtStatisticalBoxChartItem *series = new vtkQtStatisticalBoxChartItem(this->Contents);
      this->Internal->Series.insert(first, series);

      // Get the series options.
      vtkQtStatisticalBoxChartSeriesOptions *options =
          this->getBarSeriesOptions(first);

      // Create box
      series->Box = new QGraphicsRectItem(series, series->scene());
      // Set the drawing options for the series.
      series->Box->setPen(options->getPen());
      //      series->Box->setBrush(options->getBrush());

      // Create median point, and whisker points and lines
      series->WhiskerLowPoint = new QGraphicsLineItem(series, series->scene());
      series->WhiskerLowLine = new QGraphicsLineItem(series, series->scene());
      series->WhiskerHighPoint = new QGraphicsLineItem(series, series->scene());
      series->WhiskerHighLine = new QGraphicsLineItem(series, series->scene());
      series->MedianPoint = new QGraphicsLineItem(series, series->scene());

      // Set the drawing options for the whiskers
      series->WhiskerLowPoint->setPen(options->getPen());
      series->WhiskerLowLine->setPen(options->getPen());
      series->WhiskerHighPoint->setPen(options->getPen());
      series->WhiskerHighLine->setPen(options->getPen());
      series->MedianPoint->setPen(options->getPen());

      QGraphicsEllipseItem* outlier = 0;
      int numOutliers = this->Model->getNumberOfSeriesValues(first) - 5;
      for (int j=0; j<numOutliers; j++)
        {
        outlier = new QGraphicsEllipseItem(series, series->scene());
        series->Outliers.append(outlier);
        outlier->setPen(options->getPen());
        }

      // Add the series domains to the chart domains.
      if (options->isVisible())
        {
        if (this->addSeriesDomain(first))
          {
          signalDomain = true;
          }
        }
      }

    // Fix up the z-order for the new items and any subsequent items.
    for (; i < this->Internal->Series.size(); i++)
      {
      this->Internal->Series[i]->setZValue(i);
      }

    if (signalDomain)
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
  if (this->ChartArea)
    {
    // Notify the selection model of the change. The selection will be
    // adjusted for the changes in this call so it can be layed out
    // when the changes are completed.
    this->InModelChange = true;
    this->Selection->beginRemoveSeries(first, last);

    // Remove each of the series items.
    for (; last >= first; last--)
      {
      delete this->Internal->Series.takeAt(last);
      }

    // Fix the z-order for any subsequent items.
    for (; first < this->Internal->Series.size(); first++)
      {
      this->Internal->Series[first]->setZValue(first);
      }
    }
}

void vtkQtStatisticalBoxChart::finishSeriesRemoval(int first, int last)
{
  if (this->ChartArea)
    {
    // Find which groups need to be re-calculated
    QList<int> groups;
    QList<int>::Iterator iter;
    for (int i = first; i <= last; i++)
      {
      int index = this->Internal->Groups.removeSeries(i);
      if (index != -1)
        {
        // Add the group indexes in reverse order.
        bool doAdd = true;
        for (iter = groups.begin(); iter != groups.end(); ++iter)
          {
          if (index > *iter)
            {
            doAdd = false;
            groups.insert(iter, index);
            break;
            }
          else if (index == *iter)
            {
            doAdd = false;
            break;
            }
          }

        if (doAdd)
          {
          groups.append(index);
          }
        }
      }

    for (iter = groups.begin(); iter != groups.end(); ++iter)
      {
      if (this->Internal->Groups.getNumberOfSeries(*iter) == 0)
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
    if (groups.size() > 0)
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
  if (this->Model && this->ChartArea)
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
    QPen blackPen(Qt::black);
    vtkQtStatisticalBoxChartSeriesOptions *options = 0;
    int total = this->Model->getNumberOfSeries();
    for (int i = 0; i < total; i++)
      {
      options = this->getBarSeriesOptions(i);
      if (this->Options->getOutlineStyle()
          == vtkQtStatisticalBoxChartOptions::Darker)
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

void vtkQtStatisticalBoxChart::handleSeriesVisibilityChange(bool visible)
{
  // Get the series index from the options index.
  vtkQtStatisticalBoxChartSeriesOptions *options = qobject_cast<
      vtkQtStatisticalBoxChartSeriesOptions *>(this->sender());
  int series = this->getSeriesOptionsIndex(options);
  if (series >= 0 && series < this->Internal->Series.size())
    {
    if (visible)
      {
      // If the series is going to be visible, add to the domain.
      if (this->addSeriesDomain(series))
        {
        emit this->rangeChanged();
        }

      emit this->layoutNeeded();
      }
    else
      {
      int seriesGroup = this->Internal->Groups.removeSeries(series);
      if (seriesGroup != -1)
        {
        // If the group is empty, remove the domain.
        if (this->Internal->Groups.getNumberOfSeries(seriesGroup) == 0)
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

void vtkQtStatisticalBoxChart::updateHighlights()
{
  if (!this->InModelChange && this->ChartArea)
    {
    this->layoutHighlights();
    }
}

void vtkQtStatisticalBoxChart::layoutHighlights()
{

}

bool vtkQtStatisticalBoxChart::addSeriesDomain(int series)
{
  QList<QVariant> xDomain;
  QList<QVariant> yDomain = this->Model->getSeriesRange(series, 1);
  bool yIsList = yDomain.isEmpty();
  int points = this->Model->getNumberOfSeriesValues(series);
  for (int j = 0; j < points; j++)
    {
    xDomain.append(this->Model->getSeriesValue(series, j, 0));
    if (yIsList)
      {
      yDomain.append(this->Model->getSeriesValue(series, j, 1));
      }
    }

  vtkQtChartSeriesDomain seriesDomain;
  seriesDomain.getXDomain().setDomain(xDomain);
  if (yIsList)
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

void vtkQtStatisticalBoxChart::calculateDomain(int seriesGroup)
{
  // Clear the current domain information.
  vtkQtChartSeriesDomain *domain =
      this->Internal->Domain.getDomain(seriesGroup);
  domain->getXDomain().clear();
  domain->getYDomain().clear();

  // Get the list of series in the group.
  QList<int> list = this->Internal->Groups.getGroup(seriesGroup);
  for (QList<int>::Iterator iter = list.begin(); iter != list.end(); ++iter)
    {
    vtkQtStatisticalBoxChartSeriesOptions *options =
        this->getBarSeriesOptions(*iter);
    if (options && !options->isVisible())
      { 
      continue;
      }

    QList<QVariant> xDomain;
    QList<QVariant> yDomain = this->Model->getSeriesRange(*iter, 1);
    bool yIsList = yDomain.isEmpty();
    int points = this->Model->getNumberOfSeriesValues(*iter);
    for (int j = 0; j < points; j++)
      {
      xDomain.append(this->Model->getSeriesValue(*iter, j, 0));
      if (yIsList)
        {
        yDomain.append(this->Model->getSeriesValue(*iter, j, 1));
        }
      }

    domain->getXDomain().mergeDomain(xDomain);
    if (yIsList)
      {
      domain->getYDomain().mergeDomain(yDomain);
      }
    else
      {
      domain->getYDomain().mergeRange(yDomain);
      }
    }
}

