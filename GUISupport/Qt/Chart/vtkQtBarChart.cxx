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
#endif

#include "vtkQtBarChart.h"

#include "vtkMath.h"
#include "vtkQtBarChartOptions.h"
#include "vtkQtChartArea.h"
#include "vtkQtChartAxisCornerDomain.h"
#include "vtkQtChartAxisDomain.h"
#include "vtkQtChartAxis.h"
#include "vtkQtChartAxisLayer.h"
#include "vtkQtChartAxisOptions.h"
#include "vtkQtChartBar.h"
#include "vtkQtChartBarLocator.h"
#include "vtkQtChartColors.h"
#include "vtkQtChartContentsArea.h"
#include "vtkQtChartContentsSpace.h"
#include "vtkQtChartHelpFormatter.h"
#include "vtkQtChartIndexRangeList.h"
#include "vtkQtChartLayerDomain.h"
#include "vtkQtChartSeriesColors.h"
#include "vtkQtChartSeriesDomainGroup.h"
#include "vtkQtChartSeriesDomain.h"
#include "vtkQtChartSeriesModel.h"
#include "vtkQtChartSeriesOptions.h"
#include "vtkQtChartSeriesSelection.h"
#include "vtkQtChartSeriesSelectionModel.h"

#include <QBrush>
#include <QStyleOptionGraphicsItem>
#include <QList>
#include <QPen>

#include <math.h>

//copied from vtkQtChartSeriesModelRange.cxx
#ifndef isnan
// This is compiler specific not platform specific: MinGW doesn't need that.
# if defined(_MSC_VER) || defined(__BORLANDC__)
#  include <float.h>
#  define isnan(x) _isnan(x)
# endif
#endif

class vtkQtBarChartSeries
{
public:
  vtkQtBarChartSeries();
  ~vtkQtBarChartSeries();

  void updateSeries(int series);

public:
  QList<QRectF *> Bars;
  QList<vtkQtChartBar *> Bounds;
  QList<int> Highlights;
  bool IsHighlighted;
};


class vtkQtBarChartDomainGroup : public vtkQtChartSeriesDomainGroup
{
public:
  vtkQtBarChartDomainGroup();
  virtual ~vtkQtBarChartDomainGroup() {}

  virtual void clear();

protected:
  virtual void insertGroup(int group);
  virtual void removeGroup(int group);

public:
  QList<QList<vtkQtChartBar *> > Lists;
};


class vtkQtBarChartInternal
{
public:
  vtkQtBarChartInternal();
  ~vtkQtBarChartInternal();

  QList<vtkQtBarChartSeries *> Series;
  vtkQtChartAxisCornerDomain Domain;
  vtkQtBarChartDomainGroup Groups;
  vtkQtChartBarLocator BarTree;
  QRectF Bounds;
  int CurrentGroup;
};


//-----------------------------------------------------------------------------
vtkQtBarChartSeries::vtkQtBarChartSeries()
  : Bars(), Bounds(), Highlights()
{
  this->IsHighlighted = false;
}

vtkQtBarChartSeries::~vtkQtBarChartSeries()
{
  QList<QRectF *>::Iterator iter = this->Bars.begin();
  for( ; iter != this->Bars.end(); ++iter)
    {
    delete *iter;
    }

  QList<vtkQtChartBar *>::Iterator jter = this->Bounds.begin();
  for( ; jter != this->Bounds.end(); ++jter)
    {
    delete *jter;
    }
}

void vtkQtBarChartSeries::updateSeries(int series)
{
  QList<vtkQtChartBar *>::Iterator iter = this->Bounds.begin();
  for( ; iter != this->Bounds.end(); ++iter)
    {
    (*iter)->setSeries(series);
    }
}


//-----------------------------------------------------------------------------
vtkQtBarChartDomainGroup::vtkQtBarChartDomainGroup()
  : vtkQtChartSeriesDomainGroup(true), Lists()
{
}

void vtkQtBarChartDomainGroup::clear()
{
  vtkQtChartSeriesDomainGroup::clear();
  this->Lists.clear();
}

void vtkQtBarChartDomainGroup::insertGroup(int group)
{
  vtkQtChartSeriesDomainGroup::insertGroup(group);
  this->Lists.insert(group, QList<vtkQtChartBar *>());
}

