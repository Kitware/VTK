/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestScatterPlotMatrixVisible.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkScatterPlotMatrix.h"
#include "vtkRenderWindow.h"
#include "vtkChart.h"
#include "vtkPlot.h"
#include "vtkTable.h"
#include "vtkFloatArray.h"
#include "vtkContextView.h"
#include "vtkContextScene.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkNew.h"

//----------------------------------------------------------------------------
int TestScatterPlotMatrixVisible(int, char * [])
{
  // Set up a 2D scene, add a chart to it.
  vtkNew<vtkContextView> view;
  view->GetRenderWindow()->SetSize(800, 600);
  vtkNew<vtkScatterPlotMatrix> matrix;
  view->GetScene()->AddItem(matrix.GetPointer());

  // Create a table with some points in it...
  vtkNew<vtkTable> table;
  vtkNew<vtkFloatArray> arrX;
  arrX->SetName("x");
  table->AddColumn(arrX.GetPointer());
  vtkNew<vtkFloatArray> arrC;
  arrC->SetName("cos(x)");
  table->AddColumn(arrC.GetPointer());
  vtkNew<vtkFloatArray> arrS;
  arrS->SetName("sin(x)");
  table->AddColumn(arrS.GetPointer());
  vtkNew<vtkFloatArray> arrS2;
  arrS2->SetName("sin(x + 0.5)");
  table->AddColumn(arrS2.GetPointer());
  vtkNew<vtkFloatArray> tangent;
  tangent->SetName("tan(x)");
  table->AddColumn(tangent.GetPointer());
  // Test the chart scatter plot matrix
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

  // Select a few columns in the table to analyze.
  matrix->SetInput(table.GetPointer());
  matrix->SetColumnVisibilityAll(false);
  matrix->SetColumnVisibility("x", true);
  matrix->SetColumnVisibility("sin(x)", true);
  matrix->SetColumnVisibility("cos(x)", true);
  matrix->SetColumnVisibility("tan(x)", true);

  //Finally render the scene and compare the image to a reference image
  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();
  return EXIT_SUCCESS;
}
