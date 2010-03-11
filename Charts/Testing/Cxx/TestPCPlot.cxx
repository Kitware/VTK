/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPCPlot.cxx

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
#include "vtkChartParallelCoordinates.h"
#include "vtkPlot.h"
#include "vtkTable.h"
#include "vtkFloatArray.h"
#include "vtkContextView.h"
#include "vtkContextScene.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRegressionTestImage.h"

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

//----------------------------------------------------------------------------
int TestPCPlot(int argc, char* argv[])
{
  // Set up a 2D scene, add an XY chart to it
  VTK_CREATE(vtkContextView, view);
  view->GetRenderer()->SetBackground(1.0, 1.0, 1.0);
  view->GetRenderWindow()->SetSize(800, 600);
  VTK_CREATE(vtkChartParallelCoordinates, chart);
  view->GetScene()->AddItem(chart);

  // Create a table with some points in it...
  VTK_CREATE(vtkTable, table);
  VTK_CREATE(vtkFloatArray, arrX);
  arrX->SetName("Field 1");
  table->AddColumn(arrX);
  VTK_CREATE(vtkFloatArray, arrC);
  arrC->SetName("Field 2");
  table->AddColumn(arrC);
  VTK_CREATE(vtkFloatArray, arrS);
  arrS->SetName("Field 3");
  table->AddColumn(arrS);
  VTK_CREATE(vtkFloatArray, arrS2);
  arrS2->SetName("Field 4");
  table->AddColumn(arrS2);
  // Test charting with a few more points...
  int numPoints = 200;
  float inc = 7.5 / (numPoints-1);
  table->SetNumberOfRows(numPoints);
  for (int i = 0; i < numPoints; ++i)
    {
    table->SetValue(i, 0, i * inc);
    table->SetValue(i, 1, cos(i * inc) + 0.0);
    table->SetValue(i, 2, sin(i * inc) + 0.0);
    table->SetValue(i, 3, tan(i * inc) + 0.5);
    }

  chart->GetPlot(0)->SetInput(table);

  view->GetRenderWindow()->SetMultiSamples(0);
  //Finally render the scene and compare the image to a reference image
  //int retVal = vtkRegressionTestImageThreshold(view->GetRenderWindow(), 25);
  int retVal = vtkRegressionTestImage(view->GetRenderWindow());
  if(retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    view->GetInteractor()->Start();
    }

  return !retVal;
}
