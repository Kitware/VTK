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

#include "vtkColorTransferFunction.h"
#include "vtkColorTransferFunctionItem.h"
#include "vtkCompositeTransferFunctionItem.h"
#include "vtkContextScene.h"
#include "vtkContextView.h"
#include "vtkFloatArray.h"
#include "vtkLookupTable.h"
#include "vtkLookupTableItem.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPiecewiseFunctionItem.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"

//----------------------------------------------------------------------------
int TestScalarsToColors( int argc, char * argv [] )
{
  // Set up a 2D scene, add an XY chart to it
  vtkSmartPointer<vtkContextView> view =
      vtkSmartPointer<vtkContextView>::New();
  view->GetRenderer()->SetBackground(1.0, 1.0, 1.0);
  view->GetRenderWindow()->SetSize(400, 300);
  //vtkSmartPointer<vtkChartXY> chart = vtkSmartPointer<vtkChartXY>::New();
  //view->GetScene()->AddItem(chart);

  vtkSmartPointer<vtkLookupTable> lookupTable =
    vtkSmartPointer<vtkLookupTable>::New();
  lookupTable->Build();
  //vtkSmartPointer<vtkLookupTableChart> lChart =
  //  vtkSmartPointer<vtkLookupTableChart>::New();
  //lChart->SetLookupTable(lookupTable);
  //lChart->SetChart(chart);
  vtkSmartPointer<vtkLookupTableItem> item =
    vtkSmartPointer<vtkLookupTableItem>::New();
  item->SetLookupTable(lookupTable);
  //view->GetScene()->AddItem(item);

  vtkSmartPointer<vtkColorTransferFunction> colorTransferFunction =
    vtkSmartPointer<vtkColorTransferFunction>::New();
  colorTransferFunction->AddHSVSegment(0.,0.,1.,1.,0.3333,0.3333,1.,1.);
  colorTransferFunction->AddHSVSegment(0.3333,0.3333,1.,1.,0.6666,0.6666,1.,1.);
  colorTransferFunction->AddHSVSegment(0.6666,0.6666,1.,1.,1.,0.,1.,1.);

  colorTransferFunction->Build();
  vtkSmartPointer<vtkColorTransferFunctionItem> item2 =
    vtkSmartPointer<vtkColorTransferFunctionItem>::New();
  item2->SetColorTransferFunction(colorTransferFunction);
  //view->GetScene()->AddItem(item2);

  vtkSmartPointer<vtkPiecewiseFunction> opacityFunction =
    vtkSmartPointer<vtkPiecewiseFunction>::New();
  opacityFunction->AddPoint(0.,0.);
  opacityFunction->AddPoint(1.,1.);
  vtkSmartPointer<vtkCompositeTransferFunctionItem> item3 =
    vtkSmartPointer<vtkCompositeTransferFunctionItem>::New();
  item3->SetColorTransferFunction(colorTransferFunction);
  item3->SetOpacityFunction(opacityFunction);
  item3->SetOpacity(0.2);
  item3->SetMaskAboveCurve(true);
  view->GetScene()->AddItem(item3);

  vtkSmartPointer<vtkPiecewiseFunctionItem> item4 =
    vtkSmartPointer<vtkPiecewiseFunctionItem>::New();
  item4->SetPiecewiseFunction(opacityFunction);
  item4->SetColor(255,0,0);
  //item4->SetMaskAboveCurve(true);
  //view->GetScene()->AddItem(item4);

  //Finally render the scene and compare the image to a reference image
  //view->GetRenderWindow()->SetMultiSamples(0);
  int retVal = vtkRegressionTestImage(view->GetRenderWindow());
  if(retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    view->GetInteractor()->Initialize();
    view->GetInteractor()->Start();
    }

  return !retVal;
}
