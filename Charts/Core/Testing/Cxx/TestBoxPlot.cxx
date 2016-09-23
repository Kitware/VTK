/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestBoxPlot.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkAxis.h"
#include "vtkChartBox.h"
#include "vtkContextScene.h"
#include "vtkContextView.h"
#include "vtkFloatArray.h"
#include "vtkIntArray.h"
#include "vtkLookupTable.h"
#include "vtkNew.h"
#include "vtkPlotBox.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkStringArray.h"
#include "vtkTable.h"

//----------------------------------------------------------------------------
int TestBoxPlot(int , char* [])
{
  // Set up a 2D scene, add an XY chart to it
  vtkNew<vtkContextView> view;
  view->GetRenderWindow()->SetSize(400, 400);
  view->GetRenderWindow()->SetMultiSamples(0);

  vtkNew<vtkChartBox> chart;
  view->GetScene()->AddItem(chart.GetPointer());

  // Creates a vtkPlotBox input table
  // The vtkPlotBox object will display 4 (arbitrary) box plots
  int numParam = 5;
  vtkNew<vtkTable> inputBoxPlotTable;

  for (int i = 0; i < numParam; i++)
  {
    char num[10];
    sprintf(num, "P%d", i);
    vtkNew<vtkIntArray> arrIndex;
    arrIndex->SetName(num);
    inputBoxPlotTable->AddColumn(arrIndex.GetPointer());
  }

  inputBoxPlotTable->SetNumberOfRows(5);

  // This scaling parameter can be used to test Y axis positioning
  const double scale = 1e02;
  for (int i = 0; i < numParam; i++)
  {
    inputBoxPlotTable->SetValue(0, i, (i/2) * scale); //Q0
    inputBoxPlotTable->SetValue(1, i, (2*i + 2 - i) * scale); //Q1
    inputBoxPlotTable->SetValue(2, i, (2*i + 4) * scale); //Q2
    inputBoxPlotTable->SetValue(3, i, (2*i + 7) * scale); //Q3
    inputBoxPlotTable->SetValue(4, i, (2*i + 8) * scale); //Q4
  }

  vtkNew<vtkLookupTable> lookup;
  lookup->SetNumberOfColors(5);
  lookup->SetRange(0, 4);
  lookup->Build();

  chart->GetPlot(0)->SetInputData(inputBoxPlotTable.GetPointer());
  chart->SetColumnVisibilityAll(true);
  chart->SetShowLegend(true);

  // Hide one box plot
  chart->SetColumnVisibility(3, false);

  // Set the labels
  vtkNew<vtkStringArray> labels;
  labels->SetNumberOfValues(5);
  labels->SetValue(0, "Param 0");
  labels->SetValue(1, "Param 1");
  labels->SetValue(2, "Param 2");
  labels->SetValue(3, "Param 3");
  labels->SetValue(4, "Param 4");
  chart->GetPlot(0)->SetLabels(labels.GetPointer());

  // Manually change the color of one serie
  double rgb[3] = { 0.5, 0.5, 0.5 };
  vtkPlotBox::SafeDownCast(chart->GetPlot(0))->SetColumnColor("P1", rgb);

  // Render the scene
  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->Render();
  view->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
