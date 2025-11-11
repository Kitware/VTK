// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <vtkCamera.h>
#include <vtkFileResourceStream.h>
#include <vtkGLTFImporter.h>
#include <vtkLightCollection.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkStringScanner.h>

int TestGLTFImporterCameraMove(int argc, char* argv[])
{

  if (argc < 3)
  {
    std::cout << "Usage: " << argv[0] << " <gltf file> <camera index> <azimuth> <elevation>\n";
    return EXIT_FAILURE;
  }

  vtkIdType cameraIndex;
  VTK_FROM_CHARS_IF_ERROR_RETURN(argv[2], cameraIndex, EXIT_FAILURE);

  double azimuth;
  VTK_FROM_CHARS_IF_ERROR_RETURN(argv[3], azimuth, EXIT_FAILURE);

  double elevation;
  VTK_FROM_CHARS_IF_ERROR_RETURN(argv[4], elevation, EXIT_FAILURE);

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
  if (!importer->Update())
  {
    std::cerr << "ERROR: Importer failed to update\n";
    return EXIT_FAILURE;
  }

  renderer->GetActiveCamera()->Azimuth(azimuth);
  renderer->GetActiveCamera()->Elevation(elevation);

  renderWindow->Render();

  int retVal = vtkRegressionTestImage(renderWindow);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
  }
  return !retVal;
}
