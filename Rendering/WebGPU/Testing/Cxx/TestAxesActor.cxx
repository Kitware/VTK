// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkAxesActor.h"
#include "vtkCamera.h"
#include "vtkConeSource.h"
#include "vtkNew.h"
#include "vtkOrientationMarkerWidget.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

//------------------------------------------------------------------------------
int TestAxesActor(int argc, char* argv[])
{
  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(0.1, 0.1, 0.1);

  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(800, 800);
  renderWindow->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow);

  vtkNew<vtkConeSource> cone;
  // map elevation output to graphics primitives.
  vtkNew<vtkPolyDataMapper> mapper;
  // mapper->SetLookupTable(lut);
  mapper->SetInputConnection(cone->GetOutputPort());
  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  renderer->AddActor(actor);

  vtkNew<vtkAxesActor> axes;
  axes->SetShaftTypeToCylinder();
  axes->SetNormalizedTipLength(0.4, 0.4, 0.4);
  // FIXME: the vtkCaptionActor2D does not render text with webgpu. An override for
  // vtkPolyDataMapper2D must be implemented in webgpu.
  // https://gitlab.kitware.com/vtk/vtk/-/issues/19551
  axes->SetAxisLabels(false);
  vtkNew<vtkOrientationMarkerWidget> om;
  om->SetOrientationMarker(axes);
  om->SetInteractor(iren);
  om->EnabledOn();
  om->InteractiveOn();

  renderer->ResetCamera();
  renderer->GetActiveCamera()->Azimuth(45);
  renderer->GetActiveCamera()->Elevation(45);
  renderer->GetActiveCamera()->OrthogonalizeViewUp();
  renderWindow->Render();

  int retVal = vtkRegressionTestImageThreshold(renderWindow, 0.05);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
