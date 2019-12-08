/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestChartBadPoints.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkChartXY.h"
#include "vtkContextScene.h"
#include "vtkContextView.h"
#include "vtkDoubleArray.h"
#include "vtkNew.h"
#include "vtkPlotLine.h"
#include "vtkPlotPoints.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTable.h"

//----------------------------------------------------------------------------
int TestChartBadPoints(int, char*[])
{
  // Set up a 2D scene, add an XY chart to it
  vtkNew<vtkContextView> view;
  view->GetRenderWindow()->SetSize(400, 300);
  vtkNew<vtkChartXY> chart;
  view->GetScene()->AddItem(chart);

  // Create a table with polyline points
  vtkNew<vtkTable> table;
  vtkNew<vtkDoubleArray> arrX;
  arrX->SetName("X");
  table->AddColumn(arrX);
  vtkNew<vtkDoubleArray> arrC;
  arrC->SetName("f1");
  table->AddColumn(arrC);
  table->SetNumberOfRows(7);
  table->SetValue(0, 0, 0);
  table->SetValue(1, 0, 1);
  table->SetValue(2, 0, 2);
  table->SetValue(3, 0, 3);
  table->SetValue(4, 0, 4);
  table->SetValue(5, 0, 5);
  table->SetValue(6, 0, 6);
  table->SetValue(0, 1, 1.7);
  table->SetValue(1, 1, 1.9);
  table->SetValue(2, 1, vtkMath::Nan());
  table->SetValue(3, 1, 2);
  table->SetValue(4, 1, vtkMath::Nan());
  table->SetValue(5, 1, 2.3);
  table->SetValue(6, 1, 2.1);

  // Create a table with non-polyline points
  vtkNew<vtkTable> table2;
  vtkNew<vtkDoubleArray> arrX2;
  arrX2->SetName("X");
  table2->AddColumn(arrX2);
  vtkNew<vtkDoubleArray> arrC2;
  arrC2->SetName("f1");
  table2->AddColumn(arrC2);
  table2->SetNumberOfRows(12);
  table2->SetValue(0, 0, 0);
  table2->SetValue(1, 0, 1);
  table2->SetValue(2, 0, 1);
  table2->SetValue(3, 0, 2);
  table2->SetValue(4, 0, 2);
  table2->SetValue(5, 0, 3);
  table2->SetValue(6, 0, 3);
  table2->SetValue(7, 0, 4);
  table2->SetValue(8, 0, 4);
  table2->SetValue(9, 0, 5);
  table2->SetValue(10, 0, 5);
  table2->SetValue(11, 0, 6);
  table2->SetValue(0, 1, 3.7);
  table2->SetValue(1, 1, 3.9);
  table2->SetValue(2, 1, 3.9);
  table2->SetValue(3, 1, vtkMath::Nan());
  table2->SetValue(4, 1, vtkMath::Nan());
  table2->SetValue(5, 1, 4);
  table2->SetValue(6, 1, 5);
  table2->SetValue(7, 1, vtkMath::Nan());
  table2->SetValue(8, 1, vtkMath::Nan());
  table2->SetValue(9, 1, 5.3);
  table2->SetValue(10, 1, 5.3);
  table2->SetValue(11, 1, 4.3);

  // Add multiple line and point plots
  vtkNew<vtkPlotPoints> points;
  chart->AddPlot(points);
  points->SetInputData(table, 0, 1);
  points->SetMarkerSize(10.0);
  vtkNew<vtkPlotLine> line;
  chart->AddPlot(line);
  line->SetInputData(table, 0, 1);
  vtkNew<vtkPlotLine> line2;
  line2->SetPolyLine(false);
  chart->AddPlot(line2);
  line2->SetInputData(table2, 0, 1);

  // Render the scene and compare the image to a reference image
  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();
  return EXIT_SUCCESS;
}
