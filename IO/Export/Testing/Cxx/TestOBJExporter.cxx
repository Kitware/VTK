/*=========================================================================

Program:   Visualization Toolkit
Module:    TestOBJExporter.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkActor.h"
#include "vtkNew.h"
#include "vtkOBJExporter.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkSphereSource.h"
#include "vtkTestUtilities.h"

#include <cstdlib>

size_t fileSize(const std::string& filename);

int TestOBJExporter(int argc, char *argv[])
{
  char *tempDir = vtkTestUtilities::GetArgOrEnvOrDefault(
    "-T", argc, argv, "VTK_TEMP_DIR", "Testing/Temporary");
  if (!tempDir)
  {
    std::cout << "Could not determine temporary directory.\n";
    return EXIT_FAILURE;
  }
  std::string testDirectory = tempDir;
  delete[] tempDir;

  std::string filename = testDirectory
    + std::string("/") + std::string("Export");

  vtkNew<vtkSphereSource> sphere;
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(sphere->GetOutputPort());
  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper.Get());
  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor.Get());
  vtkNew<vtkRenderWindow> window;
  window->AddRenderer(renderer.Get());

  vtkNew<vtkOBJExporter> exporter;
  exporter->SetRenderWindow(window.Get());
  exporter->SetFilePrefix(filename.c_str());
  exporter->Write();

  filename += ".obj";

  size_t correctSize = fileSize(filename);
  if (correctSize == 0)
  {
    return EXIT_FAILURE;
  }

  actor->VisibilityOff();
  exporter->Write();
  size_t noDataSize = fileSize(filename);
  if (noDataSize == 0)
  {
    return EXIT_FAILURE;
  }

  if (noDataSize >= correctSize)
  {
    std::cerr << "Error: file should contain data for a visible actor"
      "and not for a hidden one." << std::endl;
    return EXIT_FAILURE;
  }

  actor->VisibilityOn();
  actor->SetMapper(NULL);
  exporter->Write();
  size_t size = fileSize(filename);
  if (size == 0)
  {
    return EXIT_FAILURE;
  }
  if (size > noDataSize)
  {
    std::cerr << "Error: file should not contain geometry"
      " (actor has no mapper)" << std::endl;
    return EXIT_FAILURE;
  }

  actor->SetMapper(mapper.Get());
  mapper->RemoveAllInputConnections(0);
  exporter->Write();
  size = fileSize(filename);
  if (size == 0)
  {
    return EXIT_FAILURE;
  }
  if (size > noDataSize)
  {
    std::cerr << "Error: file should not contain geometry"
      " (mapper has no input)" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

size_t fileSize(const std::string & filename)
{
  size_t size = 0;
  FILE* f = fopen(filename.c_str(), "r");
  if (f)
  {
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fclose(f);
  }
  else
  {
    std::cerr << "Error: cannot open file " << filename << std::endl;
  }

  return size;
}
