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
#include "vtkChartXY.h"
#include "vtkPlot.h"
#include "vtkTable.h"
#include "vtkIntArray.h"
#include "vtkContextView.h"
#include "vtkContextScene.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRegressionTestImage.h"

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()


// Monthly circulation data
static int data_2008[] = {10822, 10941, 9979, 10370, 9460, 11228, 15093, 12231, 10160, 9816, 9384, 7892};
static int data_2009[] = {9058, 9474, 9979, 9408, 8900, 11569, 14688, 12231, 10294, 9585, 8957, 8590};
static int data_2010[] = {9058, 10941, 9979, 10270, 8900, 11228, 14688, 12231, 10160, 9585, 9384, 8590};

//----------------------------------------------------------------------------
int TestBarGraph( int argc, char * argv [] )
{
  // Set up a 2D scene, add an XY chart to it
  VTK_CREATE(vtkContextView, view);
  view->GetRenderer()->SetBackground(1.0, 1.0, 1.0);
  view->GetRenderWindow()->SetSize(400, 300);
  VTK_CREATE(vtkChartXY, chart);
  view->GetScene()->AddItem(chart);

  // Create a table with some points in it...
  VTK_CREATE(vtkTable, table);

  VTK_CREATE(vtkIntArray, arrMonth);
  arrMonth->SetName("Month");
  table->AddColumn(arrMonth);

  VTK_CREATE(vtkIntArray, arr2008);
  arr2008->SetName("2008");
  table->AddColumn(arr2008);

  VTK_CREATE(vtkIntArray, arr2009);
  arr2009->SetName("2009");
  table->AddColumn(arr2009);

  VTK_CREATE(vtkIntArray, arr2010);
  arr2010->SetName("2010");
  table->AddColumn(arr2010);

  table->SetNumberOfRows(12);
  for (int i = 0; i < 12; i++)
    {
    table->SetValue(i,0,i+1);
    table->SetValue(i,1,data_2008[i]);
    table->SetValue(i,2,data_2009[i]);
    table->SetValue(i,3,data_2010[i]);
    }

  // Add multiple line plots, setting the colors etc
  vtkPlot *line = 0;

  line = chart->AddPlot(vtkChart::BAR);
  line->SetInput(table, 0, 1);
  line->SetColor(0, 255, 0, 255);

  line = chart->AddPlot(vtkChart::BAR);
  line->SetInput(table, 0, 2);
  line->SetColor(255, 0, 0, 255);

  line = chart->AddPlot(vtkChart::BAR);
  line->SetInput(table, 0, 3);
  line->SetColor(0, 0, 255, 255);

  //Finally render the scene and compare the image to a reference image
  view->GetRenderWindow()->SetMultiSamples(0);
  int retVal = vtkRegressionTestImageThreshold(view->GetRenderWindow(), 25);
  //int retVal = vtkRegressionTestImage(view->GetRenderWindow());
  if(retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    view->GetInteractor()->Initialize();
    view->GetInteractor()->Start();
    }

  return !retVal;
}
