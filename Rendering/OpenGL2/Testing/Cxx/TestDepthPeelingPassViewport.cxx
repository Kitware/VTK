// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkTestUtilities.h"

#include "vtk_glad.h" // for GLES3 detection support

#if VTK_MODULE_vtkglad_GLES3
#include "vtkDepthPeelingPass.h"
#include "vtkFramebufferPass.h"
#include "vtkOpenGLRenderer.h"
#include "vtkRenderStepsPass.h"
#include "vtkTextureObject.h"
#endif

namespace
{
void InitRenderer(vtkRenderer* renderer)
{
  renderer->LightFollowCameraOn();
  renderer->TwoSidedLightingOn();
#if VTK_MODULE_vtkglad_GLES3
  // create the basic VTK render steps
  vtkNew<vtkRenderStepsPass> basicPasses;

  // replace the default translucent pass with
  // a more advanced depth peeling pass
  vtkNew<vtkDepthPeelingPass> peeling;
  peeling->SetMaximumNumberOfPeels(8);
  peeling->SetOcclusionRatio(0.0);
  peeling->SetTranslucentPass(basicPasses->GetTranslucentPass());
  basicPasses->SetTranslucentPass(peeling);

  vtkNew<vtkFramebufferPass> fop;
  fop->SetDelegatePass(basicPasses);
  fop->SetDepthFormat(vtkTextureObject::Fixed24);
  peeling->SetOpaqueZTexture(fop->GetDepthTexture());
  peeling->SetOpaqueRGBATexture(fop->GetColorTexture());

  // tell the renderer to use our render pass pipeline
  vtkOpenGLRenderer* glrenderer = vtkOpenGLRenderer::SafeDownCast(renderer);
  glrenderer->SetPass(fop);
#else
  renderer->SetUseDepthPeeling(1);
  renderer->SetMaximumNumberOfPeels(8);
  renderer->SetOcclusionRatio(0.0);
#endif
}
} // anonymous namespace

int TestDepthPeelingPassViewport(int, char*[])
{
  vtkNew<vtkSphereSource> sphere;
  sphere->SetRadius(10);

  vtkNew<vtkRenderer> renderer;
  InitRenderer(renderer);

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetAlphaBitPlanes(1);
  renWin->SetMultiSamples(0);
  renWin->AddRenderer(renderer);

  vtkNew<vtkRenderer> renderer2;
  InitRenderer(renderer2);
  renderer2->SetViewport(0.0, 0.1, 0.2, 0.3);
  renderer2->InteractiveOff();
  renWin->AddRenderer(renderer2);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(sphere->GetOutputPort());

  {
    vtkNew<vtkActor> actor;
    actor->SetMapper(mapper);
    actor->GetProperty()->SetOpacity(0.35);
    actor->SetPosition(0.0, 0.0, 1.0);
    renderer->AddActor(actor);
  }

  {
    vtkNew<vtkActor> actor;
    actor->SetMapper(mapper);
    vtkProperty* prop = actor->GetProperty();
    prop->SetAmbientColor(1.0, 0.0, 0.0);
    prop->SetDiffuseColor(1.0, 0.8, 0.3);
    prop->SetSpecular(0.0);
    prop->SetDiffuse(0.5);
    prop->SetAmbient(0.3);
    renderer2->AddActor(actor);
  }
  {
    vtkNew<vtkActor> actor;
    actor->SetMapper(mapper);
    actor->GetProperty()->SetOpacity(0.35);
    actor->SetPosition(10.0, 0.0, 0.0);
    renderer2->AddActor(actor);
  }

  renderer->SetLayer(0);
  renderer2->SetLayer(1);
  renWin->SetNumberOfLayers(2);

  renderer->ResetCamera();
  renderer2->ResetCamera();

  renWin->Render();
  iren->Start();

  return EXIT_SUCCESS;
}
