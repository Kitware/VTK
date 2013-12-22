/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestBagPlot.cxx

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
#include "vtkDoubleArray.h"
#include "vtkIntArray.h"
#include "vtkNew.h"
#include "vtkPlotBag.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTable.h"

//----------------------------------------------------------------------------
int TestBagPlot(int, char * [])
{
  // Set up a 2D scene, add an XY chart to it
  vtkNew<vtkContextView> view;
  view->GetRenderWindow()->SetSize(400, 400);
  view->GetRenderWindow()->SetMultiSamples(0);
  vtkNew<vtkChartXY> chart;
  view->GetScene()->AddItem(chart.GetPointer());
  chart->SetShowLegend(true);

  // Creates a vtkPlotBag input table
  // We construct a 2D grid 20*20.
  int numDataI = 20;
  int numDataJ = 20;

  vtkNew<vtkIntArray> arrX;
  arrX->SetName("X");

  vtkNew<vtkDoubleArray> arrY;
  arrY->SetName("Y");

  vtkNew<vtkDoubleArray> arrDensity;
  arrDensity->SetName("Density");

  vtkNew<vtkTable> table;
  table->AddColumn(arrX.GetPointer());
  table->AddColumn(arrY.GetPointer());
  table->AddColumn(arrDensity.GetPointer());

  table->SetNumberOfRows(numDataI * numDataJ);

  // Fill the table
  for (int j = 0; j < numDataJ; ++j)
    {
    for (int i = 0; i < numDataI; ++i)
      {
      table->SetValue(i + j * numDataI, 0, i); //X
      table->SetValue(i + j * numDataI, 1, j); //Y
      double dx = (numDataI / 2. - i) / (numDataI / 2.);
      double dy = (numDataJ / 2. - j) / (numDataJ / 2.);
      double d = 1. - sqrt(dx * dx + dy * dy);
      d = floor(d * 100.) / 100.; // to avoid conflicts
      d += (i + j * numDataI) / (double)(1000. * numDataI * numDataJ);
      table->SetValue(i + j * numDataI, 2, d); // Density
      }
    }

  vtkNew<vtkPlotBag> bagPlot;
  chart->AddPlot(bagPlot.GetPointer());
  bagPlot->SetInputData(table.GetPointer(), arrX->GetName(),
    arrY->GetName(), arrDensity->GetName());
  bagPlot->SetColor(255, 0, 0, 255);
  bagPlot->SetMarkerSize(4);

  // Render the scene
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
