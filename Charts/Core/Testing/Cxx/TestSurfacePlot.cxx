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
#include "vtkContextMouseEvent.h"
#include "vtkContextView.h"
#include "vtkContextScene.h"
#include "vtkFloatArray.h"
#include "vtkNew.h"
#include "vtkPlotSurface.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTable.h"
#include "vtkRegressionTestImage.h"
#include "vtkUnsignedCharArray.h"
#include "vtkVector.h"

int TestSurfacePlot(int , char * [])
{
  vtkNew<vtkChartXYZ> chart;
  vtkNew<vtkPlotSurface> plot;
  vtkNew<vtkContextView> view;
  view->GetRenderWindow()->SetSize(400, 300);
  view->GetScene()->AddItem(chart.GetPointer());

  chart->SetGeometry(vtkRectf(75.0, 20.0, 250, 260));

  // Create a surface
  vtkNew<vtkTable> table;
  vtkIdType numPoints = 70;
  float inc = 9.424778 / (numPoints - 1);
  for (vtkIdType i = 0; i < numPoints; ++i)
    {
    vtkNew<vtkFloatArray> arr;
    table->AddColumn(arr.GetPointer());
    }
  table->SetNumberOfRows(static_cast<vtkIdType>(numPoints));
  for (vtkIdType i = 0; i < numPoints; ++i)
    {
    float x = i * inc;
    for (vtkIdType j = 0; j < numPoints; ++j)
      {
      float y  = j * inc;
      table->SetValue(i, j, sin(sqrt(x*x + y*y)));
      }
    }

  // Set up the surface plot we wish to visualize and add it to the chart.
  plot->SetXRange(0, 9.424778);
  plot->SetYRange(0, 9.424778);
  plot->SetInputData(table.GetPointer());
  chart->AddPlot(plot.GetPointer());

  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetRenderWindow()->Render();

  // rotate
  vtkContextMouseEvent mouseEvent;
  mouseEvent.SetInteractor(view->GetInteractor());
  vtkVector2i pos;
  vtkVector2i lastPos;

  mouseEvent.SetButton(vtkContextMouseEvent::LEFT_BUTTON);
  lastPos.Set(100, 50);
  mouseEvent.SetLastScreenPos(lastPos);
  pos.Set(150, 100);
  mouseEvent.SetScreenPos(pos);
  vtkVector2d sP(pos.Cast<double>().GetData());
  vtkVector2d lSP(lastPos.Cast<double>().GetData());
  vtkVector2d screenPos(mouseEvent.GetScreenPos().Cast<double>().GetData());
  vtkVector2d lastScreenPos(mouseEvent.GetLastScreenPos().Cast<double>().GetData());
  chart->MouseMoveEvent(mouseEvent);

  view->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
