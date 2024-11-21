// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

// This test draws a sphere with the edges shown.  It also turns on coincident
// topology resolution with a z-shift to both make sure the wireframe is
// visible and to exercise that type of coincident topology resolution.

#include "vtkActor.h"
#include "vtkAppendPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyLineSource.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"

int TestEdgeThickness(int argc, char* argv[])
{
  vtkMapper::SetResolveCoincidentTopologyToShiftZBuffer();
  vtkMapper::SetResolveCoincidentTopologyZShift(0.1);

  vtkNew<vtkPolyLineSource> linesLeft;
  linesLeft->SetNumberOfPoints(4);
  linesLeft->SetPoint(0, 0, -2, 0);
  linesLeft->SetPoint(1, 1, -1, 0);
  linesLeft->SetPoint(2, 0, 1, 0);
  linesLeft->SetPoint(3, 1, 2, 0);

  vtkNew<vtkSphereSource> sphere;
  sphere->SetCenter(6, 0, 0);
  sphere->SetRadius(3);

  vtkNew<vtkPolyLineSource> linesRight;
  linesRight->SetNumberOfPoints(4);
  linesRight->SetPoint(0, 12, -2, 0);
  linesRight->SetPoint(1, 11, -1, 0);
  linesRight->SetPoint(2, 12, 1, 0);
  linesRight->SetPoint(3, 11, 2, 0);

  vtkNew<vtkAppendPolyData> append;
  append->AddInputConnection(linesLeft->GetOutputPort());
  append->AddInputConnection(sphere->GetOutputPort());
  append->AddInputConnection(linesRight->GetOutputPort());

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(append->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->EdgeVisibilityOn();
  actor->GetProperty()->SetEdgeWidth(4.0);
  actor->GetProperty()->UseLineWidthForEdgeThicknessOff();
  actor->GetProperty()->SetEdgeColor(1.0, 0.0, 0.0);

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor);
  renderer->ResetCamera();

  vtkNew<vtkRenderWindow> renwin;
  renwin->AddRenderer(renderer);
  renwin->SetSize(250, 250);
  renwin->SetMultiSamples(0);

  int retVal = vtkRegressionTestImage(renwin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    vtkNew<vtkRenderWindowInteractor> iren;
    iren->SetRenderWindow(renwin);
    iren->Initialize();
    iren->Start();
    retVal = vtkRegressionTester::PASSED;
  }

  return (retVal == vtkRegressionTester::PASSED) ? 0 : 1;
}