void vtkQtBarChartDomainGroup::removeGroup(int group)
{
  vtkQtChartSeriesDomainGroup::removeGroup(group);
  this->Lists.removeAt(group);
}


//-----------------------------------------------------------------------------
vtkQtBarChartInternal::vtkQtBarChartInternal()
  : Series(), Domain(), Groups(), BarTree(), Bounds()
{
  this->CurrentGroup = -1;

  this->Domain.setHorizontalPreferences(false, false, true);
  this->Domain.setVerticalPreferences(true, true, false);
}

vtkQtBarChartInternal::~vtkQtBarChartInternal()
{
  QList<vtkQtBarChartSeries *>::Iterator iter = this->Series.begin();
  for( ; iter != this->Series.end(); ++iter)
    {
    delete *iter;
    }
}


//-----------------------------------------------------------------------------
vtkQtBarChart::vtkQtBarChart()
  : vtkQtChartSeriesLayer(false)
{
  this->Internal = new vtkQtBarChartInternal();
  this->Options = new vtkQtBarChartOptions(this);
  this->InModelChange = false;
  this->BuildNeeded = false;

  // Listen for options changes.
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
  this->Options->setOutlineStyle(options.getOutlineStyle());
  this->Options->getHelpFormat()->setFormat(
      options.getHelpFormat()->getFormat());
}

QPixmap vtkQtBarChart::getSeriesIcon(int series) const
{
  // Fill in the pixmap background.
  QPixmap icon(16, 16);
  icon.fill(QColor(255, 255, 255, 0));

  // Get the options for the series.
  vtkQtChartSeriesOptions *options = this->getSeriesOptions(series);
  if(options)
    {
    // Fill some bars with the series color(s).
    vtkQtChartSeriesColors *colors = options->getSeriesColors();
    QPainter painter(&icon);
    QPen pen = options->getPen();
    if(this->Options->getOutlineStyle() == vtkQtBarChartOptions::Darker)
      {
      pen.setColor(options->getBrush().color().dark());
      }
    else
      {
      pen.setColor(Qt::black);
      }
    painter.setPen(pen);
    if(colors)
      {
      int total = this->Model->getNumberOfSeriesValues(series);
      QPen barPen = options->getPen();
      QBrush barColor = options->getBrush();
      colors->getBrush(0, total, barColor);
      painter.setBrush(barColor);
      if(this->Options->getOutlineStyle() == vtkQtBarChartOptions::Darker)
        {
        barPen.setColor(barColor.color().dark());
        painter.setPen(barPen);
        }

      painter.drawRect(1, 4, 3, 10);
      if(total > 0)
        {
        colors->getBrush(total / 2, total, barColor);
        painter.setBrush(barColor);
        if(this->Options->getOutlineStyle() == vtkQtBarChartOptions::Darker)
          {
          barPen.setColor(barColor.color().dark());
          painter.setPen(barPen);
          }
        }

      painter.drawRect(6, 1, 3, 13);
      if(total > 0)
        {
        colors->getBrush(total - 1, total, barColor);
        painter.setBrush(barColor);
        if(this->Options->getOutlineStyle() == vtkQtBarChartOptions::Darker)
          {
          barPen.setColor(barColor.color().dark());
          painter.setPen(barPen);
          }
        }

      painter.drawRect(11, 6, 3, 8);
      }
    else
      {
      painter.setBrush(options->getBrush());
      painter.drawRect(1, 4, 3, 10);
      painter.drawRect(6, 1, 3, 13);
      painter.drawRect(11, 6, 3, 8);
      }
    }

  return icon;
}

void vtkQtBarChart::getLayerDomain(vtkQtChartLayerDomain &domain) const
{
  domain.mergeDomain(this->Internal->Domain, this->Options->getAxesCorner());
}

