// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// This test verifies that soft shadows work with ANARI.
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit
//              In interactive mode it responds to the keys listed
//              vtkAnariTestInteractor.h

#include "vtkTestUtilities.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkLight.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkOpenGLRenderer.h"
#include "vtkPlaneSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTesting.h"

#include "vtkAnariLightNode.h"
#include "vtkAnariPass.h"
#include "vtkAnariRendererNode.h"
#include "vtkAnariTestInteractor.h"

int TestAnariShadows(int argc, char* argv[])
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

  vtkNew<vtkAnariPass> anariPass;
  renderer->SetPass(anariPass);

  if (useDebugDevice)
  {
    vtkAnariRendererNode::SetUseDebugDevice(1, renderer);
    vtkNew<vtkTesting> testing;

    std::string traceDir = testing->GetTempDirectory();
    traceDir += "/anari-trace";
    traceDir += "/TestAnariShadows";
    vtkAnariRendererNode::SetDebugDeviceDirectory(traceDir.c_str(), renderer);
  }

  vtkAnariRendererNode::SetLibraryName("environment", renderer);
  vtkAnariRendererNode::SetSamplesPerPixel(5, renderer);
  vtkAnariRendererNode::SetLightFalloff(.5, renderer);
  vtkAnariRendererNode::SetUseDenoiser(1, renderer);
  vtkAnariRendererNode::SetCompositeOnGL(1, renderer);

  for (double i = 0.; i < 2.0; i += 0.25)
  {
    vtkAnariLightNode::SetRadius(i, l);
    renWin->Render();
  }

  auto anariRendererNode = anariPass->GetSceneGraph();
  auto extensions = anariRendererNode->GetAnariDeviceExtensions();

  if (extensions.ANARI_KHR_AREA_LIGHTS)
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

  std::cout << "Required feature KHR_AREA_LIGHTS not supported." << std::endl;
  return VTK_SKIP_RETURN_CODE;
}
