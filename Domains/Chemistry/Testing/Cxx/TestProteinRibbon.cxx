/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkRegressionTestImage.h"
#include "vtkTestUtilities.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkInteractorStyleSwitch.h"
#include "vtkPDBReader.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkProteinRibbonFilter.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

int TestProteinRibbon(int argc, char* argv[])
{
  char* fileName = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/3GQP.pdb");

  // read protein from pdb
  vtkNew<vtkPDBReader> reader;
  reader->SetFileName(fileName);

  delete[] fileName;

  // setup ribbon filter
  vtkSmartPointer<vtkProteinRibbonFilter> ribbonFilter =
    vtkSmartPointer<vtkProteinRibbonFilter>::New();
  ribbonFilter->SetInputConnection(reader->GetOutputPort());
  ribbonFilter->Update();

  // setup poly data mapper
  vtkSmartPointer<vtkPolyDataMapper> polyDataMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  polyDataMapper->SetInputData(ribbonFilter->GetOutput());
  polyDataMapper->Update();

  // setup actor
  vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(polyDataMapper);

  // setup render window
  vtkSmartPointer<vtkRenderer> ren = vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderWindow> win = vtkSmartPointer<vtkRenderWindow>::New();
  win->AddRenderer(ren);
  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(win);
  vtkSmartPointer<vtkInteractorStyleSwitch> is =
    vtkInteractorStyleSwitch::SafeDownCast(iren->GetInteractorStyle());
  if (is)
  {
    is->SetCurrentStyleToTrackballCamera();
  }
  ren->AddActor(actor);
  ren->SetBackground(0, 0, 0);
  win->SetSize(450, 450);
  ren->ResetCamera();
  ren->GetActiveCamera()->Zoom(1.5);
  ren->ResetCameraClippingRange();
  win->Render();

  // Finally render the scene and compare the image to a reference image
  win->SetMultiSamples(0);
  win->GetInteractor()->Initialize();
  win->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