void vtkQtBarChart::layoutChart(const QRectF &area)
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
  vtkQtChartLayer::AxesCorner corner = this->Options->getAxesCorner();
  vtkQtChartAxis *xAxis = layer->getHorizontalAxis(corner);
  vtkQtChartAxis *yAxis = layer->getVerticalAxis(corner);

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
  int index = 0;
  QRectF *bar = 0;
  float halfDistance = minDistance * 0.5;
  float base = yAxis->getZeroPixel();
  QList<int>::Iterator iter = seriesList.begin();
  for( ; iter != seriesList.end(); ++iter)
    {
    vtkQtBarChartSeries *series = this->Internal->Series[*iter];
    float xOffset = ((float)index *
        (barWidth / this->Options->getBarWidthFraction())) - halfDistance;
    index++;
    int total = this->Model->getNumberOfSeriesValues(*iter);
    for(int j = 0; j < total; j++)
      {
      float px = xAxis->getPixel(this->Model->getSeriesValue(*iter, j, 0));
      float py = yAxis->getPixel(this->Model->getSeriesValue(*iter, j, 1));
      bar = series->Bars[j];
      if (isnan(py))
        {
        bar->setRect(px + xOffset, base, barWidth, 0);
        }
      else if(py < base)
        {
        bar->setRect(px + xOffset, py, barWidth, base - py);
        }
      else
        {
        bar->setRect(px + xOffset, base, barWidth, py - base);
        }

      series->Bounds[j]->setBar(bar->adjusted(-0.5, -0.5, 0.5, 0.5));
      }
    }

  // Update the bar tree.
  if(seriesDomain)
    {
    if(this->ChartArea->isInteractivelyResizing())
      {
      this->BuildNeeded = true;
      }
    else
      {
      this->buildBarTree(domainIndex);
      }
    }
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
    const QMap<int, vtkQtChartIndexRangeList> &points = selection.getPoints();
    QMap<int, vtkQtChartIndexRangeList>::ConstIterator iter = points.begin();
    for( ; iter != points.end(); ++iter)
      {
      vtkQtChartIndexRange *range = iter->getFirst();
      while(range)
        {
        for(int i = range->getFirst(); i <= range->getSecond(); i++)
          {
          if(!text.isEmpty())
            {
            text.append("\n\n");
            }

          QStringList args;
          args.append(xAxis->formatValue(
              this->Model->getSeriesValue(iter.key(), i, 0)));
          args.append(yAxis->formatValue(
              this->Model->getSeriesValue(iter.key(), i, 1)));
          text = this->Options->getHelpFormat()->getHelpText(
              this->Model->getSeriesName(iter.key()).toString(), args);
          }

        range = iter->getNext(range);
        }
      }

    return true;
    }

  return false;
}

void vtkQtBarChart::finishInteractiveResize()
{
  if(this->BuildNeeded)
    {
    // Get the axis layer to get the axes and domain priority.
    vtkQtChartAxisLayer *layer = this->ChartArea->getAxisLayer();
    vtkQtChartLayer::AxesCorner corner = this->Options->getAxesCorner();
    vtkQtChartAxis *xAxis = layer->getHorizontalAxis(corner);
    vtkQtChartAxis *yAxis = layer->getVerticalAxis(corner);

    int seriesGroup = -1;
    const vtkQtChartSeriesDomain *seriesDomain =
        this->Internal->Domain.getDomain(xAxis->getAxisDomain(),
        yAxis->getAxisDomain(), &seriesGroup);
    if(seriesDomain)
      {
      this->buildBarTree(seriesGroup);
      }
    }
}

void vtkQtBarChart::getSeriesAt(const QPointF &point,
    vtkQtChartSeriesSelection &selection) const
{
  // Translate the point to contents coordinates.
  QPointF local = point;
  this->ChartArea->getContentsSpace()->translateToLayerContents(local);

  // Get the bar index from the search tree.
  vtkQtChartIndexRangeList indexes;
  vtkQtChartBar *bar = this->Internal->BarTree.getItemAt(local);
  if(bar)
    {
    // Add the series to the selection.
    indexes.addRange(bar->getSeries(), bar->getSeries());
    }

  selection.setSeries(indexes);
}

void vtkQtBarChart::getPointsAt(const QPointF &point,
    vtkQtChartSeriesSelection &selection) const
{
  // Translate the point to contents coordinates.
  QPointF local = point;
  this->ChartArea->getContentsSpace()->translateToLayerContents(local);

  // Get the bar index from the search tree.
  selection.clear();
  vtkQtChartBar *bar = this->Internal->BarTree.getItemAt(local);
  if(bar)
    {
    // Add the bar to the selection.
    selection.addPoints(bar->getSeries(),
        vtkQtChartIndexRangeList(bar->getIndex(), bar->getIndex()));
    }
}

