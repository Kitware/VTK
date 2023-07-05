// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include <vtkCamera.h>
#include <vtkCompositeDataGeometryFilter.h>
#include <vtkIOSSReader.h>
#include <vtkNew.h>
#include <vtkPolyDataMapper.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkTemporalInterpolator.h>
#include <vtkTestUtilities.h>

int TestTemporalInterpolator(int argc, char* argv[])
{
  vtkNew<vtkIOSSReader> reader;
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/can.ex2");
  reader->SetFileName(fname);
  delete[] fname;

  vtkNew<vtkTemporalInterpolator> interpolator;
  interpolator->SetInputConnection(reader->GetOutputPort());

  vtkNew<vtkCompositeDataGeometryFilter> geom;
  geom->SetInputConnection(interpolator->GetOutputPort());

  geom->UpdateTimeStep(0.001);

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputDataObject(geom->GetOutputDataObject(0));

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor);

  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  // Render and test
  renWin->Render();
  renderer->GetActiveCamera()->Elevation(90);

  int retVal = vtkRegressionTestImageThreshold(renWin, 10);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
