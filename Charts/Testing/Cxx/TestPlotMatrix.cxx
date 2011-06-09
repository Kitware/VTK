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
#include "vtkAxis.h"
#include "vtkPlot.h"
#include "vtkTable.h"
#include "vtkFloatArray.h"
#include "vtkContextView.h"
#include "vtkContextScene.h"
#include "vtkRenderWindowInteractor.h"

//----------------------------------------------------------------------------
int TestPlotMatrix( int, char * [] )
{
  // Set up a 2D scene, add an XY chart to it
  vtkSmartPointer<vtkContextView> view =
      vtkSmartPointer<vtkContextView>::New();
  view->GetRenderWindow()->SetSize(400, 300);
  vtkSmartPointer<vtkChartXY> chart = vtkSmartPointer<vtkChartXY>::New();
  view->GetScene()->AddItem(chart);

  // Create a table with some points in it...
  vtkSmartPointer<vtkTable> table = vtkSmartPointer<vtkTable>::New();
  vtkSmartPointer<vtkFloatArray> arrX = vtkSmartPointer<vtkFloatArray>::New();
  arrX->SetName("X Axis");
  table->AddColumn(arrX);
  vtkSmartPointer<vtkFloatArray> arrC = vtkSmartPointer<vtkFloatArray>::New();
  arrC->SetName("Cosine");
  table->AddColumn(arrC);
  vtkSmartPointer<vtkFloatArray> arrS = vtkSmartPointer<vtkFloatArray>::New();
  arrS->SetName("Sine");
  table->AddColumn(arrS);
  vtkSmartPointer<vtkFloatArray> arrS2 = vtkSmartPointer<vtkFloatArray>::New();
  arrS2->SetName("Sine2");
  table->AddColumn(arrS2);
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
    }

  // Add multiple line plots, setting the colors etc
  vtkPlot *line = chart->AddPlot(vtkChart::LINE);
  line->SetInputData(table, 0, 1);
  line->SetColor(0, 255, 0, 255);
  line->SetWidth(1.0);
  line = chart->AddPlot(vtkChart::LINE);
  line->SetInputData(table, 0, 2);
  line->SetColor(255, 0, 0, 255);
  line->SetWidth(5.0);
  line = chart->AddPlot(vtkChart::LINE);
  line->SetInputData(table, 0, 3);
  line->SetColor(0, 0, 255, 255);
  line->SetWidth(4.0);

  // Set to fixes size, and resize to make it small.
  chart->SetAutoSize(false);
  chart->SetSize(vtkRectf(0.0, 0.0, 200.0, 150.0));

  // Now Set up another plot with cos
  vtkSmartPointer<vtkChartXY> chart2 = vtkSmartPointer<vtkChartXY>::New();
  view->GetScene()->AddItem(chart2);
  line = chart2->AddPlot(vtkChart::LINE);
  line->SetInputData(table, 0, 1);
  chart2->SetAutoSize(false);
  chart2->SetSize(vtkRectf(200.0, 0.0, 200.0, 150.0));

  // Now Set up another plot with cos
  vtkSmartPointer<vtkChartXY> chart3 = vtkSmartPointer<vtkChartXY>::New();
  view->GetScene()->AddItem(chart3);
  line = chart3->AddPlot(vtkChart::POINTS);
  line->SetInputData(table, 0, 1);
  chart3->SetAutoSize(false);
  chart3->SetSize(vtkRectf(0.0, 150.0, 200.0, 150.0));

  // Now Set up another plot with cos
  vtkSmartPointer<vtkChartXY> chart4 = vtkSmartPointer<vtkChartXY>::New();
  view->GetScene()->AddItem(chart4);
  line = chart4->AddPlot(vtkChart::BAR);
  line->SetInputData(table, 0, 1);
  chart4->GetAxis(vtkAxis::BOTTOM)->SetBehavior(vtkAxis::FIXED);
  chart4->GetAxis(vtkAxis::BOTTOM)->SetRange(0.0, 10.0);
  chart4->SetAutoSize(false);
  chart4->SetSize(vtkRectf(200.0, 150.0, 200.0, 150.0));

  //Finally render the scene and compare the image to a reference image
  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();
  return EXIT_SUCCESS;
}
