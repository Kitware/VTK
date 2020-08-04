/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSurfacePlot.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkChartXYZ.h"
#include "vtkContextKeyEvent.h"
#include "vtkContextMouseEvent.h"
#include "vtkContextScene.h"
#include "vtkContextView.h"
#include "vtkFloatArray.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkNew.h"
#include "vtkPlotSurface.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTable.h"
#include "vtkUnsignedCharArray.h"
#include "vtkVector.h"

static const char* TestChartXYZOuterEdgeLabellingItemLog = "# StreamVersion 1.1\n"
                                                           "TimerEvent 0 0 0 0 0 0 0\n"
                                                           "EnterEvent 100 100 0 0 0 0 0\n"
                                                           "KeyPressEvent 100 100 1 89 1 Y\n"
                                                           "KeyPressEvent 592 285 0 0 1 Left\n"
                                                           "KeyReleaseEvent 592 285 0 0 1 Left\n"
                                                           "KeyPressEvent 592 285 0 0 1 Left\n"
                                                           "KeyReleaseEvent 592 285 0 0 1 Left\n"
                                                           "KeyPressEvent 592 285 0 0 1 Left\n"
                                                           "KeyReleaseEvent 592 285 0 0 1 Left\n"
                                                           "KeyPressEvent 592 285 0 0 1 Left\n"
                                                           "KeyReleaseEvent 592 285 0 0 1 Left\n"
                                                           "KeyPressEvent 592 285 0 0 1 Left\n"
                                                           "KeyReleaseEvent 592 285 0 0 1 Left\n"
                                                           "KeyPressEvent 592 285 0 0 1 Left\n"
                                                           "KeyReleaseEvent 592 285 0 0 1 Left\n"
                                                           "KeyPressEvent 592 285 0 0 1 Left\n"
                                                           "KeyReleaseEvent 592 285 0 0 1 Left\n"
                                                           "KeyPressEvent 592 285 0 0 1 Left\n"
                                                           "KeyReleaseEvent 592 285 0 0 1 Left\n"
                                                           "KeyPressEvent 592 285 0 0 1 Left\n"
                                                           "KeyReleaseEvent 592 285 0 0 1 Left\n"
                                                           "KeyPressEvent 592 285 0 0 1 Left\n"
                                                           "KeyReleaseEvent 592 285 0 0 1 Left\n"
                                                           "KeyPressEvent 592 285 0 0 1 Left\n"
                                                           "KeyReleaseEvent 592 285 0 0 1 Left\n"
                                                           "KeyPressEvent 592 285 0 0 1 Left\n"
                                                           "KeyReleaseEvent 592 285 0 0 1 Left\n"
                                                           "KeyPressEvent 592 285 0 0 1 Left\n"
                                                           "KeyReleaseEvent 592 285 0 0 1 Left\n"
                                                           "KeyPressEvent 592 285 0 0 1 Left\n"
                                                           "KeyReleaseEvent 592 285 0 0 1 Left\n"
                                                           "KeyPressEvent 592 285 0 0 1 Left\n"
                                                           "KeyReleaseEvent 592 285 0 0 1 Left\n"
                                                           "KeyPressEvent 592 285 0 0 1 Left\n"
                                                           "KeyReleaseEvent 592 285 0 0 1 Left\n"
                                                           "KeyPressEvent 592 285 0 0 1 Left\n"
                                                           "KeyReleaseEvent 592 285 0 0 1 Left\n"
                                                           "KeyPressEvent 592 285 0 0 1 Left\n"
                                                           "KeyReleaseEvent 592 285 0 0 1 Left\n"
                                                           "KeyPressEvent 592 285 0 0 1 Left\n"
                                                           "KeyReleaseEvent 592 285 0 0 1 Left\n"
                                                           "KeyPressEvent 592 285 0 0 1 Left\n"
                                                           "KeyReleaseEvent 592 285 0 0 1 Left\n"
                                                           "KeyPressEvent 592 285 0 0 1 Down\n"
                                                           "KeyReleaseEvent 592 285 0 0 1 Down\n"
                                                           "KeyPressEvent 592 285 0 0 1 Down\n"
                                                           "KeyReleaseEvent 592 285 0 0 1 Down\n"
                                                           "KeyPressEvent 592 285 0 0 1 Down\n"
                                                           "KeyReleaseEvent 592 285 0 0 1 Down\n"
                                                           "KeyPressEvent 592 285 0 0 1 Down\n"
                                                           "KeyReleaseEvent 592 285 0 0 1 Down\n"
                                                           "KeyPressEvent 592 285 0 0 1 Down\n"
                                                           "KeyReleaseEvent 592 285 0 0 1 Down\n"
                                                           "KeyPressEvent 592 285 0 0 1 Down\n"
                                                           "KeyReleaseEvent 592 285 0 0 1 Down\n"
                                                           "KeyPressEvent 592 285 0 0 1 Down\n"
                                                           "KeyReleaseEvent 592 285 0 0 1 Down\n"
                                                           "KeyPressEvent 592 285 0 0 1 Down\n"
                                                           "KeyReleaseEvent 592 285 0 0 1 Down\n"
                                                           "KeyPressEvent 592 285 0 0 1 Down\n"
                                                           "KeyReleaseEvent 592 285 0 0 1 Down\n"
                                                           "KeyPressEvent 592 285 0 0 1 Down\n"
                                                           "KeyReleaseEvent 592 285 0 0 1 Down\n"
                                                           "KeyPressEvent 592 285 0 0 1 Down\n"
                                                           "KeyReleaseEvent 592 285 0 0 1 Down\n"
                                                           "KeyPressEvent 592 285 0 0 1 Down\n"
                                                           "KeyReleaseEvent 592 285 0 0 1 Down\n"
                                                           "KeyPressEvent 592 285 0 0 1 Down\n"
                                                           "KeyReleaseEvent 592 285 0 0 1 Down\n"
                                                           "KeyPressEvent 592 285 0 0 1 Down\n"
                                                           "KeyReleaseEvent 592 285 0 0 1 Down\n"
                                                           "KeyPressEvent 592 285 0 0 1 Down\n"
                                                           "KeyReleaseEvent 592 285 0 0 1 Down\n"
                                                           "KeyPressEvent 592 285 0 0 1 Down\n"
                                                           "KeyReleaseEvent 592 285 0 0 1 Down\n"
                                                           "KeyPressEvent 592 285 0 0 1 Down\n"
                                                           "KeyReleaseEvent 592 285 0 0 1 Down\n"
                                                           "KeyPressEvent 592 285 0 0 1 Down\n"
                                                           "KeyReleaseEvent 592 285 0 0 1 Down\n"
                                                           "KeyPressEvent 592 285 0 0 1 Down\n"
                                                           "KeyReleaseEvent 592 285 0 0 1 Down\n"
                                                           "KeyPressEvent 592 285 0 0 1 Down\n"
                                                           "KeyReleaseEvent 592 285 0 0 1 Down\n";

