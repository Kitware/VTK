// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkGLTFImporter.h"
#include "vtkNew.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestErrorObserver.h"

#include <iostream>

// Tests that a glTF file with an image of unrecognized type loads successfully.
// The importer should emit a warning but continue loading the file.
int TestGLTFImporterUnknownImageType(int argc, char* argv[])
{
  if (argc < 2)
  {
    std::cout << "Usage: " << argv[0] << " <gltf file>" << std::endl;
    return EXIT_FAILURE;
  }

  vtkNew<vtkGLTFImporter> importer;
  importer->SetFileName(argv[1]);

  vtkNew<vtkRenderWindow> renderWindow;
  importer->SetRenderWindow(renderWindow);

  vtkNew<vtkRenderer> renderer;
  renderWindow->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(renderWindow);

  // The importer forwards vtkGLTFDocumentLoader warnings via vtkEventForwarderCommand,
  // so observing the importer is sufficient to capture loader warnings.
  vtkNew<vtkTest::ErrorObserver> warningObserver;
  importer->AddObserver(vtkCommand::WarningEvent, warningObserver);

  if (!importer->Update())
  {
    std::cerr << "ERROR: Importer failed to update - should succeed even with unknown image type\n";
    return EXIT_FAILURE;
  }

  if (!warningObserver->HasWarningMessage("No reader found for image with mime type"))
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
