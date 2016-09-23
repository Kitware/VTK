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

#include "vtkFloatArray.h"
#include "vtkMath.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"

#include "vtkSmartPointer.h"

#include "vtkContextView.h"
#include "vtkContextScene.h"
#include "vtkChartXY.h"
#include "vtkPlot.h"
#include "vtkTable.h"

#include "vtkTimerLog.h"

#include <QApplication>
#include <QWidget>
#include <QMainWindow>
#include <QHBoxLayout>

#include "QVTKWidget.h"
#include "vtkQtTableView.h"

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

//----------------------------------------------------------------------------
int main( int argc, char * argv [] )
{
  // Qt initialization
  QApplication app(argc, argv);
  QMainWindow mainWindow;
  mainWindow.setGeometry(0, 0, 1150, 600);

  // QVTK set up and initialization
  QVTKWidget *qvtkWidget = new QVTKWidget(&mainWindow);

  // Set up my 2D world...
  VTK_CREATE(vtkContextView, view); // This contains a chart object
  view->SetInteractor(qvtkWidget->GetInteractor());
  qvtkWidget->SetRenderWindow(view->GetRenderWindow());

  // Create a table with some points in it...
  VTK_CREATE(vtkTable, table);
  VTK_CREATE(vtkFloatArray, arrX);
  arrX->SetName("X Axis");
  table->AddColumn(arrX);
  VTK_CREATE(vtkFloatArray, arrC);
  arrC->SetName("Cosine");
  table->AddColumn(arrC);
  VTK_CREATE(vtkFloatArray, arrS);
  arrS->SetName("Sine");
  table->AddColumn(arrS);

  // Make a timer object - need to get some frame rates/render times
  VTK_CREATE(vtkTimerLog, timer);

  // Test charting with a few more points...
  int numPoints = 29;
  float inc = 7.0 / (numPoints-1);
  table->SetNumberOfRows(numPoints);
  for (int i = 0; i < numPoints; ++i)
  {
    table->SetValue(i, 0, i * inc);
    table->SetValue(i, 1, cos(i * inc) + 0.0);
    table->SetValue(i, 2, sin(i * inc) + 0.0);
  }

//   table->Update();

  // Add multiple line plots, setting the colors etc
  vtkSmartPointer<vtkChartXY> chart = vtkSmartPointer<vtkChartXY>::New();
  view->GetScene()->AddItem(chart);
  vtkPlot *line = chart->AddPlot(vtkChart::LINE);
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
  QWidget *widget = new QWidget(&mainWindow);
  QHBoxLayout *layout = new QHBoxLayout(widget);
  VTK_CREATE(vtkQtTableView, tableView);
  tableView->SetSplitMultiComponentColumns(true);
  tableView->AddRepresentationFromInput(table);
  tableView->Update();
  layout->addWidget(qvtkWidget, 2);
  //layout->addWidget(qtChart, 2);
  layout->addWidget(tableView->GetWidget());
  mainWindow.setCentralWidget(widget);

  // Now show the application and start the event loop
  mainWindow.show();

  return app.exec();
}
