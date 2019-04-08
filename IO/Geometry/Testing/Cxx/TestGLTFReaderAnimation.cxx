/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGLTFReaderAnimation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

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

int TestGLTFReaderAnimation(int argc, char* argv[])
{
  if (argc <= 2)
  {
    std::cout << "Usage: " << argv[0] << "<step> <gltf file>" << std::endl;
    return EXIT_FAILURE;
  }

  const int step = std::atoi(argv[1]);
  vtkNew<vtkGLTFReader> reader;
  reader->SetFileName(argv[2]);
  reader->SetFrameRate(60);
  reader->UpdateInformation();
  vtkInformation* readerInfo = reader->GetOutputInformation(0);
  int nbSteps = readerInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  if (nbSteps < step)
  {
    std::cerr << "Invalid number of steps for input argument: " << step << std::endl;
    return EXIT_FAILURE;
  }

  reader->ApplyDeformationsToGeometryOn();
  for (unsigned int i = 0; i < reader->GetNumberOfAnimations(); i++)
  {
    reader->EnableAnimation(i);
  }
  readerInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), step);
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
