// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// This test covers the PBR Interpolation shading
// It renders spheres with different materials using a skybox as image based lighting

#include "vtkActor.h"
#include "vtkActorCollection.h"
#include "vtkImageData.h"
#include "vtkImageFlip.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkJPEGReader.h"
#include "vtkLight.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkSphereSource.h"
#include "vtkTestUtilities.h"
#include "vtkTexture.h"

#include "vtkAnariPass.h"
#include "vtkAnariRendererNode.h"
#include "vtkAnariTestInteractor.h"

//------------------------------------------------------------------------------
int TestAnariPBRMaterials(int argc, char* argv[])
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

  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(1.0, 1.0, 1.0);

  vtkNew<vtkLight> light;
  light->SetColor(1.0, 1.0, 1.0);
  light->SetIntensity(0.3);
  light->SwitchOn();
  renderer->AddLight(light);

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(600, 600);
  renWin->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  vtkNew<vtkSphereSource> sphere;
  sphere->SetThetaResolution(100);
  sphere->SetPhiResolution(100);

  vtkNew<vtkPolyDataMapper> pdSphere;
  pdSphere->SetInputConnection(sphere->GetOutputPort());

  for (int i = 0; i < 6; i++)
  {
    vtkNew<vtkActor> actorSphere;
    actorSphere->SetPosition(i, 0.0, 0.0);
    actorSphere->SetMapper(pdSphere);
    actorSphere->GetProperty()->SetInterpolationToPBR();
    actorSphere->GetProperty()->SetColor(1.0, 1.0, 1.0);
    actorSphere->GetProperty()->SetMetallic(1.0);
    actorSphere->GetProperty()->SetRoughness(i / 5.0);
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
    actorSphere->GetProperty()->SetRoughness(i / 5.0);
    renderer->AddActor(actorSphere);
  }

  for (int i = 0; i < 6; i++)
  {
    vtkNew<vtkActor> actorSphere;
    actorSphere->SetPosition(i, 2.0, 0.0);
    actorSphere->SetMapper(pdSphere);
    actorSphere->GetProperty()->SetInterpolationToPBR();
    actorSphere->GetProperty()->SetColor(0.0, 0.0, 0.0);
    actorSphere->GetProperty()->SetRoughness(i / 5.0);
    renderer->AddActor(actorSphere);
  }

  for (int i = 0; i < 6; i++)
  {
    vtkNew<vtkActor> actorSphere;
    actorSphere->SetPosition(i, 3.0, 0.0);
    actorSphere->SetMapper(pdSphere);
    actorSphere->GetProperty()->SetInterpolationToPBR();
    actorSphere->GetProperty()->SetColor(0.0, 1.0, 1.0);
    actorSphere->GetProperty()->SetRoughness(i / 5.0);
    renderer->AddActor(actorSphere);
  }

  for (int i = 0; i < 6; i++)
  {
    vtkNew<vtkActor> actorSphere;
    actorSphere->SetPosition(i, 4.0, 0.0);
    actorSphere->SetMapper(pdSphere);
    actorSphere->GetProperty()->SetInterpolationToPBR();
    actorSphere->GetProperty()->SetColor(1.0, 0.0, 0.0);
    actorSphere->GetProperty()->SetRoughness(i / 5.0);
    renderer->AddActor(actorSphere);
  }

  vtkNew<vtkAnariPass> anariPass;
  renderer->SetPass(anariPass);

  if (useDebugDevice)
  {
    vtkAnariRendererNode::SetUseDebugDevice(1, renderer);
    vtkNew<vtkTesting> testing;

    std::string traceDir = testing->GetTempDirectory();
    traceDir += "/anari-trace";
    traceDir += "/TestAnariPBRMaterials";
    vtkAnariRendererNode::SetDebugDeviceDirectory(traceDir.c_str(), renderer);
  }

  vtkAnariRendererNode::SetLibraryName("environment", renderer);
  vtkAnariRendererNode::SetSamplesPerPixel(6, renderer);
  vtkAnariRendererNode::SetLightFalloff(.5, renderer);
  vtkAnariRendererNode::SetUseDenoiser(1, renderer);
  vtkAnariRendererNode::SetCompositeOnGL(1, renderer);

  renWin->Render();

  auto anariRendererNode = anariPass->GetSceneGraph();
  auto extensions = anariRendererNode->GetAnariDeviceExtensions();

  if (extensions.ANARI_KHR_MATERIAL_PHYSICALLY_BASED)
  {
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

  std::cout << "Required feature KHR_MATERIAL_PHYSICALLY_BASED not supported." << std::endl;
  return VTK_SKIP_RETURN_CODE;
}
