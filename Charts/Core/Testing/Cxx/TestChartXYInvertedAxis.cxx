// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAxis.h"
#include "vtkChartXY.h"
#include "vtkContextScene.h"
#include "vtkContextView.h"
#include "vtkFloatArray.h"
#include "vtkNew.h"
#include "vtkPlot.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTable.h"
#include "vtkTesting.h"

static const char* TestChartXYInvertedAxisLog = "# StreamVersion 1\n"
                                                "MouseWheelForwardEvent 246 108 0 0 0 0 0\n"
                                                "MouseWheelForwardEvent 246 108 0 0 0 1 0\n"
                                                "MouseWheelForwardEvent 246 108 0 0 0 0 0\n"
                                                "MouseWheelForwardEvent 246 108 0 0 0 1 0\n"
                                                "MouseWheelForwardEvent 246 108 0 0 0 0 0\n"
                                                "MouseWheelForwardEvent 246 108 0 0 0 1 0\n"
                                                "MouseWheelForwardEvent 246 108 0 0 0 0 0\n"
                                                "MouseWheelForwardEvent 246 108 0 0 0 1 0\n"
                                                "MouseWheelForwardEvent 246 108 0 0 0 0 0\n"
                                                "MouseWheelForwardEvent 246 108 0 0 0 1 0\n"
                                                "MouseWheelForwardEvent 246 108 0 0 0 0 0\n"
                                                "MouseWheelForwardEvent 246 108 0 0 0 1 0\n"
                                                "MouseWheelForwardEvent 246 108 0 0 0 0 0\n"
                                                "MouseWheelBackwardEvent 246 108 0 0 0 0 0\n"
                                                "MouseWheelBackwardEvent 246 108 0 0 0 1 0\n"
                                                "MouseWheelBackwardEvent 246 108 0 0 0 0 0\n"
                                                "MouseWheelBackwardEvent 246 108 0 0 0 1 0\n"
                                                "MouseWheelBackwardEvent 246 108 0 0 0 0 0\n"
                                                "MouseWheelBackwardEvent 246 108 0 0 0 1 0\n"
                                                "MouseWheelBackwardEvent 246 108 0 0 0 0 0\n"
                                                "MouseWheelBackwardEvent 246 108 0 0 0 1 0\n"
                                                "MouseWheelBackwardEvent 246 108 0 0 0 0 0\n"
                                                "MouseWheelBackwardEvent 246 108 0 0 0 1 0\n"
                                                "MouseWheelBackwardEvent 246 108 0 0 0 0 0\n"
                                                "MouseWheelBackwardEvent 246 108 0 0 0 1 0\n"
                                                "MouseWheelBackwardEvent 246 108 0 0 0 0 0\n"

                                                "MouseMoveEvent 279 135 0 0 0 0 0\n"
                                                "LeftButtonPressEvent 279 135 0 0 0 0 0\n"
                                                "MouseMoveEvent 184 163 0 0 0 0 0\n"
                                                "LeftButtonReleaseEvent 184 163 0 0 0 0 0\n"

                                                "MouseWheelBackwardEvent 187 171 0 0 0 0 0\n"
                                                "MouseWheelBackwardEvent 187 171 0 0 0 1 0\n"
                                                "MouseWheelBackwardEvent 187 171 0 0 0 0 0\n"

                                                "MouseMoveEvent 286 178 0 0 0 0 0\n"
                                                "LeftButtonPressEvent 286 178 0 0 0 0 0\n"
                                                "MouseMoveEvent 194 177 0 0 0 0 0\n"
                                                "LeftButtonReleaseEvent 194 177 0 0 0 0 0\n"

                                                "MouseWheelBackwardEvent 197 184 0 0 0 0 0\n"
                                                "MouseWheelBackwardEvent 197 184 0 0 0 1 0\n"
                                                "MouseWheelForwardEvent 197 184 0 0 0 0 0\n"
                                                "MouseWheelForwardEvent 197 184 0 0 0 1 0\n"
                                                "MouseWheelForwardEvent 197 184 0 0 0 0 0\n"
                                                "MouseWheelForwardEvent 197 184 0 0 0 1 0\n"
                                                "MouseWheelForwardEvent 197 184 0 0 0 0 0\n"
                                                "MouseWheelForwardEvent 197 184 0 0 0 1 0\n"

                                                "MouseMoveEvent 255 221 0 0 0 0 0\n"
                                                "LeftButtonPressEvent 255 221 0 0 0 0 0\n"
                                                "MouseMoveEvent 297 187 0 0 0 0 0\n"
                                                "LeftButtonReleaseEvent 297 187 0 0 0 0 0\n"

                                                "MouseMoveEvent 162 266 0 0 0 0 0\n"
                                                "RightButtonPressEvent 162 266 0 0 0 0 0\n"
                                                "MouseMoveEvent 269 80 0 0 0 0 0\n"
                                                "RightButtonReleaseEvent 269 80 0 0 0 0 0\n"
                                                "MouseMoveEvent 295 101 0 0 0 0 0\n"

                                                "LeftButtonPressEvent 295 101 0 0 0 0 0\n"
                                                "MouseMoveEvent 201 139 0 0 0 0 0\n"
                                                "LeftButtonReleaseEvent 201 139 0 0 0 0 0\n"

                                                "MouseMoveEvent 121 195 0 0 0 0 0\n";

