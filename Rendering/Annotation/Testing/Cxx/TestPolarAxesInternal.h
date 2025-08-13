// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkBYUReader.h"
#include "vtkCamera.h"
#include "vtkLODActor.h"
#include "vtkLight.h"
#include "vtkNew.h"
#include "vtkOutlineFilter.h"
#include "vtkPolarAxesActor.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyDataNormals.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkTextProperty.h"

#include "vtkTestUtilities.h"

namespace
{
//------------------------------------------------------------------------------
inline void InitializeAxes(vtkPolarAxesActor* polarAxes)
{
  polarAxes->SetPole(.5, 1., 3.);
  polarAxes->SetMaximumRadius(3.);
  polarAxes->SetMinimumAngle(-60.);
  polarAxes->SetMaximumAngle(210.);
  polarAxes->SetRequestedNumberOfRadialAxes(10);
  polarAxes->SetPolarLabelFormat("{:6.1f}");
  polarAxes->GetLastRadialAxisProperty()->SetColor(0.0, 1.0, 0.0);
  polarAxes->GetSecondaryRadialAxesProperty()->SetColor(0.0, 0.0, 1.0);
  polarAxes->GetPolarArcsProperty()->SetColor(1.0, 0.0, 0.0);
  polarAxes->GetSecondaryPolarArcsProperty()->SetColor(1.0, 0.0, 1.0);
  polarAxes->GetPolarAxisProperty()->SetColor(1.0, 0.5, 0.0);
  polarAxes->GetPolarAxisTitleTextProperty()->SetColor(0.0, 0.0, 0.0);
  polarAxes->GetPolarAxisTitleTextProperty()->SetFontSize(36);
  polarAxes->GetPolarAxisLabelTextProperty()->SetColor(1.0, 1.0, 0.0);
  polarAxes->GetPolarAxisLabelTextProperty()->SetFontSize(18);
  polarAxes->GetLastRadialAxisTextProperty()->SetColor(0.0, 0.5, 0.0);
  polarAxes->GetSecondaryRadialAxesTextProperty()->SetColor(0.0, 1.0, 1.0);
  polarAxes->SetScreenSize(19.0);
}

/**
 * Create a pipeline with data and rendering. Add the polar axes in it and
 * and return the interactor.
 */
inline void CreatePolarAxesPipeline(
  int argc, char* argv[], vtkPolarAxesActor* polarAxes, vtkRenderWindowInteractor* interactor)
{
  vtkNew<vtkBYUReader> reader;
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/teapot.g");
  reader->SetGeometryFileName(fname);
  delete[] fname;

  vtkNew<vtkPolyDataNormals> normals;
  normals->SetInputConnection(reader->GetOutputPort());

  vtkNew<vtkPolyDataMapper> readerMapper;
  readerMapper->SetInputConnection(normals->GetOutputPort());

  vtkNew<vtkLODActor> readerActor;
  readerActor->SetMapper(readerMapper);
  readerActor->GetProperty()->SetDiffuseColor(.5, .8, .3);

  vtkNew<vtkOutlineFilter> outline;
  outline->SetInputConnection(normals->GetOutputPort());

  vtkNew<vtkPolyDataMapper> mapOutline;
  mapOutline->SetInputConnection(outline->GetOutputPort());

  vtkNew<vtkActor> outlineActor;
  outlineActor->SetMapper(mapOutline);
  outlineActor->GetProperty()->SetColor(1., 1., 1.);

  vtkNew<vtkCamera> camera;
  camera->SetClippingRange(1., 100.);
  camera->SetFocalPoint(0., .5, 0.);
  camera->SetPosition(5., 6., 14.);

  vtkNew<vtkLight> light;
  light->SetFocalPoint(0.21406, 1.5, 0.0);
  light->SetPosition(7., 7., 4.);

  vtkNew<vtkRenderer> renderer;
  renderer->SetActiveCamera(camera);
  renderer->AddLight(light);

  // Update normals in order to get correct bounds for polar axes
  normals->Update();

  polarAxes->SetBounds(normals->GetOutput()->GetBounds());
  polarAxes->SetCamera(renderer->GetActiveCamera());

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  renWin->AddRenderer(renderer);
  renWin->SetWindowName("VTK - Polar Axes");
  renWin->SetSize(600, 600);

  interactor->SetRenderWindow(renWin);

  renderer->SetBackground(.8, .8, .8);
  renderer->AddViewProp(readerActor);
  renderer->AddViewProp(outlineActor);
  renderer->AddViewProp(polarAxes);
  renWin->Render();
}

};
