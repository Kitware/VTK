// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCamera.h"
#include "vtkNew.h"
#include "vtkOutlineSource.h"
#include "vtkPolarAxesActor2D.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty2D.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

//------------------------------------------------------------------------------
int TestPolarAxes2D(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  vtkNew<vtkPolarAxesActor2D> polarAxes;
  polarAxes->GetProperty()->SetColor(1, 0, 0);
  polarAxes->GetProperty()->SetLineWidth(2);

  polarAxes->SetNumberOfAxes(5);
  polarAxes->SetNumberOfAxesTicks(4);
  polarAxes->SetAxesLength(200);
  polarAxes->SetStartAngle(45);
  polarAxes->SetEndAngle(135);
  polarAxes->SetOrigin(0.5, 0.2);

  vtkNew<vtkOutlineSource> outlineSource;
  vtkNew<vtkPolyDataMapper> outlineMapper;
  outlineMapper->SetInputConnection(outlineSource->GetOutputPort());
  vtkNew<vtkActor> outlineActor;
  outlineActor->SetMapper(outlineMapper);

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(polarAxes);
  renderer->AddActor(outlineActor);
  renderer->GetActiveCamera()->ParallelProjectionOn();
  renderer->ResetCamera();

  vtkNew<vtkRenderWindow> window;
  window->SetSize(300, 300);
  window->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(window);

  interactor->Initialize();
  window->Render();
  interactor->Start();

  return EXIT_SUCCESS;
};
