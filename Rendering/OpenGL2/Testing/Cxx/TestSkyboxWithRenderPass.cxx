// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkCameraPass.h"
#include "vtkHDRReader.h"
#include "vtkLightsPass.h"
#include "vtkOpaquePass.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderPassCollection.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSequencePass.h"
#include "vtkSkybox.h"
#include "vtkSphereSource.h"
#include "vtkTestUtilities.h"
#include "vtkTexture.h"

/**
 * when adding a custom render pass, the skybox should be rendered.
 */
int TestSkyboxWithRenderPass(int argc, char* argv[])
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
  renderer->SetSkyboxBlurRadius(40.0f);

  // Custom render pass
  vtkNew<vtkOpaquePass> opaquePass;
  vtkNew<vtkLightsPass> lightsPass;

  vtkNew<vtkRenderPassCollection> passes;
  passes->AddItem(lightsPass);
  passes->AddItem(opaquePass);
  vtkNew<vtkSequencePass> seqPass;
  seqPass->SetPasses(passes);

  vtkNew<vtkCameraPass> cameraPass;
  cameraPass->SetDelegatePass(seqPass);

  renderer->SetPass(cameraPass);

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
