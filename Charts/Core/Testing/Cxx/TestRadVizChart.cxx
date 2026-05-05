// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "TestParallelCoordinatesUtilities.h"
#include "vtkChartRadViz.h"
#include "vtkContextScene.h"
#include "vtkContextView.h"
#include "vtkDataSet.h"
#include "vtkNew.h"
#include "vtkPen.h"
#include "vtkPlot.h"
#include "vtkPlotRadViz.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTesting.h"
#include "vtkXMLImageDataReader.h"

//------------------------------------------------------------------------------
int TestRadVizChart(int argc, char** argv)
{
  vtkNew<vtkTesting> testUtils;
  testUtils->AddArguments(argc, argv);
  std::string dataRoot = testUtils->GetDataRoot();

  // Set up a 2D scene, add an XY chart to it
  vtkNew<vtkContextView> view;
  view->GetRenderer()->SetBackground(1.0, 1.0, 1.0);
  view->GetRenderWindow()->SetSize(600, 350);
  vtkNew<vtkChartRadViz> chart;
  view->GetScene()->AddItem(chart);

  // Create a table with some points in it...
  // vtkSmartPointer<vtkTable> table = ::CreateDummyData();
  vtkNew<vtkXMLImageDataReader> reader;
  reader->SetFileName((dataRoot + "/Data/Ce_Co_Fe_Gd_small.vti").c_str());
  reader->Update();

  // Add multiple line plots, setting the colors etc
  vtkPlotRadViz* plot = vtkPlotRadViz::SafeDownCast(chart->AddPlot(0));
  plot->GetPen()->SetOpacity(20);
  plot->GetPen()->SetWidth(6);

  // plot->SetInputData(table);
  plot->SetInputData(vtkDataSet::SafeDownCast(reader->GetOutputDataObject(0)));

  chart->SetShowLegend(false);

  chart->SetTitle("Ce_Co_Fe_Gd");

  // Finally render the scene and compare the image to a reference image
  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
