// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkNew.h"
#include "vtkOBJImporter.h"

#include "vtkCamera.h"
#include "vtkLightCollection.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

#include "vtksys/SystemTools.hxx"

#include "vtkMapper.h"
#include <sstream>

int TestOBJImporter(int argc, char* argv[])
{
  // note that the executable name is stripped out already
  // so argc argv will not have it

  // Files for testing demonstrate updated functionality for OBJ import:
  //       polydata + textures + actor properties all get loaded.
  if (argc < 2)
  {
    std::cout << "expected TestName File1.obj [File2.obj.mtl]  [texture1]  ... " << std::endl;
    return EXIT_FAILURE;
  }

  std::string filenameOBJ(argv[1]);

  std::string filenameMTL, texfile1;

  if (argc >= 3 && strcmp(argv[2], "-D") != 0)
  {
    filenameMTL = argv[2];

    if (argc >= 4 && strcmp(argv[3], "-D") != 0)
    {
      texfile1 = argv[3];
      texfile1 = vtksys::SystemTools::GetFilenamePath(texfile1);
    }
  }

  vtkNew<vtkOBJImporter> importer;
  importer->SetFileName(filenameOBJ.data());

  if (!filenameMTL.empty())
  {
    importer->SetFileNameMTL(filenameMTL.data());
  }

  if (!texfile1.empty())
  {
    importer->SetTexturePath(texfile1.data());
  }

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

  ren->GetActiveCamera()->SetPosition(10, 10, -10);
  ren->ResetCamera();
  renWin->SetSize(800, 600);
  iren->Start();

  return (EXIT_SUCCESS);
}
