/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestBoxPlot.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkChartBox.h"
#include "vtkPlotBox.h"
#include "vtkTable.h"
#include "vtkLookupTable.h"
#include "vtkIntArray.h"
#include "vtkFloatArray.h"
#include "vtkContextView.h"
#include "vtkContextScene.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkNew.h"

//----------------------------------------------------------------------------
int TestBoxPlot(int , char* [])
{
  // Set up a 2D scene, add an XY chart to it
  vtkNew<vtkContextView> view;
  view->GetRenderWindow()->SetSize(400, 400);
  view->GetRenderWindow()->SetMultiSamples(0);

  vtkNew<vtkChartBox> chart;
  view->GetScene()->AddItem(chart.GetPointer());

  // Creates a vtkPlotBox input table
  // The vtkPlotBox object will display 4 (arbitrary) box plots
  int numParam = 4;
  vtkNew<vtkTable> inputBoxPlotTable;

  for (int i = 0; i < numParam; i++)
    {
    char num[3];
    sprintf(num, "%d", i);
    char name[10];
    strcpy(name,"Param ");
    strcat(name,num);

    vtkNew<vtkIntArray> arrIndex;
    arrIndex->SetName(name);
    inputBoxPlotTable->AddColumn(arrIndex.GetPointer());
    }

  inputBoxPlotTable->SetNumberOfRows(5);

  for (int i = 0; i < numParam; i++)
    {
    inputBoxPlotTable->SetValue(0, i, i/2); //Q0
    inputBoxPlotTable->SetValue(1, i, 2*i + 2 - i); //Q1
    inputBoxPlotTable->SetValue(2, i, 2*i + 4); //Q2
    inputBoxPlotTable->SetValue(3, i, 2*i + 7); //Q3
    inputBoxPlotTable->SetValue(4, i, 2*i + 8); //Q4
    }

  vtkNew<vtkLookupTable> lookup;
  lookup->SetNumberOfColors(5);
  lookup->SetRange(0, 4);
  lookup->Build();

  chart->GetPlot(0)->SetInputData(inputBoxPlotTable.GetPointer());
  chart->SetColumnVisibilityAll(true);
  chart->SetShowLegend(true);
  double rgb[3] = { 1., 1., 0. };
  vtkPlotBox::SafeDownCast(chart->GetPlot(0))->SetColumnColor("Param 1", rgb);

  // Render the scene
  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->Render();
  view->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
