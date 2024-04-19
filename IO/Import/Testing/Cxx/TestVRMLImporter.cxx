// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkVRMLImporter.h"

#include "vtkTestUtilities.h"

// This is testing a bug in vtkVRMLImporter where the importer
// would delete static data and any future importer would fail
// The test is defined to pass if it doesn't segfault.
int TestVRMLImporter(int argc, char* argv[])
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
  importer->Read();
  // delete the importer and see if it can be run again
  importer->Delete();

  importer = vtkVRMLImporter::New();
  importer->SetRenderWindow(renWin);
  importer->SetFileName(fname);
  importer->Read();
  importer->Delete();

  delete[] fname;

  iren->Delete();
  renWin->Delete();
  ren1->Delete();

  return 0;
}
