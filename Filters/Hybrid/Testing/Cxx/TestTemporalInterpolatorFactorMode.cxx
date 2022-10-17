/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTemporalInterpolator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <vtkCamera.h>
#include <vtkCompositeDataGeometryFilter.h>
#include <vtkIOSSReader.h>
#include <vtkInformation.h>
#include <vtkNew.h>
#include <vtkPolyDataMapper.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkTemporalInterpolator.h>
#include <vtkTestUtilities.h>

int TestTemporalInterpolatorFactorMode(int argc, char* argv[])
{
  vtkNew<vtkIOSSReader> reader;
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/can.ex2");
  reader->SetFileName(fname);
  delete[] fname;

  vtkNew<vtkTemporalInterpolator> interpolator;
  interpolator->SetResampleFactor(2);
  interpolator->SetInputConnection(reader->GetOutputPort());

  interpolator->UpdateInformation();
  vtkInformation* outInfo = interpolator->GetOutputInformation(0);
  int numTimes = outInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  if (numTimes != 87)
  {
    std::cerr << "Unexpected number of timesteps provided by vtkTemporalInterpolator." << std::endl;
    std::cerr << numTimes << " instead of 87." << std::endl;
    return EXIT_FAILURE;
  }

  vtkNew<vtkCompositeDataGeometryFilter> geom;
  geom->SetInputConnection(interpolator->GetOutputPort());

  geom->UpdateTimeStep(0.001);
  vtkDataObject* dataObj = geom->GetOutputDataObject(0);
  double time = dataObj->GetInformation()->Get(vtkDataObject::DATA_TIME_STEP());
  if (time != 0.001)
  {
    std::cerr << "Unexpected time in data, expecting 0.001, got " << time << "." << std::endl;
    return EXIT_FAILURE;
  }

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputDataObject(dataObj);

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor);

  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  // Render and test
  renWin->Render();
  renderer->GetActiveCamera()->Elevation(90);

  int retVal = vtkRegressionTestImageThreshold(renWin, 10);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
