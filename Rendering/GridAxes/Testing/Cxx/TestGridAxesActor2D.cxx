// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkBYUReader.h"
#include "vtkCamera.h"
#include "vtkGridAxesActor2D.h"
#include "vtkLODActor.h"
#include "vtkLight.h"
#include "vtkNew.h"
#include "vtkOutlineFilter.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyDataNormals.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"
#include "vtkTextProperty.h"

//------------------------------------------------------------------------------
int TestGridAxesActor2D(int argc, char* argv[])
{
  vtkNew<vtkBYUReader> fohe;
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/teapot.g");
  fohe->SetGeometryFileName(fname);
  delete[] fname;

  vtkNew<vtkPolyDataNormals> normals;
  normals->SetInputConnection(fohe->GetOutputPort());

  vtkNew<vtkPolyDataMapper> foheMapper;
  foheMapper->SetInputConnection(normals->GetOutputPort());

  vtkNew<vtkLODActor> foheActor;
  foheActor->SetMapper(foheMapper);
  foheActor->GetProperty()->SetDiffuseColor(0.7, 0.3, 0.0);

  vtkNew<vtkOutlineFilter> outline;
  outline->SetInputConnection(normals->GetOutputPort());

  vtkNew<vtkPolyDataMapper> mapOutline;
  mapOutline->SetInputConnection(outline->GetOutputPort());

  vtkNew<vtkActor> outlineActor;
  outlineActor->SetMapper(mapOutline);
  outlineActor->GetProperty()->SetColor(0.0, 0.0, 0.0);

  vtkNew<vtkCamera> camera;
  camera->SetClippingRange(1.0, 100.0);
  camera->SetFocalPoint(0.9, 1.0, 0.0);
  camera->SetPosition(11.63, 6.0, 10.77);

  vtkNew<vtkLight> light;
  light->SetFocalPoint(0.21406, 1.5, 0.0);
  light->SetPosition(8.3761, 4.94858, 4.12505);

  vtkNew<vtkRenderer> ren2;
  ren2->SetActiveCamera(camera);
  ren2->AddLight(light);

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  renWin->AddRenderer(ren2);
  renWin->SetWindowName("Grid Axes 2D");
  renWin->SetSize(600, 600);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  ren2->AddViewProp(foheActor);
  ren2->AddViewProp(outlineActor);
  ren2->SetBackground(0.1, 0.2, 0.4);

  normals->Update();

  double bounds[6];
  normals->GetOutput()->GetBounds(bounds);
  vtkNew<vtkGridAxesActor2D> axes;
  axes->SetGridBounds(bounds);
  axes->GetProperty()->SetFrontfaceCulling(true);

  // Use red color for X axis
  axes->GetTitleTextProperty(0)->SetColor(1., 0., 0.);
  axes->GetLabelTextProperty(0)->SetColor(.8, 0., 0.);
  axes->SetTitle(0, "X-Axis");
  axes->SetTitle(1, "Y-Axis");
  axes->SetTitle(2, "Z-Axis");

  // Use green color for Y axis
  axes->GetTitleTextProperty(1)->SetColor(0., 1., 0.);
  axes->GetLabelTextProperty(1)->SetColor(0., .8, 0.);

  ren2->AddViewProp(axes);

  vtkNew<vtkGridAxesActor2D> axes1;
  axes1->SetGridBounds(bounds);
  axes1->SetFace(vtkGridAxesHelper::MAX_ZX);
  axes1->GetProperty()->SetFrontfaceCulling(false);

  // Use red color for X axis
  axes1->GetTitleTextProperty(0)->SetColor(1., 0., 0.);
  axes1->GetLabelTextProperty(0)->SetColor(.8, 0., 0.);
  axes1->SetTitle(0, "X-Axis");
  axes1->SetTitle(1, "Y-Axis");
  axes1->SetTitle(2, "Z-Axis");

  // Use green color for Y axis
  axes1->GetTitleTextProperty(1)->SetColor(0., 1., 0.);
  axes1->GetLabelTextProperty(1)->SetColor(0., .8, 0.);

  ren2->AddViewProp(axes1);
  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
