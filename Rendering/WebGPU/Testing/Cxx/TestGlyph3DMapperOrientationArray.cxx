// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkRegressionTestImage.h"
#include "vtkTestUtilities.h"

#include "vtkActor.h"
#include "vtkArrayCalculator.h"
#include "vtkCamera.h"
#include "vtkConeSource.h"
#include "vtkElevationFilter.h"
#include "vtkInteractorStyleSwitch.h"
#include "vtkPlaneSource.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

// If USE_FILTER is defined, glyph3D->PolyDataMapper is used instead of
// Glyph3DMapper.
// #define USE_FILTER

#ifdef USE_FILTER
#include "vtkGlyph3D.h"
#else
#include "vtkGlyph3DMapper.h"
#endif

int TestGlyph3DMapperOrientationArray(int argc, char* argv[])
{
  int res = 30;
  vtkNew<vtkPlaneSource> plane;
  plane->SetResolution(res, res);
  vtkNew<vtkElevationFilter> colors;
  colors->SetInputConnection(plane->GetOutputPort());
  colors->SetLowPoint(-0.25, -0.25, -0.25);
  colors->SetHighPoint(0.25, 0.25, 0.25);
  vtkNew<vtkPolyDataMapper> planeMapper;
  planeMapper->SetInputConnection(colors->GetOutputPort());

  vtkNew<vtkArrayCalculator> calc;
  calc->SetInputConnection(colors->GetOutputPort());
  calc->AddScalarVariable("x", "Elevation");
  //  calc->AddCoordinateVectorVariable("p");
  calc->SetResultArrayName("orientation");
  calc->SetResultArrayType(VTK_DOUBLE);
  calc->SetFunction("100*x*jHat");
  calc->Update();

  vtkDataSet::SafeDownCast(calc->GetOutput())->GetPointData()->SetActiveScalars("Elevation");

  vtkNew<vtkActor> planeActor;
  planeActor->SetMapper(planeMapper);
  planeActor->GetProperty()->SetRepresentationToWireframe();

  vtkNew<vtkConeSource> squad;
  squad->SetHeight(10.0);
  squad->SetRadius(1.0);
  squad->SetResolution(50);
  squad->SetDirection(0.0, 0.0, 1.0);

#ifdef USE_FILTER
  vtkNew<vtkGlyph3D> glypher;
  glypher->SetInputConnection(colors->GetOutputPort());
#else
  vtkNew<vtkGlyph3DMapper> glypher;
  glypher->SetInputConnection(calc->GetOutputPort());
  glypher->SetOrientationArray("orientation");
  glypher->SetOrientationModeToRotation();
#endif
  glypher->SetScaleFactor(0.01);

  glypher->SetSourceConnection(squad->GetOutputPort());
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
  win->SetMultiSamples(0); // make sure regression images are the same on all platforms
  win->AddRenderer(ren);
  vtkNew<vtkRenderWindowInteractor> iren;
  vtkInteractorStyleSwitch::SafeDownCast(iren->GetInteractorStyle())
    ->SetCurrentStyleToTrackballCamera();

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
