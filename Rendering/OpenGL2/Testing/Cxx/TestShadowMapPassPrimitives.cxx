// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCameraPass.h"
#include "vtkInformation.h"
#include "vtkLight.h"
#include "vtkLineSource.h"
#include "vtkNew.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLTexture.h"
#include "vtkPlaneSource.h"
#include "vtkPointSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderPassCollection.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSequencePass.h"
#include "vtkShadowMapBakerPass.h"
#include "vtkShadowMapPass.h"

//------------------------------------------------------------------------------
int TestShadowMapPassPrimitives(int argc, char* argv[])
{
  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(600, 600);
  renderWindow->AddRenderer(renderer);
  renderWindow->SetMultiSamples(0);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow);

  // Add a light
  vtkNew<vtkLight> light;
  light->SetFocalPoint(0, 0, 0);
  light->SetPosition(0, 1, 0);
  light->SetIntensity(0.8);
  renderer->AddLight(light);

  // Add a plane on the ground
  vtkNew<vtkPlaneSource> groundPlane;
  groundPlane->SetOrigin(-1.0, 0.0, -1.0);
  groundPlane->SetPoint1(1.0, 0.0, -1.0);
  groundPlane->SetPoint2(-1.0, 0.0, 1.0);
  vtkNew<vtkPolyDataMapper> groundMapper;
  groundMapper->SetInputConnection(groundPlane->GetOutputPort());
  vtkNew<vtkActor> groundActor;
  groundActor->SetMapper(groundMapper);
  renderer->AddActor(groundActor);

  // the ground plane will NOT be part of shadow baking
  vtkNew<vtkInformation> groundKeys;
  groundKeys->Set(vtkShadowMapBakerPass::OCCLUDER(), 0);
  groundActor->SetPropertyKeys(groundKeys);

  // Add a point source
  vtkNew<vtkPointSource> pointSource;
  pointSource->SetCenter(0.0, 0.2, 0.0);
  pointSource->SetRadius(0.1);
  pointSource->SetNumberOfPoints(30);
  vtkNew<vtkPolyDataMapper> pointMapper;
  pointMapper->SetInputConnection(pointSource->GetOutputPort());
  vtkNew<vtkActor> pointActor;
  pointActor->SetMapper(pointMapper);
  pointActor->GetProperty()->SetPointSize(5);
  renderer->AddActor(pointActor);

  // Add a line source
  vtkNew<vtkLineSource> lineSource;
  lineSource->SetPoint1(-0.5, 0.2, 0.0);
  lineSource->SetPoint2(0.5, 0.2, 0.0);
  vtkNew<vtkPolyDataMapper> lineMapper;
  lineMapper->SetInputConnection(lineSource->GetOutputPort());
  vtkNew<vtkActor> lineActor;
  lineActor->SetMapper(lineMapper);
  renderer->AddActor(lineActor);

  // Add a line source but render them as tubes
  vtkNew<vtkLineSource> tubeLineSource;
  tubeLineSource->SetPoint1(-0.2, 0.1, -0.5);
  tubeLineSource->SetPoint2(-0.2, 0.1, 0.5);
  vtkNew<vtkPolyDataMapper> tubeMapper;
  tubeMapper->SetInputConnection(tubeLineSource->GetOutputPort());
  vtkNew<vtkActor> tubeActor;
  tubeActor->SetMapper(tubeMapper);
  tubeActor->GetProperty()->SetLineWidth(15);
  tubeActor->GetProperty()->RenderLinesAsTubesOn();
  renderer->AddActor(tubeActor);

  vtkNew<vtkShadowMapPass> shadows;
  vtkShadowMapBakerPass* baker = shadows->GetShadowMapBakerPass();
  baker->SetResolution(1024);

  vtkNew<vtkSequencePass> seq;
  vtkNew<vtkRenderPassCollection> passes;
  passes->AddItem(baker);
  passes->AddItem(shadows);
  seq->SetPasses(passes);

  vtkNew<vtkCameraPass> cameraP;
  cameraP->SetDelegatePass(seq);

  // tell the renderer to use our render pass pipeline
  vtkOpenGLRenderer* glrenderer = vtkOpenGLRenderer::SafeDownCast(renderer);
  glrenderer->SetPass(cameraP);

  renderer->ResetCamera();
  renderer->GetActiveCamera()->Elevation(60);
  renderer->GetActiveCamera()->Zoom(4.0);
  renderWindow->Render();

  int retVal = vtkRegressionTestImage(renderWindow);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
