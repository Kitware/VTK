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

#include "vtkChartXY.h"
#include "vtkColorTransferControlPointsItem.h"
#include "vtkColorTransferFunction.h"
#include "vtkColorTransferFunctionItem.h"
#include "vtkContextScene.h"
#include "vtkContextView.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"

//----------------------------------------------------------------------------
int TestColorTransferFunction(int ,  char * [])
{
  // Set up a 2D scene, add an XY chart to it
  vtkSmartPointer<vtkContextView> view =
      vtkSmartPointer<vtkContextView>::New();
  view->GetRenderer()->SetBackground(1.0, 1.0, 1.0);
  view->GetRenderWindow()->SetSize(400, 300);
  vtkSmartPointer<vtkChartXY> chart = vtkSmartPointer<vtkChartXY>::New();
  chart->SetTitle("Chart");
  view->GetScene()->AddItem(chart);

  vtkSmartPointer<vtkColorTransferFunction> colorTransferFunction =
    vtkSmartPointer<vtkColorTransferFunction>::New();
  colorTransferFunction->AddHSVSegment(50.,0.,1.,1.,85.,0.3333,1.,1.);
  colorTransferFunction->AddHSVSegment(85.,0.3333,1.,1.,170.,0.6666,1.,1.);
  colorTransferFunction->AddHSVSegment(170.,0.6666,1.,1.,200.,0.,1.,1.);

  colorTransferFunction->Build();

  vtkSmartPointer<vtkColorTransferFunctionItem> colorTransferItem =
    vtkSmartPointer<vtkColorTransferFunctionItem>::New();
  colorTransferItem->SetColorTransferFunction(colorTransferFunction);
  chart->AddPlot(colorTransferItem);

  vtkSmartPointer<vtkColorTransferControlPointsItem> controlPointsItem =
    vtkSmartPointer<vtkColorTransferControlPointsItem>::New();
  controlPointsItem->SetColorTransferFunction(colorTransferFunction);
  controlPointsItem->SetUserBounds(0., 255., 0., 1.);
  chart->AddPlot(controlPointsItem);

  //Finally render the scene and compare the image to a reference image
  // view->GetRenderWindow()->SetMultiSamples(1);
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
