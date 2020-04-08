/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestToneMappingPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test covers the tone mapping post-processing render pass.
// It renders an opaque actor with a lot of lights.

#include "vtkRegressionTestImage.h"
#include "vtkTestUtilities.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCameraPass.h"
#include "vtkLight.h"
#include "vtkLightsPass.h"
#include "vtkOpaquePass.h"
#include "vtkOpenGLRenderer.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderPassCollection.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSequencePass.h"
#include "vtkSphereSource.h"
#include "vtkToneMappingPass.h"

int TestToneMappingPass(int argc, char* argv[])
{
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(400, 800);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  vtkNew<vtkSphereSource> sphere;
  sphere->SetThetaResolution(20);
  sphere->SetPhiResolution(20);

  double y = 0.0;
  for (int i = 0; i < 8; i++)
  {
    vtkNew<vtkRenderer> renderer;

    // custom passes
    vtkNew<vtkCameraPass> cameraP;
    vtkNew<vtkSequencePass> seq;
    vtkNew<vtkOpaquePass> opaque;
    vtkNew<vtkLightsPass> lights;

    vtkNew<vtkRenderPassCollection> passes;
    passes->AddItem(lights);
    passes->AddItem(opaque);
    seq->SetPasses(passes);
    cameraP->SetDelegatePass(seq);

    vtkNew<vtkToneMappingPass> toneMappingP;
    switch (i)
    {
      case 0:
        toneMappingP->SetToneMappingType(vtkToneMappingPass::Clamp);
        break;
      case 1:
        toneMappingP->SetToneMappingType(vtkToneMappingPass::Reinhard);
        break;
      case 2:
        toneMappingP->SetToneMappingType(vtkToneMappingPass::Exponential);
        toneMappingP->SetExposure(1.0);
        break;
      case 3:
        toneMappingP->SetToneMappingType(vtkToneMappingPass::Exponential);
        toneMappingP->SetExposure(2.0);
        break;
      case 4:
        toneMappingP->SetToneMappingType(vtkToneMappingPass::GenericFilmic);
        toneMappingP->SetGenericFilmicUncharted2Presets();
        break;
      case 5:
        toneMappingP->SetToneMappingType(vtkToneMappingPass::GenericFilmic);
        toneMappingP->SetGenericFilmicDefaultPresets();
        break;
      case 6:
        toneMappingP->SetToneMappingType(vtkToneMappingPass::GenericFilmic);
        toneMappingP->SetUseACES(false);
        break;
      case 7:
        toneMappingP->SetToneMappingType(vtkToneMappingPass::GenericFilmic);
        toneMappingP->SetGenericFilmicUncharted2Presets();
        toneMappingP->SetUseACES(false);
        break;
    }
    toneMappingP->SetDelegatePass(cameraP);

    vtkOpenGLRenderer::SafeDownCast(renderer)->SetPass(toneMappingP);

    double x = 0.5 * (i & 1);
    if (i)
    {
      y += 1 / 4.f * !(i & 1);
    }

    renderer->SetViewport(x, y, x + 0.5, y + 1 / 4.f);
    renderer->SetBackground(0.5, 0.5, 0.5);
    renWin->AddRenderer(renderer);

    // add one light in front of the object
    vtkNew<vtkLight> light1;
    light1->SetPosition(0.0, 0.0, 1.0);
    light1->SetFocalPoint(0.0, 0.0, 0.0);
    light1->SetColor(1.0, 1.0, 1.0);
    light1->PositionalOn();
    light1->SwitchOn();
    renderer->AddLight(light1);

    // add three lights on the sides
    double c = cos(vtkMath::Pi() * 2.0 / 3.0);
    double s = sin(vtkMath::Pi() * 2.0 / 3.0);

    vtkNew<vtkLight> light2;
    light2->SetPosition(1.0, 0.0, 1.0);
    light2->SetFocalPoint(0.0, 0.0, 0.0);
    light2->SetColor(1.0, 1.0, 1.0);
    light2->PositionalOn();
    light2->SwitchOn();
    renderer->AddLight(light2);

    vtkNew<vtkLight> light3;
    light3->SetPosition(c, s, 1.0);
    light3->SetFocalPoint(0.0, 0.0, 0.0);
    light3->SetColor(1.0, 1.0, 1.0);
    light3->PositionalOn();
    light3->SwitchOn();
    renderer->AddLight(light3);

    vtkNew<vtkLight> light4;
    light4->SetPosition(c, -s, 1.0);
    light4->SetFocalPoint(0.0, 0.0, 0.0);
    light4->SetColor(1.0, 1.0, 1.0);
    light4->PositionalOn();
    light4->SwitchOn();
    renderer->AddLight(light4);

    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputConnection(sphere->GetOutputPort());

    vtkNew<vtkActor> actor;
    actor->SetMapper(mapper);
    renderer->AddActor(actor);

    renderer->ResetCamera();
    renderer->GetActiveCamera()->Zoom(1.3);
    renderer->ResetCameraClippingRange();
  }

  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
