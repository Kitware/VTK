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

#include "vtkQtChartArea.h"
#include "vtkQtChartBasicSeriesOptionsModel.h"
#include "vtkQtChartContentsArea.h"
#include "vtkQtChartContentsSpace.h"
#include "vtkQtChartSeriesColors.h"
#include "vtkQtChartSeriesModel.h"
#include "vtkQtChartSeriesOptions.h"
#include "vtkQtChartSeriesSelection.h"
#include "vtkQtChartSeriesSelectionModel.h"
#include "vtkQtChartStyleAxesCorner.h"
#include "vtkQtChartStyleBoolean.h"
#include "vtkQtChartStyleBrush.h"
#include "vtkQtChartStyleManager.h"
#include "vtkQtChartStyleMarker.h"
#include "vtkQtChartStylePen.h"
#include "vtkQtChartStyleSeriesColors.h"
#include "vtkQtChartStyleSize.h"


vtkQtChartSeriesLayer::vtkQtChartSeriesLayer(bool useContents)
  : vtkQtChartLayer(), Options(0)
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
}

void vtkQtChartSeriesLayer::setModel(vtkQtChartSeriesModel *model)
{
  if(model == this->Model)
    {
    return;
    }

  vtkQtChartSeriesModel *previous = this->Model;
  this->Model = model;
  this->Selection->setModel(model);

  if (!this->Options && model)
    {
    // Create a vtkQtChartBasicSeriesOptionsModel by default.
    this->setOptionsModel(
      new vtkQtChartBasicSeriesOptionsModel(model, this));
    }
  emit this->modelChanged(previous, this->Model);
}

void vtkQtChartSeriesLayer::setOptionsModel(vtkQtChartSeriesOptionsModel* model)
{
  if (model == this->Options)
    {
    return;
    }

  if (this->Options)
    {
    QObject::disconnect(this->Options, 0, this, 0);
    }
  this->Options = model;
  if (this->Options)
    {
    QObject::connect(this->Options,
      SIGNAL(optionsChanged(vtkQtChartSeriesOptions*, int, const QVariant&, const QVariant&)),
      this, SLOT(handleOptionsChanged(
          vtkQtChartSeriesOptions*, int, const QVariant&, const QVariant&)));
    }
}

vtkQtChartSeriesOptions *vtkQtChartSeriesLayer::getSeriesOptions(
    int series) const
{
  return this->Options? this->Options->getOptions(series) : 0;
}

int vtkQtChartSeriesLayer::getSeriesOptionsIndex(
    vtkQtChartSeriesOptions *options) const
{
  if (options && this->Options)
    {
    return this->Options->getOptionsIndex(options);
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

void vtkQtChartSeriesLayer::setupOptions(vtkQtChartSeriesOptions* options)
{
  if (!this->ChartArea || !options)
    {
    return;
    }
  // Ensure the defaults for the options are set correctly.
  vtkQtChartStyleManager *manager = this->ChartArea->getStyleManager();
  int styleindex = manager->insertStyle(this, options);

  // Set up the series options.
  vtkQtChartStyleBoolean *styleVisible = qobject_cast<vtkQtChartStyleBoolean *>(
    manager->getGenerator("Visible"));
  options->setDefaultOption(vtkQtChartSeriesOptions::VISIBLE,
    styleVisible? styleVisible->getStyleBoolean(styleindex) : true);

  vtkQtChartStyleBrush *styleBrush = qobject_cast<vtkQtChartStyleBrush *>(
    manager->getGenerator("Brush"));
  options->setDefaultOption(vtkQtChartSeriesOptions::BRUSH,
    styleBrush? styleBrush->getStyleBrush(styleindex): QBrush(Qt::red));

  vtkQtChartStyleSeriesColors *styleColors =
    qobject_cast<vtkQtChartStyleSeriesColors *>(
      manager->getGenerator("Series Colors"));
  options->setDefaultOption(vtkQtChartSeriesOptions::COLORS,
    styleColors?
    QVariant::fromValue<QObject*>(styleColors->getStyleColors(styleindex)) : 
    QVariant());

  vtkQtChartStylePen *stylePen = qobject_cast<vtkQtChartStylePen *>(
    manager->getGenerator("Pen"));
  options->setDefaultOption(vtkQtChartSeriesOptions::PEN,
    stylePen? stylePen->getStylePen(styleindex) : QPen(Qt::red));

  vtkQtChartStyleAxesCorner *styleCorner =
    qobject_cast<vtkQtChartStyleAxesCorner *>(
      manager->getGenerator("Axes Corner"));
  options->setDefaultOption(vtkQtChartSeriesOptions::AXES_CORNER,
    styleCorner? styleCorner->getStyleAxesCorner(styleindex) :
    vtkQtChartLayer::BottomLeft);

  vtkQtChartStyleMarker *styleMarker = qobject_cast<vtkQtChartStyleMarker *>(
    manager->getGenerator("Marker Style"));
  options->setDefaultOption(vtkQtChartSeriesOptions::MARKER_STYLE,
    styleMarker? styleMarker->getStyleMarker(styleindex) :
    vtkQtPointMarker::NoMarker);

  vtkQtChartStyleSize *styleSize = qobject_cast<vtkQtChartStyleSize *>(
    manager->getGenerator("Marker Size"));
  options->setDefaultOption(vtkQtChartSeriesOptions::MARKER_SIZE,
    styleSize? styleSize->getStyleSize(styleindex): QSizeF(5.0, 5.0));
}

void vtkQtChartSeriesLayer::cleanupOptions(vtkQtChartSeriesOptions* options)
{
  if (!this->ChartArea || !options)
    {
    return;
    }
  vtkQtChartStyleManager *manager = this->ChartArea->getStyleManager();
  manager->removeStyle(this, options);
}

void vtkQtChartSeriesLayer::handleOptionsChanged(
  vtkQtChartSeriesOptions* options,
  int /*type*/, const QVariant& /*newvalue*/, const QVariant&)
{
  // Get the series index from the options index.
  int series = this->getSeriesOptionsIndex(options);
  if(series >= 0 /*&& series < this->Internal->Series.size()*/)
    {
    this->update();
    emit this->modelSeriesChanged(series, series);
    }
}

