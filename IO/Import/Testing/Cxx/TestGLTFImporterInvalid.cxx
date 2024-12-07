// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkGLTFImporter.h"
#include "vtkNew.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

//------------------------------------------------------------------------------
int TestGLTFImporterInvalid(int argc, char* argv[])
{
  if (argc < 2)
  {
    std::cout << "expected testname file1.gltf" << std::endl;
    return EXIT_FAILURE;
  }

  // Just not segfaulting is considered a success
  vtkNew<vtkGLTFImporter> importer;
  importer->SetFileName(argv[1]);

  vtkNew<vtkRenderWindow> renderWindow;
  importer->SetRenderWindow(renderWindow);

  vtkNew<vtkRenderer> renderer;
  renderWindow->AddRenderer(renderer);
  renderer->SetBackground(.0, .0, .2);

  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow);

  importer->Update();
  return EXIT_SUCCESS;
}