void vtkQtBarChart::getSeriesIn(const QRectF &area,
    vtkQtChartSeriesSelection &selection) const
{
  // Translate the rectangle to contents coordinates.
  QRectF local = area;
  this->ChartArea->getContentsSpace()->translateToLayerContents(local);

  // Get the list of bar indexes from the bar tree.
  vtkQtChartIndexRangeList indexes;
  QList<vtkQtChartBar *> list = this->Internal->BarTree.getItemsIn(local);
  QList<vtkQtChartBar *>::Iterator iter = list.begin();
  for( ; iter != list.end(); ++iter)
    {
    // Add the series to the selection.
    indexes.addRange((*iter)->getSeries(), (*iter)->getSeries());
    }

  selection.setSeries(indexes);
}

void vtkQtBarChart::getPointsIn(const QRectF &area,
    vtkQtChartSeriesSelection &selection) const
{
  // Translate the rectangle to contents coordinates.
  QRectF local = area;
  this->ChartArea->getContentsSpace()->translateToLayerContents(local);

  // Get the list of bar indexes from the bar tree.
  selection.clear();
  QList<vtkQtChartBar *> list = this->Internal->BarTree.getItemsIn(local);
  QList<vtkQtChartBar *>::Iterator iter = list.begin();
  for( ; iter != list.end(); ++iter)
    {
    // Add the bar to the selection.
    selection.addPoints((*iter)->getSeries(),
        vtkQtChartIndexRangeList((*iter)->getIndex(), (*iter)->getIndex()));
    }
}

QRectF vtkQtBarChart::boundingRect() const
{
  return this->Internal->Bounds;
}

void vtkQtBarChart::paint(QPainter *painter,
    const QStyleOptionGraphicsItem *option, QWidget *)
{
  if(!this->ChartArea)
    {
    return;
    }

  // Use the exposed rectangle from the option object to determine
  // which series to draw.
  vtkQtChartContentsSpace *space = this->ChartArea->getContentsSpace();
  QRectF area = option->exposedRect.translated(space->getXOffset(),
      space->getYOffset());

  // Get the axis layer to get the axes and domain priority.
  vtkQtChartAxisLayer *layer = this->ChartArea->getAxisLayer();
  vtkQtChartLayer::AxesCorner corner = this->Options->getAxesCorner();
  vtkQtChartAxis *xAxis = layer->getHorizontalAxis(corner);
  vtkQtChartAxis *yAxis = layer->getVerticalAxis(corner);

  bool darker_outline =
    (this->Options->getOutlineStyle() == vtkQtBarChartOptions::Darker);

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
    QList<int> seriesList = this->Internal->Groups.getGroup(domainIndex);
    QList<int>::Iterator iter = seriesList.begin();
    for( ; iter != seriesList.end(); ++iter)
      {
      // Set up the painter for the series.
      vtkQtBarChartSeries *series = this->Internal->Series[*iter];
      vtkQtChartSeriesOptions *options = this->getSeriesOptions(*iter);
      vtkQtChartSeriesColors *colors = options->getSeriesColors();
      QBrush light = options->getBrush();
      light.setColor(vtkQtChartColors::lighter(light.color()));
      QPen pen = options->getPen();
      if (darker_outline)
        {
        pen.setColor(options->getBrush().color().dark());
        }
      else
        {
        pen.setColor(Qt::black);
        }
      painter->setPen(pen);
      if(series->IsHighlighted)
        {
        painter->setBrush(light);
        }
      else
        {
        painter->setBrush(options->getBrush());
        }

      // Draw each of the series bars that are in the paint area.
      int total = series->Bars.size();
      for(int index = 0; index < total; index++)
        {
        const QRectF *bar = series->Bars[index];
        if(bar->right() + 0.5 < area.left())
          {
          continue;
          }
        else if(bar->left() - 0.5 > area.right())
          {
          break;
          }
        else if(bar->height() == 0.0)
          {
          continue;
          }

        bool highlighted = !series->IsHighlighted &&
            series->Highlights.contains(index);
        if(colors)
          {
          painter->save();
          QBrush barColor = options->getBrush();
          colors->getBrush(index, total, barColor);
          if(highlighted || series->IsHighlighted)
            {
            barColor.setColor(vtkQtChartColors::lighter(barColor.color()));
            }

          if (darker_outline)
            {
            QPen barPen = options->getPen();
            barPen.setColor(barColor.color().dark());
            painter->setPen(barPen);
            }

          painter->setBrush(barColor);
          }
        else if(highlighted)
          {
          painter->save();
          painter->setBrush(light);
          }

        painter->drawRect(*bar);
        if(highlighted || colors)
          {
          painter->restore();
          }
        }
      }
    }
}

