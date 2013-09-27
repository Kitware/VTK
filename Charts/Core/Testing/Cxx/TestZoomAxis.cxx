/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestZoomAxis.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkRenderWindow.h"
#include "vtkSmartPointer.h"
#include "vtkChartXY.h"
#include "vtkPlot.h"
#include "vtkTable.h"
#include "vtkFloatArray.h"
#include "vtkContextView.h"
#include "vtkContextScene.h"
#include "vtkContextMouseEvent.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkAnnotationLink.h"
#include "vtkNew.h"

//----------------------------------------------------------------------------
int TestZoomAxis(int, char * [])
{
  // Set up a 2D scene, add an XY chart to it
  vtkNew<vtkContextView> view;
  view->GetRenderWindow()->SetSize(400, 300);
  vtkNew<vtkChartXY> chart;
  view->GetScene()->AddItem(chart.GetPointer());
  vtkNew<vtkAnnotationLink> link;
  chart->SetAnnotationLink(link.GetPointer());
  chart->SetActionToButton(vtkChart::ZOOM_AXIS,
                           vtkContextMouseEvent::LEFT_BUTTON);
  chart->SetSelectionMethod(vtkChart::SELECTION_PLOTS);

  // Create a table with some points in it...
  vtkNew<vtkTable> table;
  vtkNew<vtkFloatArray> arrX;
  arrX->SetName("X Axis");
  table->AddColumn(arrX.GetPointer());
  vtkNew<vtkFloatArray> arrS;
  arrS->SetName("Sine");
  table->AddColumn(arrS.GetPointer());
  // Test charting with a few more points...
  int numPoints = 100;
  float inc = 9.5f / (numPoints-1);
  table->SetNumberOfRows(numPoints);
  for (int i = 0; i < numPoints; ++i)
    {
    table->SetValue(i, 0, i * inc);
    table->SetValue(i, 1, sin(i * inc));
    }

  // Add multiple line plots, setting the colors etc
  vtkPlot *plot = chart->AddPlot(vtkChart::POINTS);
  plot->SetInputData(table.GetPointer(), 0, 1);
  plot->SetColor(0, 255, 0, 255);
  plot->SetWidth(1.0);

  view->Update();
  view->Render();

  // Inject some mouse events to perform zooming
  vtkContextMouseEvent event;
  event.SetInteractor(view->GetInteractor());
  event.SetButton(vtkContextMouseEvent::LEFT_BUTTON);
  event.SetScreenPos(vtkVector2i(350, 250));
  chart->MouseButtonPressEvent(event);
  event.SetLastScreenPos(event.GetScreenPos());
  event.SetScreenPos(vtkVector2i(10, 10));
  chart->MouseMoveEvent(event);
  //event.SetLastScreenPos(event.GetScreenPos());
  //event.SetScreenPos(vtkVector2i(180, 150));
  //chart->MouseMoveEvent(event);
  chart->MouseButtonReleaseEvent(event);

  //Finally render the scene and compare the image to a reference image
  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();
  return EXIT_SUCCESS;
}
