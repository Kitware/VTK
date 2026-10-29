// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <vtkCamera.h>
#include <vtkDataSetSurfaceFilter.h>
#include <vtkNew.h>
#include <vtkPlaneSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkRotationalExtrusionFilter.h>

//------------------------------------------------------------------------------
int TestRotationalExtrusionVolume(int argc, char* argv[])
{

  // -------- Setup data -------------
  // Create a plane source
  vtkNew<vtkPlaneSource> plane;
  plane->SetOrigin(0., 0., 0.);
  plane->SetPoint1(1., 0., 0.);
  plane->SetPoint2(0., 0., 1.);
  plane->SetResolution(5, 5);

  // Create 1/2 of a cylinder by rotational extrusion
  vtkNew<vtkRotationalExtrusionFilter> extrusionFilter;
  extrusionFilter->SetInputConnection(plane->GetOutputPort());
  extrusionFilter->SetAngle(180);
  extrusionFilter->SetResolution(12);
  extrusionFilter->SetVolumetric(true);
  extrusionFilter->SetRotationAxis(0., 0., 1.);
  extrusionFilter->Update();

  // Extracts surface
  vtkNew<vtkDataSetSurfaceFilter> surfaceFilter;
  surfaceFilter->SetInputConnection(extrusionFilter->GetOutputPort());
  // ----------------------------------

  // ----------- Setup render -------------
  // Create polydata mapper for wireframe representation
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(surfaceFilter->GetOutputPort());

  // Create actor
  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  // actor->GetProperty()->SetRepresentationToWireframe();
  actor->GetProperty()->SetColor(1., 0., 0.);

  // Create actor for wireframe representation
  vtkNew<vtkActor> actorW;
  actorW->SetMapper(mapper);
  actorW->GetProperty()->SetRepresentationToWireframe();
  actorW->GetProperty()->SetColor(0., 0., 1.);

  // Create a renderer, add actors to it
  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor);
  renderer->AddActor(actorW);
  renderer->SetBackground(1., 1., 1.);

  // Create a good view angle
  vtkNew<vtkCamera> camera;
  camera->SetPosition(0., 3., 5.);
  camera->SetViewUp(0., 1., 0.);
  renderer->SetActiveCamera(camera);

  // Create a renderWindow
  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);
  renWin->SetSize(300, 300);
  renWin->SetMultiSamples(0);

  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renWin);

  // Render and test
  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
  }

  return retVal ? EXIT_SUCCESS : EXIT_FAILURE;
}
