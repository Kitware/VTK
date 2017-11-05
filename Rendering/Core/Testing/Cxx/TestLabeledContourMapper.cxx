/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTextActor3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkLabeledContourMapper.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkContourFilter.h"
#include "vtkDoubleArray.h"
#include "vtkDEMReader.h"
#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkStripper.h"
#include "vtkTextProperty.h"
#include "vtkTextPropertyCollection.h"
#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"

#include <algorithm>

//----------------------------------------------------------------------------
int TestLabeledContourMapper(int argc, char *argv[])
{
  char* fname =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/SainteHelens.dem");
  vtkNew<vtkDEMReader> demReader;
  demReader->SetFileName(fname);
  delete [] fname;

  double range[2];
  demReader->Update();
  demReader->GetOutput()->GetPointData()->GetScalars()->GetRange(range);

  vtkNew<vtkContourFilter> contours;
  contours->SetInputConnection(demReader->GetOutputPort());
  contours->GenerateValues(21, range[0], range[1]);

  vtkNew<vtkStripper> contourStripper;
  contourStripper->SetInputConnection(contours->GetOutputPort());
  contourStripper->Update();

  // Setup text properties that will be rotated across the isolines:
  vtkNew<vtkTextPropertyCollection> tprops;
  vtkNew<vtkTextProperty> tprop1;
  tprop1->SetBold(1);
  tprop1->SetFontSize(12);
  tprop1->SetBackgroundColor(0.5, 0.5, 0.5);
  tprop1->SetBackgroundOpacity(0.25);
  tprop1->SetColor(1., 1., 1.);
  tprops->AddItem(tprop1);

  vtkNew<vtkTextProperty> tprop2;
  tprop2->ShallowCopy(tprop1);
  tprop2->SetColor(.8, .2, .3);
  tprops->AddItem(tprop2);

  vtkNew<vtkTextProperty> tprop3;
  tprop3->ShallowCopy(tprop1);
  tprop3->SetColor(.3, .8, .2);
  tprops->AddItem(tprop3);

  vtkNew<vtkTextProperty> tprop4;
  tprop4->ShallowCopy(tprop1);
  tprop4->SetColor(.6, .0, .8);
  tprops->AddItem(tprop4);

  vtkNew<vtkTextProperty> tprop5;
  tprop5->ShallowCopy(tprop1);
  tprop5->SetColor(.0, .0, .9);
  tprops->AddItem(tprop5);

  vtkNew<vtkTextProperty> tprop6;
  tprop6->ShallowCopy(tprop1);
  tprop6->SetColor(.7, .8, .2);
  tprops->AddItem(tprop6);

  // Create a text property mapping that will reverse the coloring:
  double *values = contours->GetValues();
  double *valuesEnd = values + contours->GetNumberOfContours();
  vtkNew<vtkDoubleArray> tpropMapping;
  tpropMapping->SetNumberOfComponents(1);
  tpropMapping->SetNumberOfTuples(valuesEnd - values);
  std::reverse_copy(values, valuesEnd, tpropMapping->Begin());

  vtkNew<vtkLabeledContourMapper> mapper;
  mapper->GetPolyDataMapper()->ScalarVisibilityOff();
  mapper->SetTextProperties(tprops);
  mapper->SetTextPropertyMapping(tpropMapping);
  mapper->SetInputConnection(contourStripper->GetOutputPort());
  mapper->SetSkipDistance(100);

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  vtkNew<vtkRenderer> ren;
  ren->AddActor(actor);

  vtkNew<vtkRenderWindow> win;
  win->SetStencilCapable(1); // Needed for vtkLabeledContourMapper
  win->AddRenderer(ren);

  double bounds[6];
  contourStripper->GetOutput()->GetBounds(bounds);

  win->SetSize(600, 600);
  ren->SetBackground(0.0, 0.0, 0.0);
  ren->GetActiveCamera()->SetViewUp(0, 1, 0);
  ren->GetActiveCamera()->SetPosition((bounds[0] + bounds[1]) / 2.,
                                      (bounds[2] + bounds[3]) / 2., 0);

  ren->GetActiveCamera()->SetFocalPoint((bounds[0] + bounds[1]) / 2.,
                                        (bounds[2] + bounds[3]) / 2.,
                                        (bounds[4] + bounds[5]) / 2.);
  ren->ResetCamera();
  ren->GetActiveCamera()->Dolly(6.5);
  ren->ResetCameraClippingRange();

  win->SetMultiSamples(0);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(win);

  int retVal = vtkRegressionTestImage(win);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