//------------------------------------------------------------------------------
int TestChartXYInvertedAxis(int argc, char* argv[])
{
  // Set up a 2D scene, add an XY chart to it
  vtkNew<vtkContextView> view;
  view->GetRenderWindow()->SetSize(400, 300);
  vtkNew<vtkChartXY> chart;
  view->GetScene()->AddItem(chart);

  // Create a table with some points in it...
  vtkNew<vtkTable> table;
  vtkNew<vtkFloatArray> arrX;
  arrX->SetName("X Axis");
  table->AddColumn(arrX);
  vtkNew<vtkFloatArray> arrC;
  arrC->SetName("Cosine");
  table->AddColumn(arrC);
  vtkNew<vtkFloatArray> arrS;
  arrS->SetName("Sine");
  table->AddColumn(arrS);
  vtkNew<vtkFloatArray> arrS2;
  arrS2->SetName("Sine2");
  table->AddColumn(arrS2);
  vtkNew<vtkFloatArray> arr1;
  arr1->SetName("One");
  table->AddColumn(arr1);
  // Test charting with a few more points...
  int numPoints = 69;
  float inc = 7.5 / (numPoints - 1);
  table->SetNumberOfRows(numPoints);
  for (int i = 0; i < numPoints; ++i)
  {
    table->SetValue(i, 0, i * inc);
    table->SetValue(i, 1, cos(i * inc) + 0.0);
    table->SetValue(i, 2, sin(i * inc) + 0.0);
    table->SetValue(i, 3, sin(i * inc) + 0.5);
    table->SetValue(i, 4, 1.0);
  }

  // Add multiple line plots, setting the colors etc
  vtkPlot* line = chart->AddPlot(vtkChart::LINE);
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

  // Invert the Y axis
  chart->GetAxis(vtkAxis::LEFT)->SetBehavior(vtkAxis::FIXED);
  chart->GetAxis(vtkAxis::LEFT)->SetUnscaledRange(2.0, -2.0);
  chart->GetAxis(vtkAxis::BOTTOM)->SetBehavior(vtkAxis::FIXED);
  chart->GetAxis(vtkAxis::BOTTOM)->SetUnscaledRange(10.0, 0.0);

  // Render the scene and compare the image to a reference image
  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();

  return vtkTesting::InteractorEventLoop(
    argc, argv, view->GetInteractor(), TestChartXYInvertedAxisLog);
}
