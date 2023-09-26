// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkAlembicExporter.h"
#include "vtkElevationFilter.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkSuperquadricSource.h"
#include "vtkTestUtilities.h"
#include <vtksys/SystemTools.hxx>

#include <cstdlib>

int TestAlembicExporter(int argc, char* argv[])
{
  char* tempDir =
    vtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "VTK_TEMP_DIR", "Testing/Temporary");
  if (!tempDir)
  {
    std::cout << "Could not determine temporary directory.\n";
    return EXIT_FAILURE;
  }
  std::string testDirectory = tempDir;
  delete[] tempDir;

  std::string rootname = testDirectory + "/Export";

  vtkNew<vtkSphereSource> sphere;
  vtkNew<vtkSuperquadricSource> torus;
  torus->ToroidalOn();
  vtkNew<vtkElevationFilter> elev;
  elev->SetInputConnection(sphere->GetOutputPort());
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(elev->GetOutputPort());
  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  torus->SetCenter(1., 2., 0.);
  vtkNew<vtkPolyDataMapper> mapper2;
  mapper2->SetInputConnection(torus->GetOutputPort());
  vtkNew<vtkActor> actor2;
  actor2->SetMapper(mapper2);

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor);
  renderer->AddActor(actor2);
  renderer->ResetCamera();
  vtkNew<vtkRenderWindow> window;
  window->AddRenderer(renderer);
  window->Render();

  std::string filename = rootname + "_full.abc";

  vtkNew<vtkAlembicExporter> exporter;
  exporter->SetRenderWindow(window);
  exporter->SetFileName(filename.c_str());
  exporter->Write();

  auto correctSize = vtksys::SystemTools::FileLength(filename);
  if (correctSize == 0)
  {
    return EXIT_FAILURE;
  }

  actor->VisibilityOff();
  actor2->VisibilityOff();
  filename = rootname + "_empty.abc";
  exporter->SetFileName(filename.c_str());
  exporter->Write();
  auto noDataSize = vtksys::SystemTools::FileLength(filename);
  if (noDataSize == 0)
  {
    return EXIT_FAILURE;
  }

  if (noDataSize >= correctSize)
  {
    std::cerr << "Error: file should contain data for a visible actor"
                 "and not for a hidden one."
              << std::endl;
    return EXIT_FAILURE;
  }

  actor->VisibilityOn();
  actor->SetMapper(nullptr);
  exporter->Write();
  auto size = vtksys::SystemTools::FileLength(filename);
  if (size == 0)
  {
    return EXIT_FAILURE;
  }
  if (size > noDataSize)
  {
    std::cerr << "Error: file should not contain geometry"
                 " (actor has no mapper)"
              << std::endl;
    return EXIT_FAILURE;
  }

  actor->SetMapper(mapper);
  mapper->RemoveAllInputConnections(0);
  exporter->Write();
  size = vtksys::SystemTools::FileLength(filename);
  if (size == 0)
  {
    return EXIT_FAILURE;
  }
  if (size > noDataSize)
  {
    std::cerr << "Error: file should not contain geometry"
                 " (mapper has no input)"
              << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
