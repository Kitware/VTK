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
#include "vtkQtChartInteractor.h"
#include "vtkQtChartInteractorSetup.h"
#include "vtkQtChartKeyboardFunction.h"
#include "vtkQtChartLegend.h"
#include "vtkQtChartLegendManager.h"
#include "vtkQtChartMouseSelection.h"
#include "vtkQtChartSeriesModelCollection.h"
#include "vtkQtChartSeriesSelectionHandler.h"
#include "vtkQtChartTableSeriesModel.h"
#include "vtkQtChartWidget.h"
#include "vtkQtLineChart.h"
#include "vtkQtChartSeriesOptions.h"

#include <QStandardItemModel>
#include <QVariant>

#include <QFile>
#include <QFileDialog>

#include "QTestApp.h"

class LineChartFileOpener : public vtkQtChartKeyboardFunction
{
public:
  LineChartFileOpener(QObject *parent=0);
  virtual ~LineChartFileOpener() {}

  virtual void activate();

public:
  vtkQtChartTableSeriesModel *Model;
};

LineChartFileOpener::LineChartFileOpener(QObject *parentObject)
  : vtkQtChartKeyboardFunction(parentObject)
{
  this->Model = 0;
}

void LineChartFileOpener::activate()
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

class LineChartFileReseter : public vtkQtChartKeyboardFunction
{
public:
  LineChartFileReseter(QObject *parent=0);
  virtual ~LineChartFileReseter() {}

  virtual void activate();

public:
  vtkQtChartTableSeriesModel *Model;
};

LineChartFileReseter::LineChartFileReseter(QObject *parentObject)
  : vtkQtChartKeyboardFunction(parentObject)
{
  this->Model = 0;
}

void LineChartFileReseter::activate()
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
  model = new QStandardItemModel(3, 2, this->Model);
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

  // Set the new model in the chart.
  this->Model->setItemModel(model);
}

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

  // Add the file opener to the interactor.
  vtkQtChartInteractor *interactor = area->getInteractor();
  LineChartFileOpener *opener = new LineChartFileOpener(interactor);
  interactor->addKeyboardFunction(
      QKeySequence(Qt::Key_O | Qt::ControlModifier), opener);
  LineChartFileReseter *reseter = new LineChartFileReseter(interactor);
  interactor->addKeyboardFunction(
      QKeySequence(Qt::Key_N | Qt::ControlModifier), reseter);

  // Set up the chart table model.
  vtkQtChartTableSeriesModel *adaptor =
      new vtkQtChartTableSeriesModel(0, line);
  opener->Model = adaptor;
  reseter->Model = adaptor;

  // Add the default model to the chart.
  reseter->activate();

  // For fun, add the table model to a collection.
  vtkQtChartSeriesModelCollection *collection =
      new vtkQtChartSeriesModelCollection(line);
  collection->addSeriesModel(adaptor);

  line->setModel(collection);

  vtkQtChartSeriesOptions *opts = line->getSeriesOptions(0);
  opts->setMarkerStyle(vtkQtPointMarker::Circle);

  opts = line->getSeriesOptions(1);
  opts->setMarkerStyle(vtkQtPointMarker::Diamond);
  opts->setMarkerSize(QSizeF(7.0, 7.0));

  chart->show();
  int status = app.exec();

  delete chart;

  return status; 
}

