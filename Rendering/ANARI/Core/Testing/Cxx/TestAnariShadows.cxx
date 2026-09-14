// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkLight.h"
#include "vtkNew.h"
#include "vtkPlaneSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTesting.h"

#include "vtkAnariLightNode.h"
#include "vtkAnariSceneGraph.h"
#include "vtkAnariTestUtilities.h"

/**
 * This test verifies that soft shadows work with ANARI.
 *
 * This test requires the ANARI_KHR_AREA_LIGHTS ANARI extension. If this extension is not available
 * (with helide backend for example), it behaves as a smoke test to make sure the VTK API does not
 * crash. If the loaded backend supports the extension, it will perform an image comparison.
 */
int TestAnariShadows(int argc, char* argv[])
{
  bool useDebugDevice = false;

  for (int i = 0; i < argc; i++)
  {
    if (!strcmp(argv[i], "--trace"))
    {
      useDebugDevice = true;
    }
  }

  vtkNew<vtkRenderWindowInteractor> iren;
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(400, 400);
  iren->SetRenderWindow(renWin);
  vtkNew<vtkRenderer> renderer;
  renderer->AutomaticLightCreationOff();
  renderer->SetBackground(0.0, 0.0, 0.0);
  renderer->UseShadowsOn();
  renWin->AddRenderer(renderer);

  vtkNew<vtkCamera> c;
  c->SetPosition(0, 0, 80);
  c->SetFocalPoint(0, 0, 0);
  c->SetViewUp(0, 1, 0);
  renderer->SetActiveCamera(c);

  vtkNew<vtkLight> l;
  l->PositionalOn();
  l->SetPosition(4, 8, 20);
  l->SetFocalPoint(0, 0, 0);
  l->SetLightTypeToSceneLight();
  l->SetIntensity(200.0);
  renderer->AddLight(l);

  vtkNew<vtkPlaneSource> shadowee;
  shadowee->SetOrigin(-10, -10, 0);
  shadowee->SetPoint1(10, -10, 0);
  shadowee->SetPoint2(-10, 10, 0);
  vtkNew<vtkPolyDataMapper> mapper1;
  mapper1->SetInputConnection(shadowee->GetOutputPort());
  vtkNew<vtkActor> actor1;
  renderer->AddActor(actor1);
  actor1->SetMapper(mapper1);

  vtkNew<vtkPlaneSource> shadower;
  shadower->SetOrigin(-5, -5, 10);
  shadower->SetPoint1(5, -5, 10);
  shadower->SetPoint2(-5, 5, 10);
  vtkNew<vtkPolyDataMapper> mapper2;
  mapper2->SetInputConnection(shadower->GetOutputPort());
  vtkNew<vtkActor> actor2;
  renderer->AddActor(actor2);
  actor2->SetMapper(mapper2);

  vtkAnariTestUtilities::SetParameterDefaults(renWin, useDebugDevice, "TestAnariShadows");

  for (double i = 0.; i < 2.0; i += 0.25)
  {
    vtkAnariLightNode::SetRadius(i, l);
    renWin->Render();
  }

  bool testSuccess = true;
  const auto& extensions = vtkAnariTestUtilities::GetDeviceExtensions(renWin);
  if (extensions.ANARI_KHR_LIGHT_QUAD)
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
