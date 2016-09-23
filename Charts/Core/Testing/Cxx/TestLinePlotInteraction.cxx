/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestLinePlot.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkSmartPointer.h"
#include "vtkChartXY.h"
#include "vtkAxis.h"
#include "vtkPlot.h"
#include "vtkTable.h"
#include "vtkFloatArray.h"
#include "vtkContextView.h"
#include "vtkContextScene.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkNew.h"

static char TestLinePlotInteractionLog[] =
"# StreamVersion 1\n"
"RenderEvent 0 0 0 0 0 0 0\n"
"EnterEvent 198 5 0 0 0 0 0\n"
"MouseMoveEvent 198 5 0 0 0 0 0\n"
"MouseMoveEvent 190 49 0 0 0 0 0\n"
"MouseMoveEvent 190 54 0 0 0 0 0\n"
"MouseMoveEvent 190 59 0 0 0 0 0\n"
"MouseMoveEvent 190 64 0 0 0 0 0\n"
"MouseMoveEvent 190 69 0 0 0 0 0\n"
"MouseMoveEvent 192 73 0 0 0 0 0\n"
"MouseMoveEvent 192 77 0 0 0 0 0\n"
"MouseMoveEvent 193 83 0 0 0 0 0\n"
"LeftButtonPressEvent 131 124 0 0 0 0 0\n"
"MouseMoveEvent 132 124 0 0 0 0 0\n"
"MouseMoveEvent 134 124 0 0 0 0 0\n"
"MouseMoveEvent 135 124 0 0 0 0 0\n"
"MouseMoveEvent 136 124 0 0 0 0 0\n"
"MouseMoveEvent 137 124 0 0 0 0 0\n"
"MouseMoveEvent 138 124 0 0 0 0 0\n"
"MouseMoveEvent 139 124 0 0 0 0 0\n"
"MouseMoveEvent 140 124 0 0 0 0 0\n"
"MouseMoveEvent 141 124 0 0 0 0 0\n"
"MouseMoveEvent 142 124 0 0 0 0 0\n"
"LeftButtonReleaseEvent 166 143 0 0 0 0 0\n"
"MouseMoveEvent 165 143 0 0 0 0 0\n"
"MouseMoveEvent 164 144 0 0 0 0 0\n"
"MouseMoveEvent 163 144 0 0 0 0 0\n"
"LeftButtonPressEvent 131 246 0 0 0 0 0\n"
"MouseMoveEvent 132 245 0 0 0 0 0\n"
"MouseMoveEvent 133 244 0 0 0 0 0\n"
"MouseMoveEvent 136 241 0 0 0 0 0\n"
"MouseMoveEvent 137 240 0 0 0 0 0\n"
"MouseMoveEvent 138 239 0 0 0 0 0\n"
"MouseMoveEvent 139 238 0 0 0 0 0\n"
"MouseMoveEvent 140 237 0 0 0 0 0\n"
"MouseMoveEvent 140 236 0 0 0 0 0\n"
"MouseMoveEvent 141 235 0 0 0 0 0\n"
"MouseMoveEvent 142 234 0 0 0 0 0\n"
"MouseMoveEvent 143 233 0 0 0 0 0\n"
"LeftButtonReleaseEvent 104 251 0 0 0 0 0\n"
"MouseMoveEvent 104 252 0 0 0 0 0\n"
"MouseMoveEvent 104 253 0 0 0 0 0\n"
"MouseMoveEvent 104 254 0 0 0 0 0\n"
"MouseMoveEvent 104 255 0 0 0 0 0\n"
"RightButtonPressEvent 104 258 0 0 0 0 0\n"
"MouseMoveEvent 105 258 0 0 0 0 0\n"
"MouseMoveEvent 105 257 0 0 0 0 0\n"
"MouseMoveEvent 106 256 0 0 0 0 0\n"
"MouseMoveEvent 106 255 0 0 0 0 0\n"
"MouseMoveEvent 106 254 0 0 0 0 0\n"
"MouseMoveEvent 107 253 0 0 0 0 0\n"
"MouseMoveEvent 108 252 0 0 0 0 0\n"
"MouseMoveEvent 216 136 0 0 0 0 0\n"
"MouseMoveEvent 278 64 0 0 0 0 0\n"
"RightButtonReleaseEvent 278 64 0 0 0 0 0\n"
"MouseWheelBackwardEvent 271 185 0 0 0 0 0\n"
"MouseWheelBackwardEvent 271 185 0 0 0 0 0\n"
"MouseWheelBackwardEvent 271 185 0 0 0 1 0\n"
"RightButtonPressEvent 177 248 0 0 0 0 0\n"
"MouseMoveEvent 178 247 0 0 0 0 0\n"
"MouseMoveEvent 178 246 0 0 0 0 0\n"
"MouseMoveEvent 179 245 0 0 0 0 0\n"
"MouseMoveEvent 181 243 0 0 0 0 0\n"
"MouseMoveEvent 182 241 0 0 0 0 0\n"
"MouseMoveEvent 184 239 0 0 0 0 0\n"
"MouseMoveEvent 186 237 0 0 0 0 0\n"
"MouseMoveEvent 295 79 0 0 0 0 0\n"
"RightButtonReleaseEvent 295 79 0 0 0 0 0\n"
"MouseMoveEvent 295 80 0 0 0 0 0\n"
"MiddleButtonPressEvent 304 74 0 0 0 0 0\n"
"MouseMoveEvent 303 74 0 0 0 0 0\n"
"MouseMoveEvent 300 76 0 0 0 0 0\n"
"MouseMoveEvent 296 77 0 0 0 0 0\n"
"MouseMoveEvent 294 78 0 0 0 0 0\n"
"MouseMoveEvent 214 177 0 0 0 0 0\n"
"MiddleButtonReleaseEvent 214 177 0 0 0 0 0\n"
"MouseWheelBackwardEvent 214 177 0 0 0 0 0\n"
"MouseWheelBackwardEvent 214 177 0 0 0 1 0\n"
"MouseWheelBackwardEvent 214 177 0 0 0 0 0\n"
"MouseWheelBackwardEvent 214 177 0 0 0 1 0\n"
"MouseMoveEvent 213 177 0 0 0 0 0\n"
"LeftButtonPressEvent 133 138 0 0 0 0 0\n"
"MouseMoveEvent 133 137 0 0 0 0 0\n"
"MouseMoveEvent 178 116 0 0 0 0 0\n"
"LeftButtonReleaseEvent 178 116 0 0 0 0 0\n"
"MouseMoveEvent 177 117 0 0 0 0 0\n"
"MouseMoveEvent 377 293 0 0 0 0 0\n"
"LeaveEvent 379 300 0 0 0 0 0\n"
"ExitEvent 379 300 0 0 0 0 0\n"
"ExitEvent 379 300 0 0 0 0 0\n"
"EnterEvent 399 285 0 0 0 0 0\n"
"MouseMoveEvent 399 285 0 0 0 0 0\n"
"MouseMoveEvent 398 286 0 0 0 0 0\n"
"MouseMoveEvent 395 299 0 0 0 0 0\n"
"LeaveEvent 395 300 0 0 0 0 0\n"
"ExitEvent 395 300 0 0 0 0 0";

