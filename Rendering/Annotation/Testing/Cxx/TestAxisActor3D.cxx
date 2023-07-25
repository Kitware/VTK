// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkActor.h"
#include "vtkAxisActor.h"
#include "vtkCamera.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"
#include "vtkStringArray.h"
#include "vtkTestUtilities.h"

//------------------------------------------------------------------------------
int TestAxisActor3D(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  // Create the axis actor
  vtkSmartPointer<vtkAxisActor> axis = vtkSmartPointer<vtkAxisActor>::New();
  axis->SetPoint1(0, 0, 0);
  axis->SetPoint2(1, 1, 0);
  axis->SetBounds(0, 1, 0, 0, 0, 0);
  axis->SetTickLocationToBoth();
  axis->SetAxisTypeToX();
  axis->SetTitle("1.0");
  axis->SetTitleScale(0.5);
  axis->SetTitleVisibility(true);
  axis->SetMajorTickSize(0.01);
  axis->SetRange(0, 1);

  vtkSmartPointer<vtkStringArray> labels = vtkSmartPointer<vtkStringArray>::New();
  labels->SetNumberOfTuples(1);
  labels->SetValue(0, "X");

  axis->SetLabels(labels);
  axis->SetLabelScale(.2);
  axis->MinorTicksVisibleOff();
  axis->SetDeltaMajor(0, .1);
  axis->SetCalculateTitleOffset(false);
  axis->SetCalculateLabelOffset(false);
  axis->Print(std::cout);

  vtkSmartPointer<vtkSphereSource> source = vtkSmartPointer<vtkSphereSource>::New();
  source->SetCenter(1, 1, 1);
  vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputConnection(source->GetOutputPort());

  vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);

  // Create the RenderWindow, Renderer and both Actors
  vtkSmartPointer<vtkRenderer> ren1 = vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderWindow> renWin = vtkSmartPointer<vtkRenderWindow>::New();
  renWin->AddRenderer(ren1);
  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  axis->SetCamera(ren1->GetActiveCamera());

  ren1->AddActor(actor);
  ren1->AddActor(axis);

  ren1->SetBackground(.3, .4, .5);
  renWin->SetSize(500, 200);
  ren1->ResetCamera();
  ren1->ResetCameraClippingRange();

  // render the image
  iren->Initialize();
  renWin->Render();

  iren->Start();

  return EXIT_SUCCESS;
}
