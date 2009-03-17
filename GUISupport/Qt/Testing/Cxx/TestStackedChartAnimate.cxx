/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestStackedChartAnimate.cxx

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
#include "vtkQtChartBasicStyleManager.h"
#include "vtkQtChartColors.h"
#include "vtkQtChartInteractorSetup.h"
#include "vtkQtChartMouseSelection.h"
#include "vtkQtChartSeriesSelectionHandler.h"
#include "vtkQtChartStyleManager.h"
#include "vtkQtChartTableSeriesModel.h"
#include "vtkQtChartWidget.h"
#include "vtkQtSeriesFilterLineEdit.h"
#include "vtkQtStackedChart.h"
#include "vtkQtStackedChartOptions.h"

#include <QVariant>
#include <QStandardItemModel>
#include <QListView>

#include "QTestApp.h"

int TestStackedChartAnimate(int argc, char* argv[])
{
  QTestApp app(argc, argv);

  vtkQtChartWidget *chart = new vtkQtChartWidget();
  vtkQtChartArea *area = chart->getChartArea();
  vtkQtChartBasicStyleManager *style =
      qobject_cast<vtkQtChartBasicStyleManager *>(area->getStyleManager());
  if(style)
    {
    style->getColors()->setColorScheme(vtkQtChartColors::WildFlower);
    }

  // Set up the stacked chart.
  vtkQtStackedChart *stacked = new vtkQtStackedChart();
  area->insertLayer(area->getAxisLayerIndex(), stacked);

  // Set up the default interactor.
  vtkQtChartMouseSelection *selector =
      vtkQtChartInteractorSetup::createDefault(area);
  vtkQtChartSeriesSelectionHandler *handler =
      new vtkQtChartSeriesSelectionHandler(selector);
  handler->setModeNames("Stacked Chart - Series", "Stacked Chart - Points");
  handler->setMousePressModifiers(Qt::ControlModifier, Qt::ControlModifier);
  handler->setLayer(stacked);
  selector->addHandler(handler);
  selector->setSelectionMode("Stacked Chart - Series");
  //selector->setSelectionMode("Stacked Chart - Points");

  stacked->getOptions()->setSumNormalized(true);
  stacked->getOptions()->setGradientDisplayed(true);

  // Set up the model for the bar chart.
  QStandardItemModel *model = new QStandardItemModel(9, 3, stacked);
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
  model->verticalHeaderItem(0)->setData(QVariant("Apple"), Qt::DisplayRole);
  model->verticalHeaderItem(1)->setData(QVariant("Orange"), Qt::DisplayRole);
  model->verticalHeaderItem(2)->setData(QVariant("Pear"), Qt::DisplayRole);
  model->verticalHeaderItem(3)->setData(QVariant("Banana"), Qt::DisplayRole);
  model->verticalHeaderItem(4)->setData(QVariant("Pineapple"), Qt::DisplayRole);
  model->verticalHeaderItem(5)->setData(QVariant("Feijoa"), Qt::DisplayRole);
  model->verticalHeaderItem(6)->setData(QVariant("Guava"), Qt::DisplayRole);
  model->verticalHeaderItem(7)->setData(QVariant("Peach"), Qt::DisplayRole);
  model->verticalHeaderItem(8)->setData(QVariant("Mango"), Qt::DisplayRole);

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
  model->item(0, 0)->setData(1.00, Qt::DisplayRole);
  model->item(1, 0)->setData(0.80, Qt::DisplayRole);
  model->item(2, 0)->setData(0.75, Qt::DisplayRole);
  model->item(3, 0)->setData(1.30, Qt::DisplayRole);
  model->item(4, 0)->setData(0.90, Qt::DisplayRole);
  model->item(5, 0)->setData(0.90, Qt::DisplayRole);
  model->item(6, 0)->setData(0.40, Qt::DisplayRole);
  model->item(7, 0)->setData(0.60, Qt::DisplayRole);
  model->item(8, 0)->setData(0.80, Qt::DisplayRole);

  model->setItem(0, 1, new QStandardItem());
  model->setItem(1, 1, new QStandardItem());
  model->setItem(2, 1, new QStandardItem());
  model->setItem(3, 1, new QStandardItem());
  model->setItem(4, 1, new QStandardItem());
  model->setItem(5, 1, new QStandardItem());
  model->setItem(6, 1, new QStandardItem());
  model->setItem(7, 1, new QStandardItem());
  model->setItem(8, 1, new QStandardItem());
  model->item(0, 1)->setData(0.35, Qt::DisplayRole);
  model->item(1, 1)->setData(0.60, Qt::DisplayRole);
  model->item(2, 1)->setData(0.85, Qt::DisplayRole);
  model->item(3, 1)->setData(0.70, Qt::DisplayRole);
  model->item(4, 1)->setData(0.60, Qt::DisplayRole);
  model->item(5, 1)->setData(0.90, Qt::DisplayRole);
  model->item(6, 1)->setData(1.00, Qt::DisplayRole);
  model->item(7, 1)->setData(0.70, Qt::DisplayRole);
  model->item(8, 1)->setData(0.40, Qt::DisplayRole);

  model->setItem(0, 2, new QStandardItem());
  model->setItem(1, 2, new QStandardItem());
  model->setItem(2, 2, new QStandardItem());
  model->setItem(3, 2, new QStandardItem());
  model->setItem(4, 2, new QStandardItem());
  model->setItem(5, 2, new QStandardItem());
  model->setItem(6, 2, new QStandardItem());
  model->setItem(7, 2, new QStandardItem());
  model->setItem(8, 2, new QStandardItem());
  model->item(0, 2)->setData(1.35, Qt::DisplayRole);
  model->item(1, 2)->setData(1.25, Qt::DisplayRole);
  model->item(2, 2)->setData(1.00, Qt::DisplayRole);
  model->item(3, 2)->setData(0.80, Qt::DisplayRole);
  model->item(4, 2)->setData(0.70, Qt::DisplayRole);
  model->item(5, 2)->setData(0.60, Qt::DisplayRole);
  model->item(6, 2)->setData(1.20, Qt::DisplayRole);
  model->item(7, 2)->setData(1.50, Qt::DisplayRole);
  model->item(8, 2)->setData(1.80, Qt::DisplayRole);

  vtkQtChartTableSeriesModel *table =
      new vtkQtChartTableSeriesModel(model, stacked);
  stacked->setModel(table);

  vtkQtSeriesFilterLineEdit *edit =
      new vtkQtSeriesFilterLineEdit(chart);
  edit->setLayer(stacked);
  edit->show();

  chart->show();
  int status = app.exec();

  delete chart;

  return status; 
}