void vtkQtBarChart::reset()
{
  // Make sure the selection model is notified of the change.
  this->InModelChange = true;
  this->Selection->beginModelReset();

  // Clean up the old view items.
  bool needsLayout = this->Internal->Series.size() > 0;
  QList<vtkQtBarChartSeries *>::Iterator iter = this->Internal->Series.begin();
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

    QList<int> groups;
    bool signalDomain = false;
    int i = first;
    for( ; i <= last; i++)
      {
      // Add an item for each series.
      vtkQtBarChartSeries *series = new vtkQtBarChartSeries();
      this->Internal->Series.insert(i, series);

      // Get the series options.
      vtkQtChartSeriesOptions *options = this->getSeriesOptions(i);
      this->setupOptions(options);

      // Add bars to the series for each series point.
      int total = this->Model->getNumberOfSeriesValues(i);
      for(int j = 0; j < total; j++)
        {
        series->Bars.append(new QRectF());
        series->Bounds.append(new vtkQtChartBar(i, j));
        }

      // Add the series domains to the chart domains.
      if(options->isVisible())
        {
        int seriesGroup = -1;
        if(this->addSeriesDomain(i, seriesGroup))
          {
          signalDomain = true;
          }

        // Keep track of the series groups that need new bar lists.
        if(!groups.contains(seriesGroup))
          {
          groups.append(seriesGroup);
          }
        }
      }

    // Fix the series indexes in the search lists.
    this->Internal->Groups.finishInsert();
    for(i = last + 1; i < this->Internal->Series.size(); i++)
      {
      this->Internal->Series[i]->updateSeries(i);
      }

    // Create the bar lists for the modified domains.
    QList<int>::Iterator iter = groups.begin();
    for( ; iter != groups.end(); ++iter)
      {
      this->createBarList(*iter);
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
      if(this->Internal->Groups.getNumberOfSeries(*iter) == 0)
        {
        // Remove the empty domain.
        this->Internal->Domain.removeDomain(*iter);
        }
      else
        {
        // Re-calculate the chart domain.
        this->calculateDomain(*iter);
        this->createBarList(*iter);
        }
      }

    // Fix the stored indexes in the domain groups.
    this->Internal->Groups.finishRemoval(first, last);

    // Remove each of the series items.
    for( ; last >= first; last--)
      {
      // Get the series options.
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

void vtkQtBarChart::finishSeriesRemoval(int first, int last)
{
  if (this->ChartArea)
    {
    emit this->rangeChanged();
    emit this->layoutNeeded();

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
    int total = this->Model->getNumberOfSeries();
    emit this->modelSeriesChanged(0, total-1);
    this->update();
    }
}

void vtkQtBarChart::handleOptionsChanged(vtkQtChartSeriesOptions* options,
  int ltype, const QVariant& newvalue, const QVariant& oldvalue)
{
  if (ltype == vtkQtChartSeriesOptions::VISIBLE)
    {
    // visibility changed.
    bool visible = options->isVisible();
    this->handleSeriesVisibilityChange(options, visible);

    // TODO: Update the series rectangle.
    }

  this->vtkQtChartSeriesLayer::handleOptionsChanged(
    options, ltype, newvalue, oldvalue);
}

void vtkQtBarChart::handleSeriesVisibilityChange(
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
      this->createBarList(seriesGroup);
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
          this->createBarList(seriesGroup);
          }

        this->Internal->Groups.finishRemoval();
        emit this->rangeChanged();
        emit this->layoutNeeded();
        }
      }
    }
}

