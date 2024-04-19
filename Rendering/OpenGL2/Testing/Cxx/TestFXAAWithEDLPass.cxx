// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Description
// Test FXAA with EDL pass

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCylinderSource.h"
#include "vtkEDLShading.h"
#include "vtkNew.h"
#include "vtkOpenGLFXAAPass.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderStepsPass.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTesting.h"

int TestFXAAWithEDLPass(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  vtkNew<vtkCylinderSource> cylinder;
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(cylinder->GetOutputPort());
  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  vtkNew<vtkRenderer> ren;
  ren->SetBackground(1.0, 1.0, 1.0);
  ren->AddActor(actor);
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  renWin->AddRenderer(ren);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  vtkNew<vtkRenderStepsPass> basicPasses;
  vtkNew<vtkEDLShading> edl;
  edl->SetDelegatePass(basicPasses);
  vtkNew<vtkOpenGLFXAAPass> fxaa;
  fxaa->SetDelegatePass(edl);
  ren->SetPass(fxaa);

  ren->ResetCamera();
  auto cam = ren->GetActiveCamera();
  cam->SetViewUp(-0.45365, 0.78693, -0.418262);
  cam->SetPosition(-0.388464, 0.574701, 0.0925649);
  cam->SetFocalPoint(-0.50418, 0.453051, -0.0108049);
  ren->ResetCameraClippingRange();

  return vtkTesting::InteractorEventLoop(argc, argv, iren, nullptr);
}
