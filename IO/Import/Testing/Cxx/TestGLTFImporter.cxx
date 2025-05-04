// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <vtkFileResourceStream.h>
#include <vtkGLTFImporter.h>
#include <vtkLightCollection.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>

int TestGLTFImporter(int argc, char* argv[])
{
  if (argc < 8)
  {
    std::cout << "Usage: " << argv[0]
              << " <gltf file> <use_stream> <camera index> <expected nb of actors> <expected nb of "
                 "lights> <expected nb of animations>"
                 "<expected nb of cameras>"
              << std::endl;
    return EXIT_FAILURE;
  }

  vtkIdType cameraIndex = atoi(argv[3]);

  vtkNew<vtkGLTFImporter> importer;
  if (atoi(argv[2]) > 0)
  {
    bool is_binary = false;
    std::string extension = vtksys::SystemTools::GetFilenameLastExtension(argv[1]);
    if (extension == ".glb")
    {
      is_binary = true;
    }
    vtkNew<vtkFileResourceStream> file;
    file->Open(argv[1]);
    if (file->EndOfStream())
    {
      std::cerr << "Can not open test file " << argv[1] << std::endl;
      return EXIT_FAILURE;
    }
    importer->SetStream(file);
    importer->SetStreamIsBinary(is_binary);
  }
  else
  {
    importer->SetFileName(argv[1]);
  }

  importer->ImportArmatureOn();

  vtkNew<vtkRenderWindow> renderWindow;
  importer->SetRenderWindow(renderWindow);

  vtkNew<vtkRenderer> renderer;
  renderWindow->AddRenderer(renderer);
  renderer->SetBackground(.0, .0, .2);

  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow);

  importer->SetCamera(cameraIndex);
  if (!importer->Update())
  {
    std::cerr << "ERROR: Importer failed to update\n";
    return EXIT_FAILURE;
  }

  if (importer->GetImportedActors()->GetNumberOfItems() != atoi(argv[4]))
  {
    std::cerr << "ERROR: Unexpected number of imported actors: "
              << importer->GetImportedActors()->GetNumberOfItems() << "\n";
    return EXIT_FAILURE;
  }
  if (importer->GetImportedLights()->GetNumberOfItems() != atoi(argv[5]))
  {
    std::cerr << "ERROR: Unexpected number of imported lights: "
              << importer->GetImportedActors()->GetNumberOfItems() << "\n";
    return EXIT_FAILURE;
  }
  if (importer->GetImportedCameras()->GetNumberOfItems() != atoi(argv[6]))
  {
    std::cerr << "ERROR: Unexpected number of imported cameras: "
              << importer->GetImportedCameras()->GetNumberOfItems() << "\n";
    return EXIT_FAILURE;
  }
  if (importer->GetAnimationSupportLevel() != vtkImporter::AnimationSupportLevel::MULTI)
  {
    std::cerr << "ERROR: Unexpected animation level support"
              << "\n";
    return EXIT_FAILURE;
  }
  if (importer->GetNumberOfAnimations() != atoi(argv[7]))
  {
    std::cerr << "ERROR: Unexpected number of imported animations: "
              << importer->GetNumberOfAnimations() << "\n";
    return EXIT_FAILURE;
  }

  std::cout << importer->GetImportedActors()->GetNumberOfItems() << std::endl;
  std::cout << importer->GetImportedLights()->GetNumberOfItems() << std::endl;
  std::cout << importer->GetImportedCameras()->GetNumberOfItems() << std::endl;
  std::cout << importer->GetNumberOfAnimations() << std::endl;

  auto hierarchy = importer->GetSceneHierarchy();
  if (hierarchy == nullptr || hierarchy->GetNumberOfChildren(0) == 0)
  {
    hierarchy->Print(std::cout);
    std::cerr << "ERROR: scene hierarchy cannot be null!\n";
    return EXIT_FAILURE;
  }
  renderWindow->Render();

  int retVal = vtkRegressionTestImage(renderWindow);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
  }
  return !retVal;
}
