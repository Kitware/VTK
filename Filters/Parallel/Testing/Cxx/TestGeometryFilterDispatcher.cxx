// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Test vtkGeometryFilterDispatcher with surface extraction from a wavelet source.

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkGeometryFilterDispatcher.h"
#include "vtkNew.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRTAnalyticSource.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

int TestGeometryFilterDispatcher(int argc, char* argv[])
{
  // Wavelet source (produces vtkImageData)
  vtkNew<vtkRTAnalyticSource> wavelet;
  wavelet->SetWholeExtent(-10, 10, -10, 10, -10, 10);

  // Geometry filter dispatcher — surface extraction mode
  vtkNew<vtkGeometryFilterDispatcher> geometry;
  geometry->SetInputConnection(wavelet->GetOutputPort());
  geometry->SetUseOutline(false);

  // Mapper
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(geometry->GetOutputPort());
  mapper->SetScalarModeToUsePointFieldData();
  mapper->SelectColorArray("RTData");
  mapper->SetScalarRange(37.35, 276.83);

  // Actor
  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  // Renderer
  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor);
  renderer->SetBackground(1.0, 1.0, 1.0);
  renderer->ResetCamera();

  // Render window
  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);
  renWin->SetSize(400, 400);
  renWin->SetMultiSamples(0);

  // Interactor
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  // Render and test
  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
