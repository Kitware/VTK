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
#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"

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

  vtkNew<vtkLabeledContourMapper> mapper;
  mapper->GetPolyDataMapper()->ScalarVisibilityOff();
  mapper->GetTextProperty()->SetBold(1);
  mapper->GetTextProperty()->SetFontSize(12);
  mapper->SetInputConnection(contourStripper->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper.GetPointer());

  vtkNew<vtkRenderer> ren;
  ren->AddActor(actor.GetPointer());

  vtkNew<vtkRenderWindow> win;
  win->SetStencilCapable(1); // Needed for vtkLabeledContourMapper
  win->AddRenderer(ren.GetPointer());

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
  iren->SetRenderWindow(win.GetPointer());

  int retVal = vtkRegressionTestImage(win.GetPointer());
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }
  return !retVal;
}
