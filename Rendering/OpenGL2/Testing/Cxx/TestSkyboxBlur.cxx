// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkHDRReader.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSkybox.h"
#include "vtkSphereSource.h"
#include "vtkTestUtilities.h"
#include "vtkTexture.h"

/**
 * This test aims to verify that the skybox blur is displayed properly in the background.
 */
int TestSkyboxBlur(int argc, char* argv[])
{
  const char* hrdiFileName =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/spiaggia_di_mondello_1k.hdr");

  vtkNew<vtkSphereSource> sphere;
  sphere->SetThetaResolution(100);
  sphere->SetPhiResolution(100);
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(sphere->GetOutputPort());

  vtkNew<vtkHDRReader> hdrReader;
  hdrReader->SetFileName(hrdiFileName);

  vtkNew<vtkTexture> hdrTexture;
  hdrTexture->SetColorModeToDirectScalars();
  hdrTexture->MipmapOn();
  hdrTexture->InterpolateOn();
  hdrTexture->SetInputConnection(hdrReader->GetOutputPort());

  vtkNew<vtkRenderer> renderer;
  renderer->UseImageBasedLightingOn();
  renderer->SetEnvironmentTexture(hdrTexture);
  renderer->SetSkyboxBlurEnabled(true);
  renderer->SetSkyboxBlurRadius(40.0f);

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->SetInterpolationToPBR();
  actor->GetProperty()->SetColor(1.0, 0.0, 0.0);
  actor->GetProperty()->SetRoughness(0.0);
  renderer->AddActor(actor);

  vtkNew<vtkSkybox> skybox;
  skybox->SetTexture(hdrTexture);
  skybox->SetProjectionToSphere();
  renderer->AddActor(skybox);

  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> interactor;
  renderWindow->SetInteractor(interactor);

  renderWindow->Render();

  int retVal = vtkRegressionTestImage(renderWindow);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    interactor->Start();
  }

  return !retVal;
}
