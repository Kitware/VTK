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
                              "EnterEvent 373 383 0 0 0 0 0\n"
                              "MouseMoveEvent 373 383 0 0 0 0 0\n"
                              "MouseMoveEvent 215 316 0 0 0 0 0\n"
                              "LeftButtonPressEvent 215 316 0 0 0 0 0\n"
                              "MouseMoveEvent 216 316 0 0 0 0 0\n"
                              "TimerEvent 216 316 0 0 0 0 0\n"
                              "MouseMoveEvent 217 313 0 0 0 0 0\n"
                              "MouseMoveEvent 215 300 0 0 0 0 0\n"
                              "TimerEvent 215 300 0 0 0 0 0\n"
                              "MouseMoveEvent 215 297 0 0 0 0 0\n"
                              "MouseMoveEvent 211 283 0 0 0 0 0\n"
                              "TimerEvent 211 283 0 0 0 0 0\n"
                              "MouseMoveEvent 211 279 0 0 0 0 0\n"
                              "MouseMoveEvent 211 271 0 0 0 0 0\n"
                              "TimerEvent 211 271 0 0 0 0 0\n"
                              "MouseMoveEvent 211 268 0 0 0 0 0\n"
                              "MouseMoveEvent 211 254 0 0 0 0 0\n"
                              "TimerEvent 211 254 0 0 0 0 0\n"
                              "MouseMoveEvent 211 250 0 0 0 0 0\n"
                              "MouseMoveEvent 211 241 0 0 0 0 0\n"
                              "TimerEvent 211 241 0 0 0 0 0\n"
                              "MouseMoveEvent 211 238 0 0 0 0 0\n"
                              "MouseMoveEvent 211 231 0 0 0 0 0\n"
                              "TimerEvent 211 231 0 0 0 0 0\n"
                              "MouseMoveEvent 212 228 0 0 0 0 0\n"
                              "MouseMoveEvent 212 227 0 0 0 0 0\n"
                              "TimerEvent 212 227 0 0 0 0 0\n"
                              "MouseMoveEvent 212 225 0 0 0 0 0\n"
                              "TimerEvent 212 225 0 0 0 0 0\n"
                              "LeftButtonReleaseEvent 212 225 0 0 0 0 0\n"
                              "TimerEvent 212 225 0 0 0 0 0\n"
                              "MouseMoveEvent 213 222 0 0 0 0 0\n"
                              "MouseMoveEvent 214 218 0 0 0 0 0\n"
                              "KeyPressEvent 214 218 0 0 1 Control_L 0\n"
                              "MouseMoveEvent 215 216 2 0 0 Control_L 0\n"
                              "MouseMoveEvent 215 202 2 0 0 Control_L 0\n"
                              "LeftButtonPressEvent 215 202 2 0 0 Control_L 0\n"
                              "MouseMoveEvent 215 201 2 0 0 Control_L 0\n"
                              "MouseMoveEvent 215 197 2 0 0 Control_L 0\n"
                              "TimerEvent 215 197 2 0 0 Control_L 0\n"
                              "MouseMoveEvent 215 193 2 0 0 Control_L 0\n"
                              "MouseMoveEvent 215 178 2 0 0 Control_L 0\n"
                              "TimerEvent 215 178 2 0 0 Control_L 0\n"
                              "MouseMoveEvent 215 171 2 0 0 Control_L 0\n"
                              "MouseMoveEvent 215 158 2 0 0 Control_L 0\n"
                              "TimerEvent 215 158 2 0 0 Control_L 0\n"
                              "MouseMoveEvent 215 152 2 0 0 Control_L 0\n"
                              "MouseMoveEvent 215 144 2 0 0 Control_L 0\n"
                              "TimerEvent 215 144 2 0 0 Control_L 0\n"
                              "MouseMoveEvent 216 142 2 0 0 Control_L 0\n"
                              "MouseMoveEvent 216 134 2 0 0 Control_L 0\n"
                              "TimerEvent 216 134 2 0 0 Control_L 0\n"
                              "MouseMoveEvent 216 131 2 0 0 Control_L 0\n"
                              "MouseMoveEvent 217 126 2 0 0 Control_L 0\n"
                              "TimerEvent 217 126 2 0 0 Control_L 0\n"
                              "MouseMoveEvent 217 124 2 0 0 Control_L 0\n"
                              "MouseMoveEvent 217 122 2 0 0 Control_L 0\n"
                              "TimerEvent 217 122 2 0 0 Control_L 0\n"
                              "LeftButtonReleaseEvent 217 122 2 0 0 Control_L 0\n"
                              "TimerEvent 217 122 2 0 0 Control_L 0\n"
                              "MouseMoveEvent 217 122 2 0 0 Control_L 0\n"
                              "MouseMoveEvent 217 96 2 0 0 Control_L 0\n"
                              "LeftButtonPressEvent 217 96 2 0 0 Control_L 0\n"
                              "TimerEvent 217 96 2 0 0 Control_L 0\n"
                              "LeftButtonReleaseEvent 217 96 2 0 0 Control_L 0\n"
                              "TimerEvent 217 96 2 0 0 Control_L 0\n"
                              "KeyReleaseEvent 217 96 2 0 1 Control_L 0\n"
                              "MouseMoveEvent 218 96 0 0 0 Control_L 0\n"
                              "MouseMoveEvent 592 348 0 0 0 Control_L 0\n";

int TestParallelCoordinatesClearAxisSelection(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
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
