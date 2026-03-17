// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtk3DSImporter.h"
#include "vtkNew.h"

#include "vtkCamera.h"
#include "vtkFileResourceStream.h"
#include "vtkLightCollection.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkStringScanner.h"

#include "vtksys/SystemTools.hxx"

#include "vtkMapper.h"
#include <sstream>

#include <iostream>

int Test3DSImporterStream(int argc, char* argv[])
{
  if (argc < 3)
  {
    std::cout << "expected TestName File1.3ds <hierarchy_node_count>" << std::endl;
    return EXIT_FAILURE;
  }

  vtkNew<vtkFileResourceStream> fileStream;
  fileStream->Open(argv[1]);

  if (!vtk3DSImporter::CanReadFile(fileStream))
  {
    std::cout << "CanReadFile(stream) unexpected failure" << std::endl;
    return EXIT_FAILURE;
  }

  vtkNew<vtk3DSImporter> importer;
  importer->SetStream(fileStream);

  vtkNew<vtkRenderer> ren;
  vtkNew<vtkRenderWindow> renWin;
  vtkNew<vtkRenderWindowInteractor> iren;

  renWin->AddRenderer(ren);
  iren->SetRenderWindow(renWin);
  importer->SetRenderWindow(renWin);
  if (!importer->Update())
  {
    std::cerr << "ERROR: Importer failed to update\n";
    return EXIT_FAILURE;
  }

  ren->ResetCamera();

  if (ren->GetActors()->GetNumberOfItems() < 1 ||
    importer->GetImportedActors()->GetNumberOfItems() < 1)
  {
    std::cout << "failed to get an actor created?!" << std::endl;
    return EXIT_FAILURE;
  }

  vtkDataAssembly* sceneHierarchy = importer->GetSceneHierarchy();

  if (sceneHierarchy->GetNumberOfChildren(vtkDataAssembly::GetRootNode()) !=
    vtk::scan_int<int>(argv[2])->value())
  {
    std::cout << "expected " << argv[2] << " scene hierarchy nodes" << std::endl;
    return EXIT_FAILURE;
  }

  ren->GetActiveCamera()->SetPosition(10, 10, -10);
  ren->ResetCamera();
  renWin->SetSize(800, 600);
  iren->Start();

  return (EXIT_SUCCESS);
}
