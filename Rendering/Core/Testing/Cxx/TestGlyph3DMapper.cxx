// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkRegressionTestImage.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkElevationFilter.h"
#include "vtkPlaneSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"

// If USE_FILTER is defined, glyph3D->PolyDataMapper is used instead of
// Glyph3DMapper.
// #define USE_FILTER

#ifdef USE_FILTER
#include "vtkGlyph3D.h"
#else
#include "vtkGlyph3DMapper.h"
#endif

int TestGlyph3DMapper(int argc, char* argv[])
{
  int res = 6;
  vtkNew<vtkPlaneSource> plane;
  plane->SetResolution(res, res);
  vtkNew<vtkElevationFilter> colors;
  colors->SetInputConnection(plane->GetOutputPort());
  colors->SetLowPoint(-0.25, -0.25, -0.25);
  colors->SetHighPoint(0.25, 0.25, 0.25);
  vtkNew<vtkPolyDataMapper> planeMapper;
  planeMapper->SetInputConnection(colors->GetOutputPort());

  vtkNew<vtkActor> planeActor;
  planeActor->SetMapper(planeMapper);
  planeActor->GetProperty()->SetRepresentationToWireframe();

  // create simple poly data so we can apply glyph
  vtkNew<vtkSphereSource> sphere;
  sphere->SetRadius(0.1);

#ifdef USE_FILTER
  vtkNew<vtkGlyph3D> glypher;
#else
  vtkNew<vtkGlyph3DMapper> glypher;
#endif
  glypher->SetInputConnection(colors->GetOutputPort());
  glypher->SetSourceConnection(sphere->GetOutputPort());

  // Useful code should you want to test clipping planes
  // with a glyph mapper, might should just uncomment
  // this and add a new valid image
  // vtkNew<vtkPlane> cplane;
  // cplane->SetNormal(-0.5,0.5,0);
  // cplane->SetOrigin(0.2,0,0);
  // glypher->AddClippingPlane(cplane);

#ifdef USE_FILTER
  vtkNew<vtkPolyDataMapper> glyphMapper;
  glyphMapper->SetInputConnection(glypher->GetOutputPort());
#endif

  vtkNew<vtkActor> glyphActor;
#ifdef USE_FILTER
  glyphActor->SetMapper(glyphMapper);
#else
  glyphActor->SetMapper(glypher);
#endif

  // Create the rendering stuff

  vtkNew<vtkRenderer> ren;
  vtkNew<vtkRenderWindow> win;
  win->AddRenderer(ren);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(win);

  ren->AddActor(planeActor);
  ren->AddActor(glyphActor);
  ren->SetBackground(0.5, 0.5, 0.5);
  win->SetSize(450, 450);
  win->Render();
  ren->GetActiveCamera()->Zoom(1.5);

  int retVal = vtkRegressionTestImage(win);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
