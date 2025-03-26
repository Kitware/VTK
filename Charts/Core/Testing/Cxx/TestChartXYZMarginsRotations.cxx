// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkChartXYZ.h"
#include "vtkContextMouseEvent.h"
#include "vtkContextScene.h"
#include "vtkContextView.h"
#include "vtkFloatArray.h"
#include "vtkNew.h"
#include "vtkPlotLine3D.h"
#include "vtkPlotPoints3D.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTable.h"
#include "vtkUnsignedCharArray.h"
#include "vtkVector.h"
#include <array>

int TestChartXYZMarginsRotations(int, char*[])
{
  // implicitly tested here:
  // 1. no problems when away from the origin
  // 2. the positions on the scale are right even after panning
  // 3. rotating a panned plot does not make it move relative to the box
  // 4. the plot and box nicely stay 40 pixels away from the sides (textlabels aside), even when
  // tilted. FitToScene==true
  // 6. the plot remains nicely parallel with the box, no deformations there.
  // 7. clipping planes work (they are supposed to hide half the box)

  vtkNew<vtkChartXYZ> chart;
  vtkNew<vtkContextView> view;
  view->GetRenderWindow()->SetSize(600, 500);
  view->GetScene()->AddItem(chart);

  chart->SetMargins({ 40, 40, 40, 40 });

  chart->SetFitToScene(true);

  vtkNew<vtkTable> table;
  for (auto& name : std::array<std::string, 3>{ "X", "Y", "Z" })
  {
    vtkNew<vtkFloatArray> arr;
    arr->SetName(name.c_str());
    table->AddColumn(arr);
  }

  table->SetNumberOfRows(8);
  vtkIdType pointindex = 0;
  for (int x = 0; x < 2; ++x)
  {
    for (int y = 0; y < 2; ++y)
    {
      for (int z = 0; z < 2; ++z)
      {
        table->SetValue(pointindex, 0, x + 100);
        table->SetValue(pointindex, 1, y - 75);
        table->SetValue(pointindex, 2, z + 50);
        pointindex++;
      }
    }
  }

  vtkNew<vtkPlotPoints3D> plot;
  plot->SetInputData(table);
  chart->AddPlot(plot);

  vtkNew<vtkTable> table2;
  for (auto& name : std::array<std::string, 3>{ "X", "Y", "Z" })
  {
    vtkNew<vtkFloatArray> arr;
    arr->SetName(name.c_str());
    table2->AddColumn(arr);
  }

  pointindex = 0;
  auto p = [&table2, &pointindex](float x, float y, float z) {
    table2->SetValue(pointindex, 0, x * 0.8 + 100.1);
    table2->SetValue(pointindex, 1, y * 0.8 - 74.9);
    table2->SetValue(pointindex, 2, z * 0.8 + 50.1);
    pointindex++;
  };

  table2->SetNumberOfRows(16);
  p(0, 0, 0);
  p(1, 0, 0);
  p(1, 1, 0);
  p(0, 1, 0);
  p(0, 0, 0);
  p(0, 0, 1);
  p(1, 0, 1);
  p(1, 0, 0);
  p(1, 0, 1);
  p(1, 1, 1);
  p(1, 1, 0);
  p(1, 1, 1);
  p(0, 1, 1);
  p(0, 1, 0);
  p(0, 1, 1);
  p(0, 0, 1);

  vtkNew<vtkPlotLine3D> plot2;
  plot2->SetInputData(table2);
  chart->AddPlot(plot2);

  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetRenderWindow()->Render();

  vtkContextMouseEvent mouseEvent;
  mouseEvent.SetInteractor(view->GetInteractor());
  vtkVector2f pos;
  vtkVector2f lastPos;

  // rotate
  mouseEvent.SetButton(vtkContextMouseEvent::LEFT_BUTTON);
  lastPos.Set(114, 55);
  mouseEvent.SetLastScenePos(lastPos);
  pos.Set(174, 121);
  mouseEvent.SetScenePos(pos);

  vtkVector2d sP(pos.Cast<double>().GetData());
  vtkVector2d lSP(lastPos.Cast<double>().GetData());

  vtkVector2d scenePos(mouseEvent.GetScenePos().Cast<double>().GetData());
  vtkVector2d lastScenePos(mouseEvent.GetLastScenePos().Cast<double>().GetData());
  chart->MouseMoveEvent(mouseEvent);

  // spin
  mouseEvent.SetButton(vtkContextMouseEvent::LEFT_BUTTON);
  mouseEvent.GetInteractor()->SetShiftKey(1);
  lastPos.Set(0, 0);
  mouseEvent.SetLastScenePos(lastPos);
  pos.Set(20, 10);
  mouseEvent.SetScenePos(pos);
  chart->MouseMoveEvent(mouseEvent);

  // zoom
  mouseEvent.SetButton(vtkContextMouseEvent::RIGHT_BUTTON);
  mouseEvent.GetInteractor()->SetShiftKey(0);
  lastPos.Set(0, 0);
  mouseEvent.SetLastScenePos(lastPos);
  pos.Set(0, 10);
  mouseEvent.SetScenePos(pos);
  chart->MouseMoveEvent(mouseEvent);

  // mouse wheel zoom
  chart->MouseWheelEvent(mouseEvent, -1);

  // pan
  mouseEvent.SetButton(vtkContextMouseEvent::RIGHT_BUTTON);
  mouseEvent.GetInteractor()->SetShiftKey(1);
  lastPos.Set(0, 0);
  mouseEvent.SetLastScenePos(lastPos);
  pos.Set(100, 100);
  mouseEvent.SetScenePos(pos);
  chart->MouseMoveEvent(mouseEvent);

  view->GetRenderWindow()->Render();

  view->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