//----------------------------------------------------------------------------
int TestLinePlotInteraction(int, char * [])
{
  // Set up a 2D scene, add an XY chart to it
  vtkNew<vtkContextView> view;
  view->GetRenderer()->SetBackground(1.0, 1.0, 1.0);
  view->GetRenderWindow()->SetSize(400, 300);
  vtkNew<vtkChartXY> chart;
  chart->GetAxis(vtkAxis::LEFT)->SetTitle("");
  chart->GetAxis(vtkAxis::BOTTOM)->SetTitle("");
  view->GetScene()->AddItem(chart.GetPointer());

  // Create a table with some points in it...
  vtkSmartPointer<vtkTable> table = vtkSmartPointer<vtkTable>::New();
  vtkSmartPointer<vtkFloatArray> arrX = vtkSmartPointer<vtkFloatArray>::New();
  arrX->SetName("X Axis");
  table->AddColumn(arrX);
  vtkSmartPointer<vtkFloatArray> arrC = vtkSmartPointer<vtkFloatArray>::New();
  arrC->SetName("Cosine");
  table->AddColumn(arrC);
  vtkSmartPointer<vtkFloatArray> arrS = vtkSmartPointer<vtkFloatArray>::New();
  arrS->SetName("Sine");
  table->AddColumn(arrS);
  vtkSmartPointer<vtkFloatArray> arrS2 = vtkSmartPointer<vtkFloatArray>::New();
  arrS2->SetName("Sine2");
  table->AddColumn(arrS2);
  // Test charting with a few more points...
  int numPoints = 69;
  float inc = 7.5 / (numPoints-1);
  table->SetNumberOfRows(numPoints);
  for (int i = 0; i < numPoints; ++i)
  {
    table->SetValue(i, 0, i * inc);
    table->SetValue(i, 1, cos(i * inc) + 0.0);
    table->SetValue(i, 2, sin(i * inc) + 0.0);
    table->SetValue(i, 3, sin(i * inc) + 0.5);
  }

  // Add multiple line plots, setting the colors etc
  vtkPlot *line = chart->AddPlot(vtkChart::LINE);
  line->SetInputData(table, 0, 1);
  line->SetColor(0, 255, 0, 255);
  line->SetWidth(1.0);
  line = chart->AddPlot(vtkChart::LINE);
  line->SetInputData(table, 0, 2);
  line->SetColor(255, 0, 0, 255);
  line->SetWidth(5.0);
  line = chart->AddPlot(vtkChart::LINE);
  line->SetInputData(table, 0, 3);
  line->SetColor(0, 0, 255, 255);
  line->SetWidth(4.0);

  //Finally render the scene and compare the image to a reference image
  view->GetRenderWindow()->SetMultiSamples(0);

  // recorder to play back previously events
  vtkSmartPointer<vtkInteractorEventRecorder> recorder =
    vtkSmartPointer<vtkInteractorEventRecorder>::New();
  recorder->SetInteractor(view->GetInteractor());
//  recorder->SetFileName("record.log");
//  recorder->SetKeyPressActivationValue('b');

  recorder->ReadFromInputStringOn();
  recorder->SetInputString(TestLinePlotInteractionLog);

  view->GetInteractor()->Initialize();
  view->Render();
  recorder->Play();

  // Remove the observers so we can go interactive. Without this the "-I"
  // testing option fails.
  recorder->Off();

  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