void vtkQtBarChart::updateHighlights()
{
  if(!this->InModelChange && this->ChartArea)
    {
    // Remove the current selection.
    QList<vtkQtBarChartSeries *>::Iterator iter =
        this->Internal->Series.begin();
    for( ; iter != this->Internal->Series.end(); ++iter)
      {
      (*iter)->IsHighlighted = false;
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
            this->Internal->Series[i]->IsHighlighted = true;
            }

          range = series.getNext(range);
          }
        }
      else if(current.getType() == vtkQtChartSeriesSelection::PointSelection)
        {
        const QMap<int, vtkQtChartIndexRangeList> &points =
            current.getPoints();
        QMap<int, vtkQtChartIndexRangeList>::ConstIterator jter;
        for(jter = points.begin(); jter != points.end(); ++jter)
          {
          vtkQtBarChartSeries *series = this->Internal->Series[jter.key()];
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

bool vtkQtBarChart::addSeriesDomain(int series, int &seriesGroup)
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
  vtkQtChartAxisDomain::sort(xDomain);
  seriesDomain.getXDomain().setDomain(xDomain);
  if(yIsList)
    {
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
    vtkQtChartSeriesOptions *options = this->getSeriesOptions(*iter);
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

    vtkQtChartAxisDomain::sort(xDomain);
    domain->getXDomain().mergeDomain(xDomain);
    if(yIsList)
      {
      vtkQtChartAxisDomain::sort(yDomain);
      domain->getYDomain().mergeDomain(yDomain);
      }
    else
      {
      domain->getYDomain().mergeRange(yDomain);
      }
    }
}

void vtkQtBarChart::createBarList(int seriesGroup)
{
  // Clear the bar tree if this is the displayed group.
  if(seriesGroup == this->Internal->CurrentGroup)
    {
    this->Internal->BarTree.clear();
    this->Internal->CurrentGroup = -1;
    }

  // Clear the current bar list.
  this->Internal->Groups.Lists[seriesGroup].clear();

  // Get the x-axis domain.
  bool isRange = false;
  vtkQtChartSeriesDomain *seriesDomain =
      this->Internal->Domain.getDomain(seriesGroup);
  QList<QVariant> xDomain = seriesDomain->getXDomain().getDomain(isRange);
  if(xDomain.size() > 0)
    {
    // Use a temporary list to organize the series bars. Make a list
    // for each domain entry.
    int k = 0;
    QList<QList<vtkQtChartBar *> > temp;
    for( ; k < xDomain.size(); k++)
      {
      temp.append(QList<vtkQtChartBar *>());
      }

    // Get the list of series for the group.
    QList<int> seriesList = this->Internal->Groups.getGroup(seriesGroup);
    QList<int>::Iterator iter = seriesList.begin();
    for( ; iter != seriesList.end(); ++iter)
      {
      k = 0;
      vtkQtBarChartSeries *series = this->Internal->Series[*iter];
      QVariant xValue, yValue;
      int points = this->Model->getNumberOfSeriesValues(*iter);
      for(int j = 0; j < points; j++, k++)
        {
        // Find the matching x-axis value in the domain.
        xValue = this->Model->getSeriesValue(*iter, j, 0);
        while(k < xDomain.size() && xValue != xDomain[k])
          {
          k++;
          }

        if(k >= xDomain.size())
          {
          break;
          }

        // Add the bar to the appropriate column.
        temp[k].append(series->Bounds[j]);
        }
      }

    // Flatten the bar list.
    QList<QList<vtkQtChartBar *> >::Iterator jter = temp.begin();
    QList<vtkQtChartBar *>::Iterator kter;
    for( ; jter != temp.end(); ++jter)
      {
      for(kter = jter->begin(); kter != jter->end(); ++kter)
        {
        this->Internal->Groups.Lists[seriesGroup].append(*kter);
        }
      }
    }
}

void vtkQtBarChart::buildBarTree(int seriesGroup)
{
  this->BuildNeeded = false;
  if(seriesGroup == this->Internal->CurrentGroup)
    {
    this->Internal->BarTree.update();
    }
  else
    {
    this->Internal->CurrentGroup = seriesGroup;
    this->Internal->BarTree.build(this->Internal->Groups.Lists[seriesGroup]);
    }
}


