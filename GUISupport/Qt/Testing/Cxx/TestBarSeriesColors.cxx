/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestBarSeriesColors.cxx

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

#include "vtkQtBarChart.h"
#include "vtkQtBarChartOptions.h"
#include "vtkQtChartArea.h"
#include "vtkQtChartAxis.h"
#include "vtkQtChartAxisLayer.h"
#include "vtkQtChartAxisOptions.h"
#include "vtkQtChartColorStyleGenerator.h"
#include "vtkQtChartInteractorSetup.h"
#include "vtkQtChartLegend.h"
#include "vtkQtChartLegendManager.h"
#include "vtkQtChartMouseSelection.h"
#include "vtkQtChartSeriesHueRange.h"
#include "vtkQtChartSeriesOptions.h"
#include "vtkQtChartSeriesSelectionHandler.h"
#include "vtkQtChartStyleManager.h"
#include "vtkQtChartTableSeriesModel.h"
#include "vtkQtChartWidget.h"

#include <QStandardItemModel>
#include <QVariant>

#include <QFile>

#include "QTestApp.h"

int TestBarSeriesColors(int argc, char* argv[])
{
  QTestApp app(argc, argv);

  vtkQtChartWidget *chart = new vtkQtChartWidget();
  vtkQtChartArea *area = chart->getChartArea();

  // Set up the bar chart.
  vtkQtBarChart *bars = new vtkQtBarChart();
  area->insertLayer(area->getAxisLayerIndex(), bars);

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
  handler->setModeNames("Bar Chart - Series", "Bar Chart - Bars");
  handler->setMousePressModifiers(Qt::ControlModifier, Qt::ControlModifier);
  handler->setLayer(bars);
  selector->addHandler(handler);
  selector->setSelectionMode("Bar Chart - Bars");
  vtkQtChartInteractorSetup::setupDefaultKeys(area->getInteractor());

  // Hide the x-axis grid.
  vtkQtChartAxisLayer *axisLayer = area->getAxisLayer();
  vtkQtChartAxis *xAxis = axisLayer->getAxis(vtkQtChartAxis::Bottom);
  xAxis->getOptions()->setGridVisible(false);

  // Set up the model for the bar chart.
  QStandardItemModel *model = new QStandardItemModel(9, 2, bars);
  model->setItemPrototype(new QStandardItem());
  model->setVerticalHeaderItem(0, new QStandardItem());
  model->setVerticalHeaderItem(1, new QStandardItem());
  model->setVerticalHeaderItem(2, new QStandardItem());
  model->setVerticalHeaderItem(3, new QStandardItem());
  model->setVerticalHeaderItem(4, new QStandardItem());
  model->setVerticalHeaderItem(5, new QStandardItem());
  model->setVerticalHeaderItem(6, new QStandardItem());
  model->setVerticalHeaderItem(7, new QStandardItem());
  model->setVerticalHeaderItem(8, new QStandardItem());
  model->verticalHeaderItem(0)->setData(QVariant((int)0), Qt::DisplayRole);
  model->verticalHeaderItem(1)->setData(QVariant((int)10), Qt::DisplayRole);
  model->verticalHeaderItem(2)->setData(QVariant((int)20), Qt::DisplayRole);
  model->verticalHeaderItem(3)->setData(QVariant((int)30), Qt::DisplayRole);
  model->verticalHeaderItem(4)->setData(QVariant((int)40), Qt::DisplayRole);
  model->verticalHeaderItem(5)->setData(QVariant((int)50), Qt::DisplayRole);
  model->verticalHeaderItem(6)->setData(QVariant((int)60), Qt::DisplayRole);
  model->verticalHeaderItem(7)->setData(QVariant((int)70), Qt::DisplayRole);
  model->verticalHeaderItem(8)->setData(QVariant((int)80), Qt::DisplayRole);

  model->setHorizontalHeaderItem(0, new QStandardItem("series 1"));
  model->setHorizontalHeaderItem(1, new QStandardItem("series 2"));

  model->setItem(0, 0, new QStandardItem());
  model->setItem(1, 0, new QStandardItem());
  model->setItem(2, 0, new QStandardItem());
  model->setItem(3, 0, new QStandardItem());
  model->setItem(4, 0, new QStandardItem());
  model->setItem(5, 0, new QStandardItem());
  model->setItem(6, 0, new QStandardItem());
  model->setItem(7, 0, new QStandardItem());
  model->setItem(8, 0, new QStandardItem());
  model->item(0, 0)->setData(1.35, Qt::DisplayRole);
  model->item(1, 0)->setData(1.40, Qt::DisplayRole);
  model->item(2, 0)->setData(1.60, Qt::DisplayRole);
  model->item(3, 0)->setData(2.00, Qt::DisplayRole);
  model->item(4, 0)->setData(1.50, Qt::DisplayRole);
  model->item(5, 0)->setData(1.80, Qt::DisplayRole);
  model->item(6, 0)->setData(1.40, Qt::DisplayRole);
  model->item(7, 0)->setData(1.30, Qt::DisplayRole);
  model->item(8, 0)->setData(1.20, Qt::DisplayRole);

  model->setItem(0, 1, new QStandardItem());
  model->setItem(1, 1, new QStandardItem());
  model->setItem(2, 1, new QStandardItem());
  model->setItem(3, 1, new QStandardItem());
  model->setItem(4, 1, new QStandardItem());
  model->setItem(5, 1, new QStandardItem());
  model->setItem(6, 1, new QStandardItem());
  model->setItem(7, 1, new QStandardItem());
  model->setItem(8, 1, new QStandardItem());
  model->item(0, 1)->setData(1.65, Qt::DisplayRole);
  model->item(1, 1)->setData(1.60, Qt::DisplayRole);
  model->item(2, 1)->setData(1.40, Qt::DisplayRole);
  model->item(3, 1)->setData(1.00, Qt::DisplayRole);
  model->item(4, 1)->setData(1.50, Qt::DisplayRole);
  model->item(5, 1)->setData(1.20, Qt::DisplayRole);
  model->item(6, 1)->setData(1.60, Qt::DisplayRole);
  model->item(7, 1)->setData(1.70, Qt::DisplayRole);
  model->item(8, 1)->setData(1.80, Qt::DisplayRole);

  vtkQtChartTableSeriesModel *table =
      new vtkQtChartTableSeriesModel(model, bars);
  bars->setModel(table);

  // Set up the multi-colored series.
  vtkQtChartSeriesOptions *options = bars->getSeriesOptions(0);
  vtkQtChartSeriesHueRange *colors = new vtkQtChartSeriesHueRange(options);
  colors->setRange(QColor(113, 217, 21), QColor(15, 142, 246));
  options->setSeriesColors(colors);

  options = bars->getSeriesOptions(1);
  colors = new vtkQtChartSeriesHueRange(options);
  colors->setRange(QColor(255, 239, 96), QColor(255, 123, 16));
  options->setSeriesColors(colors);

  chart->show();
  int status = app.exec();

  delete chart;

  return status; 
}

