// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "TestParallelCoordinatesUtilities.h"

#include <vtkCallbackCommand.h>
#include <vtkChartParallelCoordinates.h>
#include <vtkContextActor.h>
#include <vtkContextKeyEvent.h>
#include <vtkContextMouseEvent.h>
#include <vtkContextView.h>
#include <vtkDoubleArray.h>
#include <vtkIdTypeArray.h>
#include <vtkInteractorEventRecorder.h>
#include <vtkNew.h>
#include <vtkPlotParallelCoordinates.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkTable.h>

static const char* eventLog = "# StreamVersion 1.2\n"
                              "ExposeEvent 0 399 0 0 0 0 0\n"
                              "TimerEvent 0 399 0 0 0 0 0\n"
                              "EnterEvent 597 18 0 0 0 0 0\n"
                              "MouseMoveEvent 597 18 0 0 0 0 0\n"
                              "MouseMoveEvent 205 283 0 0 0 0 0\n"
                              "LeftButtonPressEvent 205 283 0 0 0 0 0\n"
                              "MouseMoveEvent 206 283 0 0 0 0 0\n"
                              "MouseMoveEvent 211 264 0 0 0 0 0\n"
                              "TimerEvent 211 264 0 0 0 0 0\n"
                              "MouseMoveEvent 218 247 0 0 0 0 0\n"
                              "MouseMoveEvent 228 220 0 0 0 0 0\n"
                              "TimerEvent 228 220 0 0 0 0 0\n"
                              "MouseMoveEvent 228 210 0 0 0 0 0\n"
                              "MouseMoveEvent 230 184 0 0 0 0 0\n"
                              "TimerEvent 230 184 0 0 0 0 0\n"
                              "MouseMoveEvent 230 174 0 0 0 0 0\n"
                              "MouseMoveEvent 230 169 0 0 0 0 0\n"
                              "TimerEvent 230 169 0 0 0 0 0\n"
                              "LeftButtonReleaseEvent 230 169 0 0 0 0 0\n"
                              "MouseMoveEvent 230 170 0 0 0 0 0\n"
                              "MouseMoveEvent 228 175 0 0 0 0 0\n"
                              "TimerEvent 228 175 0 0 0 0 0\n"
                              "MouseMoveEvent 225 181 0 0 0 0 0\n"
                              "MouseMoveEvent 217 265 0 0 0 0 0\n"
                              "LeftButtonPressEvent 217 265 0 0 0 0 0\n"
                              "MouseMoveEvent 217 265 0 0 0 0 0\n"
                              "MouseMoveEvent 217 263 0 0 0 0 0\n"
                              "TimerEvent 217 263 0 0 0 0 0\n"
                              "MouseMoveEvent 220 241 0 0 0 0 0\n"
                              "MouseMoveEvent 225 199 0 0 0 0 0\n"
                              "TimerEvent 225 199 0 0 0 0 0\n"
                              "MouseMoveEvent 225 185 0 0 0 0 0\n"
                              "MouseMoveEvent 223 172 0 0 0 0 0\n"
                              "TimerEvent 223 172 0 0 0 0 0\n"
                              "MouseMoveEvent 223 170 0 0 0 0 0\n"
                              "TimerEvent 223 170 0 0 0 0 0\n"
                              "LeftButtonReleaseEvent 223 170 0 0 0 0 0\n"
                              "TimerEvent 223 170 0 0 0 0 0\n"
                              "MouseMoveEvent 224 170 0 0 0 0 0\n"
                              "MouseMoveEvent 339 212 0 0 0 0 0\n"
                              "LeftButtonPressEvent 339 212 0 0 0 0 0\n"
                              "TimerEvent 339 212 0 0 0 0 0\n"
                              "MouseMoveEvent 339 213 0 0 0 0 0\n"
                              "LeftButtonReleaseEvent 339 213 0 0 0 0 0\n"
                              "TimerEvent 339 213 0 0 0 0 0\n"
                              "MouseMoveEvent 339 213 0 0 0 0 0\n"
                              "MouseMoveEvent 568 398 0 0 0 0 0\n"
                              "MouseMoveEvent 585 397 0 0 0 0 0\n"
                              "MouseMoveEvent 599 370 0 0 0 0 0\n";

int TestParallelCoordinatesClearAllAxesSelections(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  // Set up a 2D scene, add an parallel coordinate chart to it
  vtkNew<vtkContextView> view;
  view->GetRenderWindow()->SetWindowName("TestParallelCoordinatesClearAxisSelection");
  view->GetRenderWindow()->SetSize(600, 400);
  view->GetRenderWindow()->SetMultiSamples(0);
  vtkNew<vtkChartParallelCoordinates> chart;
  view->GetScene()->AddItem(chart);

  // Create a table with some points in it...
  vtkSmartPointer<vtkTable> table = ::CreateDummyData();
  chart->GetPlot(0)->SetInputData(table);

  // record events
  vtkSmartPointer<vtkInteractorEventRecorder> recorder =
    vtkSmartPointer<vtkInteractorEventRecorder>::New();
  view->GetInteractor()->Initialize();
  recorder->SetInteractor(view->GetInteractor());

#ifdef RECORD_TESTING
  recorder->SetFileName("record.txt");
  recorder->SetEnabled(true);
  recorder->Record();
  view->GetInteractor()->Start();
#else
  recorder->ReadFromInputStringOn();
  recorder->SetInputString(eventLog);
  recorder->Play();
#endif

  if (chart->GetNumberOfPlots() != 1)
  {
    std::cerr << "Wrong number of plot. Expected 1 but got " << chart->GetNumberOfPlots()
              << std::endl;
    return EXIT_FAILURE;
  }

  vtkIdTypeArray* selectedIds = chart->GetPlot(0)->GetSelection();
  if (!selectedIds)
  {
    std::cerr << "Selection shouldn't be null" << std::endl;
    return EXIT_FAILURE;
  }

  if (selectedIds->GetNumberOfValues() != 0)
  {
    std::cerr << "Wrong number of id selection. Expected to have 14 ids but got "
              << selectedIds->GetNumberOfValues() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
