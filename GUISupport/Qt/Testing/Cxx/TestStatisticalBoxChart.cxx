/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestStatisticalBoxChart.cxx

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

#include "vtkQtChartArea.h"
#include "vtkQtChartAxis.h"
#include "vtkQtChartAxisLayer.h"
#include "vtkQtChartAxisOptions.h"
#include "vtkQtChartBasicStyleManager.h"
#include "vtkQtChartColors.h"
#include "vtkQtChartInteractorSetup.h"
#include "vtkQtChartLegend.h"
#include "vtkQtChartLegendManager.h"
#include "vtkQtChartMouseSelection.h"
#include "vtkQtChartSeriesSelectionHandler.h"
#include "vtkQtChartStyleManager.h"
#include "vtkQtChartTableSeriesModel.h"
#include "vtkQtChartWidget.h"
#include "vtkQtStatisticalBoxChart.h"

#include <QVariant>
#include <QStandardItemModel>

#include "QTestApp.h"

int TestStatisticalBoxChart(int argc, char* argv[])
{
  QTestApp app(argc, argv);

  vtkQtChartWidget *chart = new vtkQtChartWidget();
  vtkQtChartArea *area = chart->getChartArea();
  vtkQtChartBasicStyleManager *style =
      qobject_cast<vtkQtChartBasicStyleManager *>(area->getStyleManager());
  if(style)
    {
    style->getColors()->setColorScheme(vtkQtChartColors::Blues);
    }

  // Set up the box chart.
  vtkQtStatisticalBoxChart *boxes = new vtkQtStatisticalBoxChart();
  area->insertLayer(area->getAxisLayerIndex(), boxes);

  // Add a legend to the chart.
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
  handler->setModeNames("Box Chart - Series", "Box Chart - Outliers");
  handler->setMousePressModifiers(Qt::ControlModifier, Qt::ControlModifier);
  handler->setLayer(boxes);
  selector->addHandler(handler);
  selector->setSelectionMode("Box Chart - Series");
  vtkQtChartInteractorSetup::setupDefaultKeys(area->getInteractor());

  // Hide the x-axis grid.
  vtkQtChartAxisLayer *axisLayer = area->getAxisLayer();
  vtkQtChartAxis *xAxis = axisLayer->getAxis(vtkQtChartAxis::Bottom);
  xAxis->getOptions()->setGridVisible(false);

  // Set up the model for the box chart.
  QStandardItemModel *model = new QStandardItemModel(9, 3, boxes);
  model->setItemPrototype(new QStandardItem());

  model->setHorizontalHeaderItem(0, new QStandardItem("series 1"));
  model->setHorizontalHeaderItem(1, new QStandardItem("series 2"));
  model->setHorizontalHeaderItem(2, new QStandardItem("series 3"));

  model->setItem(0, 0, new QStandardItem());
  model->setItem(1, 0, new QStandardItem());
  model->setItem(2, 0, new QStandardItem());
  model->setItem(3, 0, new QStandardItem());
  model->setItem(4, 0, new QStandardItem());
  model->setItem(5, 0, new QStandardItem());
  model->setItem(6, 0, new QStandardItem());
  model->setItem(7, 0, new QStandardItem());
  model->setItem(8, 0, new QStandardItem());

  // The data values must be floats, not ints.  i.e. you have to use the .0
  model->item(0, 0)->setData(25.0, Qt::DisplayRole);
  model->item(1, 0)->setData(50.0, Qt::DisplayRole);
  model->item(2, 0)->setData(75.0, Qt::DisplayRole);
  model->item(3, 0)->setData(90.0, Qt::DisplayRole);
  model->item(4, 0)->setData(195.0, Qt::DisplayRole);
  model->item(5, 0)->setData(1.80, Qt::DisplayRole);
  model->item(6, 0)->setData(200.0, Qt::DisplayRole);
  model->item(7, 0)->setData(215.0, Qt::DisplayRole);
  model->item(8, 0)->setData(300.0, Qt::DisplayRole);

  model->setItem(0, 1, new QStandardItem());
  model->setItem(1, 1, new QStandardItem());
  model->setItem(2, 1, new QStandardItem());
  model->setItem(3, 1, new QStandardItem());
  model->setItem(4, 1, new QStandardItem());
  model->setItem(5, 1, new QStandardItem());
  model->setItem(6, 1, new QStandardItem());
  model->setItem(7, 1, new QStandardItem());
  model->setItem(8, 1, new QStandardItem());
  model->item(0, 1)->setData(30.0, Qt::DisplayRole);
  model->item(1, 1)->setData(40.0, Qt::DisplayRole);
  model->item(2, 1)->setData(65.0, Qt::DisplayRole);
  model->item(3, 1)->setData(85.0, Qt::DisplayRole);
  model->item(4, 1)->setData(112.0, Qt::DisplayRole);
  model->item(5, 1)->setData(-40.0, Qt::DisplayRole);
  model->item(6, 1)->setData(-10.0, Qt::DisplayRole);
  model->item(7, 1)->setData(0.0, Qt::DisplayRole);
  model->item(8, 1)->setData(150.0, Qt::DisplayRole);

  model->setItem(0, 2, new QStandardItem());
  model->setItem(1, 2, new QStandardItem());
  model->setItem(2, 2, new QStandardItem());
  model->setItem(3, 2, new QStandardItem());
  model->setItem(4, 2, new QStandardItem());
  model->setItem(5, 2, new QStandardItem());
  model->setItem(6, 2, new QStandardItem());
  model->setItem(7, 2, new QStandardItem());
  model->setItem(8, 2, new QStandardItem());
  model->item(0, 2)->setData(-15.0, Qt::DisplayRole);
  model->item(1, 2)->setData(20.0, Qt::DisplayRole);
  model->item(2, 2)->setData(50.0, Qt::DisplayRole);
  model->item(3, 2)->setData(90.0, Qt::DisplayRole);
  model->item(4, 2)->setData(120.0, Qt::DisplayRole);
  model->item(5, 2)->setData(-20.0, Qt::DisplayRole);
  model->item(6, 2)->setData(130.0, Qt::DisplayRole);
  model->item(7, 2)->setData(150.0, Qt::DisplayRole);
  model->item(8, 2)->setData(250.0, Qt::DisplayRole);

  vtkQtChartTableSeriesModel *table = new vtkQtChartTableSeriesModel(model, boxes);
  boxes->setModel(table);

  chart->show();
  int status = app.exec();

  delete chart;

  return status;
}

