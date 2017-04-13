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

#include "vtkSmartPointer.h"

#include "vtkChartBox.h"
#include "vtkStatisticsAlgorithm.h"
#include "vtkComputeQuartiles.h"
#include "vtkContextScene.h"
#include "vtkContextView.h"
#include "vtkIntArray.h"
#include "vtkLookupTable.h"
#include "vtkPlotBox.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkAxis.h"
#include "vtkTextProperty.h"
#include "vtkCommand.h"

#include "vtkTestErrorObserver.h"

//----------------------------------------------------------------------------
int TestBoxPlot2(int , char* [])
{
  // Set up a 2D scene, add an XY chart to it
  vtkSmartPointer<vtkContextView> view =
    vtkSmartPointer<vtkContextView>::New();
  view->GetRenderWindow()->SetSize(400, 400);
  view->GetRenderWindow()->SetMultiSamples(0);

  vtkSmartPointer<vtkChartBox> chart =
    vtkSmartPointer<vtkChartBox>::New();
  view->GetScene()->AddItem(chart);

  // Creates a vtkPlotBox input table
  int numberOfColumns = 5;
  vtkSmartPointer<vtkTable> inputBoxPlotTable =
    vtkSmartPointer<vtkTable>::New();

  for (int i = 0; i < numberOfColumns; ++i)
    {
    char num[10];
    snprintf(num, sizeof(num), "Run %d", i + 1);
    vtkSmartPointer<vtkIntArray> arrIndex =
      vtkSmartPointer<vtkIntArray>::New();
    arrIndex->SetName(num);
    inputBoxPlotTable->AddColumn(arrIndex);
    }

  // Data from the Michelson Morley experiment
  inputBoxPlotTable->SetNumberOfRows(20);
  double values[20][5] =
    {
      {850,     960,     880,     890,     890},
      {740,     940,     880,     810,     840},
      {900,     960,     880,     810,     780},
      {1070,    940,     860,     820,     810},
      {930,     880,     720,     800,     760},
      {850,     800,     720,     770,     810},
      {950,     850,     620,     760,     790},
      {980,     880,     860,     740,     810},
      {980,     900,     970,     750,     820},
      {880,     840,     950,     760,     850},
      {1000,    830,     880,     910,     870},
      {980,     790,     910,     920,     870},
      {930,     810,     850,     890,     810},
      {650,     880,     870,     860,     740},
      {760,     880,     840,     880,     810},
      {810,     830,     840,     720,     940},
      {1000,    800,     850,     840,     950},
      {1000,    790,     840,     850,     800},
      {960,     760,     840,     850,     810},
      {960,     800,     840,     780,     870}
    };
  for (int j = 0; j < 20; ++j)
    {
    for (int i = 0; i < 5; ++i)
      {
      inputBoxPlotTable->SetValue(j, i, values[j][i]);
      }
    }
  // Compute the min, q1, median, q3 qnd max.
  vtkSmartPointer<vtkComputeQuartiles> quartiles =
    vtkSmartPointer<vtkComputeQuartiles>::New();
  quartiles->SetInputData(vtkStatisticsAlgorithm::INPUT_DATA, inputBoxPlotTable);
  quartiles->Update();

  vtkSmartPointer<vtkLookupTable> lookup =
    vtkSmartPointer<vtkLookupTable>::New();
  lookup->SetNumberOfColors(5);
  lookup->SetRange(0, 4);
  lookup->Build();

  chart->GetPlot(0)->SetInputData(inputBoxPlotTable);
  chart->GetPlot(0)->LegendVisibilityOn();
  chart->SetColumnVisibilityAll(true);
  chart->SetTitle("Michelson-Morley experiment");
  chart->GetTitleProperties()->SetVerticalJustificationToTop();
  chart->GetTitleProperties()->SetFontSize(20);
  chart->GetTitleProperties()->FrameOn();
  chart->GetYAxis()->SetTitle("Speed of Light (km/s - 299000)");
  vtkSmartPointer<vtkTest::ErrorObserver> errorObserver =
    vtkSmartPointer<vtkTest::ErrorObserver>::New();
  chart->GetPlot(0)->AddObserver(vtkCommand::ErrorEvent,errorObserver);


  // Set the labels
  vtkSmartPointer<vtkStringArray> labels =
    vtkSmartPointer<vtkStringArray>::New();
  labels->SetNumberOfValues(5);
  labels->SetValue(0, "Run 1");
  labels->SetValue(1, "Run 2");
  labels->SetValue(2, "Run 3");
  labels->SetValue(3, "Run 4");
  labels->SetValue(4, "Run 5");
  chart->GetPlot(0)->SetLabels(labels);

  // Render the scene
  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetRenderer()->SetBackground(.8, .8, .8);
  view->GetInteractor()->Initialize();

  // Detect bad input error message from PlotBox
  view->Render();
  int status = errorObserver->CheckErrorMessage("Input table must contain 5 rows per column");

  // Now render a valid plot
  vtkTable *outTable = quartiles->GetOutput();
  chart->GetPlot(0)->SetInputData(outTable);
  view->Render();

  view->GetInteractor()->Start();

  return status;
}
