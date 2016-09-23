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

#include "vtkChartMatrix.h"
#include "vtkRenderWindow.h"
#include "vtkChartXY.h"
#include "vtkPlot.h"
#include "vtkTable.h"
#include "vtkFloatArray.h"
#include "vtkContextView.h"
#include "vtkContextScene.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkNew.h"

//----------------------------------------------------------------------------
int TestChartMatrix( int, char * [] )
{
  // Set up a 2D scene, add an XY chart to it
  vtkNew<vtkContextView> view;
  view->GetRenderWindow()->SetSize(400, 300);
  vtkNew<vtkChartMatrix> matrix;
  view->GetScene()->AddItem(matrix.GetPointer());
  matrix->SetSize(vtkVector2i(2, 2));
  matrix->SetGutter(vtkVector2f(30.0, 30.0));

  vtkChart *chart = matrix->GetChart(vtkVector2i(0, 0));

  // Create a table with some points in it...
  vtkNew<vtkTable> table;
  vtkNew<vtkFloatArray> arrX;
  arrX->SetName("X Axis");
  table->AddColumn(arrX.GetPointer());
  vtkNew<vtkFloatArray> arrC;
  arrC->SetName("Cosine");
  table->AddColumn(arrC.GetPointer());
  vtkNew<vtkFloatArray> arrS;
  arrS->SetName("Sine");
  table->AddColumn(arrS.GetPointer());
  vtkNew<vtkFloatArray> arrS2;
  arrS2->SetName("Sine2");
  table->AddColumn(arrS2.GetPointer());
  vtkNew<vtkFloatArray> tangent;
  tangent->SetName("Tangent");
  table->AddColumn(tangent.GetPointer());
  // Test charting with a few more points...
  int numPoints = 42;
  float inc = 7.5 / (numPoints-1);
  table->SetNumberOfRows(numPoints);
  for (int i = 0; i < numPoints; ++i)
  {
    table->SetValue(i, 0, i * inc);
    table->SetValue(i, 1, cos(i * inc));
    table->SetValue(i, 2, sin(i * inc));
    table->SetValue(i, 3, sin(i * inc) + 0.5);
    table->SetValue(i, 4, tan(i * inc));
  }

  // Add multiple line plots, setting the colors etc
  vtkPlot *line = chart->AddPlot(vtkChart::POINTS);
  line->SetInputData(table.GetPointer(), 0, 1);
  line->SetColor(0, 255, 0, 255);

  chart = matrix->GetChart(vtkVector2i(0, 1));
  line = chart->AddPlot(vtkChart::POINTS);
  line->SetInputData(table.GetPointer(), 0, 2);
  line->SetColor(255, 0, 0, 255);

  chart = matrix->GetChart(vtkVector2i(1, 0));
  line = chart->AddPlot(vtkChart::LINE);
  line->SetInputData(table.GetPointer(), 0, 3);
  line->SetColor(0, 0, 255, 255);

  chart = matrix->GetChart(vtkVector2i(1, 1));
  line = chart->AddPlot(vtkChart::BAR);
  line->SetInputData(table.GetPointer(), 0, 4);

  //Finally render the scene and compare the image to a reference image
  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();
  return EXIT_SUCCESS;
}
