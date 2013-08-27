/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestChartDouble.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkRenderWindow.h"
#include "vtkSmartPointer.h"
#include "vtkChartXY.h"
#include "vtkPlot.h"
#include "vtkTable.h"
#include "vtkDoubleArray.h"
#include "vtkContextView.h"
#include "vtkContextScene.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkNew.h"

#include "vtkAxis.h"

//----------------------------------------------------------------------------
int TestChartDouble(int, char *[])
{
  // Set up a 2D scene, add an XY chart to it
  vtkNew<vtkContextView> view;
  view->GetRenderWindow()->SetSize(400, 300);
  vtkNew<vtkChartXY> chart;
  view->GetScene()->AddItem(chart.GetPointer());

  // Create a table with some points in it...
  vtkNew<vtkTable> table;
  vtkNew<vtkDoubleArray> arrX;
  arrX->SetName("X");
  table->AddColumn(arrX.GetPointer());
  vtkNew<vtkDoubleArray> arrC;
  arrC->SetName("f1");
  table->AddColumn(arrC.GetPointer());
  vtkNew<vtkDoubleArray> arrS;
  arrS->SetName("f2");
  table->AddColumn(arrS.GetPointer());
  vtkNew<vtkDoubleArray> arrS2;
  arrS2->SetName("f3");
  table->AddColumn(arrS2.GetPointer());
  // Test charting with a few more points...
  int numPoints = 69;
  float inc = 7.5 / (numPoints - 1);
  table->SetNumberOfRows(numPoints);
  for (int i = 0; i < numPoints; ++i)
    {
    double x(i * inc + 0.2);
    table->SetValue(i, 0, x);
    table->SetValue(i, 1, 1.0e-80 * (cos(x - 1.0) + sin(x - 3.14 / 4.0)));
    table->SetValue(i, 2, 1.0e-80 * sin(x) * 1e-12);
    table->SetValue(i, 3, 1.0e-80 * sin(x - 1.0));
    }

  // Add multiple line plots, setting the colors etc
  vtkPlot *line = chart->AddPlot(vtkChart::POINTS);
  line->SetInputData(table.GetPointer(), 0, 1);
  line = chart->AddPlot(vtkChart::LINE);
  line->SetInputData(table.GetPointer(), 0, 2);
  // Put this plot in a different corner - it is orders of magnitude smaller.
  chart->SetPlotCorner(line, 1);
  line = chart->AddPlot(vtkChart::BAR);
  line->SetInputData(table.GetPointer(), 0, 3);

  chart->GetAxis(vtkAxis::LEFT)->SetTitle("A tiny range");
  chart->GetAxis(vtkAxis::BOTTOM)->SetTitle("A normal range");
  chart->GetAxis(vtkAxis::RIGHT)->SetTitle("An even tinier range");

  // Render the scene and compare the image to a reference image
  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
