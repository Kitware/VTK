// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <vtkGLTFImporter.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>

#include <iostream>

int TestGLTFImporterHierarchy(int argc, char* argv[])
{
  if (argc < 2)
  {
    std::cout << "Usage: " << argv[0] << " <gltf file>\n";
    return EXIT_FAILURE;
  }

  vtkNew<vtkGLTFImporter> importer;
  importer->SetFileName(argv[1]);

  vtkNew<vtkRenderWindow> renderWindow;
  importer->SetRenderWindow(renderWindow);

  vtkNew<vtkRenderer> renderer;
  renderWindow->AddRenderer(renderer);
  renderer->SetBackground(1, 1, 1);

  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow);

  if (!importer->Update())
  {
    std::cerr << "ERROR: Importer failed to update\n";
    return EXIT_FAILURE;
  }

  auto hierarchy = importer->GetSceneHierarchy();
  if (hierarchy == nullptr || hierarchy->GetNumberOfChildren(0) == 0)
  {
    std::cerr << "ERROR: scene hierarchy cannot be null!\n";
    return 1;
  }

  std::ifstream expectedSceneFile(argv[2]);
  if (!expectedSceneFile.is_open())
  {
    std::cerr << "ERROR: cannot open expected scene file: " << argv[2] << "\n";
    return 1;
  }

  std::string expectedScene;
  expectedScene.assign(
    (std::istreambuf_iterator<char>(expectedSceneFile)), std::istreambuf_iterator<char>());

  const std::string serializedXML = hierarchy->SerializeToXML(vtkIndent(2));
  if (serializedXML != expectedScene)
  {
    std::cerr << "ERROR: generated scene hierarchy doesn't match expected result!\n";
    std::cerr << "Generated: \n"
              << serializedXML << "---------\n"
              << "Expected: \n"
              << expectedScene << "\n---------\n";
    return 1;
  }

  int retVal = vtkRegressionTestImage(renderWindow);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
  }
  return !retVal;
}
