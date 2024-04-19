// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkChartLegend.h"
#include "vtkChartParallelCoordinates.h"
#include "vtkContextScene.h"
#include "vtkContextView.h"
#include "vtkFloatArray.h"
#include "vtkLookupTable.h"
#include "vtkNew.h"
#include "vtkPen.h"
#include "vtkPlotParallelCoordinates.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkUnsignedCharArray.h"

//------------------------------------------------------------------------------
int TestParallelCoordinatesLegend(int, char*[])
{
  // Set up a 2D scene, add an XY chart to it
  vtkNew<vtkContextView> view;
  view->GetRenderer()->SetBackground(1.0, 1.0, 1.0);
  view->GetRenderWindow()->SetSize(800, 600);
  vtkNew<vtkChartParallelCoordinates> chart;
  view->GetScene()->AddItem(chart);

  // Create a table with some points in it...
  vtkNew<vtkTable> table;
  vtkNew<vtkFloatArray> arrX;
  arrX->SetName("Field 1");
  table->AddColumn(arrX);
  vtkNew<vtkFloatArray> arrC;
  arrC->SetName("Field 2");
  table->AddColumn(arrC);
  vtkNew<vtkFloatArray> arrS;
  arrS->SetName("Field 3");
  table->AddColumn(arrS);
  vtkNew<vtkFloatArray> arrS2;
  arrS2->SetName("Field 4");
  table->AddColumn(arrS2);
  vtkNew<vtkUnsignedCharArray> colorsArr;
  colorsArr->SetName("Colors");
  colorsArr->SetNumberOfComponents(4);
  table->AddColumn(colorsArr);
  vtkNew<vtkStringArray> labelsArr;
  labelsArr->SetName("Labels");

  int numPoints = 10;
  vtkNew<vtkLookupTable> lut;
  lut->SetVectorModeToRGBColors();
  lut->SetTableRange(0, numPoints - 1);
  lut->SetHueRange(0, 0.667);
  lut->SetSaturationRange(1, 1);
  lut->SetValueRange(1, 1);
  lut->Build();

  // Test charting with a few more points...
  float inc = 7.5 / (numPoints - 1);
  table->SetNumberOfRows(numPoints);
  for (int i = 0; i < numPoints; ++i)
  {
    table->SetValue(i, 0, i * inc);
    table->SetValue(i, 1, cos(i * inc) + 0.0);
    table->SetValue(i, 2, sin(i * inc) + 0.0);
    table->SetValue(i, 3, tan(i * inc) + 0.5);
    colorsArr->SetTypedTuple(i, lut->MapValue(i));
    const std::string label = "Label " + std::to_string(i);
    labelsArr->InsertNextValue(label.c_str());
  }

  auto plot = vtkPlotParallelCoordinates::SafeDownCast(chart->GetPlot(0));
  plot->GetPen()->SetLineType(vtkPen::SOLID_LINE);
  // change default opacity from 25 to 255
  plot->GetPen()->SetColor(0, 0, 0, 255);
  plot->SetLabels(labelsArr);
  plot->SetInputData(table);
  plot->SetWidth(2);
  plot->SetColorModeToDefault();
  plot->SetLookupTable(lut);
  plot->SetScalarVisibility(true);
  plot->SelectColorArray("Colors");
  chart->SetColumnVisibility("Colors", false);
  chart->SetShowLegend(true);
  chart->GetLegend()->SetInline(false);

  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();
  return EXIT_SUCCESS;
}
