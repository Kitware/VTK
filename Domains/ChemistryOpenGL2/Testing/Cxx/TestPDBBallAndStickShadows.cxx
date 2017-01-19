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
#include "vtkPlaneSource.h"
#include "vtkPolyDataMapper.h"

#include "vtkTimerLog.h"
#include "vtkCamera.h"

int TestPDBBallAndStickShadows(int argc, char *argv[])
{
  char* fileName =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/2LYZ.pdb");

  // read protein from pdb
  vtkNew<vtkPDBReader> reader;
  reader->SetFileName(fileName);
  reader->Update();

  delete [] fileName;

  vtkNew<vtkMoleculeMapper> molmapper;
  molmapper->SetInputConnection(reader->GetOutputPort(1));

  cerr << "Class: " << molmapper->GetClassName() << endl;
  cerr << "Atoms: " << molmapper->GetInput()->GetNumberOfAtoms() << endl;
  cerr << "Bonds: " << molmapper->GetInput()->GetNumberOfBonds() << endl;

  molmapper->UseBallAndStickSettings();

  vtkNew<vtkActor> actor;
  actor->SetMapper(molmapper.GetPointer());
  actor->GetProperty()->SetAmbient(0.2);
  actor->GetProperty()->SetDiffuse(0.7);
  actor->GetProperty()->SetSpecular(0.3);
  actor->GetProperty()->SetSpecularPower(40);

  vtkNew<vtkRenderer> ren;
  vtkNew<vtkRenderWindow> win;
  win->AddRenderer(ren.GetPointer());
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(win.GetPointer());

  ren->AddActor(actor.GetPointer());
  ren->ResetCamera();
  ren->GetActiveCamera()->Zoom(1.7);
  ren->SetBackground(0.4, 0.5, 0.6);
  win->SetSize(450, 450);

  // add a plane
  vtkNew<vtkPlaneSource> plane;
  const double *bounds = molmapper->GetBounds();
  plane->SetOrigin(bounds[0], bounds[2], bounds[4]);
  plane->SetPoint1(bounds[1], bounds[2], bounds[4]);
  plane->SetPoint2(bounds[0], bounds[2], bounds[5]);
  vtkNew<vtkPolyDataMapper> planeMapper;
  planeMapper->SetInputConnection(plane->GetOutputPort());
  vtkNew<vtkActor> planeActor;
  planeActor->SetMapper(planeMapper.Get());
  ren->AddActor(planeActor.Get());

  vtkNew<vtkLight> light1;
  light1->SetFocalPoint(0,0,0);
  light1->SetPosition(0,1,0.2);
  light1->SetColor(0.95,0.97,1.0);
  light1->SetIntensity(0.8);
  ren->AddLight(light1.Get());

  vtkNew<vtkLight> light2;
  light2->SetFocalPoint(0,0,0);
  light2->SetPosition(1.0,1.0,1.0);
  light2->SetColor(1.0,0.8,0.7);
  light2->SetIntensity(0.3);
  ren->AddLight(light2.Get());

  ren->UseShadowsOn();

  vtkNew<vtkTimerLog> timer;
  timer->StartTimer();
  win->Render();
  timer->StopTimer();
  double firstRender = timer->GetElapsedTime();
  cerr << "first render time: " << firstRender << endl;

/*
  int numRenders = 500;
  timer->StartTimer();
  for (int i = 0; i < numRenders; ++i)
    {
    ren->GetActiveCamera()->Azimuth(85.0/numRenders);
    ren->GetActiveCamera()->Elevation(85.0/numRenders);
    win->Render();
    }
  timer->StopTimer();
  double elapsed = timer->GetElapsedTime();
  cerr << "interactive render time: " << elapsed / numRenders << endl;
*/

  ren->GetActiveCamera()->SetPosition(0,0,1);
  ren->GetActiveCamera()->SetFocalPoint(0,0,0);
  ren->GetActiveCamera()->SetViewUp(0,1,0);
  ren->ResetCamera();
  ren->GetActiveCamera()->Zoom(1.7);

  win->Render();

  // Finally render the scene and compare the image to a reference image
  win->SetMultiSamples(0);
  win->GetInteractor()->Initialize();
  win->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
