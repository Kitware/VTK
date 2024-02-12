// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCamera.h"
#include "vtkNew.h"
#include "vtkOutlineSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty2D.h"
#include "vtkRadialGridActor2D.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTextProperty.h"

//------------------------------------------------------------------------------
int TestRadialGrid2D(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  vtkNew<vtkRadialGridActor2D> radialGrid;
  radialGrid->GetProperty()->SetColor(1, 0, 0);
  radialGrid->GetProperty()->SetLineWidth(2);
  radialGrid->GetTextProperty()->SetColor(1, 0, 1);
  radialGrid->GetTextProperty()->SetFontSize(18);
  radialGrid->GetTextProperty()->BoldOn();

  radialGrid->SetNumberOfAxes(4);
  radialGrid->SetNumberOfTicks(3);
  radialGrid->SetStartAngle(42);
  // go reverse side
  radialGrid->SetEndAngle(-87);
  // move grid
  radialGrid->SetOrigin(0.3, 0.6);
  radialGrid->SetAxesViewportLength(150);

  vtkNew<vtkOutlineSource> outlineSource;
  vtkNew<vtkPolyDataMapper> outlineMapper;
  outlineMapper->SetInputConnection(outlineSource->GetOutputPort());
  vtkNew<vtkActor> outlineActor;
  outlineActor->SetMapper(outlineMapper);

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(radialGrid);
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
