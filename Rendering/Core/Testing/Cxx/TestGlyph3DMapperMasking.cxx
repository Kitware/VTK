// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkRegressionTestImage.h"
#include "vtkTestUtilities.h"

#include "vtkActor.h"
#include "vtkArrayCalculator.h"
#include "vtkCamera.h"
#include "vtkElevationFilter.h"
#include "vtkInteractorStyleSwitch.h"
#include "vtkPlaneSource.h"
#include "vtkPointData.h"
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

int TestGlyph3DMapperMasking(int argc, char* argv[])
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
  calc->SetResultArrayName("mask");
  calc->SetResultArrayType(VTK_BIT);
  calc->AddScalarArrayName("Elevation");
  calc->SetFunction("Elevation>0.2 & Elevation<0.4");
  calc->Update();

  vtkDataSet::SafeDownCast(calc->GetOutput())->GetPointData()->GetArray("mask");
  vtkDataSet::SafeDownCast(calc->GetOutput())->GetPointData()->SetActiveScalars("Elevation");

  vtkNew<vtkActor> planeActor;
  planeActor->SetMapper(planeMapper);
  planeActor->GetProperty()->SetRepresentationToWireframe();

  // create simple poly data so we can apply glyph
  vtkNew<vtkSphereSource> squad;
  squad->SetPhiResolution(45);
  squad->SetThetaResolution(45);

#ifdef USE_FILTER
  vtkNew<vtkGlyph3D> glypher;
  glypher->SetInputConnection(colors->GetOutputPort());
#else
  vtkNew<vtkGlyph3DMapper> glypher;
  glypher->SetMasking(true);
  glypher->SetMaskArray("mask");
  glypher->SetInputConnection(calc->GetOutputPort());
  // glypher->SetInputConnection(colors->GetOutputPort());
#endif
  //  glypher->SetScaleModeToDataScalingOn();
  glypher->SetScaleFactor(0.1);

  // glypher->SetInputConnection(plane->GetOutputPort());
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
