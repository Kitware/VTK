// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

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
#include "vtkTestUtilities.h"
#include "vtkTexture.h"

#include "vtkAnariSceneGraph.h"
#include "vtkAnariTestUtilities.h"

#include <set>
#include <string>

/**
 * This test verifies that we can load a set of ANARI materials specification
 * from disk and use them.
 */
int TestAnariMaterialLibrary(int argc, char* argv[])
{
  bool useDebugDevice = false;

  for (int i = 0; i < argc; i++)
  {
    if (!strcmp(argv[i], "--trace"))
    {
      useDebugDevice = true;
    }
  }

  vtkSmartPointer<vtkANARIMaterialLibrary> lib = vtkSmartPointer<vtkANARIMaterialLibrary>::New();

  // Try MTL file first (simpler format)
  const char* mtlFile = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/anari_mats.mtl");
  vtkLogF(INFO, "Open %s", mtlFile);
  lib->ReadFile(mtlFile);
  vtkLogF(INFO, "Parsed MTL file OK, now check for expected contents.");
  std::set<std::string> mats = lib->GetMaterialNames();

  vtkLogF(INFO, "Materials are:");
  for (const std::string& materialName : mats)
  {
    vtkLogF(INFO, " - %s", materialName.c_str());
  }

  // Test mat1 from MTL file
  if (mats.find("mat1") == mats.end())
  {
    vtkLogF(ERROR, "Problem, could not find expected material named mat1 from MTL.");
    return VTK_ERROR;
  }
  vtkLogF(INFO, "Found mat1 material from MTL.");

  if (lib->LookupImplName("mat1") != "obj")
  {
    vtkLogF(ERROR, "Problem, expected mat1 to be of type obj.");
    return VTK_ERROR;
  }
  vtkLogF(INFO, "mat1 is the correct type.");

  if (mats.find("mat2") == mats.end())
  {
    vtkLogF(ERROR, "Problem, could not find expected material named mat2.");
    return VTK_ERROR;
  }
  vtkLogF(INFO, "Found mat2 material.");

  if (mats.find("mat3") == mats.end())
  {
    vtkLogF(ERROR, "Problem, could not find expected material named mat3.");
    return VTK_ERROR;
  }
  if (lib->LookupImplName("mat3") != "metal")
  {
    vtkLogF(ERROR, "Problem, expected mat3 to be implemented by the metal material.");
    return VTK_ERROR;
  }
  vtkLogF(INFO, "mat3 is the right type.");

  vtkLogF(INFO, "We're all clear kid.");

  // Add matte material programmatically for the rendering test
  lib->AddMaterial("armadillo_matte", "matte");
  double matteColor[3] = { 0.3, 0.4, 0.5 };
  lib->AddShaderVariable("armadillo_matte", "color", 3, matteColor);

  // serialize and deserialize
  vtkLogF(INFO, "Serialize");
  const char* buf = lib->WriteBuffer();

  vtkLogF(INFO, "Deserialize");
  lib->ReadBuffer(buf);

  // Set up rendering pipeline
  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(0.5, 0.5, 0.5);

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(301, 300);
  renWin->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  vtkAnariTestUtilities::SetParameterDefaults(renWin, useDebugDevice, "TestAnariMaterialLibrary");
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
    iren->Start();
  }

  return !retVal;
}
