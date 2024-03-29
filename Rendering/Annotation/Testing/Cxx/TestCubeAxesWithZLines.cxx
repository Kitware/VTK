// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// .SECTION Thanks
// This test was written by Philippe Pebay, Kitware SAS 2011

#include "vtkBYUReader.h"
#include "vtkCamera.h"
#include "vtkCubeAxesActor.h"
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
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkTextProperty.h"

//------------------------------------------------------------------------------
int TestCubeAxesWithZLines(int argc, char* argv[])
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
  renWin->SetWindowName("Cube Axes with Z Outer Grid Lines");
  renWin->SetSize(600, 600);
  renWin->SetMultiSamples(0);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  ren2->AddViewProp(foheActor);
  ren2->AddViewProp(outlineActor);
  ren2->SetGradientBackground(true);
  ren2->SetBackground(.1, .1, .1);
  ren2->SetBackground2(.8, .8, .8);

  normals->Update();

  vtkNew<vtkCubeAxesActor> axes2;
  axes2->SetBounds(normals->GetOutput()->GetBounds());
  axes2->SetXAxisRange(20, 300);
  axes2->SetYAxisRange(-0.01, 0.01);
  axes2->SetCamera(ren2->GetActiveCamera());
  axes2->SetXLabelFormat("%6.1f");
  axes2->SetYLabelFormat("%6.1f");
  axes2->SetZLabelFormat("%6.1f");
  axes2->SetScreenSize(15.0);
  axes2->SetFlyModeToClosestTriad();
  axes2->SetCornerOffset(0.0);

  // Draw Z (outer) grid lines
  axes2->SetDrawZGridlines(true);

  // Use bluee color for Z axis lines, gridlines, title, and labels
  axes2->GetTitleTextProperty(2)->SetColor(0., 0., 1.);
  axes2->GetLabelTextProperty(2)->SetColor(0., 0., 1.);
  axes2->GetZAxesLinesProperty()->SetColor(0., 0., 1.);
  axes2->GetZAxesGridlinesProperty()->SetColor(0., 0., 1.);

  ren2->AddViewProp(axes2);
  renWin->Render();

  int retVal = vtkRegressionTestImageThreshold(renWin, 0.05);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
