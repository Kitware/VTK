// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "TestParallelCoordinatesUtilities.h"
#include "vtkChartRadViz.h"
#include "vtkContextScene.h"
#include "vtkContextView.h"
#include "vtkNew.h"
#include "vtkPen.h"
#include "vtkPlot.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkTesting.h"
// #include "vtkXMLTableWriter.h"

//------------------------------------------------------------------------------
int TestRadVizChartTable(int argc, char** argv)
{
  vtkNew<vtkTesting> testUtils;
  testUtils->AddArguments(argc, argv);
  std::string dataRoot = testUtils->GetDataRoot();

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
  vtkPlot* plot = chart->AddPlot(0);
  plot->GetPen()->SetWidth(4);

  plot->SetInputData(table);

  chart->SetShowLegend(false);

  // Finally render the scene and compare the image to a reference image
  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
