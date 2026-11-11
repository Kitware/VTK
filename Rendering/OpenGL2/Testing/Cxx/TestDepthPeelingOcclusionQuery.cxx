// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Ensure that opaque geometry does not trigger early termination of depth peeling.
#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCubeSource.h"
#include "vtkNew.h"
#include "vtkOpenGLRenderer.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
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

int TestDepthPeelingOcclusionQuery(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  vtkNew<vtkRenderWindowInteractor> iren;
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  renWin->SetAlphaBitPlanes(1);
  iren->SetRenderWindow(renWin);
  vtkNew<vtkRenderer> renderer;
  renWin->AddRenderer(renderer);

  vtkNew<vtkPolyDataMapper> mapperBox;
  vtkNew<vtkCubeSource> box;
  box->SetXLength(3.0);
  box->SetYLength(3.0);
  mapperBox->SetInputConnection(box->GetOutputPort());

  vtkNew<vtkPolyDataMapper> mapperSphere;
  vtkNew<vtkSphereSource> sphere;
  mapperSphere->SetInputConnection(sphere->GetOutputPort());

  vtkNew<vtkActor> actorBox;
  actorBox->GetProperty()->SetColor(0.1, 0.1, 0.1);
  actorBox->SetMapper(mapperBox);
  renderer->AddActor(actorBox);

  vtkNew<vtkActor> actorSphere1;
  actorSphere1->GetProperty()->SetColor(1.0, 0.0, 0.0);
  actorSphere1->GetProperty()->SetOpacity(0.5);
  actorSphere1->GetProperty()->LightingOff();
  actorSphere1->SetPosition(-0.15, 0.0, 1.0);
  actorSphere1->SetMapper(mapperSphere);
  renderer->AddActor(actorSphere1);

  vtkNew<vtkActor> actorSphere2;
  actorSphere2->GetProperty()->SetColor(0.0, 1.0, 0.0);
  actorSphere2->GetProperty()->SetOpacity(0.5);
  actorSphere2->GetProperty()->LightingOff();
  actorSphere2->SetPosition(0.15, 0.0, 2.0);
  actorSphere2->SetMapper(mapperSphere);
  renderer->AddActor(actorSphere2);

  // Occluded fragments must not expand the translucent depth range.
  vtkNew<vtkActor> actorSphere3;
  actorSphere3->GetProperty()->SetColor(0.0, 0.0, 1.0);
  actorSphere3->GetProperty()->SetOpacity(0.5);
  actorSphere3->SetPosition(0.0, 0.0, -1.0);
  actorSphere3->SetMapper(mapperSphere);
  renderer->AddActor(actorSphere3);

#if VTK_MODULE_vtkglad_GLES3
  // create the basic VTK render steps
  vtkNew<vtkRenderStepsPass> basicPasses;

  // replace the default translucent pass with
  // a more advanced depth peeling pass
  vtkNew<vtkDepthPeelingPass> peeling;
  peeling->SetMaximumNumberOfPeels(20);
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
  // Two dual peels must composite all four visible sphere surfaces.
  renderer->SetMaximumNumberOfPeels(2);
  renderer->SetOcclusionRatio(0.0);
#endif

  renWin->SetSize(500, 500);
  renderer->GetActiveCamera()->ParallelProjectionOn();
  renderer->GetActiveCamera()->SetPosition(0.0, 0.0, 5.0);
  renderer->GetActiveCamera()->SetFocalPoint(0.0, 0.0, 0.0);
  renderer->GetActiveCamera()->SetParallelScale(0.75);
  renderer->ResetCameraClippingRange();

  renWin->Render();
  iren->Start();

  return EXIT_SUCCESS;
}
