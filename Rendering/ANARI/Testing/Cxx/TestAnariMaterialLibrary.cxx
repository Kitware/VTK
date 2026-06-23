// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// This test verifies that we can load a set of ANARI materials specification
// from disk and use them.

#include "vtkTestUtilities.h"

#include "vtkANARIMaterialLibrary.h"
#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPLYReader.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyDataNormals.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkTexture.h"

#include "vtkAnariPass.h"
#include "vtkAnariSceneGraph.h"
#include "vtkAnariTestInteractor.h"
#include "vtkAnariTestUtilities.h"

#include <iostream>
#include <set>
#include <string>

int TestAnariMaterialLibrary(int argc, char* argv[])
{
  vtkLogger::SetStderrVerbosity(vtkLogger::Verbosity::VERBOSITY_WARNING);
  bool useDebugDevice = false;

  for (int i = 0; i < argc; i++)
  {
    if (!strcmp(argv[i], "-trace"))
    {
      useDebugDevice = true;
      vtkLogger::SetStderrVerbosity(vtkLogger::Verbosity::VERBOSITY_INFO);
    }
  }

  vtkSmartPointer<vtkANARIMaterialLibrary> lib = vtkSmartPointer<vtkANARIMaterialLibrary>::New();

  // Try MTL file first (simpler format)
  const char* mtlFile = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/anari_mats.mtl");
  std::cout << "Open " << mtlFile << std::endl;
  lib->ReadFile(mtlFile);
  std::cout << "Parsed MTL file OK, now check for expected contents." << std::endl;
  std::set<std::string> mats = lib->GetMaterialNames();

  std::cout << "Materials are:" << std::endl;
  std::set<std::string>::iterator it = mats.begin();
  while (it != mats.end())
  {
    std::cout << *it << std::endl;
    ++it;
  }

  // Test mat1 from MTL file
  if (mats.find("mat1") == mats.end())
  {
    std::cerr << "Problem, could not find expected material named mat1 from MTL." << std::endl;
    return VTK_ERROR;
  }
  std::cout << "Found mat1 material from MTL." << std::endl;

  if (lib->LookupImplName("mat1") != "obj")
  {
    std::cerr << "Problem, expected mat1 to be of type obj." << std::endl;
    return VTK_ERROR;
  }
  std::cout << "mat1 is the correct type." << std::endl;

  if (mats.find("mat2") == mats.end())
  {
    std::cerr << "Problem, could not find expected material named mat2." << std::endl;
    return VTK_ERROR;
  }
  std::cout << "Found mat2 material." << std::endl;

  if (mats.find("mat3") == mats.end())
  {
    std::cerr << "Problem, could not find expected material named mat3." << std::endl;
    return VTK_ERROR;
  }
  if (lib->LookupImplName("mat3") != "metal")
  {
    std::cerr << "Problem, expected mat3 to be implemented by the metal material." << std::endl;
    return VTK_ERROR;
  }
  std::cout << "mat3 is the right type." << std::endl;

  std::cout << "We're all clear kid." << std::endl;

  // Add matte material programmatically for the rendering test
  lib->AddMaterial("armadillo_matte", "matte");
  double matteColor[3] = { 0.3, 0.4, 0.5 };
  lib->AddShaderVariable("armadillo_matte", "color", 3, matteColor);

  // serialize and deserialize
  std::cout << "Serialize" << std::endl;
  const char* buf = lib->WriteBuffer();

  std::cout << "Deserialize" << std::endl;
  lib->ReadBuffer(buf);

  // Set up rendering pipeline
  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(0.5, 0.5, 0.5);

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(301, 300);
  renWin->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  vtkNew<vtkAnariPass> anariPass;
  renderer->SetPass(anariPass);

  SetParameterDefaults(anariPass, renderer, useDebugDevice, "TestAnariMaterialLibrary");
  vtkAnariSceneGraph::SetMaterialLibrary(lib, renderer);

  // Load armadillo model once and create three instances with different materials
  const char* armadilloFile =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/Armadillo.ply");
  vtkNew<vtkPLYReader> armadilloReader;
  armadilloReader->SetFileName(armadilloFile);

  vtkNew<vtkPolyDataNormals> armadilloNormals;
  armadilloNormals->SetInputConnection(armadilloReader->GetOutputPort());

  vtkNew<vtkPolyDataMapper> armadilloMapper;
  armadilloMapper->SetInputConnection(armadilloNormals->GetOutputPort());

  // Armadillo with matte material
  vtkNew<vtkActor> actor;
  actor->SetMapper(armadilloMapper);
  actor->GetProperty()->SetMaterialName("armadillo_matte");
  renderer->AddActor(actor);

  renWin->Render();
  renderer->ResetCamera();

  // Set up camera for a visually interesting view - elevated angle
  vtkCamera* camera = renderer->GetActiveCamera();
  camera->Azimuth(30);
  camera->Elevation(20);

  renderer->ResetCameraClippingRange();
  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);

  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    vtkNew<vtkAnariTestInteractor> style;
    style->SetPipelineControlPoints(renderer, anariPass, nullptr);
    iren->SetInteractorStyle(style);
    style->SetCurrentRenderer(renderer);

    iren->Start();
  }

  return !retVal;
}
