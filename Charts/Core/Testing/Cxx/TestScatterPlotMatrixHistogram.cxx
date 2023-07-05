// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkChart.h"
#include "vtkContextView.h"
#include "vtkFloatArray.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkPlot.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkScatterPlotMatrix.h"
#include "vtkTable.h"

namespace
{
void PopulateMatrixPlot(vtkScatterPlotMatrix* matrix, int numberOfPoints)
{
  // Create a table with some points in it...
  vtkNew<vtkTable> table;
  vtkNew<vtkFloatArray> arrX;
  arrX->SetName("x");
  table->AddColumn(arrX);
  vtkNew<vtkFloatArray> arrC;
  arrC->SetName("cos(x)");
  table->AddColumn(arrC);
  vtkNew<vtkFloatArray> arrS;
  arrS->SetName("sin(x)");
  table->AddColumn(arrS);
  vtkNew<vtkFloatArray> arrS2;
  arrS2->SetName("sin(x + 0.5)");
  table->AddColumn(arrS2);
  vtkNew<vtkFloatArray> tangent;
  tangent->SetName("tan(x)");
  table->AddColumn(tangent);
  // Test the chart scatter plot matrix
  double inc = 4.0 * vtkMath::Pi() / (numberOfPoints - 1);
  table->SetNumberOfRows(numberOfPoints);
  for (int i = 0; i < numberOfPoints; ++i)
  {
    table->SetValue(i, 0, i * inc);
    table->SetValue(i, 1, cos(i * inc));
    table->SetValue(i, 2, sin(i * inc));
    table->SetValue(i, 3, sin(i * inc) + 0.5);
    table->SetValue(i, 4, tan(i * inc));
  }

  // Set the scatter plot matrix up to analyze all columns in the table.
  matrix->SetInput(table);
  matrix->SetNumberOfBins(7);
}
}

int TestScatterPlotMatrixHistogram(int, char*[])
{
  vtkNew<vtkContextView> view;
  view->GetRenderWindow()->SetSize(800, 600);
  vtkNew<vtkScatterPlotMatrix> matrix;
  view->GetScene()->AddItem(matrix);

  ::PopulateMatrixPlot(matrix, 100);

  view->Render();

  ::PopulateMatrixPlot(matrix, 400);

  // Finally render the scene and compare the image to a reference image
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();
  return EXIT_SUCCESS;
}
