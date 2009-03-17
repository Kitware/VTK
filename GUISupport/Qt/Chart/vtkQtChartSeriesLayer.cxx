/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartSeriesLayer.cxx

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

/// \file vtkQtChartSeriesLayer.cxx
/// \date February 14, 2008

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#endif

#include "vtkQtChartSeriesLayer.h"

#include "vtkQtChartContentsArea.h"
#include "vtkQtChartContentsSpace.h"
#include "vtkQtChartSeriesModel.h"
#include "vtkQtChartSeriesOptions.h"
#include "vtkQtChartSeriesSelection.h"
#include "vtkQtChartSeriesSelectionModel.h"
#include "vtkQtChartStyleManager.h"
#include "vtkQtChartArea.h"


vtkQtChartSeriesLayer::vtkQtChartSeriesLayer(bool useContents)
  : vtkQtChartLayer(), Options()
{
  this->Selection = new vtkQtChartSeriesSelectionModel(this);
  this->Model = 0;
  this->Contents = 0;
  if(useContents)
    {
    this->Contents = new vtkQtChartContentsArea(this, this->scene());
    }
}

void vtkQtChartSeriesLayer::setChartArea(vtkQtChartArea *area)
{
  // Remove options from the previous area's style manager.
  if(this->ChartArea)
    {
    this->clearOptions();
    this->disconnect(this->ChartArea->getContentsSpace(), 0, this, 0);
    }

  vtkQtChartLayer::setChartArea(area);
  if(this->ChartArea)
    {
    vtkQtChartContentsSpace *space = this->ChartArea->getContentsSpace();
    this->connect(space, SIGNAL(xOffsetChanged(float)),
        this, SLOT(setXOffset(float)));
    this->connect(space, SIGNAL(yOffsetChanged(float)),
        this, SLOT(setYOffset(float)));
    }

  this->resetSeriesOptions();
}

void vtkQtChartSeriesLayer::setModel(vtkQtChartSeriesModel *model)
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

  vtkQtChartSeriesModel *previous = this->Model;
  this->Model = model;
  this->Selection->setModel(model);
  if(this->Model)
    {
    // Listen for model changes.
    this->connect(this->Model, SIGNAL(modelReset()),
        this, SLOT(resetSeriesOptions()));
    this->connect(this->Model, SIGNAL(seriesInserted(int, int)),
        this, SLOT(insertSeriesOptions(int, int)));
    this->connect(this->Model, SIGNAL(seriesRemoved(int, int)),
        this, SLOT(removeSeriesOptions(int, int)));
    }

  this->resetSeriesOptions();

  emit this->modelChanged(previous, this->Model);
}

vtkQtChartSeriesOptions *vtkQtChartSeriesLayer::getSeriesOptions(
    int series) const
{
  if(series >= 0 && series < this->Options.count())
    {
    return this->Options[series];
    }

  return 0;
}

int vtkQtChartSeriesLayer::getSeriesOptionsIndex(
    vtkQtChartSeriesOptions *options) const
{
  if(options)
    {
    return this->Options.indexOf(options);
    }

  return -1;
}

QPixmap vtkQtChartSeriesLayer::getSeriesIcon(int) const
{
  QPixmap icon(16, 16);
  icon.fill(QColor(255, 255, 255, 0));
  return icon;
}

vtkQtChartSeriesSelectionModel *
    vtkQtChartSeriesLayer::getSelectionModel() const
{
  return this->Selection;
}

void vtkQtChartSeriesLayer::getSeriesAt(const QPointF &,
    vtkQtChartSeriesSelection &) const
{
}

void vtkQtChartSeriesLayer::getPointsAt(const QPointF &,
    vtkQtChartSeriesSelection &) const
{
}

void vtkQtChartSeriesLayer::getSeriesIn(const QRectF &,
    vtkQtChartSeriesSelection &) const
{
}

void vtkQtChartSeriesLayer::getPointsIn(const QRectF &,
    vtkQtChartSeriesSelection &) const
{
}

void vtkQtChartSeriesLayer::setXOffset(float offset)
{
  if(this->Contents)
    {
    this->Contents->setPos(-offset, this->Contents->pos().y());
    }
}

void vtkQtChartSeriesLayer::setYOffset(float offset)
{
  if(this->Contents)
    {
    this->Contents->setPos(this->Contents->pos().x(), -offset);
    }
}

void vtkQtChartSeriesLayer::resetSeriesOptions()
{
  if(this->ChartArea)
    {
    // Clean up the current list of options.
    this->clearOptions();

    // Create new options objects for the model series.
    if(this->Model)
      {
      int total = this->Model->getNumberOfSeries();
      if(total > 0)
        {
        this->insertSeriesOptions(0, total - 1);
        }
      }
    }
}

void vtkQtChartSeriesLayer::insertSeriesOptions(int first, int last)
{
  if(this->ChartArea)
    {
    vtkQtChartStyleManager *manager = this->ChartArea->getStyleManager();
    for( ; first <= last; first++)
      {
      vtkQtChartSeriesOptions *options = this->createOptions(this);
      this->Options.insert(first, options);
      int style = manager->insertStyle(this, options);
      this->setupOptions(style, options);
      }
    }
}

void vtkQtChartSeriesLayer::removeSeriesOptions(int first, int last)
{
  if(this->ChartArea)
    {
    vtkQtChartStyleManager *manager = this->ChartArea->getStyleManager();
    for( ; last >= first; last--)
      {
      manager->removeStyle(this, this->Options[last]);
      delete this->Options.takeAt(last);
      }
    }
}

void vtkQtChartSeriesLayer::clearOptions()
{
  vtkQtChartStyleManager *manager = this->ChartArea->getStyleManager();
  QList<vtkQtChartSeriesOptions *>::Iterator iter = this->Options.begin();
  for( ; iter != this->Options.end(); ++iter)
    {
    manager->removeStyle(this, *iter);
    delete *iter;
    }

  this->Options.clear();
}

