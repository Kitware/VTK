// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include <vtkActor.h>
#include <vtkNew.h>
#include <vtkPTSReader.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>

#include <iostream>

int TestPTSReader(int argc, char* argv[])
{
  if (argc < 2)
  {
    std::cerr << "Required parameters: <filename> maxNumberOfPoints(optional)" << std::endl;
    return EXIT_FAILURE;
  }

  std::string inputFilename = argv[1];

  if (!vtkPTSReader::CanReadFile(inputFilename.c_str()))
  {
    std::cerr << "Unexpected CanReadFile result" << std::endl;
    return EXIT_FAILURE;
  }

  vtkNew<vtkPTSReader> reader;
  reader->SetFileName(inputFilename.c_str());
  reader->SetLimitToMaxNumberOfPoints(true);
  reader->SetMaxNumberOfPoints(100000);

  reader->Update();
  if (reader->GetOutput()->GetNumberOfPoints() != 446)
  {
    std::cerr << "Unexpected number of points" << std::endl;
    return EXIT_FAILURE;
  }

  // Visualize
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(reader->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer);
  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow);

  renderer->AddActor(actor);
  renderer->SetBackground(.3, .6, .3); // Background color green

  renderWindow->Render();

  int retVal = vtkRegressionTestImage(renderWindow);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
  }

  return !retVal;
}
