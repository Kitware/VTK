/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestAdjustLowerBoundForLogPlot.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAxis.h"
#include "vtkChartXY.h"
#include "vtkContextScene.h"
#include "vtkContextView.h"
#include "vtkFloatArray.h"
#include "vtkNew.h"
#include "vtkPlot.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTable.h"

#include <cstdio>

int TestAdjustLowerBoundForLogPlot(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  // Set up a 2D scene, add an XY chart to it
  vtkNew<vtkContextView> view;
  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetRenderer()->SetBackground(1.0, 1.0, 1.0);
  view->GetRenderWindow()->SetSize(300, 300);
  vtkNew<vtkChartXY> chart;
  chart->AdjustLowerBoundForLogPlotOn();
  view->GetScene()->AddItem(chart);

  // Create a table with some points in it...
  vtkNew<vtkTable> table;

  vtkNew<vtkFloatArray> xArray;
  xArray->SetName("X");
  table->AddColumn(xArray);

  vtkNew<vtkFloatArray> dataArray;
  dataArray->SetName("Data");
  table->AddColumn(dataArray);

  int numRows = 100;
  table->SetNumberOfRows(numRows);
  for (int i = 0; i < numRows; ++i)
  {
    float x = 0.1 * ((-0.5 * (numRows - 1)) + i);
    table->SetValue(i, 0, x);
    float y = std::abs(x * x - 10.0);
    table->SetValue(i, 1, y);
  }

  vtkPlot* plot = chart->AddPlot(vtkChart::LINE);
  plot->SetInputData(table, 0, 1);

  vtkAxis* axis = chart->GetAxis(vtkAxis::LEFT);
  axis->LogScaleOn();

  // This sequence is necessary to invoke the logic when AdjustLowerBoundForLogPlot is enabled.
  view->GetRenderWindow()->Render();
  chart->RecalculateBounds();

  // Finally render the scene and compare the image to a reference image
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
