/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestLinePlot2.cxx

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
#include "vtkFloatArray.h"
#include "vtkContextView.h"
#include "vtkContextScene.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkAxis.h"
#include "vtkNew.h"

// Traced data from Talbot et. al paper
static double data_x[] = {8.1, 8.6, 8.65, 8.9, 8.95, 9.2, 9.4, 9.6, 9.9, 10,
                          10.1, 10.1, 10.15, 10.3, 10.35, 10.5, 10.52, 10.55,
                          10.85, 10.95, 11.05, 11.07, 11.15, 11.3, 11.4, 11.6,
                          11.95, 12.6, 12.85, 13.1, 14.1};
static double data_y[] = {59.9, 60.5, 54.1, 54.25, 49, 50, 48, 45.2, 51.1, 47,
                          51, 45.8, 51.1, 47.2, 52, 46, 48, 47.6, 49, 41.5,
                          45.5, 44.7, 46.5, 44.1, 48.5, 44.8, 45.1, 39, 38.7,
                          38.9, 37.8};

//----------------------------------------------------------------------------
int TestLinePlot2(int, char * [])
{
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
  arrC->SetName("Y Axis");
  table->AddColumn(arrC.GetPointer());
  int numPoints = 31;
  table->SetNumberOfRows(numPoints);
  for (int i = 0; i < numPoints; ++i)
    {
    table->SetValue(i, 0, data_x[i] );
    table->SetValue(i, 1, data_y[i]);
    }

  // Add a plot of points, setting the colors etc
  vtkPlot *line = chart->AddPlot(vtkChart::POINTS);
  line->SetInput(table.GetPointer(), 0, 1);
  line->SetColor(0, 255, 0, 255);
  line->SetWidth(1.0);

  // Tell the axes to use the new tick label placement algorithm.
  chart->GetAxis(vtkAxis::LEFT)
      ->SetTickLabelAlgorithm(vtkAxis::TICK_WILKINSON_EXTENDED);
  chart->GetAxis(vtkAxis::BOTTOM)
      ->SetTickLabelAlgorithm(vtkAxis::TICK_WILKINSON_EXTENDED);

  // Finally, render the scene.
  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();
  return EXIT_SUCCESS;
}
