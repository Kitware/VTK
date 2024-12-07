// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkVRMLImporter.h"

#include "vtkRegressionTestImage.h"
#include "vtkTestUtilities.h"

int TestVRMLNormals(int argc, char* argv[])
{
  // Now create the RenderWindow, Renderer and Interactor
  vtkRenderer* ren1 = vtkRenderer::New();
  vtkRenderWindow* renWin = vtkRenderWindow::New();
  renWin->AddRenderer(ren1);

  vtkRenderWindowInteractor* iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  vtkVRMLImporter* importer = vtkVRMLImporter::New();
  importer->SetRenderWindow(renWin);

  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/WineGlass.wrl");
  importer->SetFileName(fname);
  if (!importer->Update())
  {
    std::cerr << "ERROR: Importer failed to update\n";
    return EXIT_FAILURE;
  }

  delete[] fname;

  renWin->SetSize(400, 400);

  // render the image
  iren->Initialize();

  // This starts the event loop and as a side effect causes an initial render.
  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  // Delete everything
  importer->Delete();
  ren1->Delete();
  renWin->Delete();
  iren->Delete();

  return !retVal;
}
