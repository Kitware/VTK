/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestStackedChart.cxx

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
#include "vtkQtChartInteractor.h"
#include "vtkQtChartInteractorSetup.h"
#include "vtkQtChartKeyboardFunction.h"
#include "vtkQtChartLegend.h"
#include "vtkQtChartLegendManager.h"
#include "vtkQtChartMouseSelection.h"
#include "vtkQtChartSeriesSelectionHandler.h"
#include "vtkQtChartStyleManager.h"
#include "vtkQtChartTableSeriesModel.h"
#include "vtkQtChartWidget.h"
#include "vtkQtStackedChart.h"
#include "vtkQtStackedChartOptions.h"

#include <QStandardItemModel>
#include <QVariant>

#include <QFile>
#include <QFileDialog>

#include "QTestApp.h"

class StackedChartFileOpener : public vtkQtChartKeyboardFunction
{
public:
  StackedChartFileOpener(QObject *parent=0);
  virtual ~StackedChartFileOpener() {}

  virtual void activate();

public:
  vtkQtChartTableSeriesModel *Model;
};

StackedChartFileOpener::StackedChartFileOpener(QObject *parentObject)
  : vtkQtChartKeyboardFunction(parentObject)
{
  this->Model = 0;
}

void StackedChartFileOpener::activate()
{
  // Get the file from the user.
  QString fileName = QFileDialog::getOpenFileName(0, "Open Chart File",
      QString(), "Chart Files (*.csv)");
  if(!fileName.isEmpty())
    {
    // Clear the current chart model.
    QStandardItemModel *model = qobject_cast<QStandardItemModel *>(
        this->Model->getItemModel());
    if(model)
      {
      this->Model->setItemModel(0);
      delete model;
      }

    // Open the file.
    QFile movies(fileName);
    if(movies.open(QIODevice::ReadOnly))
      {
      // Create a new item model for the file.
      model = new QStandardItemModel(0, 0, this->Model);
      model->setItemPrototype(new QStandardItem());

      // Read in the file data.
      for(int row = -1; movies.bytesAvailable() > 0; ++row)
        {
        QString line = movies.readLine(256);
        QStringList tokens = line.trimmed().split(",");
        QStringList::Iterator iter = tokens.begin();
        for(int column = -1; iter != tokens.end(); ++iter, ++column)
          {
          if(row == -1)
            {
            if(column == -1)
              {
              continue;
              }

            model->setHorizontalHeaderItem(column, new QStandardItem());
            model->horizontalHeaderItem(column)->setData(QVariant(*iter),
                Qt::DisplayRole);
            }
          else if(column == -1)
            {
            model->setVerticalHeaderItem(row, new QStandardItem());
            model->verticalHeaderItem(row)->setData(QVariant(iter->toInt()),
                Qt::DisplayRole);
            }
          else
            {
            model->setItem(row, column, new QStandardItem());
            model->item(row, column)->setData(QVariant(iter->toDouble()),
                Qt::DisplayRole);
            }
          }
        }

      // Set the new model in the chart.
      this->Model->setItemModel(model);
      }
    }
}

class StackedChartFileReseter : public vtkQtChartKeyboardFunction
{
public:
  StackedChartFileReseter(QObject *parent=0);
  virtual ~StackedChartFileReseter() {}

  virtual void activate();

public:
  vtkQtChartTableSeriesModel *Model;
};

StackedChartFileReseter::StackedChartFileReseter(QObject *parentObject)
  : vtkQtChartKeyboardFunction(parentObject)
{
  this->Model = 0;
}

void StackedChartFileReseter::activate()
{
  // Clear the current chart model.
  QStandardItemModel *model = qobject_cast<QStandardItemModel *>(
      this->Model->getItemModel());
  if(model)
    {
    this->Model->setItemModel(0);
    delete model;
    }

  // Create a new item model for the file.
  model = new QStandardItemModel(9, 3, this->Model);
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

  // Set the new model in the chart.
  this->Model->setItemModel(model);
}

int TestStackedChart(int argc, char* argv[])
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
  handler->setModeNames("Stacked Chart - Series", "Stacked Chart - Points");
  handler->setMousePressModifiers(Qt::ControlModifier, Qt::ControlModifier);
  handler->setLayer(stacked);
  selector->addHandler(handler);
  selector->setSelectionMode("Stacked Chart - Series");
  vtkQtChartInteractorSetup::setupDefaultKeys(area->getInteractor());

  // Add the file opener to the interactor.
  vtkQtChartInteractor *interactor = area->getInteractor();
  StackedChartFileOpener *opener = new StackedChartFileOpener(interactor);
  interactor->addKeyboardFunction(
      QKeySequence(Qt::Key_O | Qt::ControlModifier), opener);
  StackedChartFileReseter *reseter = new StackedChartFileReseter(interactor);
  interactor->addKeyboardFunction(
      QKeySequence(Qt::Key_N | Qt::ControlModifier), reseter);

  //stacked->getOptions()->setSumNormalized(true);
  //stacked->getOptions()->setGradientDisplayed(true);

  // Set up the model for the stacked chart.
  vtkQtChartTableSeriesModel *table =
      new vtkQtChartTableSeriesModel(0, stacked);
  opener->Model = table;
  reseter->Model = table;
  reseter->activate();
  stacked->setModel(table);

  chart->show();
  int status = app.exec();

  delete chart;

  return status; 
}

