/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestChartXYZ.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkInteractiveChartXYZ.h"
#include "vtkContextMouseEvent.h"
#include "vtkContextView.h"
#include "vtkContextScene.h"
#include "vtkFloatArray.h"
#include "vtkNew.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTable.h"
#include "vtkRegressionTestImage.h"
#include "vtkUnsignedCharArray.h"
#include "vtkVector.h"

int TestInteractiveChartXYZ(int argc, char **argv)
{
  // Now the chart
  vtkNew<vtkInteractiveChartXYZ> chart;
  vtkNew<vtkContextView> view;
  view->GetRenderWindow()->SetSize(400, 300);
  view->GetScene()->AddItem(chart.GetPointer());

  chart->SetGeometry(vtkRectf(75.0, 20.0, 250, 260));

  // Create a table with some points in it...
  vtkNew<vtkTable> table;
  vtkNew<vtkFloatArray> arrX;
  arrX->SetName("X Axis");
  table->AddColumn(arrX.GetPointer());
  vtkNew<vtkFloatArray> arrC;
  arrC->SetName("Cosine");
  table->AddColumn(arrC.GetPointer());
  vtkNew<vtkFloatArray> arrS;
  arrS->SetName("Sine");
  table->AddColumn(arrS.GetPointer());
  vtkNew<vtkFloatArray> arrColor;
  arrColor->SetName("Color");
  table->AddColumn(arrColor.GetPointer());
  // Test charting with a few more points...
  int numPoints = 69;
  float inc = 7.5 / (numPoints-1);
  table->SetNumberOfRows(numPoints);
  table->SetNumberOfRows(numPoints);
  for (int i = 0; i < numPoints; ++i)
    {
    table->SetValue(i, 0, i * inc);
    table->SetValue(i, 1, cos(i * inc) + 0.0);
    table->SetValue(i, 2, sin(i * inc) + 0.0);
    table->SetValue(i, 3, i);
    }

  // Add the dimensions we are interested in visualizing.
  chart->SetInput(table.GetPointer(), "X Axis", "Sine", "Cosine", "Color");
  chart->RecalculateBounds();
  chart->RecalculateTransform();

  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();

  vtkContextMouseEvent mouseEvent;
  mouseEvent.SetInteractor(view->GetInteractor());
  vtkVector2i pos;
  vtkVector2i lastPos;

  // rotate
  mouseEvent.SetButton(vtkContextMouseEvent::LEFT_BUTTON);
  lastPos.Set(114, 55);
  mouseEvent.SetLastScreenPos(lastPos);
  pos.Set(174, 121);
  mouseEvent.SetScreenPos(pos);

  vtkVector2d sP(pos.Cast<double>().GetData());
  vtkVector2d lSP(lastPos.Cast<double>().GetData());

  vtkVector2d screenPos(mouseEvent.GetScreenPos().Cast<double>().GetData());
  vtkVector2d lastScreenPos(mouseEvent.GetLastScreenPos().Cast<double>().GetData());
  chart->MouseMoveEvent(mouseEvent);

  // spin
  mouseEvent.SetButton(vtkContextMouseEvent::LEFT_BUTTON);
  mouseEvent.GetInteractor()->SetShiftKey(1);
  lastPos.Set(0, 0);
  mouseEvent.SetLastScreenPos(lastPos);
  pos.Set(10, 10);
  mouseEvent.SetScreenPos(pos);
  chart->MouseMoveEvent(mouseEvent);

  // zoom
  mouseEvent.SetButton(vtkContextMouseEvent::RIGHT_BUTTON);
  mouseEvent.GetInteractor()->SetShiftKey(0);
  lastPos.Set(0, 0);
  mouseEvent.SetLastScreenPos(lastPos);
  pos.Set(0, 10);
  mouseEvent.SetScreenPos(pos);
  chart->MouseMoveEvent(mouseEvent);

  // mouse wheel zoom
  chart->MouseWheelEvent(mouseEvent, -1);

  // pan
  mouseEvent.SetButton(vtkContextMouseEvent::RIGHT_BUTTON);
  mouseEvent.GetInteractor()->SetShiftKey(1);
  lastPos.Set(10, 10);
  mouseEvent.SetLastScreenPos(lastPos);
  pos.Set(0, 0);
  mouseEvent.SetScreenPos(pos);
  chart->MouseMoveEvent(mouseEvent);

  int retVal = vtkRegressionTestImage(view->GetRenderWindow());
  if(retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    view->GetRenderWindow()->Render();
    view->GetInteractor()->Start();
    }
  return !retVal;
}
