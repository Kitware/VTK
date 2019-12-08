/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSingleVTPExporter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkNew.h"
#include "vtkOBJImporter.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSingleVTPExporter.h"
#include "vtksys/SystemTools.hxx"

#include <sstream>

int main(int argc, char* argv[])
{
  if (argc < 2)
  {
    std::cerr << "expected objtovtk File1.obj [File2.obj.mtl]" << std::endl;
    return -1;
  }

  std::string filenameOBJ(argv[1]);

  std::string filenameMTL;

  if (argc >= 3)
  {
    filenameMTL = argv[2];
  }
  std::string texturePath = vtksys::SystemTools::GetFilenamePath(filenameOBJ);

  vtkNew<vtkOBJImporter> importer;

  importer->SetFileName(filenameOBJ.data());
  importer->SetFileNameMTL(filenameMTL.data());
  importer->SetTexturePath(texturePath.data());

  vtkNew<vtkRenderer> ren;
  vtkNew<vtkRenderWindow> renWin;
  vtkNew<vtkRenderWindowInteractor> iren;

  renWin->AddRenderer(ren);
  iren->SetRenderWindow(renWin);
  importer->SetRenderWindow(renWin);
  importer->Update();

  renWin->SetSize(800, 600);
  ren->SetBackground(0.4, 0.5, 0.6);
  ren->ResetCamera();
  renWin->Render();

  // export to a single vtk file
  vtkNew<vtkSingleVTPExporter> exporter;

  std::string outputPrefix = "o2v";
  outputPrefix += vtksys::SystemTools::GetFilenameWithoutLastExtension(filenameOBJ);

  exporter->SetFilePrefix(outputPrefix.c_str());
  exporter->SetRenderWindow(renWin);
  exporter->Write();

  iren->Start();

  return 0;
}
