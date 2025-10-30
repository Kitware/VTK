// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkChartXY.h"
#include "vtkContextActor.h"
#include "vtkContextScene.h"
#include "vtkContextView.h"
#include "vtkDoubleArray.h"
#include "vtkIntArray.h"
#include "vtkNew.h"
#include "vtkPlot.h"
#include "vtkPlotBar.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTable.h"

// Monthly circulation data
static int data_2008[] = { 10822, 10941, 9979, 10370, 9460, 11228, 15093, 12231, 10160, 9816, 9384,
  7892 };
static int data_2009[] = { 9058, 9474, 9979, 9408, 8900, 11569, 14688, 12231, 10294, 9585, 8957,
  8590 };
static int data_2010[] = { 9058, 10941, 9979, 10270, 8900, 11228, 14688, 12231, 10160, 9585, 9384,
  8590 };

//------------------------------------------------------------------------------
int TestBarGraphShiftScale(int, char*[])
{
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(600, 300);
  vtkNew<vtkRenderer> lRen;
  ;
  renWin->AddRenderer(lRen);
  lRen->SetViewport(0.0, 0.0, 0.5, 1.0);
  lRen->SetBackground(1.0, 1.0, 1.0);
  vtkNew<vtkRenderer> rRen;
  ;
  renWin->AddRenderer(rRen);
  rRen->SetViewport(0.5, 0.0, 1.0, 1.0);
  rRen->SetBackground(1.0, 1.0, 1.0);
  // Set up a 2D scene on the left, add an XY chart to it
  vtkNew<vtkContextScene> lScene;
  lScene->SetRenderer(lRen);
  vtkNew<vtkChartXY> lChart;
  lScene->AddItem(lChart);
  vtkNew<vtkContextActor> lChartActor;
  lChartActor->SetScene(lScene);
  lRen->AddActor(lChartActor);

  // Set up a 2D scene, add an XY chart to it
  vtkNew<vtkContextScene> rScene;
  rScene->SetRenderer(rRen);
  vtkNew<vtkChartXY> rChart;
  rScene->AddItem(rChart);
  vtkNew<vtkContextActor> rChartActor;
  rChartActor->SetScene(rScene);
  rRen->AddActor(rChartActor);

  // Create a table with some points in it...
  vtkNew<vtkTable> table;

  vtkNew<vtkDoubleArray> arrMonth;
  arrMonth->SetName("Month");
  table->AddColumn(arrMonth);

  vtkNew<vtkIntArray> arr2008;
  arr2008->SetName("2008");
  table->AddColumn(arr2008);

  vtkNew<vtkIntArray> arr2009;
  arr2009->SetName("2009");
  table->AddColumn(arr2009);

  vtkNew<vtkIntArray> arr2010;
  arr2010->SetName("2010");
  table->AddColumn(arr2010);

  table->SetNumberOfRows(2);
  for (int i = 0; i < 2; i++)
  {
    table->SetValue(i, 0, (i + 3) * 1e20 + 1e24);
    table->SetValue(i, 1, data_2008[i] + 2e6);
    table->SetValue(i, 2, data_2009[i] * 1e2);
    table->SetValue(i, 3, data_2010[i] + 3e6);
  }

  // Add multiple bar plots, setting the colors etc
  vtkPlot* plot = nullptr;
  vtkPlotBar* barPlot = nullptr;

  plot = lChart->AddPlot(vtkChart::BAR);
  plot->SetInputData(table, 0, 1);
  plot->SetColor(0, 255, 0, 255);

  plot = lChart->AddPlot(vtkChart::BAR);
  plot->SetInputData(table, 0, 2);
  plot->SetColor(255, 0, 0, 255);

  plot = lChart->AddPlot(vtkChart::BAR);
  plot->SetInputData(table, 0, 3);
  plot->SetColor(0, 0, 255, 255);

  plot = rChart->AddPlot(vtkChart::BAR);
  barPlot = vtkPlotBar::SafeDownCast(plot);
  barPlot->SetOrientation(vtkPlotBar::HORIZONTAL);
  plot->SetInputData(table, 0, 1);
  plot->SetColor(0, 255, 0, 255);

  plot = rChart->AddPlot(vtkChart::BAR);
  barPlot = vtkPlotBar::SafeDownCast(plot);
  barPlot->SetOrientation(vtkPlotBar::HORIZONTAL);
  plot->SetInputData(table, 0, 2);
  plot->SetColor(255, 0, 0, 255);

  plot = rChart->AddPlot(vtkChart::BAR);
  barPlot = vtkPlotBar::SafeDownCast(plot);
  barPlot->SetOrientation(vtkPlotBar::HORIZONTAL);
  plot->SetInputData(table, 0, 3);
  plot->SetColor(0, 0, 255, 255);

  // Finally render the scene and compare the image to a reference image
  renWin->SetMultiSamples(0);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);
  renWin->Render();
  iren->Initialize();
  iren->Start();

  return EXIT_SUCCESS;
}
