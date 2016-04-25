/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPolarAxes.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .SECTION Thanks
// This test was written by Philippe Pebay, Kitware SAS 2011

#include "vtkBYUReader.h"
#include "vtkCamera.h"
#include "vtkLight.h"
#include "vtkLODActor.h"
#include "vtkNew.h"
#include "vtkOutlineFilter.h"
#include "vtkPolarAxesActor.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyDataNormals.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkTextProperty.h"

#include "vtkTestUtilities.h"

//----------------------------------------------------------------------------
int TestPolarAxes(int argc, char* argv[])
{
  vtkNew<vtkBYUReader> reader;
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/teapot.g");
  reader->SetGeometryFileName(fname);
  delete [] fname;

  vtkNew<vtkPolyDataNormals> normals;
  normals->SetInputConnection(reader->GetOutputPort());

  vtkNew<vtkPolyDataMapper> readerMapper;
  readerMapper->SetInputConnection(normals->GetOutputPort());

  vtkNew<vtkLODActor> readerActor;
  readerActor->SetMapper(readerMapper.GetPointer());
  readerActor->GetProperty()->SetDiffuseColor(.5, .8, .3);

  vtkNew<vtkOutlineFilter> outline;
  outline->SetInputConnection(normals->GetOutputPort());

  vtkNew<vtkPolyDataMapper> mapOutline;
  mapOutline->SetInputConnection(outline->GetOutputPort());

  vtkNew<vtkActor> outlineActor;
  outlineActor->SetMapper(mapOutline.GetPointer());
  outlineActor->GetProperty()->SetColor(1., 1., 1.);

  vtkNew<vtkCamera> camera;
  camera->SetClippingRange(1., 100.);
  camera->SetFocalPoint(0., .5, 0.);
  camera->SetPosition(5., 6., 14.);

  vtkNew<vtkLight> light;
  light->SetFocalPoint(0.21406, 1.5, 0.0);
  light->SetPosition(7., 7., 4.);

  vtkNew<vtkRenderer> renderer;
  renderer->SetActiveCamera(camera.GetPointer());
  renderer->AddLight(light.GetPointer());

  // Update normals in order to get correct bounds for polar axes
  normals->Update();

  vtkNew<vtkPolarAxesActor> polaxes;
  polaxes->SetBounds(normals->GetOutput()->GetBounds());
  polaxes->SetPole(.5, 1., 3.);
  polaxes->SetAutoScaleRadius(false);
  polaxes->SetMaximumRadius(3.);
  polaxes->SetMinimumAngle(-60.);
  polaxes->SetMaximumAngle(210.);
  polaxes->SetNumberOfRadialAxes(10);
  polaxes->SetCamera(renderer->GetActiveCamera());
  polaxes->SetPolarLabelFormat("%6.1f");
  polaxes->GetLastRadialAxisProperty()->SetColor(.0, .0, 1.);
  polaxes->GetSecondaryRadialAxesProperty()->SetColor(.0, .0, 1.);
  polaxes->GetPolarArcsProperty()->SetColor(1., .0, 0.);
  polaxes->GetSecondaryPolarArcsProperty()->SetColor(1., 1., 1.);
  polaxes->GetPolarAxisProperty()->SetColor(.2, .2, .2);
  polaxes->GetPolarAxisTitleTextProperty()->SetColor(.2, .2, .2);
  polaxes->GetPolarAxisLabelTextProperty()->SetColor(.2, .2, .2);
  polaxes->SetNumberOfPolarAxisTicks(9);
  polaxes->SetAutoSubdividePolarAxis(false);
  polaxes->SetScreenSize(9.0);

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  renWin->AddRenderer(renderer.GetPointer());
  renWin->SetWindowName("VTK - Polar Axes");
  renWin->SetSize(600, 600);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.GetPointer());

  renderer->SetBackground(.8, .8, .8);
  renderer->AddViewProp(readerActor.GetPointer());
  renderer->AddViewProp(outlineActor.GetPointer());
  renderer->AddViewProp(polaxes.GetPointer());
  renWin->Render();

  iren->Start();

  return EXIT_SUCCESS;
}
