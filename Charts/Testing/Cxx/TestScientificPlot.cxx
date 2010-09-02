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
#include "vtkAxis.h"
#include "vtkChartXY.h"
#include "vtkPlot.h"
#include "vtkTable.h"
#include "vtkFloatArray.h"
#include "vtkContextView.h"
#include "vtkContextScene.h"
#include "vtkRenderWindowInteractor.h"

//----------------------------------------------------------------------------
int TestScientificPlot(int, char * [])
{
  // Set up a 2D scene, add an XY chart to it
  vtkSmartPointer<vtkContextView> view =
      vtkSmartPointer<vtkContextView>::New();
  view->GetRenderWindow()->SetSize(400, 400);
  vtkSmartPointer<vtkChartXY> chart = vtkSmartPointer<vtkChartXY>::New();
  view->GetScene()->AddItem(chart);

  // Create a table with some points in it...
  vtkSmartPointer<vtkTable> table = vtkSmartPointer<vtkTable>::New();
  vtkSmartPointer<vtkFloatArray> arrX = vtkSmartPointer<vtkFloatArray>::New();
  arrX->SetName("X Axis");
  table->AddColumn(arrX);
  vtkSmartPointer<vtkFloatArray> arrC = vtkSmartPointer<vtkFloatArray>::New();
  arrC->SetName("cos");
  table->AddColumn(arrC);
  vtkSmartPointer<vtkFloatArray> arrS = vtkSmartPointer<vtkFloatArray>::New();
  arrS->SetName("sin");
  table->AddColumn(arrS);
  vtkSmartPointer<vtkFloatArray> arrS2 = vtkSmartPointer<vtkFloatArray>::New();
  arrS2->SetName("x^3");
  table->AddColumn(arrS2);
  // Test charting with a few more points...
  int numPoints = 69;
  float inc = 3.0 / (numPoints-1);
  table->SetNumberOfRows(numPoints);
  for (int i = 0; i < numPoints; ++i)
    {
    double v = -1.0 + i * inc;
    table->SetValue(i, 0, v);
    table->SetValue(i, 1, cos(v));
    table->SetValue(i, 2, sin(v) + 0.0);
    table->SetValue(i, 3, v*v*v);
    }

  // Add multiple line plots, setting the colors etc
  vtkPlot *line = chart->AddPlot(vtkChart::LINE);
  line->SetInput(table, 0, 1);
  line->SetColor(0, 255, 0, 255);
  line = chart->AddPlot(vtkChart::LINE);
  line->SetInput(table, 0, 2);
  line->SetColor(255, 0, 0, 255);
  line = chart->AddPlot(vtkChart::POINTS);
  line->SetInput(table, 0, 3);
  line->SetColor(0, 0, 255, 255);

  // Set up a scientific plot...
  chart->SetDrawAxesAtOrigin(true);
  chart->SetShowLegend(true);
  chart->GetAxis(vtkAxis::LEFT)->SetRange(1.0, -1.5);
  chart->GetAxis(vtkAxis::LEFT)->SetNotation(2);
  chart->GetAxis(vtkAxis::LEFT)->SetPrecision(1);
  chart->GetAxis(vtkAxis::LEFT)->SetBehavior(vtkAxis::FIXED);
  chart->GetAxis(vtkAxis::LEFT)->SetTitle("");
  chart->GetAxis(vtkAxis::BOTTOM)->SetRange(-1.0, 1.5);
  chart->GetAxis(vtkAxis::BOTTOM)->SetNotation(2);
  chart->GetAxis(vtkAxis::BOTTOM)->SetPrecision(1);
  chart->GetAxis(vtkAxis::BOTTOM)->SetBehavior(vtkAxis::FIXED);
  chart->GetAxis(vtkAxis::BOTTOM)->SetTitle("");

  //Finally render the scene and compare the image to a reference image
  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();
  return EXIT_SUCCESS;
}
