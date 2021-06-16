/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestChartMatrix.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkAxis.h"
#include "vtkChartMatrix.h"
#include "vtkChartXY.h"
#include "vtkContextScene.h"
#include "vtkContextView.h"
#include "vtkFloatArray.h"
#include "vtkNamedColors.h"
#include "vtkNew.h"
#include "vtkPen.h"
#include "vtkPlot.h"
#include "vtkPlotPoints.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTable.h"

//----------------------------------------------------------------------------
int TestChartMatrix2(int, char*[])
{
  vtkNew<vtkNamedColors> colors;

  // Set up a 2D scene, add an XY chart to it
  vtkNew<vtkContextView> view;
  view->GetRenderWindow()->SetSize(600, 400);
  view->GetRenderWindow()->SetWindowName("ChartMatrixExt");

  vtkNew<vtkChartMatrix> matrix;
  view->GetScene()->AddItem(matrix);
  int m = 4, n = 4;
  matrix->SetSize(vtkVector2i(m, n));
  matrix->SetGutter(vtkVector2f(40, 40));

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

  vtkNew<vtkFloatArray> arrS2;
  arrS2->SetName("Sine2");
  table->AddColumn(arrS2);

  vtkNew<vtkFloatArray> tangent;
  tangent->SetName("Tangent");
  table->AddColumn(tangent);

  int numPoints = 42;
  float inc = 7.5f / (static_cast<float>(numPoints - 1.0f));
  table->SetNumberOfRows(numPoints);
  for (int i = 0; i < numPoints; ++i)
  {
    table->SetValue(i, 0, i * inc);
    table->SetValue(i, 1, cos(i * inc));
    table->SetValue(i, 2, sin(i * inc));
    table->SetValue(i, 3, sin(i * inc) + 0.5);
    table->SetValue(i, 4, tan(i * inc));
  }

  for (int i = 0; i < m; i += 2)
  {
    for (int j = 0; j < n; j += 2)
    {
      // Add multiple line plots, setting the colors etc
      // lower left plot, a point chart
      vtkChart* chart = matrix->GetChart(vtkVector2i(0 + i, 0 + j));
      vtkPlot* plot = chart->AddPlot(vtkChart::POINTS);
      plot->SetInputData(table, 0, 1);
      vtkPlotPoints::SafeDownCast(plot)->SetMarkerStyle(vtkPlotPoints::DIAMOND);
      plot->GetXAxis()->GetGridPen()->SetColorF(colors->GetColor3d("warm_grey").GetData());
      plot->GetYAxis()->GetGridPen()->SetColorF(colors->GetColor3d("warm_grey").GetData());
      plot->SetColor(colors->GetColor3ub("sea_green").GetRed(),
        colors->GetColor3ub("sea_green").GetGreen(), colors->GetColor3ub("sea_green").GetBlue(),
        255);

      // upper left plot, a point chart
      chart = matrix->GetChart(vtkVector2i(0 + i, 1 + j));
      plot = chart->AddPlot(vtkChart::POINTS);
      plot->SetInputData(table, 0, 2);
      plot->GetXAxis()->GetGridPen()->SetColorF(colors->GetColor3d("warm_grey").GetData());
      plot->GetYAxis()->GetGridPen()->SetColorF(colors->GetColor3d("warm_grey").GetData());
      plot->SetColor(colors->GetColor3ub("rose_madder").GetRed(),
        colors->GetColor3ub("rose_madder").GetGreen(), colors->GetColor3ub("rose_madder").GetBlue(),
        255);

      // lower right plot, a line chart
      chart = matrix->GetChart(vtkVector2i(1 + i, 0 + j));
      plot = chart->AddPlot(vtkChart::LINE);
      plot->SetInputData(table, 0, 3);
      plot->GetXAxis()->GetGridPen()->SetColorF(colors->GetColor3d("warm_grey").GetData());
      plot->GetYAxis()->GetGridPen()->SetColorF(colors->GetColor3d("warm_grey").GetData());
      plot->SetColor(colors->GetColor3ub("dark_orange").GetRed(),
        colors->GetColor3ub("dark_orange").GetGreen(), colors->GetColor3ub("dark_orange").GetBlue(),
        255);

      // upper right plot, a bar and point chart
      chart = matrix->GetChart(vtkVector2i(1 + i, 1 + j));
      plot = chart->AddPlot(vtkChart::BAR);
      plot->SetInputData(table, 0, 4);
      plot->GetXAxis()->GetGridPen()->SetColorF(colors->GetColor3d("warm_grey").GetData());
      plot->GetYAxis()->GetGridPen()->SetColorF(colors->GetColor3d("warm_grey").GetData());
      plot->SetColor(colors->GetColor3ub("burnt_sienna").GetRed(),
        colors->GetColor3ub("burnt_sienna").GetGreen(),
        colors->GetColor3ub("burnt_sienna").GetBlue(), 255);

      plot = chart->AddPlot(vtkChart::POINTS);
      plot->SetInputData(table, 0, 1);
      vtkPlotPoints::SafeDownCast(plot)->SetMarkerStyle(vtkPlotPoints::CROSS);
      plot->GetXAxis()->GetGridPen()->SetColorF(colors->GetColor3d("warm_grey").GetData());
      plot->GetYAxis()->GetGridPen()->SetColorF(colors->GetColor3d("warm_grey").GetData());
      plot->SetColor(colors->GetColor3ub("rose_madder").GetRed(),
        colors->GetColor3ub("rose_madder").GetGreen(), colors->GetColor3ub("rose_madder").GetBlue(),
        255);

      // lower right plot, 2 line charts with different colors
      chart = matrix->GetChart(vtkVector2i(1 + i, 0 + j));
      plot = chart->AddPlot(vtkChart::LINE);
      plot->SetInputData(table, 0, 3);
      plot->GetXAxis()->GetGridPen()->SetColorF(colors->GetColor3d("warm_grey").GetData());
      plot->GetYAxis()->GetGridPen()->SetColorF(colors->GetColor3d("warm_grey").GetData());
      plot->SetColor(colors->GetColor3ub("dark_orange").GetRed(),
        colors->GetColor3ub("dark_orange").GetGreen(), colors->GetColor3ub("dark_orange").GetBlue(),
        255);

      plot = chart->AddPlot(vtkChart::LINE);
      plot->SetInputData(table, 0, 3);
      plot->GetXAxis()->GetGridPen()->SetColorF(colors->GetColor3d("warm_grey").GetData());
      plot->GetYAxis()->GetGridPen()->SetColorF(colors->GetColor3d("warm_grey").GetData());
      plot->SetColor(colors->GetColor3ub("royal_blue").GetRed(),
        colors->GetColor3ub("royal_blue").GetGreen(), colors->GetColor3ub("royal_blue").GetBlue(),
        255);
    }
  }
  matrix->LabelOuter({ 1, 1 }, { m - 1, n - 1 });
  // Finally render the scene and compare the image to a reference image
  view->GetRenderer()->SetBackground(colors->GetColor3d("navajo_white").GetData());
  view->GetRenderWindow()->Render();
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();
  return EXIT_SUCCESS;
}
