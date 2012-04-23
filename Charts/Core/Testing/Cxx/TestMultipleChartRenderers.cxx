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

#include "vtkActor.h"
#include "vtkChartXY.h"
#include "vtkContextView.h"
#include "vtkContextScene.h"
#include "vtkContextActor.h"
#include "vtkFloatArray.h"
#include "vtkPlot.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

//----------------------------------------------------------------------------
int TestMultipleChartRenderers(int , char * [])
{

  VTK_CREATE(vtkRenderWindow, renwin);
  renwin->SetMultiSamples(0);
  renwin->SetSize(800, 640);

  VTK_CREATE(vtkRenderWindowInteractor, iren);
  iren->SetRenderWindow(renwin);

  //setup the 4charts view ports
  double viewports[16] ={
    0.0,0.0,0.3,0.5,
    0.3,0.0,1.0,0.5,
    0.0,0.5,0.5,1.0,
    0.5,0.5,1.0,1.0};

  for ( int i=0; i < 4; ++i)
    {
    VTK_CREATE(vtkRenderer, ren);
    ren->SetBackground(1.0,1.0,1.0);
    ren->SetViewport(&viewports[i*4]);
    renwin->AddRenderer(ren);

    VTK_CREATE(vtkChartXY, chart);
    VTK_CREATE(vtkContextScene, chartScene);
    VTK_CREATE(vtkContextActor, chartActor);

    chartScene->AddItem(chart);
    chartActor->SetScene(chartScene);

    //both needed
    ren->AddActor(chartActor);
    chartScene->SetRenderer(ren);

    // Create a table with some points in it...
    VTK_CREATE(vtkTable, table);
    VTK_CREATE(vtkFloatArray, arrX);
    arrX->SetName("X Axis");
    table->AddColumn(arrX);
    VTK_CREATE(vtkFloatArray, arrC);
    arrC->SetName("Cosine");
    table->AddColumn(arrC);
    VTK_CREATE(vtkFloatArray, arrS);
    arrS->SetName("Sine");
    table->AddColumn(arrS);
    VTK_CREATE(vtkFloatArray, arrS2);
    arrS2->SetName("Sine2");
    table->AddColumn(arrS2);
    // Test charting with a few more points...
    int numPoints = 69;
    float inc = 7.5 / (numPoints-1);
    table->SetNumberOfRows(numPoints);
    for (int j = 0; j < numPoints; ++j)
      {
      table->SetValue(j, 0, j * inc);
      table->SetValue(j, 1, cos(j * inc) + 0.0);
      table->SetValue(j, 2, sin(j * inc) + 0.0);
      table->SetValue(j, 3, sin(j * inc) + 0.5);
      }

    // Add multiple line plots, setting the colors etc
    vtkPlot *line = chart->AddPlot(vtkChart::LINE);
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
    }

  iren->Initialize();
  iren->Start();

  return EXIT_SUCCESS;
}
