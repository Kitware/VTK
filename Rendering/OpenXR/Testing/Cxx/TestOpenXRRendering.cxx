/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOpenGLPolyDataMapper.h"
#include "vtkOpenXRCamera.h"
#include "vtkOpenXRRenderWindow.h"
#include "vtkOpenXRRenderWindowInteractor.h"
#include "vtkOpenXRRenderer.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkSphereSource.h"

//------------------------------------------------------------------------------
// Render a non-opaque sphere with a background
// ensuring the generated image looks the same
// whatever the viewpoint
int TestOpenXRRendering(int argc, char* argv[])
{
  vtkNew<vtkOpenXRRenderer> renderer;
  vtkNew<vtkOpenXRRenderWindow> renderWindow;
  vtkNew<vtkOpenXRCamera> cam;
  vtkNew<vtkOpenXRRenderWindowInteractor> iren;
  vtkNew<vtkActor> actor;

  renderer->SetBackground(0.2, 0.3, 0.4);
  renderer->SetActiveCamera(cam);
  renderer->AddActor(actor);
  renderWindow->AddRenderer(renderer);
  iren->SetRenderWindow(renderWindow);
  iren->SetActionManifestDirectory("../../");

  vtkNew<vtkSphereSource> sphere;
  sphere->SetPhiResolution(80);
  sphere->SetThetaResolution(80);
  sphere->SetRadius(100);
  sphere->Update();

  vtkNew<vtkOpenGLPolyDataMapper> mapper;
  mapper->SetInputConnection(sphere->GetOutputPort());
  actor->SetMapper(mapper);
  actor->GetProperty()->SetOpacity(0.5);

  iren->Initialize();
  iren->DoOneEvent(renderWindow, renderer);
  iren->DoOneEvent(renderWindow, renderer); // Needed by monado so that it starts to render

  renderWindow->Render();
  int retVal = vtkRegressionTester::Test(argc, argv, renderWindow, 10);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return 0;
}
