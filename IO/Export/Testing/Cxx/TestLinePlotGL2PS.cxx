/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestLinePlotGL2PS.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkChartXY.h"
#include "vtkContextScene.h"
#include "vtkContextView.h"
#include "vtkFloatArray.h"
#include "vtkGL2PSExporter.h"
#include "vtkNew.h"
#include "vtkPlot.h"
#include "vtkPlotLine.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"
#include "vtkTestingInteractor.h"

//----------------------------------------------------------------------------
int TestLinePlotGL2PS(int , char * [])
{
  // Set up a 2D scene, add an XY chart to it
  vtkNew<vtkContextView> view;
  view->GetRenderWindow()->SetSize(400, 300);
  vtkNew<vtkChartXY> chart;
  view->GetScene()->AddItem(chart.GetPointer());
  chart->SetShowLegend(true);

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
  vtkNew<vtkFloatArray> arrS2;
  arrS2->SetName("Sine2");
  table->AddColumn(arrS2.GetPointer());
  vtkNew<vtkFloatArray> arr1;
  arr1->SetName("One");
  table->AddColumn(arr1.GetPointer());
  vtkNew<vtkFloatArray> arr0;
  arr0->SetName("Zero");
  table->AddColumn(arr0.GetPointer());
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
    table->SetValue(i, 4, 1.0);
    table->SetValue(i, 5, 0.0);
  }

  // Add multiple line plots, setting the colors etc
  vtkPlotLine *line = vtkPlotLine::SafeDownCast(chart->AddPlot(vtkChart::LINE));
  line->SetInputData(table.GetPointer(), 0, 1);
  line->SetColor(0, 255, 0, 255);
  line->SetWidth(1.0);
  line->SetMarkerStyle(vtkPlotLine::CIRCLE);
  line = vtkPlotLine::SafeDownCast(chart->AddPlot(vtkChart::LINE));
  line->SetInputData(table.GetPointer(), 0, 2);
  line->SetColor(255, 0, 0, 255);
  line->SetWidth(5.0);
  line->SetMarkerStyle(vtkPlotLine::SQUARE);
  line = vtkPlotLine::SafeDownCast(chart->AddPlot(vtkChart::LINE));
  line->SetInputData(table.GetPointer(), 0, 3);
  line->SetColor(0, 0, 255, 255);
  line->SetWidth(4.0);
  line->SetMarkerStyle(vtkPlotLine::DIAMOND);
  line = vtkPlotLine::SafeDownCast(chart->AddPlot(vtkChart::LINE));
  line->SetInputData(table.GetPointer(), 0, 4);
  line->SetColor(0, 255, 255, 255);
  line->SetWidth(4.0);
  line->SetMarkerStyle(vtkPlotLine::CROSS);
  line = vtkPlotLine::SafeDownCast(chart->AddPlot(vtkChart::LINE));
  line->SetInputData(table.GetPointer(), 0, 5);
  line->SetColor(255, 255, 0, 255);
  line->SetWidth(4.0);
  line->SetMarkerStyle(vtkPlotLine::PLUS);


  // Render the scene and compare the image to a reference image
  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetRenderWindow()->Render();

  vtkNew<vtkGL2PSExporter> exp;
  exp->SetRenderWindow(view->GetRenderWindow());
  exp->SetFileFormatToPS();
  exp->UsePainterSettings();
  exp->CompressOff();
  exp->DrawBackgroundOn();

  std::string fileprefix = vtkTestingInteractor::TempDirectory +
      std::string("/TestLinePlotGL2PS");

  exp->SetFilePrefix(fileprefix.c_str());
  exp->Write();

  //Finally render the scene and compare the image to a reference image
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
