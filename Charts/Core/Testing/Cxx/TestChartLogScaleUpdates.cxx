// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAxis.h"
#include "vtkChartLegend.h"
#include "vtkChartXY.h"
#include "vtkContextView.h"
#include "vtkFloatArray.h"
#include "vtkPlot.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTable.h"

//------------------------------------------------------------------------------
// This unit test verifies that log scale can be turned on at a later time and
// also checks that updating other parameters after turning on log scale does
// not reset the bounds of the axis which uses log scale.
int TestChartLogScaleUpdates(int, char*[])
{
  // Set up a 2D scene, add an XY chart to it
  vtkNew<vtkContextView> view;
  view->GetRenderWindow()->SetSize(400, 300);
  vtkNew<vtkChartXY> chart;
  view->GetScene()->AddItem(chart);

  // Create a table with some points in it...
  vtkNew<vtkTable> table;
  vtkNew<vtkFloatArray> arrX;
  arrX->SetName("X Axis");
  table->AddColumn(arrX);
  vtkNew<vtkFloatArray> arrY1;
  arrY1->SetName("y=x");
  table->AddColumn(arrY1);
  vtkNew<vtkFloatArray> arrY2;
  arrY2->SetName("y=-x");
  table->AddColumn(arrY2);
  // Test charting with a few more points...
  int numPoints = 10;
  float inc = 7.5 / (numPoints - 1);
  table->SetNumberOfRows(numPoints);
  for (int i = 0; i < numPoints; ++i)
  {
    float x = 1.0e-5 + i * inc;
    table->SetValue(i, 0, x);
    table->SetValue(i, 1, x);
    table->SetValue(i, 2, -x);
  }
  chart->SetShowLegend(true);
  chart->GetLegend()->SetHorizontalAlignment(vtkChartLegend::CENTER);

  // Add a bar plot
  vtkPlot* bar = chart->AddPlot(vtkChart::BAR);
  bar->SetInputData(table, 0, 1);
  bar->SetColor(255, 0, 0, 255);

  // Add a line plot
  vtkPlot* line = chart->AddPlot(vtkChart::LINE);
  line->SetInputData(table, 0, 2);
  line->SetColor(255, 0, 255, 255);
  line->SetWidth(4.0);

  // Render the scene and compare the image to a reference image
  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->Render();

  // turn on log scale.
  double xRange[2] = { 1, -1 };
  arrX->GetRange(xRange);
  // Initialize unscaled min/max to fit x axis data so that LogScaleActive is correctly brought
  // up to date.
  chart->GetAxis(vtkAxis::BOTTOM)->SetUnscaledMinimum(xRange[0]);
  chart->GetAxis(vtkAxis::BOTTOM)->SetUnscaledMaximum(xRange[1]);
  chart->GetAxis(vtkAxis::BOTTOM)->LogScaleOn();
  chart->GetAxis(vtkAxis::BOTTOM)->Update();
  chart->Update();
  chart->RecalculateBounds();
  view->Render();

  // Change the line color to navy blue.
  line->SetColor(0, 0, 255, 255);
  chart->GetAxis(vtkAxis::BOTTOM)->SetCustomTickPositions(nullptr);
  chart->RecalculateBounds();
  view->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
