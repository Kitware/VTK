/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSphereSource.h"
#include "vtkTestUtilities.h"

namespace {

void InitRenderer(vtkRenderer* renderer)
{
   renderer->SetUseDepthPeeling(1);
   renderer->SetMaximumNumberOfPeels(8);
   renderer->LightFollowCameraOn();
   renderer->TwoSidedLightingOn();
   renderer->SetOcclusionRatio(0.0);
}

} // end anon namespace

int TestDepthPeelingPassViewport(int, char*[])
{
  vtkNew<vtkSphereSource> sphere;
  sphere->SetRadius(10);

  vtkNew<vtkRenderer> renderer;
  InitRenderer(renderer.Get());

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetAlphaBitPlanes(1);
  renWin->SetMultiSamples(0);
  renWin->AddRenderer(renderer.Get());

  vtkNew<vtkRenderer> renderer2;
  InitRenderer(renderer2.Get());
  renderer2->SetViewport(0.0, 0.1, 0.2, 0.3);
  renderer2->InteractiveOff();
  renWin->AddRenderer(renderer2.Get());

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.Get());

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(sphere->GetOutputPort());

  {
    vtkNew<vtkActor> actor;
    actor->SetMapper(mapper.Get());
    actor->GetProperty()->SetOpacity(0.35);
    actor->SetPosition(0.0, 0.0, 1.0);
    renderer->AddActor(actor.Get());
  }

  {
    vtkNew<vtkActor> actor;
    actor->SetMapper(mapper.Get());
    vtkProperty* prop = actor->GetProperty();
    prop->SetAmbientColor(1.0, 0.0, 0.0);
    prop->SetDiffuseColor(1.0, 0.8, 0.3);
    prop->SetSpecular(0.0);
    prop->SetDiffuse(0.5);
    prop->SetAmbient(0.3);
    renderer2->AddActor(actor.Get());
  }
  {
    vtkNew<vtkActor> actor;
    actor->SetMapper(mapper.Get());
    actor->GetProperty()->SetOpacity(0.35);
    actor->SetPosition(10.0, 0.0, 0.0);
    renderer2->AddActor(actor.Get());
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