int TestChartXYZOuterEdgeLabelling(int, char*[])
{
  vtkNew<vtkChartXYZ> chart;
  vtkNew<vtkPlotSurface> plot;
  vtkNew<vtkContextView> view;
  view->GetRenderWindow()->SetSize(800, 600);
  view->GetScene()->AddItem(chart);

  chart->SetGeometry(vtkRectf(75.0, 20.0, 400, 420));
  chart->SetMargins(vtkVector4i(80, 160, 80, 160));
  chart->SetEnsureOuterEdgeAxisLabelling(true);

  // Create a surface
  vtkNew<vtkTable> table;
  vtkIdType numPoints = 70;
  float inc = 9.424778 / (numPoints - 1);
  for (vtkIdType i = 0; i < numPoints; ++i)
  {
    vtkNew<vtkFloatArray> arr;
    table->AddColumn(arr);
  }
  table->SetNumberOfRows(static_cast<vtkIdType>(numPoints));
  for (vtkIdType i = 0; i < numPoints; ++i)
  {
    float x = i * inc;
    for (vtkIdType j = 0; j < numPoints; ++j)
    {
      float y = j * inc;
      table->SetValue(i, j, sin(sqrt(x * x + y * y)));
    }
  }

  // Set up the surface plot we wish to visualize and add it to the chart.
  plot->SetXRange(0, 9.424778);
  plot->SetYRange(0, 9.424778);
  plot->SetInputData(table);
  chart->AddPlot(plot);

  chart->GetAxesTextProperty()->SetFontFamilyToTimes();
  chart->GetAxesTextProperty()->SetFontSize(24);
  chart->SetXAxisLabel("X axis");
  chart->SetYAxisLabel("Y axis");
  chart->SetZAxisLabel("Z axis");

  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->Render();

  // Use recorded information to set the initial orientation of the chart and then rotate
  // it left and down to set a 3d view.
  vtkSmartPointer<vtkInteractorEventRecorder> recorder =
    vtkSmartPointer<vtkInteractorEventRecorder>::New();

  recorder->SetInteractor(view->GetInteractor());

  recorder->ReadFromInputStringOn();
  recorder->SetInputString(TestChartXYZOuterEdgeLabellingItemLog);

  recorder->Play();
  recorder->Off();

  view->GetInteractor()->Start();
  view->GetInteractor();

  return EXIT_SUCCESS;
}
