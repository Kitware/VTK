/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestLinePlot.cxx

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
#include "vtkFloatArray.h"
#include "vtkContextView.h"
#include "vtkContextScene.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkNew.h"

//----------------------------------------------------------------------------
int TestLinePlot( int, char * [] )
{
  int status = EXIT_SUCCESS;
  // Set up a 2D scene, add an XY chart to it
  vtkNew<vtkContextView> view;
  view->GetRenderWindow()->SetSize(400, 300);
  vtkNew<vtkChartXY> chart;
  view->GetScene()->AddItem(chart.GetPointer());

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
  vtkNew<vtkFloatArray> arr1;
  arr1->SetName("One");
  table->AddColumn(arr1.GetPointer());
  // Test charting with a few more points...
  int numPoints = 69;
  float inc = 7.5 / (numPoints-1);
  table->SetNumberOfRows(numPoints);
  for (int i = 0; i < numPoints; ++i)
  {
    table->SetValue(i, 0, i * inc);
    table->SetValue(i, 1, cos(i * inc) + 0.0);
    table->SetValue(i, 2, sin(i * inc) + 0.0);
    table->SetValue(i, 3, sin(i * inc) + 0.5);
    table->SetValue(i, 4, 1.0);
  }

  // Add multiple line plots, setting the colors etc
  vtkPlot *line = chart->AddPlot(vtkChart::LINE);
  line->SetInputData(table.GetPointer(), 0, 1);
  line->SetColor(0, 255, 0, 255);
  line->SetWidth(1.0);
  line = chart->AddPlot(vtkChart::LINE);
  line->SetInputData(table.GetPointer(), 0, 2);
  line->SetColor(255, 0, 0, 255);
  line->SetWidth(5.0);
  line = chart->AddPlot(vtkChart::LINE);
  line->SetInputData(table.GetPointer(), 0, 3);
  line->SetColor(0, 0, 255, 255);
  line->SetWidth(4.0);

  // Render the scene and compare the image to a reference image
  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();

  // Verify that log-scaling is improper for both x & y axes
  double bds[4];
  line->GetUnscaledInputBounds(bds);
  if (bds[0] * bds[1] > 0. || bds[2] * bds[3] > 0.)
  {
    cerr
      << "ERROR: Data on both X and Y axes expected to cross origin.\n";
    status = EXIT_FAILURE;
  }

  // Verify that log-scaling is proper for arr1 y axis (which
  // is not plotted so as to avoid changing baseline images).
  line = chart->AddPlot(vtkChart::LINE);
  line->SetInputData(table.GetPointer(), 0, 4);
  line->Update();
  line->GetUnscaledInputBounds(bds);
  if (bds[0] * bds[1] > 0. || bds[2] * bds[3] <= 0.)
  {
    cerr
      << "ERROR: Data on X axis expected to cross origin.\n";
    status = EXIT_FAILURE;
  }

  return status;
}
