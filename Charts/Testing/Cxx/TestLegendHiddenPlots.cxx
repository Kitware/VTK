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

#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkNew.h"
#include "vtkChartXY.h"
#include "vtkAxis.h"
#include "vtkPlotPoints.h"
#include "vtkTable.h"
#include "vtkFloatArray.h"
#include "vtkContextView.h"
#include "vtkContextScene.h"
#include "vtkRenderWindowInteractor.h"

//----------------------------------------------------------------------------
int TestLegendHiddenPlots(int , char * [])
{
  // Set up a 2D scene, add an XY chart to it
  vtkNew<vtkContextView> view;
  view->GetRenderer()->SetBackground(1.0, 1.0, 1.0);
  view->GetRenderWindow()->SetSize(400, 300);
  vtkNew<vtkChartXY> chart;
  view->GetScene()->AddItem(chart.GetPointer());
  chart->SetShowLegend(true);
  chart->GetAxis(vtkAxis::LEFT)->SetTitle("");
  chart->GetAxis(vtkAxis::LEFT)->SetRange(-1.5, 1.5);
  chart->GetAxis(vtkAxis::LEFT)->SetBehavior(vtkAxis::FIXED);
  chart->GetAxis(vtkAxis::BOTTOM)->SetTitle("");


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
  vtkNew<vtkFloatArray> arrT;
  arrT->SetName("Tan");
  table->AddColumn(arrT.GetPointer());
  // Test charting with a few more points...
  int numPoints = 40;
  float inc = 7.5 / (numPoints-1);
  table->SetNumberOfRows(numPoints);
  for (int i = 0; i < numPoints; ++i)
    {
    table->SetValue(i, 0, i * inc);
    table->SetValue(i, 1, cos(i * inc) + 0.0);
    table->SetValue(i, 2, sin(i * inc) + 0.0);
    table->SetValue(i, 3, tan(i * inc) + 0.5);
    }

  // Add multiple line plots, setting the colors etc
  vtkPlot *points = chart->AddPlot(vtkChart::POINTS);
  points->SetInputData(table.GetPointer(), 0, 1);
  points->SetColor(0, 0, 0, 255);
  points->SetWidth(1.0);
  points->SetLabel("cos(x)");
  vtkPlotPoints::SafeDownCast(points)->SetMarkerStyle(vtkPlotPoints::CROSS);
  points = chart->AddPlot(vtkChart::POINTS);
  points->SetInputData(table.GetPointer(), 0, 2);
  points->SetColor(0, 0, 0, 255);
  points->SetWidth(1.0);
  points->SetLabel("sin(x)");
  vtkPlotPoints::SafeDownCast(points)->SetMarkerStyle(vtkPlotPoints::PLUS);
  points = chart->AddPlot(vtkChart::POINTS);
  points->SetInputData(table.GetPointer(), 0, 3);
  points->SetColor(0, 0, 255, 255);
  points->SetWidth(2.0);
  // Hide this plot in the legend
  points->SetLabel("");

  //Finally render the scene and compare the image to a reference image
  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
