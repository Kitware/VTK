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
#include "vtkDataSetSurfaceFilter.h"
#include "vtkInformation.h"
#include "vtkMolecule.h"
#include "vtkMoleculeMapper.h"
#include "vtkNew.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkVASPTessellationReader.h"

int TestVASPTessellationReader(int argc, char *argv[])
{
  if (argc < 2)
  {
    std::cerr << "Missing test file argument." << std::endl;
    return EXIT_FAILURE;
  }

  std::string fname(argv[1]);

  vtkNew<vtkVASPTessellationReader> reader;
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

  vtkNew<vtkDataSetSurfaceFilter> geomFilter;
  geomFilter->SetInputConnection(reader->GetOutputPort(1));

  // Show different time steps in each renderer:
  vtkNew<vtkRenderer> rens[4];
  rens[0]->SetViewport(0.0, 0.5, 0.5, 1.0);
  rens[1]->SetViewport(0.5, 0.5, 1.0, 1.0);
  rens[2]->SetViewport(0.0, 0.0, 0.5, 0.5);
  rens[3]->SetViewport(0.5, 0.0, 1.0, 0.5);

  vtkNew<vtkMoleculeMapper> molMappers[4];
  vtkNew<vtkActor> molActors[4];
  vtkNew<vtkPolyDataMapper> tessMappers[4];
  vtkNew<vtkActor> tessActors[4];
  vtkNew<vtkRenderWindow> win;
  for (size_t i = 0; i < 4; ++i)
  {
    // Render different timestamps for each:
    vtkNew<vtkMolecule> mol;
    reader->UpdateTimeStep(times[2 * i]);
    mol->ShallowCopy(reader->GetOutput(0));

    vtkNew<vtkPolyData> polyData;
    geomFilter->UpdateTimeStep(times[2 * i]);
    polyData->ShallowCopy(geomFilter->GetOutput(0));

    // Rendering setup:
    molMappers[i]->SetInputData(mol.Get());
    molMappers[i]->UseBallAndStickSettings();
    molMappers[i]->RenderLatticeOn();
    molActors[i]->SetMapper(molMappers[i].Get());
    rens[i]->AddActor(molActors[i].Get());

    tessMappers[i]->SetInputData(polyData.Get());
    tessMappers[i]->SelectColorArray("Atomic Numbers");
    tessMappers[i]->SetLookupTable(molMappers[i]->GetLookupTable());
    tessActors[i]->SetMapper(tessMappers[i].Get());
    tessActors[i]->GetProperty()->SetOpacity(0.5);
    rens[i]->AddActor(tessActors[i].Get());

    rens[i]->SetBackground(0.0, 0.0, 0.0);
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
