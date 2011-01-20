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
#include "vtkChartPie.h"
#include "vtkPlot.h"
#include "vtkPlotPie.h"
#include "vtkTable.h"
#include "vtkIntArray.h"
#include "vtkStringArray.h"
#include "vtkContextView.h"
#include "vtkContextScene.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRegressionTestImage.h"
#include "vtkColorSeries.h"

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

#define NUM_ITEMS (5)
static int data[] = {77938,9109,2070,12806,19514};
//static int data[] = {200,200,200,200,200};
static const char *labels[] = {"Books","New and Popular","Periodical","Audiobook","Video"};

//----------------------------------------------------------------------------
int TestPieChart(int , char * [])
{
  // Set up a 2D scene, add an XY chart to it
  VTK_CREATE(vtkContextView, view);
  view->GetRenderer()->SetBackground(1.0, 1.0, 1.0);
  view->GetRenderWindow()->SetSize(600, 350);
  VTK_CREATE(vtkChartPie, chart);
  view->GetScene()->AddItem(chart);

  // Create a table with some points in it...
  VTK_CREATE(vtkTable, table);

  VTK_CREATE(vtkIntArray,arrData);
  VTK_CREATE(vtkStringArray,labelArray);

  arrData->SetName("2008 Circulation");
  for (int i = 0; i < NUM_ITEMS; i++)
    {
    arrData->InsertNextValue(data[i]);
    labelArray->InsertNextValue(labels[i]);
    }

  table->AddColumn(arrData);

  // Create a color series to use with our stacks.
  VTK_CREATE(vtkColorSeries,colorSeries);
  colorSeries->SetColorScheme(vtkColorSeries::WARM);

  // Add multiple line plots, setting the colors etc
  vtkPlotPie *pie = 0;

  pie = vtkPlotPie::SafeDownCast(chart->AddPlot(0));
  pie->SetColorSeries(colorSeries);
  pie->SetInput(table);
  pie->SetInputArray(0,"2008 Circulation");
  pie->SetLabels(labelArray);

  chart->SetShowLegend(true);

  chart->SetTitle("Circulation 2008");

  //Finally render the scene and compare the image to a reference image
  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
