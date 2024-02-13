// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <vtkGLTFImporter.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>

int TestGLTFImporter(int argc, char* argv[])
{
  if (argc < 3)
  {
    std::cout << "Usage: " << argv[0] << " <gltf file> <camera index>" << std::endl;
    return EXIT_FAILURE;
  }

  vtkIdType cameraIndex = atoi(argv[2]);

  vtkNew<vtkGLTFImporter> importer;
  importer->SetFileName(argv[1]);

  vtkNew<vtkRenderWindow> renderWindow;
  importer->SetRenderWindow(renderWindow);

  vtkNew<vtkRenderer> renderer;
  renderWindow->AddRenderer(renderer);
  renderer->SetBackground(.0, .0, .2);

  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow);

  importer->SetCamera(cameraIndex);
  importer->Update();
  auto hierarchy = importer->GetSceneHierarchy();
  if (hierarchy == nullptr || hierarchy->GetNumberOfChildren(0) == 0)
  {
    hierarchy->Print(std::cout);
    std::cerr << "ERROR: scene hierarchy cannot be null!\n";
    return 1;
  }
  renderWindow->Render();

  int retVal = vtkRegressionTestImage(renderWindow);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
  }
  return !retVal;
}
