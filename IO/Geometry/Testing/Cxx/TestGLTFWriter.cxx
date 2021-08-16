/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCityGMLReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME Test of an RGBA texture on a vtkActor.
// .SECTION Description
// this program tests the CityGML Reader and setting of textures to
// individual datasets of the multiblock tree.

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCityGMLReader.h"
#include "vtkGLTFImporter.h"
#include "vtkGLTFWriter.h"

#include "vtkCompositeDataIterator.h"
#include "vtkFieldData.h"
#include "vtkJPEGReader.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkStringArray.h"
#include "vtkTestUtilities.h"
#include "vtkTexture.h"
#include "vtksys/SystemTools.hxx"

int TestGLTFWriter(int argc, char* argv[])
{
  char* fname =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/CityGML/Part-4-Buildings-V4-one.gml");
  std::string fileName(fname);
  delete[] fname;
  std::string filePath = vtksys::SystemTools::GetFilenamePath(fileName);

  std::cout << fileName << std::endl;
  std::cout << filePath << std::endl;
  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(0.5, 0.7, 0.7);

  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(renWin);

  vtkNew<vtkCityGMLReader> reader;
  reader->SetFileName(fileName.c_str());

  char* tname =
    vtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "VTK_TEMP_DIR", "Testing/Temporary");
  std::string tmpDir(tname);
  delete[] tname;
  std::string filename = tmpDir + "/TestGLTFWriter.gltf";

  vtkNew<vtkGLTFWriter> writer;
  writer->SetFileName(filename.c_str());
  writer->SetTextureBaseDirectory((filePath).c_str());
  writer->SetInputConnection(reader->GetOutputPort());
  writer->Write();

  vtkNew<vtkGLTFImporter> importer;
  importer->SetFileName("test.gltf");
  importer->SetCamera(-1);
  importer->SetRenderWindow(renWin);
  importer->Update();

  renderer->ResetCamera();
  renderer->GetActiveCamera()->Azimuth(90);
  renderer->GetActiveCamera()->Roll(-90);
  renderer->GetActiveCamera()->Zoom(1.5);

  renWin->SetSize(400, 400);
  renWin->Render();
  interactor->Initialize();
  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    interactor->Start();
  }

  return !retVal;
}
