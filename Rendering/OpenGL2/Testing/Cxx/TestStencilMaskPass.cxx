// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkAppendPolyData.h"
#include "vtkCameraPass.h"
#include "vtkCubeSource.h"
#include "vtkInformation.h"
#include "vtkLightsPass.h"
#include "vtkNew.h"
#include "vtkOpaquePass.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderPassCollection.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSequencePass.h"
#include "vtkSphereSource.h"
#include "vtkStencilMaskPass.h"

//------------------------------------------------------------------------------
int TestStencilMaskPass(int argc, char* argv[])
{
  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(0.3, 0.4, 0.6);
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(600, 600);
  renderWindow->AddRenderer(renderer);
  renderWindow->StencilCapableOn();
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow);

  // Sphere
  vtkNew<vtkSphereSource> sphereSource;
  sphereSource->SetCenter(0.0, 0.0, 8.0);
  sphereSource->SetRadius(4.0);
  sphereSource->Update();

  // Cube behind the sphere
  vtkNew<vtkCubeSource> cubeSource;
  cubeSource->SetCenter(4.0, 0.0, 0.0);
  cubeSource->SetXLength(7.0);
  cubeSource->SetYLength(7.0);
  cubeSource->SetZLength(7.0);

  // Cube in front of the sphere
  vtkNew<vtkCubeSource> cubeSource2;
  cubeSource2->SetCenter(-4.0, 0.0, 16.0);
  cubeSource2->SetXLength(7.0);
  cubeSource2->SetYLength(7.0);
  cubeSource2->SetZLength(7.0);

  vtkNew<vtkAppendPolyData> appendFilter;
  appendFilter->SetInputConnection(cubeSource->GetOutputPort());
  appendFilter->AddInputConnection(cubeSource2->GetOutputPort());
  appendFilter->Update();

  vtkNew<vtkPolyDataMapper> mapper1;
  mapper1->SetInputConnection(sphereSource->GetOutputPort());

  vtkNew<vtkActor> actor1;
  vtkNew<vtkInformation> information;
  actor1->SetPropertyKeys(information);
  actor1->GetPropertyKeys()->Set(vtkStencilMaskPass::GLStencilWrite(), 1);
  actor1->SetMapper(mapper1);

  vtkNew<vtkPolyDataMapper> mapper2;
  mapper2->SetInputConnection(appendFilter->GetOutputPort());

  vtkNew<vtkActor> actor2;
  actor2->SetMapper(mapper2);

  renderer->AddActor(actor1);
  renderer->AddActor(actor2);

  vtkNew<vtkStencilMaskPass> stencilPass;
  vtkNew<vtkOpaquePass> opaqueP;
  vtkNew<vtkLightsPass> lightsP;

  vtkNew<vtkRenderPassCollection> passes;
  passes->AddItem(stencilPass);
  passes->AddItem(lightsP);
  passes->AddItem(opaqueP);
  vtkNew<vtkSequencePass> seqP;
  seqP->SetPasses(passes);

  vtkNew<vtkCameraPass> cameraP;
  cameraP->SetDelegatePass(seqP);

  renderer->SetPass(cameraP);

  renderWindow->Render();

  int retVal = vtkRegressionTestImage(renderWindow);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
