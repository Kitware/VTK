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
#include "vtkLogger.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkOBJReader.h"
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

#include <sstream>

//------------------------------------------------------------------------------
void SetField(vtkDataObject* obj, const char* name, const char* value)
{
  vtkFieldData* fd = obj->GetFieldData();
  if (!fd)
  {
    vtkNew<vtkFieldData> newfd;
    obj->SetFieldData(newfd);
    fd = newfd;
  }
  vtkNew<vtkStringArray> sa;
  sa->SetNumberOfTuples(1);
  sa->SetValue(0, value);
  sa->SetName(name);
  fd->AddArray(sa);
}

//------------------------------------------------------------------------------
std::array<double, 3> ReadOBJOffset(const char* comment)
{
  std::array<double, 3> translation = { 0, 0, 0 };
  if (comment)
  {
    std::istringstream istr(comment);
    std::array<std::string, 3> axesNames = { "x", "y", "z" };
    for (int i = 0; i < 3; ++i)
    {
      std::string axis;
      std::string s;
      istr >> axis >> s >> translation[i];
      if (istr.fail())
      {
        vtkLog(WARNING, "Cannot read axis " << axesNames[i] << " from comment.");
      }
      if (axis != axesNames[i])
      {
        vtkLog(WARNING, "Invalid axis " << axesNames[i] << ": " << axis);
      }
    }
  }
  else
  {
    vtkLog(WARNING, "nullptr comment.");
  }
  return translation;
}

//------------------------------------------------------------------------------
std::string GetOBJTextureFileName(const std::string& file)
{
  std::string fileNoExt = vtksys::SystemTools::GetFilenameWithoutExtension(file);
  return fileNoExt + ".png";
}

vtkSmartPointer<vtkMultiBlockDataSet> ReadOBJFiles(int numberOfBuildings, int vtkNotUsed(lod),
  const std::vector<std::string>& files, std::array<double, 3>& fileOffset)
{
  auto root = vtkSmartPointer<vtkMultiBlockDataSet>::New();
  for (size_t i = 0; i < files.size() && i < static_cast<size_t>(numberOfBuildings); ++i)
  {
    vtkNew<vtkOBJReader> reader;
    reader->SetFileName(files[i].c_str());
    reader->Update();
    if (i == 0)
    {
      fileOffset = ReadOBJOffset(reader->GetComment());
    }
    auto polyData = reader->GetOutput();
    std::string textureFileName = GetOBJTextureFileName(files[i]);
    SetField(polyData, "texture_uri", textureFileName.c_str());
    auto building = vtkSmartPointer<vtkMultiBlockDataSet>::New();
    building->SetBlock(0, polyData);
    root->SetBlock(root->GetNumberOfBlocks(), building);
  }
  return root;
}

int TestGLTFWriter(int argc, char* argv[])
{
  std::string fileName = argv[1];
  std::string filePath = vtksys::SystemTools::GetFilenamePath(fileName);
  std::string fileExt = vtksys::SystemTools::GetFilenameExtension(fileName);
  std::array<double, 3> fileOffset;

  std::cout << fileName << std::endl;
  std::cout << filePath << std::endl;
  bool cityGML = false;
  if (fileExt == ".gml")
  {
    cityGML = true;
  }
  else if (fileExt == ".obj")
  {
    cityGML = false;
  }
  else
  {
    vtkLog(ERROR, "Invalid file type: " << fileName);
    return 0;
  }

  vtkSmartPointer<vtkMultiBlockDataSet> data;
  if (cityGML)
  {
    vtkNew<vtkCityGMLReader> reader;
    reader->SetFileName(fileName.c_str());
    reader->Update();
    data = vtkMultiBlockDataSet::SafeDownCast(reader->GetOutputDataObject(0));
  }
  else
  {
    data = ReadOBJFiles(1, 0 /*lod - not used*/, { fileName }, fileOffset);
  }

  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(0.5, 0.7, 0.7);

  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(renWin);

  char* tname =
    vtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "VTK_TEMP_DIR", "Testing/Temporary");
  std::string tmpDir(tname);
  delete[] tname;
  std::string outputName = tmpDir + "/TestGLTFWriter.gltf";

  vtkNew<vtkGLTFWriter> writer;
  writer->SetFileName(outputName.c_str());
  writer->SetTextureBaseDirectory((filePath).c_str());
  writer->SetInputDataObject(data);
  writer->Write();

  vtkNew<vtkGLTFImporter> importer;
  importer->SetFileName(outputName.c_str());
  importer->SetCamera(-1);
  importer->SetRenderWindow(renWin);
  importer->Update();

  renderer->ResetCamera();
  if (cityGML)
  {
    renderer->GetActiveCamera()->Azimuth(90);
    renderer->GetActiveCamera()->Roll(-90);
    renderer->GetActiveCamera()->Zoom(1.5);
  }

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
