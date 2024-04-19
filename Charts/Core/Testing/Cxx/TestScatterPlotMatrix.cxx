// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCallbackCommand.h"
#include "vtkChart.h"
#include "vtkContextMouseEvent.h"
#include "vtkContextScene.h"
#include "vtkContextView.h"
#include "vtkFloatArray.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkPlot.h"
#include "vtkRenderTimerLog.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkScatterPlotMatrix.h"
#include "vtkTable.h"

namespace
{

void RenderComplete(vtkObject* obj, unsigned long, void*, void*)
{
  vtkRenderWindow* renWin = vtkRenderWindow::SafeDownCast(obj);
  assert(renWin);

  vtkRenderTimerLog* timer = renWin->GetRenderTimer();
  while (timer->FrameReady())
  {
    std::cout << "-- Frame Timing:------------------------------------------\n";
    timer->PopFirstReadyFrame().Print(std::cout);
    std::cout << "\n";
  }
}

} // end anon namespace

//------------------------------------------------------------------------------
int TestScatterPlotMatrix(int argc, char* argv[])
{
  // Set up a 2D scene, add a chart to it
  vtkNew<vtkContextView> view;
  view->GetRenderWindow()->SetSize(800, 600);
  vtkNew<vtkScatterPlotMatrix> matrix;
  view->GetScene()->AddItem(matrix);

  // Create a table with some points in it...
  vtkNew<vtkTable> table;
  vtkNew<vtkFloatArray> arrX;
  arrX->SetName("x");
  table->AddColumn(arrX);
  vtkNew<vtkFloatArray> arrC;
  arrC->SetName("cos(x)");
  table->AddColumn(arrC);
  vtkNew<vtkFloatArray> arrS;
  arrS->SetName("sin(x)");
  table->AddColumn(arrS);
  vtkNew<vtkFloatArray> arrS2;
  arrS2->SetName("sin(x + 0.5)");
  table->AddColumn(arrS2);
  vtkNew<vtkFloatArray> tangent;
  tangent->SetName("tan(x)");
  table->AddColumn(tangent);
  // Test the chart scatter plot matrix
  int numPoints = 100;
  // Setup the rendertimer observer:
  for (int i = 0; i < argc; ++i)
  {
    if (std::string(argv[i]) == "-timeit")
    {
      numPoints = 10000000; // 10 million
      vtkNew<vtkCallbackCommand> renderCompleteCB;
      renderCompleteCB->SetCallback(RenderComplete);
      view->GetRenderWindow()->GetRenderTimer()->LoggingEnabledOn();
      view->GetRenderWindow()->AddObserver(vtkCommand::EndEvent, renderCompleteCB);
      break;
    }
  }
  float inc = 4.0 * vtkMath::Pi() / (numPoints - 1);
  table->SetNumberOfRows(numPoints);
  for (int i = 0; i < numPoints; ++i)
  {
    table->SetValue(i, 0, i * inc);
    table->SetValue(i, 1, cos(i * inc));
    table->SetValue(i, 2, sin(i * inc));
    table->SetValue(i, 3, sin(i * inc) + 0.5);
    table->SetValue(i, 4, tan(i * inc));
  }

  // Set the scatter plot matrix up to analyze all columns in the table.
  matrix->SetInput(table);

  matrix->SetNumberOfBins(7);

  view->Render();
  matrix->GetMainChart()->SetActionToButton(
    vtkChart::SELECT_POLYGON, vtkContextMouseEvent::RIGHT_BUTTON);

  // Test animation by releasing a right click on subchart (1,2)
  vtkContextMouseEvent mouseEvent;
  mouseEvent.SetInteractor(view->GetInteractor());
  vtkVector2f pos;

  mouseEvent.SetButton(vtkContextMouseEvent::RIGHT_BUTTON);
  pos.Set(245, 301);
  mouseEvent.SetPos(pos);
  matrix->MouseButtonReleaseEvent(mouseEvent);

  // Finally render the scene and compare the image to a reference image
  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();
  return EXIT_SUCCESS;
}
