// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// This test verifies that lighting works as expected with ANARI.
// When advanced materials are exposed in ANARI, it will also validate
// refractions and reflections
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit
//              In interactive mode it responds to the keys listed
//              vtkOSPRayTestInteractor.h

#include "vtkTestUtilities.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkLight.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkOpenGLRenderer.h"
#include "vtkPLYReader.h"
#include "vtkPlaneSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyDataNormals.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkTesting.h"

#include "vtkAnariPass.h"
#include "vtkAnariRendererNode.h"
#include "vtkAnariTestInteractor.h"

int TestAnariLights(int argc, char* argv[])
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

  vtkNew<vtkRenderWindowInteractor> iren;
  vtkNew<vtkRenderWindow> renWin;
  iren->SetRenderWindow(renWin);
  vtkNew<vtkRenderer> renderer;
  renderer->AutomaticLightCreationOff();
  renWin->AddRenderer(renderer);

  const char* fileName = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/bunny.ply");
  vtkNew<vtkPLYReader> polysource;
  polysource->SetFileName(fileName);

  // measure so we can place things sensibly
  polysource->Update();
  double bds[6];
  polysource->GetOutput()->GetBounds(bds);
  double x0 = bds[0] * 2;
  double x1 = bds[1] * 2;
  double y0 = bds[2];
  double y1 = bds[3] * 2;
  double z0 = bds[4];
  double z1 = bds[5] * 4;

  // TODO: ospray acts strangely without these such that Diff and Spec are not 0..255 instead of
  // 0..1
  vtkNew<vtkPolyDataNormals> normals;
  normals->SetInputConnection(polysource->GetOutputPort());

  vtkNew<vtkPolyDataMapper> mapper1;
  mapper1->SetInputConnection(normals->GetOutputPort());
  vtkNew<vtkActor> actor1;
  actor1->SetMapper(mapper1);
  actor1->GetProperty()->SetColor(1.0, 1.0, 1.0);
  actor1->GetProperty()->SetAmbient(0.1);
  actor1->GetProperty()->SetDiffuse(1);
  actor1->GetProperty()->SetSpecularColor(1, 1, 1);
  actor1->GetProperty()->SetSpecular(0.9);
  actor1->GetProperty()->SetSpecularPower(500);
  renderer->AddActor(actor1);

  vtkNew<vtkPlaneSource> backwall;
  backwall->SetOrigin(x0, y0, z0);
  backwall->SetPoint1(x1, y0, z0);
  backwall->SetPoint2(x0, y1, z0);
  vtkNew<vtkPolyDataMapper> mapper2;
  mapper2->SetInputConnection(backwall->GetOutputPort());
  vtkNew<vtkActor> actor2;
  actor2->SetMapper(mapper2);
  actor2->GetProperty()->SetColor(1.0, 1.0, 1.0);
  actor2->GetProperty()->SetAmbient(0.1);
  actor2->GetProperty()->SetDiffuse(1);
  actor2->GetProperty()->SetSpecular(0);
  renderer->AddActor(actor2);

  vtkNew<vtkPlaneSource> floor;
  floor->SetOrigin(x0, y0, z0);
  floor->SetPoint1(x0, y0, z1);
  floor->SetPoint2(x1, y0, z0);
  vtkNew<vtkPolyDataMapper> mapper3;
  mapper3->SetInputConnection(floor->GetOutputPort());
  vtkNew<vtkActor> actor3;
  actor3->GetProperty()->SetColor(1.0, 1.0, 1.0);
  actor3->GetProperty()->SetAmbient(0.1);
  actor3->GetProperty()->SetDiffuse(1);
  actor3->GetProperty()->SetSpecular(0);
  actor3->SetMapper(mapper3);
  renderer->AddActor(actor3);

  vtkNew<vtkPlaneSource> left;
  left->SetOrigin(x0, y0, z0);
  left->SetPoint1(x0, y1, z0);
  left->SetPoint2(x0, y0, z1);
  vtkNew<vtkPolyDataMapper> mapper4;
  mapper4->SetInputConnection(left->GetOutputPort());
  vtkNew<vtkActor> actor4;
  actor4->GetProperty()->SetColor(1.0, 1.0, 1.0);
  actor4->GetProperty()->SetAmbient(0.1);
  actor4->GetProperty()->SetDiffuse(1);
  actor4->GetProperty()->SetSpecular(0);
  actor4->SetMapper(mapper4);
  renderer->AddActor(actor4);

  vtkNew<vtkSphereSource> magnifier;
  // TODO: use PathTracer_Dielectric material for this when available
  magnifier->SetCenter(x0 + (x1 - x0) * 0.6, y0 + (y1 - y0) * 0.2, z0 + (z1 - z0) * 0.7);
  magnifier->SetRadius((x1 - x0) * 0.05);
  magnifier->SetPhiResolution(30);
  magnifier->SetThetaResolution(30);
  vtkNew<vtkPolyDataMapper> mapper5;
  mapper5->SetInputConnection(magnifier->GetOutputPort());
  vtkNew<vtkActor> actor5;
  actor5->GetProperty()->SetColor(1.0, 1.0, 1.0);
  actor5->GetProperty()->SetAmbient(0.1);
  actor5->GetProperty()->SetDiffuse(1);
  actor5->GetProperty()->SetSpecular(0);
  actor5->SetMapper(mapper5);
  renderer->AddActor(actor5);

  vtkNew<vtkSphereSource> discoball;
  // TODO: use PathTracer_Metal material for this when available
  discoball->SetCenter(x0 + (x1 - x0) * 0.5, y0 + (y1 - y0) * 0.85, z0 + (z1 - z0) * 0.5);
  discoball->SetRadius((x1 - x0) * 0.1);
  discoball->SetPhiResolution(30);
  discoball->SetThetaResolution(30);
  vtkNew<vtkPolyDataMapper> mapper6;
  mapper6->SetInputConnection(discoball->GetOutputPort());
  vtkNew<vtkActor> actor6;
  actor6->GetProperty()->SetColor(1.0, 1.0, 1.0);
  actor6->GetProperty()->SetAmbient(0.1);
  actor6->GetProperty()->SetDiffuse(1);
  actor6->GetProperty()->SetSpecular(0);
  actor6->SetMapper(mapper6);
  renderer->AddActor(actor6);

  vtkNew<vtkLight> blueLight;
  // blue light casting shadows from infinity toward bottom left back corner
  blueLight->PositionalOff();
  blueLight->SetPosition(x0 + (x1 - x0) * 1, y0 + (y1 - y0) * 1, z0 + (z1 + z0) * 1);
  blueLight->SetFocalPoint(x0, y0, z0);
  blueLight->SetLightTypeToSceneLight();
  blueLight->SetColor(0.0, 0.0, 1.0);
  blueLight->SetIntensity(0.3);
  blueLight->SwitchOn();
  renderer->AddLight(blueLight);

  vtkNew<vtkLight> redLight;
  // red light casting shadows from top to bottom
  redLight->PositionalOn();
  double t = 1.8; // adjust t to see effect of positional
  redLight->SetPosition(x0 + (x1 - x0) * 0.5, y0 + (y1 - y0) * t, z0 + (z1 - z0) * 0.5);
  redLight->SetFocalPoint(x0 + (x1 - x0) * 0.5, y0 + (y1 - y0) * 0, z0 + (z1 - z0) * 0.5);
  redLight->SetLightTypeToSceneLight();
  redLight->SetColor(1.0, 0.0, 0.0);
  redLight->SetIntensity(0.3);
  redLight->SwitchOn();
  renderer->AddLight(redLight);

  vtkNew<vtkLight> greenLight;
  // green light following camera
  greenLight->PositionalOn();
  greenLight->SetLightTypeToHeadlight();
  greenLight->SetColor(0.0, 1.0, 0.0);
  greenLight->SetIntensity(0.3);
  greenLight->SwitchOn();
  renderer->AddLight(greenLight);

  renderer->SetBackground(0.0, 0.0, 0.0);
  renderer->UseShadowsOn();
  renWin->SetSize(400, 400);

  vtkNew<vtkAnariPass> anariPass;
  renderer->SetPass(anariPass);

  if (useDebugDevice)
  {
    vtkAnariRendererNode::SetUseDebugDevice(1, renderer);
    vtkNew<vtkTesting> testing;

    std::string traceDir = testing->GetTempDirectory();
    traceDir += "/anari-trace";
    traceDir += "/TestAnariLights";
    vtkAnariRendererNode::SetDebugDeviceDirectory(traceDir.c_str(), renderer);
  }

  vtkAnariRendererNode::SetLibraryName("environment", renderer);
  vtkAnariRendererNode::SetSamplesPerPixel(4, renderer);
  vtkAnariRendererNode::SetLightFalloff(.5, renderer);
  vtkAnariRendererNode::SetUseDenoiser(1, renderer);
  vtkAnariRendererNode::SetCompositeOnGL(1, renderer);
  vtkAnariRendererNode::SetAmbientIntensity(0.2, renderer);

  renWin->Render();

  auto anariRendererNode = anariPass->GetSceneGraph();
  auto extensions = anariRendererNode->GetAnariDeviceExtensions();

  if (extensions.ANARI_KHR_LIGHT_SPOT)
  {
    int retVal = vtkRegressionTestImageThreshold(renWin, 0.05);

    if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
      vtkNew<vtkAnariTestInteractor> style;
      style->SetPipelineControlPoints(renderer, anariPass, nullptr);
      style->SetCurrentRenderer(renderer);

      iren->SetInteractorStyle(style);
      iren->Start();
    }

    return !retVal;
  }

  std::cout << "Required feature KHR_LIGHT_SPOT not supported." << std::endl;
  return VTK_SKIP_RETURN_CODE;
}
