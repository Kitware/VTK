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

#include "vtkAxis.h"
#include "vtkChartXY.h"
#include "vtkContextScene.h"
#include "vtkContextView.h"
#include "vtkFloatArray.h"
#include "vtkNew.h"
#include "vtkPlot.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"
#include "vtkTextProperty.h"

#include <string>

//----------------------------------------------------------------------------
int TestChartUnicode(int argc, char *argv[])
{
  if (argc < 2)
  {
    cout << "Missing font filename." << endl;
    return EXIT_FAILURE;
  }

  std::string fontFile(argv[1]);

  // Set up a 2D scene, add an XY chart to it
  vtkNew<vtkContextView> view;
  view->GetRenderWindow()->SetSize(400, 300);
  vtkNew<vtkChartXY> chart;
  view->GetScene()->AddItem(chart.GetPointer());

  // Exercise the support for extended characters using UTF8 encoded strings.
  chart->GetTitleProperties()->SetFontFamily(VTK_FONT_FILE);
  chart->GetTitleProperties()->SetFontFile(fontFile.c_str());
  chart->SetTitle("\xcf\x85\xcf\x84\xce\xba");

  vtkAxis *axis1 = chart->GetAxis(0);
  axis1->GetTitleProperties()->SetFontFamily(VTK_FONT_FILE);
  axis1->GetTitleProperties()->SetFontFile(fontFile.c_str());
  axis1->SetTitle("\xcf\x87(m)");

  vtkAxis *axis2 = chart->GetAxis(1);
  axis2->GetTitleProperties()->SetFontFamily(VTK_FONT_FILE);
  axis2->GetTitleProperties()->SetFontFile(fontFile.c_str());
  axis2->SetTitle("\xcf\x80\xcf\x86");

  // Create a table with some points in it...
  vtkNew<vtkTable> table;
  vtkNew<vtkFloatArray> arrX;
  arrX->SetName("X Axis");
  table->AddColumn(arrX.GetPointer());
  vtkNew<vtkFloatArray> arrC;
  arrC->SetName("Cosine");
  table->AddColumn(arrC.GetPointer());
  int numPoints = 69;
  float inc = 7.5 / (numPoints - 1);
  table->SetNumberOfRows(numPoints);
  for (int i = 0; i < numPoints; ++i)
  {
    table->SetValue(i, 0, i * inc);
    table->SetValue(i, 1, cos(i * inc) + sin(i * (inc - 3.14)));
  }

  // Add multiple line plots, setting the colors etc
  vtkPlot *line = chart->AddPlot(vtkChart::LINE);
  line->SetInputData(table.GetPointer(), 0, 1);
  line->SetColor(42, 55, 69, 255);

  // Render the scene and compare the image to a reference image
  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
