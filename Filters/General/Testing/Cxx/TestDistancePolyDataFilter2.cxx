// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <vtkActor.h>
#include <vtkArrowSource.h>
#include <vtkCamera.h>
#include <vtkDistancePolyDataFilter.h>
#include <vtkGlyph3DMapper.h>
#include <vtkNamedColors.h>
#include <vtkPointData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkScalarBarActor.h>
#include <vtkSphereSource.h>

int TestDistancePolyDataFilter2(int, char*[])
{
  vtkNew<vtkNamedColors> colors;

  vtkNew<vtkSphereSource> model1;
  model1->SetRadius(20.0);
  model1->SetStartTheta(180.0);
  model1->SetPhiResolution(11);
  model1->SetThetaResolution(11);
  model1->SetCenter(0.0, 0.0, 0.0);
  model1->Update();

  vtkNew<vtkSphereSource> model2;
  model2->SetRadius(20.0);
  model2->SetStartTheta(180.0);
  model2->SetPhiResolution(11);
  model2->SetThetaResolution(11);
  model2->SetCenter(6.0, 1.0, 2.0);
  model2->Update();

  vtkNew<vtkDistancePolyDataFilter> displacementFilter;

  displacementFilter->SetInputConnection(0, model1->GetOutputPort());
  displacementFilter->SetInputConnection(1, model2->GetOutputPort());
  displacementFilter->SignedDistanceOn();
  displacementFilter->NegateDistanceOn();
  displacementFilter->ComputeDirectionOn();
  displacementFilter->Update();

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(model1->GetOutputPort());

  auto range = displacementFilter->GetOutput()->GetPointData()->GetScalars()->GetRange();
  double lim = std::max(std::fabs(range[0]), std::fabs(range[1]));

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->SetOpacity(0.2);

  vtkNew<vtkPolyDataMapper> mapper2;
  mapper2->SetInputConnection(model2->GetOutputPort());

  vtkNew<vtkActor> actor2;
  actor2->SetMapper(mapper2);
  actor2->GetProperty()->SetColor(colors->GetColor3d("Green").GetData());
  actor2->GetProperty()->SetRepresentationToWireframe();

  vtkNew<vtkArrowSource> arrowSource;

  vtkNew<vtkGlyph3DMapper> mapper3;
  mapper3->SetInputConnection(displacementFilter->GetOutputPort());
  mapper3->SetSourceConnection(arrowSource->GetOutputPort());
  mapper3->SetScaleArray("Distance");
  mapper3->SetScalarRange(-lim, lim);
  mapper3->ScalingOn();
  mapper3->SetScaleMode(vtkGlyph3DMapper::SCALE_BY_MAGNITUDE);
  mapper3->SetColorModeToMapScalars();
  vtkNew<vtkActor> actor3;
  actor3->SetMapper(mapper3);

  vtkNew<vtkScalarBarActor> scalarBar;
  scalarBar->SetLookupTable(mapper3->GetLookupTable());
  scalarBar->SetTitle("Distance");
  scalarBar->SetNumberOfLabels(5);
  scalarBar->SetTextPad(4);
  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(colors->GetColor3d("Silver").GetData());
  renderer->SetBackground2(colors->GetColor3d("Gold").GetData());
  renderer->GradientBackgroundOn();

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetWindowName("DisplacementPolyDataFilter");
  renWin->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> renWinInteractor;
  renWinInteractor->SetRenderWindow(renWin);

  renderer->AddActor(actor);
  renderer->AddActor(actor2);
  renderer->AddActor(actor3);
  renderer->AddViewProp(scalarBar);
  renWin->Render();
  displacementFilter->Print(std::cout);

  renWinInteractor->Start();

  return EXIT_SUCCESS;
}
