/*=========================================================================

Program:   Visualization Toolkit

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkActor.h"
#include "vtkLight.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkTestUtilities.h"
#include "vtkWindowNode.h"
#include "vtksys/SystemTools.hxx"

#include "vtkArchiver.h"
#include "vtkJSONRenderWindowExporter.h"

int TestJSONRenderWindowExporter(int argc, char* argv[])
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

  std::string filename = testDirectory + std::string("/") + std::string("ExportVtkJS");

  vtkNew<vtkSphereSource> sphere;
  vtkNew<vtkPolyDataMapper> pmap;
  pmap->SetInputConnection(sphere->GetOutputPort());

  vtkNew<vtkRenderWindow> rwin;

  vtkNew<vtkRenderer> ren;
  rwin->AddRenderer(ren);

  vtkNew<vtkLight> light;
  ren->AddLight(light);

  vtkNew<vtkActor> actor;
  ren->AddActor(actor);

  actor->SetMapper(pmap);

  vtkNew<vtkJSONRenderWindowExporter> exporter;
  exporter->GetArchiver()->SetArchiveName(filename.c_str());
  exporter->SetRenderWindow(rwin);
  exporter->Write();

  vtksys::SystemTools::RemoveADirectory(filename.c_str());

  return EXIT_SUCCESS;
}
