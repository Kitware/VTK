// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include <vtkAxis.h>
#include <vtkChartLegend.h>
#include <vtkChartXY.h>
#include <vtkContextScene.h>
#include <vtkContextView.h>
#include <vtkFloatArray.h>
#include <vtkIntArray.h>
#include <vtkInteractorEventRecorder.h>
#include <vtkPlot.h>
#include <vtkPlotBar.h>
#include <vtkPlotLine.h>
#include <vtkPlotRangeHandlesItem.h>
#include <vtkRangeHandlesItem.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkTable.h>

int TestChartPicking(int, char*[])
{
  // Create a table with some points in it
  vtkNew<vtkTable> table;

  vtkNew<vtkFloatArray> arrX;
  arrX->SetName("X Axis");
  table->AddColumn(arrX);

  vtkNew<vtkFloatArray> arrC;
  arrC->SetName("Cosine");
  table->AddColumn(arrC);

  // Fill in the table with some example values
  int numPoints = 12;
  table->SetNumberOfRows(numPoints);
  for (int i = 1; i <= numPoints; ++i)
  {
    table->SetValue(i - 1, 0, i);
    table->SetValue(i - 1, 1, i);
  }

  // Set up the view
  vtkNew<vtkContextView> view;
  view->GetRenderer()->SetBackground(1.0, 1.0, 1.0);

  // Add multiple line plots, setting the colors etc
  vtkNew<vtkChartXY> chart;
  view->GetScene()->AddItem(chart);
  vtkPlot* line = chart->AddPlot(vtkChart::LINE);
  line->SetInputData(table, 0, 1);
  line->SetColor(0, 255, 0, 255);
  line->SetWidth(1.0);
  line->SetLegendVisibility(true);

  vtkNew<vtkPlotRangeHandlesItem> rangeItem;
  rangeItem->SetExtent(0, 12, 0, 30);
  chart->AddPlot(rangeItem);

  vtkNew<vtkPlotRangeHandlesItem> HRangeItem;
  HRangeItem->SetHandleOrientationToHorizontal();
  chart->AddPlot(HRangeItem);
  chart->RaisePlot(HRangeItem);
  chart->GetAxis(vtkAxis::TOP)->SetVisible(true);
  chart->GetAxis(vtkAxis::RIGHT)->SetVisible(true);
  chart->GetAxis(vtkAxis::BOTTOM)->SetVisible(false);
  chart->DrawAxesAtOriginOff();
  chart->AutoAxesOff();

  vtkNew<vtkTable> plotBarTable;

  vtkNew<vtkIntArray> arrMonth;
  arrMonth->SetNumberOfComponents(1);
  arrMonth->SetName("Month");
  for (int i = 1; i < 12; i++)
  {
    arrMonth->InsertNextTuple1(i);
  }
  plotBarTable->AddColumn(arrMonth);

  int books[12] = { 6, 9, 3, 9, 5, 3, 8, 0, 4, 9, 5, 1 };
  vtkNew<vtkIntArray> arrBooks;
  arrBooks->SetName("Books");
  for (int i = 1; i < 12; i++)
  {
    arrBooks->InsertNextTuple1(books[i]);
  }
  plotBarTable->AddColumn(arrBooks);

  vtkPlotBar* bar1 = vtkPlotBar::SafeDownCast(chart->AddPlot(vtkChart::BAR));
  bar1->SetInputData(plotBarTable, "Month", "Books");

  chart->RaisePlot(rangeItem);
  chart->RaisePlot(HRangeItem);
  chart->RaisePlot(line);
  chart->SetShowLegend(true);

  // Start interactor
  view->GetRenderWindow()->Render();
  view->GetInteractor()->Initialize();

  vtkNew<vtkInteractorEventRecorder> recorder;
  recorder->SetInteractor(view->GetInteractor());
  recorder->ReadFromInputStringOn();
  recorder->SetInputString("LeftButtonPressEvent 33 105 0 0 0 0 0\n");
  recorder->Play();
  if (vtkAxis::SafeDownCast(view->GetScene()->GetPickedItem()) == nullptr)
  {
    return EXIT_FAILURE;
  }
  recorder->SetInputString("LeftButtonPressEvent 55 115 0 0 0 0 0\n");
  recorder->Play();
  if (vtkPlotBar::SafeDownCast(view->GetScene()->GetPickedItem()) == nullptr)
  {
    return EXIT_FAILURE;
  }
  recorder->SetInputString("LeftButtonPressEvent 139 144 0 0 0 0 0\n");
  recorder->Play();
  if (vtkPlotLine::SafeDownCast(view->GetScene()->GetPickedItem()) == nullptr)
  {
    return EXIT_FAILURE;
  }
  recorder->SetInputString("LeftButtonPressEvent 230 37 0 0 0 0 0\n");
  recorder->Play();
  if (vtkPlotRangeHandlesItem::SafeDownCast(view->GetScene()->GetPickedItem()) == nullptr)
  {
    return EXIT_FAILURE;
  }
  recorder->SetInputString("LeftButtonPressEvent 236 257 0 0 0 0 0\n");
  recorder->Play();
  if (vtkChartLegend::SafeDownCast(view->GetScene()->GetPickedItem()) == nullptr)
  {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
