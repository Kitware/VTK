/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestLinePlotDouble2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkRenderWindow.h"
#include "vtkChartXY.h"
#include "vtkPlot.h"
#include "vtkTable.h"
#include "vtkDoubleArray.h"
#include "vtkContextView.h"
#include "vtkContextScene.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkNew.h"

//----------------------------------------------------------------------------
int TestLinePlotDouble2(int, char *[])
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
  arrC->SetName("Cosine");
  table->AddColumn(arrC.GetPointer());
  // Test charting some very small points.
  int numPoints = 69;
  float inc = 7.5 / (numPoints - 1);
  table->SetNumberOfRows(numPoints);
  for (int i = 0; i < numPoints; ++i)
    {
    double x(1 + 1e-11 * inc * i);
    table->SetValue(i, 0, x);
    table->SetValue(i, 1, cos((x - 1.0) * 1.0e11));
    }
  vtkPlot *line = chart->AddPlot(vtkChart::LINE);
  line->SetInputData(table.GetPointer(), 0, 1);

  // Render the scene and compare the image to a reference image
  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
