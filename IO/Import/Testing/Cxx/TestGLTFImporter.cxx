// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <vtkDoubleArray.h>
#include <vtkFileResourceStream.h>
#include <vtkGLTFImporter.h>
#include <vtkLightCollection.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkStringScanner.h>

#include <iostream>

int TestGLTFImporter(int argc, char* argv[])
{
  if (argc < 8)
  {
    std::cout << "Usage: " << argv[0]
              << " <gltf file> <use_stream> <camera index> <expected nb of actors> <expected nb of "
                 "lights> <expected nb of cameras> <expected nb of animations>"
                 "<expected nb of timesteps in first animation>"
                 "<time value to load>"
              << std::endl;
    return EXIT_FAILURE;
  }

  vtkIdType cameraIndex;
  VTK_FROM_CHARS_IF_ERROR_RETURN(argv[3], cameraIndex, EXIT_FAILURE);

  vtkNew<vtkGLTFImporter> importer;
  int useStream;
  VTK_FROM_CHARS_IF_ERROR_RETURN(argv[2], useStream, EXIT_FAILURE);
  if (useStream > 0)
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

  int numberOfImportedActors;
  VTK_FROM_CHARS_IF_ERROR_RETURN(argv[4], numberOfImportedActors, EXIT_FAILURE);
  if (importer->GetImportedActors()->GetNumberOfItems() != numberOfImportedActors)
  {
    std::cerr << "ERROR: Unexpected number of imported actors: "
              << importer->GetImportedActors()->GetNumberOfItems() << "\n";
    return EXIT_FAILURE;
  }
  int numberOfImportedLights;
  VTK_FROM_CHARS_IF_ERROR_RETURN(argv[5], numberOfImportedLights, EXIT_FAILURE);
  if (importer->GetImportedLights()->GetNumberOfItems() != numberOfImportedLights)
  {
    std::cerr << "ERROR: Unexpected number of imported lights: "
              << importer->GetImportedActors()->GetNumberOfItems() << "\n";
    return EXIT_FAILURE;
  }
  int numberOfImportedCameras;
  VTK_FROM_CHARS_IF_ERROR_RETURN(argv[6], numberOfImportedCameras, EXIT_FAILURE);
  if (importer->GetImportedCameras()->GetNumberOfItems() != numberOfImportedCameras)
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
  int numberOfAnimations;
  VTK_FROM_CHARS_IF_ERROR_RETURN(argv[7], numberOfAnimations, EXIT_FAILURE);
  if (importer->GetNumberOfAnimations() != numberOfAnimations)
  {
    std::cerr << "ERROR: Unexpected number of imported animations: "
              << importer->GetNumberOfAnimations() << "\n";
    return EXIT_FAILURE;
  }

  if (numberOfAnimations > 0)
  {
    int expectedNumberOfTimeSteps;
    VTK_FROM_CHARS_IF_ERROR_RETURN(argv[8], expectedNumberOfTimeSteps, EXIT_FAILURE);

    double timeRange[2];
    int nbTimeSteps;
    vtkNew<vtkDoubleArray> timeSteps;
    if (!importer->GetTemporalInformation(0, timeRange, nbTimeSteps, timeSteps))
    {
      std::cerr << "ERROR: Unexpected GetTemporalInformation failure\n";
      return EXIT_FAILURE;
    }
    if (nbTimeSteps != expectedNumberOfTimeSteps)
    {
      std::cerr << "ERROR: Unexpected number of time steps: " << nbTimeSteps << "\n";
      return EXIT_FAILURE;
    }

    double timeValue;
    VTK_FROM_CHARS_IF_ERROR_RETURN(argv[9], timeValue, EXIT_FAILURE);

    importer->EnableAnimation(0);
    if (!importer->UpdateAtTimeValue(1))
    {
      std::cerr << "ERROR: Unexpected UpdateAtTimeValue failure\n";
      return EXIT_FAILURE;
    }
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
