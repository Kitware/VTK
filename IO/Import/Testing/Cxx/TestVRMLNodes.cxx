// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen, Ash Carter
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCamera.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkStringFormatter.h"
#include "vtkStringScanner.h"
#include "vtkVRMLImporter.h"

#include "vtkRegressionTestImage.h"

#include <iostream>

int TestVRMLNodes(int argc, char* argv[])
{
  if (argc < 14 || argc > 15)
  {
    vtk::print(stderr,
      "Usage: {:s} <file> <camera_x> <camera_y> <camera_z> <focus_x> <focus_y> <focus_z> "
      "<projection:optional>\n",
      argv[0]);
    return EXIT_FAILURE;
  }

  // Parse input
  const char* filename = argv[1];
  double cameraPos[3] = { 0, 0, 0 };
  double focusPos[3] = { 0, 0, 0 };
  double cameraProjection = 0.0;

  VTK_FROM_CHARS_IF_ERROR_RETURN(argv[2], cameraPos[0], EXIT_FAILURE);
  VTK_FROM_CHARS_IF_ERROR_RETURN(argv[3], cameraPos[1], EXIT_FAILURE);
  VTK_FROM_CHARS_IF_ERROR_RETURN(argv[4], cameraPos[2], EXIT_FAILURE);

  VTK_FROM_CHARS_IF_ERROR_RETURN(argv[5], focusPos[0], EXIT_FAILURE);
  VTK_FROM_CHARS_IF_ERROR_RETURN(argv[6], focusPos[1], EXIT_FAILURE);
  VTK_FROM_CHARS_IF_ERROR_RETURN(argv[7], focusPos[2], EXIT_FAILURE);

  if (argc == 15)
  {
    VTK_FROM_CHARS_IF_ERROR_RETURN(argv[8], cameraProjection, EXIT_FAILURE);
  }

  // Now create the RenderWindow, Renderer and Interactor
  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkRenderWindow> renderWindow;
  vtkNew<vtkRenderWindowInteractor> iRenderWindow;
  vtkNew<vtkVRMLImporter> importer;

  renderWindow->AddRenderer(renderer);
  iRenderWindow->SetRenderWindow(renderWindow);
  importer->SetRenderWindow(renderWindow);
  importer->SetFileName(filename);

  if (!importer->Update())
  {
    std::cerr << "ERROR: Importer failed to update\n";
    return EXIT_FAILURE;
  }

  // Configure render
  renderWindow->SetSize(400, 400);
  renderer->SetBackground(1.0, 1.0, 1.0);
  auto camera = renderer->GetActiveCamera();
  camera->SetPosition(cameraPos);
  camera->SetFocalPoint(focusPos);

  if (argc == 15)
  {
    camera->ParallelProjectionOn();
    camera->SetParallelScale(cameraProjection);
  }

  // render the image
  iRenderWindow->Initialize();

  // This starts the event loop and as a side effect causes an initial render.
  int retVal = vtkRegressionTestImage(renderWindow);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iRenderWindow->Start();
  }

  return !retVal;
}
