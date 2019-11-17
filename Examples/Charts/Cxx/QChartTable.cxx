/*=========================================================================

  Program:   Visualization Toolkit
  Module:    QChartTable.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "QVTKOpenGLWidget.h"
#include "vtkChartXY.h"
#include "vtkContextScene.h"
#include "vtkContextView.h"
#include "vtkFloatArray.h"
#include "vtkGenericOpenGLRenderWindow.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkPlot.h"
#include "vtkQtTableView.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"
#include "vtkTimerLog.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QMainWindow>
#include <QSurfaceFormat>
#include <QWidget>

//----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
  // needed to ensure appropriate OpenGL context is created for VTK rendering.
  QSurfaceFormat::setDefaultFormat(QVTKOpenGLWidget::defaultFormat());

  // Qt initialization
  QApplication app(argc, argv);
  QMainWindow mainWindow;
  mainWindow.setGeometry(0, 0, 1150, 600);

  QVTKOpenGLWidget* qvtkWidget = new QVTKOpenGLWidget(&mainWindow);

  vtkNew<vtkGenericOpenGLRenderWindow> renderWindow;
  qvtkWidget->setRenderWindow(renderWindow);

  // Set up my 2D world...
  vtkNew<vtkContextView> view; // This contains a chart object
  view->SetRenderWindow(renderWindow);
  view->SetInteractor(renderWindow->GetInteractor());

  // Create a table with some points in it...
  vtkNew<vtkTable> table;
  vtkNew<vtkFloatArray> arrX;
  arrX->SetName("X Axis");
  table->AddColumn(arrX);
  vtkNew<vtkFloatArray> arrC;
  arrC->SetName("Cosine");
  table->AddColumn(arrC);
  vtkNew<vtkFloatArray> arrS;
  arrS->SetName("Sine");
  table->AddColumn(arrS);

  // Make a timer object - need to get some frame rates/render times
  vtkNew<vtkTimerLog> timer;

  // Test charting with a few more points...
  int numPoints = 29;
  float inc = 7.0 / (numPoints - 1);
  table->SetNumberOfRows(numPoints);
  for (int i = 0; i < numPoints; ++i)
  {
    table->SetValue(i, 0, i * inc);
    table->SetValue(i, 1, cos(i * inc) + 0.0);
    table->SetValue(i, 2, sin(i * inc) + 0.0);
  }

  //   table->Update();

  // Add multiple line plots, setting the colors etc
  vtkNew<vtkChartXY> chart;
  view->GetScene()->AddItem(chart);
  vtkPlot* line = chart->AddPlot(vtkChart::LINE);
  line->SetInputData(table, 0, 1);
  line->SetColor(255, 0, 0, 255);
  line = chart->AddPlot(vtkChart::LINE);
  line->SetInputData(table, 0, 2);
  line->SetColor(0, 255, 0, 255);
  line->SetWidth(2.0);

  // Instantiate a vtkQtChart and use that too
  /*  vtkQtChart *qtChart = new vtkQtChart;
    chart = qtChart->chart();
    line = chart->AddPlot(vtkChart::LINE);
    line->SetTable(table, 0, 1);
    line->SetColor(255, 0, 0, 255);
    line = chart->AddPlot(vtkChart::LINE);
    line->SetTable(table, 0, 2);
    line->SetColor(0, 255, 0, 255);
    line->SetWidth(2.0);
  */
  // Now lets try to add a table view
  QWidget* widget = new QWidget(&mainWindow);
  QHBoxLayout* layout = new QHBoxLayout(widget);
  vtkNew<vtkQtTableView> tableView;
  tableView->SetSplitMultiComponentColumns(true);
  tableView->AddRepresentationFromInput(table);
  tableView->Update();
  layout->addWidget(qvtkWidget, 2);
  // layout->addWidget(qtChart, 2);
  layout->addWidget(tableView->GetWidget());
  mainWindow.setCentralWidget(widget);

  // Now show the application and start the event loop
  mainWindow.show();

  return app.exec();
}
