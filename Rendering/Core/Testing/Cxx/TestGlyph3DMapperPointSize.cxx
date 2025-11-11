// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkRegressionTestImage.h"
#include "vtkTestUtilities.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkElevationFilter.h"
#include "vtkGlyph3DMapper.h"
#include "vtkNew.h"
#include "vtkPlane.h"
#include "vtkPlaneSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSuperquadricSource.h"

int TestGlyph3DMapperPointSize(int argc, char* argv[])
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
  vtkNew<vtkSuperquadricSource> squad;
  vtkNew<vtkGlyph3DMapper> glypher;
  glypher->SetInputConnection(colors->GetOutputPort());
  glypher->SetSourceConnection(squad->GetOutputPort());

  vtkNew<vtkActor> glyphActor;
  glyphActor->SetMapper(glypher);
  glyphActor->GetProperty()->SetRepresentationToPoints();
  glyphActor->GetProperty()->SetPointSize(10.0);

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

  win->Render();

  int retVal = vtkRegressionTestImage(win);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
