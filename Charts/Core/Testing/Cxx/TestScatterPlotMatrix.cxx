/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestScatterPlotMatrix.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkChart.h"
#include "vtkContextMouseEvent.h"
#include "vtkContextScene.h"
#include "vtkContextView.h"
#include "vtkFloatArray.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkPlot.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkScatterPlotMatrix.h"
#include "vtkTable.h"

//----------------------------------------------------------------------------
int TestScatterPlotMatrix(int, char*[])
{
  // Set up a 2D scene, add a chart to it
  vtkNew<vtkContextView> view;
  view->GetRenderWindow()->SetSize(800, 600);
  vtkNew<vtkScatterPlotMatrix> matrix;
  view->GetScene()->AddItem(matrix);

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
  int numPoints = 100;
  float inc = 4.0 * vtkMath::Pi() / (numPoints - 1);
  table->SetNumberOfRows(numPoints);
  for (int i = 0; i < numPoints; ++i)
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

  view->Render();
  matrix->GetMainChart()->SetActionToButton(
    vtkChart::SELECT_POLYGON, vtkContextMouseEvent::RIGHT_BUTTON);

  // Test animation by releasing a right click on subchart (1,2)
  vtkContextMouseEvent mouseEvent;
  mouseEvent.SetInteractor(view->GetInteractor());
  vtkVector2f pos;

  mouseEvent.SetButton(vtkContextMouseEvent::RIGHT_BUTTON);
  pos.Set(245, 301);
  mouseEvent.SetPos(pos);
  matrix->MouseButtonReleaseEvent(mouseEvent);

  // Finally render the scene and compare the image to a reference image
  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();
  return EXIT_SUCCESS;
}
