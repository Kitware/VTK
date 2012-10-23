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
#include "vtkMolecule.h"
#include "vtkLight.h"
#include "vtkMoleculeMapper.h"
#include "vtkNew.h"
#include "vtkProperty.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkPDBReader.h"

int TestPDBBallAndStick(int argc, char *argv[])
{
  char* fileName =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/2LYZ.pdb");

  // read protein from pdb
  vtkNew<vtkPDBReader> reader;
  reader->SetFileName(fileName);

  delete [] fileName;

  vtkNew<vtkMoleculeMapper> molmapper;
  molmapper->SetInputConnection(reader->GetOutputPort(1));

  molmapper->UseBallAndStickSettings();

  vtkNew<vtkActor> actor;
  actor->SetMapper(molmapper.GetPointer());
  actor->GetProperty()->SetAmbient(0.0);
  actor->GetProperty()->SetDiffuse(0.0);
  actor->GetProperty()->SetSpecular(0.0);
  actor->GetProperty()->SetSpecularPower(40);

  vtkNew<vtkLight> light;
  light->SetLightTypeToCameraLight();
  light->SetPosition(1.0, 1.0, 1.0);

  vtkNew<vtkRenderer> ren;
  vtkNew<vtkRenderWindow> win;
  win->AddRenderer(ren.GetPointer());
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(win.GetPointer());

  ren->AddActor(actor.GetPointer());
  ren->ResetCamera();
  ren->GetActiveCamera()->Zoom(1.7);
  ren->SetBackground(0.0, 0.0, 0.0);
  win->SetSize(450, 450);
  win->Render();

  // Finally render the scene and compare the image to a reference image
  win->SetMultiSamples(0);
  win->GetInteractor()->Initialize();
  win->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
