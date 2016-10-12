/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestFunctionalBagPlot.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkChartXY.h"
#include "vtkChartLegend.h"
#include "vtkContextScene.h"
#include "vtkContextView.h"
#include "vtkDoubleArray.h"
#include "vtkLookupTable.h"
#include "vtkPlotFunctionalBag.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkPen.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include <sstream>

//----------------------------------------------------------------------------
int TestFunctionalBagPlot(int, char * [])
{
  // Creates an input table
  const int numCols = 7;
  const int numVals = 100;

  vtkNew<vtkTable> inputTable;
  vtkNew<vtkDoubleArray> arr[numCols];
  for (int i = 0; i < numCols; i++)
  {
    std::stringstream ss;
    ss << "Y" << i;
    arr[i]->SetName(ss.str().c_str());
    arr[i]->SetNumberOfValues(numVals);
    for (int j = 0; j < numVals; j++)
    {
      arr[i]->SetValue(j, (i+1) *
        fabs(sin((j * 2.f *vtkMath::Pi()) /
        static_cast<float>(numVals))) * j + i * 20);
    }
    inputTable->AddColumn(arr[i].GetPointer());
  }

  // Create a X-axis column
  vtkNew<vtkDoubleArray> xArr;
  xArr->SetName("X");
  xArr->SetNumberOfValues(numVals);
  for (int j = 0; j < numVals; j++)
  {
    xArr->SetValue(j, j * 2.0);
  }
  inputTable->AddColumn(xArr.GetPointer());

  // Create the bag columns
  vtkNew<vtkDoubleArray> q3Arr;
  q3Arr->SetName("Q3");
  q3Arr->SetNumberOfComponents(2);
  q3Arr->SetNumberOfTuples(numVals);
  vtkNew<vtkDoubleArray> q2Arr;
  q2Arr->SetName("Q2");
  q2Arr->SetNumberOfComponents(2);
  q2Arr->SetNumberOfTuples(numVals);

  for (int i = 0; i < numVals; i++)
  {
    double v0, v1;
    v0 = arr[1]->GetVariantValue(i).ToFloat();
    v1 = arr[5]->GetVariantValue(i).ToFloat();
    q3Arr->SetTuple2(i, v0, v1);

    v0 = arr[2]->GetVariantValue(i).ToFloat();
    v1 = arr[4]->GetVariantValue(i).ToFloat();
    q2Arr->SetTuple2(i, v0, v1);
  }

  inputTable->AddColumn(q3Arr.GetPointer());
  inputTable->AddColumn(q2Arr.GetPointer());

  // Set up a 2D scene and add an XY chart to it
  vtkNew<vtkContextView> view;
  view->GetRenderWindow()->SetSize(400, 400);
  view->GetRenderWindow()->SetMultiSamples(0);
  vtkNew<vtkChartXY> chart;
  view->GetScene()->AddItem(chart.GetPointer());
  chart->SetShowLegend(true);
  chart->GetLegend()->SetHorizontalAlignment(vtkChartLegend::LEFT);
  chart->GetLegend()->SetVerticalAlignment(vtkChartLegend::TOP);

  // Create the functional bag plots
  vtkNew<vtkPlotFunctionalBag> q3Plot;
  q3Plot->SetColor(0.5, 0, 0);
  q3Plot->SetInputData(inputTable.GetPointer(), "X", "Q3");
  chart->AddPlot(q3Plot.GetPointer());

  vtkNew<vtkPlotFunctionalBag> q2Plot;
  q2Plot->SetColor(1., 0, 0);
  q2Plot->SetInputData(inputTable.GetPointer(), "X", "Q2");
  chart->AddPlot(q2Plot.GetPointer());

  vtkNew<vtkLookupTable> lookup;
  lookup->SetNumberOfColors(numCols);
  lookup->SetRange(0, numCols-1);
  lookup->Build();
  for (int j = 0; j < numCols; j++)
  {
    vtkNew<vtkPlotFunctionalBag> plot;
    double rgb[3];
    lookup->GetColor(j, rgb);
    plot->SetColor(rgb[0], rgb[1], rgb[2]);
    plot->SetInputData(inputTable.GetPointer(), "X",
      inputTable->GetColumn(j)->GetName());
    chart->AddPlot(plot.GetPointer());
  }

  // Render the scene
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
