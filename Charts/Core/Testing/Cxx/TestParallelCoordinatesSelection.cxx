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

#include <iostream>

static constexpr int expectedValues[] = { 23, 24, 25, 26, 49, 50, 51, 52, 53, 54, 55, 56, 57, 110,
  111, 112, 113, 114, 115, 116, 117, 118, 126, 140, 141, 142, 143, 190, 191, 192, 193 };

static const char* eventLog = "# StreamVersion 1.2\n"
                              "MouseMoveEvent 598 101 0 0 0 0 0\n"
                              "MouseMoveEvent 593 125 0 0 0 0 0\n"
                              "MouseMoveEvent 598 120 0 0 0 0 0\n"
                              "MouseMoveEvent 219 302 0 0 0 0 0\n"
                              "LeftButtonPressEvent 219 302 0 0 0 0 0\n"
                              "MouseMoveEvent 219 301 0 0 0 0 0\n"
                              "MouseMoveEvent 216 295 0 0 0 0 0\n"
                              "MouseMoveEvent 217 292 0 0 0 0 0\n"
                              "MouseMoveEvent 217 285 0 0 0 0 0\n"
                              "MouseMoveEvent 217 282 0 0 0 0 0\n"
                              "MouseMoveEvent 218 274 0 0 0 0 0\n"
                              "MouseMoveEvent 218 272 0 0 0 0 0\n"
                              "MouseMoveEvent 219 263 0 0 0 0 0\n"
                              "MouseMoveEvent 219 257 0 0 0 0 0\n"
                              "MouseMoveEvent 219 248 0 0 0 0 0\n"
                              "MouseMoveEvent 219 244 0 0 0 0 0\n"
                              "MouseMoveEvent 221 238 0 0 0 0 0\n"
                              "MouseMoveEvent 221 236 0 0 0 0 0\n"
                              "MouseMoveEvent 222 232 0 0 0 0 0\n"
                              "MouseMoveEvent 222 231 0 0 0 0 0\n"
                              "MouseMoveEvent 222 231 0 0 0 0 0\n"
                              "MouseMoveEvent 222 229 0 0 0 0 0\n"
                              "MouseMoveEvent 223 225 0 0 0 0 0\n"
                              "MouseMoveEvent 223 223 0 0 0 0 0\n"
                              "LeftButtonReleaseEvent 223 223 0 0 0 0 0\n"
                              "MouseMoveEvent 223 222 0 0 0 0 0\n"
                              "MouseMoveEvent 216 200 0 0 0 0 0\n"
                              "KeyPressEvent 216 200 0 0 1 Control_L 0\n"
                              "MouseMoveEvent 216 199 2 0 0 Control_L 0\n"
                              "MouseMoveEvent 216 189 2 0 0 Control_L 0\n"
                              "LeftButtonPressEvent 216 189 2 0 0 Control_L 0\n"
                              "MouseMoveEvent 216 187 2 0 0 Control_L 0\n"
                              "MouseMoveEvent 216 176 2 0 0 Control_L 0\n"
                              "MouseMoveEvent 215 170 2 0 0 Control_L 0\n"
                              "MouseMoveEvent 213 158 2 0 0 Control_L 0\n"
                              "MouseMoveEvent 213 156 2 0 0 Control_L 0\n"
                              "MouseMoveEvent 213 148 2 0 0 Control_L 0\n"
                              "MouseMoveEvent 213 144 2 0 0 Control_L 0\n"
                              "MouseMoveEvent 212 138 2 0 0 Control_L 0\n"
                              "MouseMoveEvent 211 137 2 0 0 Control_L 0\n"
                              "MouseMoveEvent 211 131 2 0 0 Control_L 0\n"
                              "MouseMoveEvent 211 129 2 0 0 Control_L 0\n"
                              "MouseMoveEvent 212 124 2 0 0 Control_L 0\n"
                              "MouseMoveEvent 212 123 2 0 0 Control_L 0\n"
                              "LeftButtonReleaseEvent 212 123 2 0 0 Control_L 0\n"
                              "MouseMoveEvent 212 123 2 0 0 Control_L 0\n"
                              "MouseMoveEvent 218 203 2 0 0 Control_L 0\n"
                              "KeyReleaseEvent 218 203 2 0 1 Control_L 0\n"
                              "KeyPressEvent 218 203 0 0 1 Shift_L 0\n"
                              "LeftButtonPressEvent 218 203 1 0 0 Shift_L 0\n"
                              "MouseMoveEvent 218 203 1 0 0 Shift_L 0\n"
                              "MouseMoveEvent 220 202 1 0 0 Shift_L 0\n"
                              "MouseMoveEvent 220 201 1 0 0 Shift_L 0\n"
                              "MouseMoveEvent 221 198 1 0 0 Shift_L 0\n"
                              "MouseMoveEvent 221 196 1 0 0 Shift_L 0\n"
                              "MouseMoveEvent 221 194 1 0 0 Shift_L 0\n"
                              "MouseMoveEvent 221 194 1 0 0 Shift_L 0\n"
                              "MouseMoveEvent 221 191 1 0 0 Shift_L 0\n"
                              "MouseMoveEvent 221 190 1 0 0 Shift_L 0\n"
                              "MouseMoveEvent 222 186 1 0 0 Shift_L 0\n"
                              "MouseMoveEvent 221 185 1 0 0 Shift_L 0\n"
                              "MouseMoveEvent 221 184 1 0 0 Shift_L 0\n"
                              "MouseMoveEvent 221 183 1 0 0 Shift_L 0\n"
                              "MouseMoveEvent 221 176 1 0 0 Shift_L 0\n"
                              "MouseMoveEvent 221 174 1 0 0 Shift_L 0\n"
                              "MouseMoveEvent 221 172 1 0 0 Shift_L 0\n"
                              "LeftButtonReleaseEvent 221 172 1 0 0 Shift_L 0\n"
                              "MouseMoveEvent 221 172 1 0 0 Shift_L 0\n"
                              "MouseMoveEvent 224 172 1 0 0 Shift_L 0\n"
                              "KeyReleaseEvent 224 172 1 0 1 Shift_L 0\n"
                              "MouseMoveEvent 224 173 0 0 0 Shift_L 0\n"
                              "MouseMoveEvent 218 218 0 0 0 Shift_L 0\n"
                              "KeyPressEvent 218 218 0 0 1 Shift_L 0\n"
                              "MouseMoveEvent 218 218 1 0 0 Shift_L 0\n"
                              "MouseMoveEvent 216 252 1 0 0 Shift_L 0\n"
                              "KeyPressEvent 216 252 1 0 1 Control_L 0\n"
                              "MouseMoveEvent 216 258 3 0 0 Control_L 0\n"
                              "MouseMoveEvent 223 322 3 0 0 Control_L 0\n"
                              "LeftButtonPressEvent 223 322 3 0 0 Control_L 0\n"
                              "MouseMoveEvent 223 321 3 0 0 Control_L 0\n"
                              "MouseMoveEvent 222 317 3 0 0 Control_L 0\n"
                              "MouseMoveEvent 220 312 3 0 0 Control_L 0\n"
                              "MouseMoveEvent 219 303 3 0 0 Control_L 0\n"
                              "MouseMoveEvent 219 300 3 0 0 Control_L 0\n"
                              "MouseMoveEvent 219 292 3 0 0 Control_L 0\n"
                              "MouseMoveEvent 219 289 3 0 0 Control_L 0\n"
                              "MouseMoveEvent 219 281 3 0 0 Control_L 0\n"
                              "MouseMoveEvent 219 276 3 0 0 Control_L 0\n"
                              "MouseMoveEvent 219 270 3 0 0 Control_L 0\n"
                              "MouseMoveEvent 219 265 3 0 0 Control_L 0\n"
                              "MouseMoveEvent 219 256 3 0 0 Control_L 0\n"
                              "MouseMoveEvent 219 253 3 0 0 Control_L 0\n"
                              "MouseMoveEvent 219 246 3 0 0 Control_L 0\n"
                              "MouseMoveEvent 219 244 3 0 0 Control_L 0\n"
                              "MouseMoveEvent 219 237 3 0 0 Control_L 0\n"
                              "MouseMoveEvent 220 233 3 0 0 Control_L 0\n"
                              "MouseMoveEvent 220 229 3 0 0 Control_L 0\n"
                              "MouseMoveEvent 220 226 3 0 0 Control_L 0\n"
                              "MouseMoveEvent 220 222 3 0 0 Control_L 0\n"
                              "MouseMoveEvent 220 221 3 0 0 Control_L 0\n"
                              "MouseMoveEvent 220 220 3 0 0 Control_L 0\n"
                              "LeftButtonReleaseEvent 220 220 3 0 0 Control_L 0\n"
                              "MouseMoveEvent 220 220 3 0 0 Control_L 0\n"
                              "MouseMoveEvent 225 217 3 0 0 Control_L 0\n"
                              "KeyReleaseEvent 225 217 3 0 1 Control_L 0\n"
                              "KeyReleaseEvent 225 217 1 0 1 Shift_L 0\n"
                              "MouseMoveEvent 226 217 0 0 0 Shift_L 0\n"
                              "MouseMoveEvent 428 398 0 0 0 Shift_L 0\n";

int TestParallelCoordinatesSelection(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  // Set up a 2D scene, add an parallel coordinate chart to it
  vtkNew<vtkContextView> view;
  view->GetRenderWindow()->SetWindowName("TestParallelCoordinateSelection");
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

  auto ids = chart->GetPlot(0)->GetSelection();
  if (!ids)
  {
    std::cerr << "Selection shouldn't be null" << std::endl;
    return EXIT_FAILURE;
  }

  if (ids->GetNumberOfValues() != 31)
  {
    std::cerr << "Wrong number of id selection. Expected to have 14 ids but got "
              << ids->GetNumberOfValues() << std::endl;
    return EXIT_FAILURE;
  }

  for (int i = 0; i < ids->GetNumberOfValues(); i++)
  {
    if (ids->GetValue(i) != expectedValues[i])
    {
      std::cerr << "Wrong id values in the current selection. Expected to have "
                << expectedValues[i] << " id but got " << ids->GetValue(i) << std::endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
