// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkConeSource.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"

#include "vtkAnariOpenGLTestUtilities.h"
#include "vtkAnariPass.h"
#include "vtkAnariSceneGraph.h"

/**
 * This test verifies that ANARI can render in stereo modes.
 */
int TestAnariStereo(int argc, char* argv[])
{
  bool useDebugDevice = false;
  for (int i = 0; i < argc; ++i)
  {
    if (!strcmp(argv[i], "--trace"))
    {
      useDebugDevice = true;
    }
  }

  vtkNew<vtkSphereSource> sphere1;
  sphere1->SetCenter(0.2, 0.0, -7.0);
  sphere1->SetRadius(0.5);
  sphere1->SetThetaResolution(100);
  sphere1->SetPhiResolution(100);

  vtkNew<vtkPolyDataMapper> mapper1;
  mapper1->SetInputConnection(sphere1->GetOutputPort());

  vtkNew<vtkActor> actor1;
  actor1->SetMapper(mapper1);
  actor1->GetProperty()->SetColor(0.8, 0.8, 0.0);

  vtkNew<vtkConeSource> cone1;
  cone1->SetCenter(0.0, 0.0, -6.0);
  cone1->SetResolution(100);

  vtkNew<vtkPolyDataMapper> mapper2;
  mapper2->SetInputConnection(cone1->GetOutputPort());

  vtkNew<vtkActor> actor2;
  actor2->SetMapper(mapper2);
  actor2->GetProperty()->SetAmbient(0.1);

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor1);
  renderer->AddActor(actor2);
  renderer->SetAmbient(1.0, 1.0, 1.0);

  vtkNew<vtkRenderWindow> renwin;
  renwin->AddRenderer(renderer);
  renwin->SetStereoType(VTK_STEREO_SPLITVIEWPORT_HORIZONTAL);
  renwin->StereoRenderOn();
  renwin->SetMultiSamples(0);
  renwin->SetSize(400, 400);

  vtkNew<vtkAnariPass> anariPass;
  renderer->SetPass(anariPass);
  vtkAnariOpenGLTestUtilities::SetParameterDefaults(
    anariPass, renderer, useDebugDevice, "TestAnariStereo");

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renwin);

  renderer->ResetCamera();
  vtkCamera* camera = renderer->GetActiveCamera();
  camera->SetScreenBottomLeft(-1.0, -1.0, -10.0);
  camera->SetScreenBottomRight(1.0, -1.0, -10.0);
  camera->SetScreenTopRight(1.0, 1.0, -10.0);
  camera->SetUseOffAxisProjection(1);
  double eyePosition[3] = { 0.0, 0.0, 2.0 };
  camera->SetEyePosition(eyePosition);
  camera->SetEyeSeparation(0.05);
  camera->SetPosition(0.0, 0.0, 2.0);
  camera->SetFocalPoint(0.0, 0.0, -6.6);

  int retVal = vtkRegressionTestImage(renwin);

  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return retVal == vtkTesting::PASSED ? EXIT_SUCCESS : EXIT_FAILURE;
}
