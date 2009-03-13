/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestLineSeries.cxx

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

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#endif

#include "vtkQtChartArea.h"
#include "vtkQtChartAxis.h"
#include "vtkQtChartAxisLayer.h"
#include "vtkQtChartAxisModel.h"
#include "vtkQtChartInteractorSetup.h"
#include "vtkQtChartLegend.h"
#include "vtkQtChartLegendManager.h"
#include "vtkQtChartMouseSelection.h"
#include "vtkQtChartSeriesModelCollection.h"
#include "vtkQtChartSeriesSelectionHandler.h"
#include "vtkQtChartTableSeriesModel.h"
#include "vtkQtChartWidget.h"
#include "vtkQtLineChart.h"
#include "vtkQtLineChartSeriesOptions.h"

#include <QStandardItemModel>
#include <QVariant>

#include <QFile>

#include "QTestApp.h"

int TestLineSeries(int argc, char* argv[])
{
  QTestApp app(argc, argv);

  vtkQtChartWidget *chart = new vtkQtChartWidget();

  vtkQtChartArea *area = chart->getChartArea();

  vtkQtLineChart* line = new vtkQtLineChart();
  area->addLayer(line);

  // Set up the legend.
  vtkQtChartLegend *legend = new vtkQtChartLegend();
  vtkQtChartLegendManager *manager = new vtkQtChartLegendManager(legend);
  manager->setChartLegend(legend);
  manager->setChartArea(area);
  chart->setLegend(legend);

  // Set up the default interactor.
  vtkQtChartMouseSelection *selector =
      vtkQtChartInteractorSetup::createDefault(area);
  vtkQtChartSeriesSelectionHandler *handler =
      new vtkQtChartSeriesSelectionHandler(selector);
  handler->setModeNames("Line Chart - Series", "Line Chart - Points");
  handler->setMousePressModifiers(Qt::ControlModifier, Qt::ControlModifier);
  handler->setLayer(line);
  selector->addHandler(handler);
  selector->setSelectionMode("Line Chart - Points");
  vtkQtChartInteractorSetup::setupDefaultKeys(area->getInteractor());

  QStandardItemModel * model = new QStandardItemModel(3, 2, line);
  model->setItemPrototype(new QStandardItem());
  model->setVerticalHeaderItem(0, new QStandardItem());
  model->setVerticalHeaderItem(1, new QStandardItem());
  model->setVerticalHeaderItem(2, new QStandardItem());
  model->verticalHeaderItem(0)->setData(0.0, Qt::DisplayRole);
  model->verticalHeaderItem(1)->setData(1.0, Qt::DisplayRole);
  model->verticalHeaderItem(2)->setData(2.0, Qt::DisplayRole);

  model->setHorizontalHeaderItem(0, new QStandardItem("series 1"));
  model->setHorizontalHeaderItem(1, new QStandardItem("series 2"));

  model->setItem(0, 0, new QStandardItem());
  model->setItem(1, 0, new QStandardItem());
  model->setItem(2, 0, new QStandardItem());
  model->item(0, 0)->setData(0.5, Qt::DisplayRole);
  model->item(1, 0)->setData(0.4, Qt::DisplayRole);
  model->item(2, 0)->setData(0.6, Qt::DisplayRole);

  model->setItem(0, 1, new QStandardItem());
  model->setItem(1, 1, new QStandardItem());
  model->setItem(2, 1, new QStandardItem());
  model->item(0, 1)->setData(0.4, Qt::DisplayRole);
  model->item(1, 1)->setData(0.5, Qt::DisplayRole);
  model->item(2, 1)->setData(0.4, Qt::DisplayRole);

  vtkQtChartTableSeriesModel* adaptor = new
    vtkQtChartTableSeriesModel(model, line);

  vtkQtChartSeriesModelCollection* collection = new
    vtkQtChartSeriesModelCollection(line);
  collection->addSeriesModel(adaptor);

  line->setModel(collection);

  vtkQtLineChartSeriesOptions* opts = line->getLineSeriesOptions(0);
  opts->setPointsVisible(true);
  opts->setMarkerStyle(vtkQtPointMarker::Circle);

  opts = line->getLineSeriesOptions(1);
  opts->setPointsVisible(true);
  opts->setMarkerStyle(vtkQtPointMarker::Diamond);
  opts->setMarkerSize(QSizeF(7.0, 7.0));

  chart->show();
  int status = app.exec();

  delete chart;

  return status; 
}

