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
#include "vtkExecutive.h"
#include "vtkInformation.h"
#include "vtkMolecule.h"
#include "vtkMoleculeMapper.h"
#include "vtkNew.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkVASPAnimationReader.h"

int TestVASPAnimationReader(int argc, char *argv[])
{
  if (argc < 2)
  {
    std::cerr << "Missing test file argument." << std::endl;
    return EXIT_FAILURE;
  }

  std::string fname(argv[1]);

  vtkNew<vtkVASPAnimationReader> reader;
  reader->SetFileName(fname.c_str());

  reader->UpdateInformation();
  vtkInformation *outInfo = reader->GetExecutive()->GetOutputInformation(0);
  double *times = outInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  int nTimes = outInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  if (nTimes < 8)
  {
    std::cerr << "Need at least 8 timesteps, only " << nTimes << " found.\n";
    return EXIT_FAILURE;
  }

  // Show different time steps in each renderer:
  vtkNew<vtkRenderer> rens[4];
  rens[0]->SetViewport(0.0, 0.5, 0.5, 1.0);
  rens[1]->SetViewport(0.5, 0.5, 1.0, 1.0);
  rens[2]->SetViewport(0.0, 0.0, 0.5, 0.5);
  rens[3]->SetViewport(0.5, 0.0, 1.0, 0.5);

  vtkNew<vtkMoleculeMapper> mappers[4];
  vtkNew<vtkActor> actors[4];
  vtkNew<vtkRenderWindow> win;
  for (size_t i = 0; i < 4; ++i)
  {
    // Render different timestamps for each:
    reader->UpdateTimeStep(times[2 * i]);
    vtkNew<vtkMolecule> mol;
    mol->ShallowCopy(reader->GetOutput());
    mappers[i]->SetInputData(mol.Get());

    // Rendering setup:
    mappers[i]->UseBallAndStickSettings();
    mappers[i]->SetAtomicRadiusTypeToCustomArrayRadius();
    mappers[i]->RenderLatticeOn();
    actors[i]->SetMapper(mappers[i].Get());
    rens[i]->SetBackground(0.0, 0.0, 0.0);
    rens[i]->AddActor(actors[i].Get());
    win->AddRenderer(rens[i].Get());
  }

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(win.GetPointer());

  win->SetSize(450,450);
  win->Render();

  for (size_t i = 0; i < 4; ++i)
  {
    rens[i]->GetActiveCamera()->Dolly(1.5);
    rens[i]->ResetCameraClippingRange();
  }
  win->Render();

  // Finally render the scene and compare the image to a reference image
  win->SetMultiSamples(0);
  win->GetInteractor()->Initialize();
  win->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
