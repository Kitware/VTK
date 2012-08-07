/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestChartXYZ.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkInteractiveChartXYZ.h"
#include "vtkContextView.h"
#include "vtkContextScene.h"
#include "vtkFloatArray.h"
#include "vtkUnsignedCharArray.h"

#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkNew.h"
#include "vtkTable.h"
#include "vtkCallbackCommand.h"

int TestInteractiveChartXYZ(int , char * [])
{
  // Now the chart
  vtkNew<vtkInteractiveChartXYZ> chart;
  vtkNew<vtkContextView> view;
  view->GetRenderWindow()->SetSize(400, 300);
  view->GetScene()->AddItem(chart.GetPointer());

  chart->SetGeometry(vtkRectf(75.0, 20.0, 250, 260));

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
  vtkNew<vtkUnsignedCharArray> arrR;
  arrR->SetName("Red");
  table->AddColumn(arrR.GetPointer());
  vtkNew<vtkUnsignedCharArray> arrG;
  arrG->SetName("Green");
  table->AddColumn(arrG.GetPointer());
  vtkNew<vtkUnsignedCharArray> arrB;
  arrB->SetName("Blue");
  table->AddColumn(arrB.GetPointer());
  // Test charting with a few more points...
  int numPoints = 69;
  float inc = 7.5 / (numPoints-1);
  table->SetNumberOfRows(numPoints);
  table->SetNumberOfRows(numPoints);
  for (int i = 0; i < numPoints; ++i)
    {
    table->SetValue(i, 0, i * inc);
    table->SetValue(i, 1, cos(i * inc) + 0.0);
    table->SetValue(i, 2, sin(i * inc) + 0.0);
    table->SetValue(i, 3, i * 3);
    table->SetValue(i, 4, 0);
    table->SetValue(i, 5, 255 - i * 3);

    }

  // Add the dimensions we are interested in visualizing.
  chart->SetInput(table.GetPointer(), "X Axis", "Sine", "Cosine", "Red",
                  "Green", "Blue");
  chart->RecalculateBounds();
  chart->RecalculateTransform();

  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
