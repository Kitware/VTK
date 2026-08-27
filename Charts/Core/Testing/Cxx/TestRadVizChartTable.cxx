// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "TestParallelCoordinatesUtilities.h"
#include "vtkChartRadViz.h"
#include "vtkContextScene.h"
#include "vtkContextView.h"
#include "vtkNew.h"
#include "vtkPen.h"
#include "vtkPlotRadViz.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkStringScanner.h"
#include "vtkTesting.h"
#include "vtkXMLTableWriter.h"

enum TestType
{
  ALL_COLUMNS,
  HIDE_COLUMN,
  HIDE_TWO_COLUMNS,
};

//------------------------------------------------------------------------------
int TestRadVizChartTable(int argc, char* argv[])
{
  int testType;
  VTK_FROM_CHARS_IF_ERROR_RETURN(argv[1], testType, EXIT_FAILURE);

  vtkNew<vtkTesting> testUtils;
  testUtils->AddArguments(argc, argv);

  // Set up a 2D scene, add an RadViz chart to it
  vtkNew<vtkContextView> view;
  view->GetRenderer()->SetBackground(1.0, 1.0, 1.0);
  view->GetRenderWindow()->SetSize(600, 350);
  vtkNew<vtkChartRadViz> chart;
  view->GetScene()->AddItem(chart);

  // Create a table with some points in it...
  vtkSmartPointer<vtkTable> table = ::CreateDummyData();
  // vtkNew<vtkXMLTableWriter> tableWriter;
  // tableWriter->SetFileName("table.vtt");
  // tableWriter->SetInputDataObject(table);
  // tableWriter->Write();

  // Add a RadViz plot
  vtkPlotRadViz* plot = vtkPlotRadViz::SafeDownCast(chart->AddPlot(0));

  plot->GetPen()->SetWidth(4);
  plot->SetInputData(table);

  chart->SetShowLegend(false);
  if (testType == ALL_COLUMNS)
  {
    plot->SetPartitionIndices({ 0, 1, 2, 3 });
    plot->SetPartitionOffsets({ 1, 3, 4 });
  }
  else if (testType == HIDE_COLUMN)
  {
    chart->SetColumnVisibility("tangent", false);
    plot->SetPartitionIndices({ 0, 1, 2 });
    plot->SetPartitionOffsets({ 1, 3 });
  }
  else if (testType == HIDE_TWO_COLUMNS)
  {
    chart->SetColumnVisibility("tangent", false);
    chart->SetColumnVisibility("x", false);
  }
  else
  {
    return EXIT_FAILURE;
  }

  // Finally render the scene and compare the image to a reference image
  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
