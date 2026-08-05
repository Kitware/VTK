// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkActorCollection.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkTexture.h"

#include "vtkAnariSceneGraph.h"
#include "vtkAnariTestUtilities.h"

/**
 * This test covers the PBR Clear coat feature
 * It renders spheres with different coat materials using a skybox as image based lighting
 *
 * This test requires the ANARI_KHR_MATERIAL_PHYSICALLY_BASED ANARI extension. If this extension is
 * not available (with helide backend for example), it behaves as a smoke test to make sure the VTK
 * API does not crash. If the loaded backend supports the extension, it will perform an image
 * comparison.
 */
int TestAnariPBRMaterialsCoat(int argc, char* argv[])
{
  bool useDebugDevice = false;

  for (int i = 0; i < argc; i++)
  {
    if (!strcmp(argv[i], "--trace"))
    {
      useDebugDevice = true;
    }
  }

  vtkNew<vtkRenderer> renderer;

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(600, 600);
  renWin->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  vtkNew<vtkSphereSource> sphere;
  sphere->SetThetaResolution(75);
  sphere->SetPhiResolution(75);

  vtkNew<vtkPolyDataMapper> pdSphere;
  pdSphere->SetInputConnection(sphere->GetOutputPort());

  for (int i = 0; i < 6; i++)
  {
    vtkNew<vtkActor> actorSphere;
    actorSphere->SetPosition(i, 0.0, 0.0);
    actorSphere->SetMapper(pdSphere);
    actorSphere->GetProperty()->SetInterpolationToPBR();
    actorSphere->GetProperty()->SetColor(0.72, 0.45, 0.2);
    actorSphere->GetProperty()->SetMetallic(1.0);
    actorSphere->GetProperty()->SetRoughness(0.1);
    actorSphere->GetProperty()->SetCoatStrength(1.0);
    actorSphere->GetProperty()->SetCoatRoughness(i / 5.0);
    renderer->AddActor(actorSphere);
  }

  for (int i = 0; i < 6; i++)
  {
    vtkNew<vtkActor> actorSphere;
    actorSphere->SetPosition(i, 1.0, 0.0);
    actorSphere->SetMapper(pdSphere);
    actorSphere->GetProperty()->SetInterpolationToPBR();
    actorSphere->GetProperty()->SetColor(0.72, 0.45, 0.2);
    actorSphere->GetProperty()->SetMetallic(1.0);
    actorSphere->GetProperty()->SetRoughness(1.0);
    actorSphere->GetProperty()->SetCoatStrength(1.0);
    actorSphere->GetProperty()->SetCoatRoughness(i / 5.0);
    renderer->AddActor(actorSphere);
  }

  for (int i = 0; i < 6; i++)
  {
    vtkNew<vtkActor> actorSphere;
    actorSphere->SetPosition(i, 2.0, 0.0);
    actorSphere->SetMapper(pdSphere);
    actorSphere->GetProperty()->SetInterpolationToPBR();
    actorSphere->GetProperty()->SetMetallic(1.0);
    actorSphere->GetProperty()->SetRoughness(0.1);
    actorSphere->GetProperty()->SetCoatColor(1.0, 0.0, 0.0);
    actorSphere->GetProperty()->SetCoatRoughness(0.1);
    actorSphere->GetProperty()->SetCoatStrength(i / 5.0);
    renderer->AddActor(actorSphere);
  }

  for (int i = 0; i < 6; i++)
  {
    vtkNew<vtkActor> actorSphere;
    actorSphere->SetPosition(i, 3.0, 0.0);
    actorSphere->SetMapper(pdSphere);
    actorSphere->GetProperty()->SetInterpolationToPBR();
    actorSphere->GetProperty()->SetRoughness(0.1);
    actorSphere->GetProperty()->SetCoatColor(1.0, 0.0, 0.0);
    actorSphere->GetProperty()->SetCoatRoughness(1.0);
    actorSphere->GetProperty()->SetCoatStrength(i / 5.0);
    renderer->AddActor(actorSphere);
  }

  for (int i = 0; i < 6; i++)
  {
    vtkNew<vtkActor> actorSphere;
    actorSphere->SetPosition(i, 4.0, 0.0);
    actorSphere->SetMapper(pdSphere);
    actorSphere->GetProperty()->SetInterpolationToPBR();
    actorSphere->GetProperty()->SetColor(0.0, 0.5, 0.30);
    actorSphere->GetProperty()->SetBaseIOR(1.0 + (i / 3.0));
    renderer->AddActor(actorSphere);
  }

  vtkAnariTestUtilities::SetParameterDefaults(renWin, useDebugDevice, "TestAnariPBRMaterialsCoat");

  renWin->Render();

  bool testSuccess = true;
  const auto& extensions = vtkAnariTestUtilities::GetDeviceExtensions(renWin);
  if (extensions.ANARI_KHR_MATERIAL_PHYSICALLY_BASED)
  {
    int retVal = vtkRegressionTestImage(renWin);

    if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
      iren->Start();
    }

    testSuccess = retVal == vtkRegressionTester::PASSED;
  }

  return testSuccess ? EXIT_SUCCESS : EXIT_FAILURE;
}
