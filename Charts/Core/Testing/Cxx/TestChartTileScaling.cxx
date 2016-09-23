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

#include "vtkChartXY.h"
#include "vtkContextScene.h"
#include "vtkContextView.h"
#include "vtkFloatArray.h"
#include "vtkNew.h"
#include "vtkPlot.h"
#include "vtkPNGWriter.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"

//------------------------------------------------------------------------------
// This test mainly checks that the tick marks have the same relative spacing
// regardless of the current vtkRenderWindow::TileScale. Take care if replacing
// baselines, as the tick spacing should match the result obtained without the
// SetTileScale call.
//
// Note: At the moment (6/2/2015), there is an issue with the data / gridmarks
// not rendering properly at the tile 'seams', as can be seen in the 'valid'
// baseline. Just noting that this is expected for now.
//
int TestChartTileScaling( int, char *[])
{
  vtkNew<vtkContextView> view;
  view->GetRenderWindow()->SetMultiSamples(0);
  // Needed for image export to work on all platforms:
  view->GetRenderWindow()->SwapBuffersOff();
  view->GetRenderWindow()->SetSize(400, 300);
  // Set tile scale up.
  view->GetRenderWindow()->SetTileScale(2);

  vtkNew<vtkChartXY> chart;
  view->GetScene()->AddItem(chart.GetPointer());

  // Create a table with some points in it...
  vtkNew<vtkTable> table;
  vtkNew<vtkFloatArray> arrX;
  arrX->SetName("X Axis");
  table->AddColumn(arrX.GetPointer());
  vtkNew<vtkFloatArray> arrS;
  arrS->SetName("Sine");
  table->AddColumn(arrS.GetPointer());
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
    table->SetValue(i, 1, sin(i * inc) + 0.0);
    table->SetValue(i, 2, 1.0);
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

  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
