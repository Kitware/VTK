// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkCompositePolyDataMapper.h>
#include <vtkGLTFReader.h>
#include <vtkInformation.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkStreamingDemandDrivenPipeline.h>

int TestGLTFReaderAnimationRange(int argc, char* argv[])
{
  if (argc <= 2)
  {
    std::cout << "Usage: " << argv[0] << "<timevalue> <gltf file>" << std::endl;
    return EXIT_FAILURE;
  }

  const int timevalue = std::atof(argv[1]);
  vtkNew<vtkGLTFReader> reader;
  reader->SetFileName(argv[2]);
  reader->SetFrameRate(0);
  reader->ApplyDeformationsToGeometryOn();

  reader->UpdateInformation(); // Read model metadata to get the number of animations
  for (vtkIdType i = 0; i < reader->GetNumberOfAnimations(); i++)
  {
    reader->EnableAnimation(i);
  }

  reader->UpdateInformation(); // Update number of time steps now that animations are enabled
  vtkInformation* readerInfo = reader->GetOutputInformation(0);

  if (!readerInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_RANGE()))
  {
    std::cerr << "Expecting TIME_RANGE to be present" << std::endl;
    return EXIT_FAILURE;
  }

  double* readerTimeRange = readerInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_RANGE());
  if (readerTimeRange[0] > timevalue || timevalue > readerTimeRange[1])
  {
    std::cerr << "Invalid timevalue input argument: " << timevalue << std::endl;
    return EXIT_FAILURE;
  }

  readerInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), timevalue);
  reader->Update();

  vtkNew<vtkCompositePolyDataMapper> mapper;
  mapper->SetInputConnection(reader->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor);
  renderer->SetBackground(.0, .0, .2);

  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow);

  renderWindow->Render();
  renderer->GetActiveCamera()->Azimuth(30);
  renderer->GetActiveCamera()->Elevation(30);
  renderer->GetActiveCamera()->SetClippingRange(0.1, 1000);
  int retVal = vtkRegressionTestImage(renderWindow);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
  }
  return !retVal;
}
