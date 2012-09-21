/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkPolyDataMapper.h"
#include "vtkNew.h"
#include "vtkProperty.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkProteinRibbonFilter.h"
#include "vtkPDBReader.h"

int TestProteinRibbon(int argc, char *argv[])
{
  char* fileName =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/2LYZ.pdb");

  // read protein from pdb
  vtkNew<vtkPDBReader> reader;
  reader->SetFileName(fileName);

  delete [] fileName;

  // setup ribbon filter
  vtkNew<vtkProteinRibbonFilter> ribbonFilter;
  ribbonFilter->SetInputConnection(reader->GetOutputPort());
  ribbonFilter->Update();

  // setup poly data mapper
  vtkNew<vtkPolyDataMapper> polyDataMapper;
  polyDataMapper->SetInputData(ribbonFilter->GetOutput());
  polyDataMapper->Update();

  // setup actor
  vtkNew<vtkActor> actor;
  actor->SetMapper(polyDataMapper.GetPointer());
  actor->GetProperty()->SetColor(0.0f, 1.0f, 0.0f);

  // setup render window
  vtkNew<vtkRenderer> ren;
  vtkNew<vtkRenderWindow> win;
  win->AddRenderer(ren.GetPointer());
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(win.GetPointer());
  ren->AddActor(actor.GetPointer());
  ren->SetBackground(0.0, 0.0, 0.0);
  win->SetSize(450, 450);
  win->Render();
  ren->GetActiveCamera()->Zoom(2.2);
  ren->ResetCamera();

  // Finally render the scene and compare the image to a reference image
  win->SetMultiSamples(0);
  win->GetInteractor()->Initialize();
  win->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
