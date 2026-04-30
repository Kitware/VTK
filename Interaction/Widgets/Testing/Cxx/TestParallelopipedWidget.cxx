// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAppendPolyData.h"
#include "vtkCommand.h"
#include "vtkConeSource.h"
#include "vtkCubeAxesActor2D.h"
#include "vtkCubeSource.h"
#include "vtkGlyph3D.h"
#include "vtkMatrix4x4.h"
#include "vtkMatrixToLinearTransform.h"
#include "vtkNew.h"
#include "vtkParallelopipedRepresentation.h"
#include "vtkParallelopipedWidget.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkTransformFilter.h"

//------------------------------------------------------------------------------
int TestParallelopipedWidget(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);
  renderer->SetBackground(0.8, 0.8, 1.0);
  renWin->SetSize(800, 600);

  vtkNew<vtkConeSource> cone;
  cone->SetResolution(6);
  vtkNew<vtkSphereSource> sphere;
  sphere->SetThetaResolution(8);
  sphere->SetPhiResolution(8);
  vtkNew<vtkGlyph3D> glyph;
  glyph->SetInputConnection(sphere->GetOutputPort());
  glyph->SetSourceConnection(cone->GetOutputPort());
  glyph->SetVectorModeToUseNormal();
  glyph->SetScaleModeToScaleByVector();
  glyph->SetScaleFactor(0.25);

  vtkNew<vtkAppendPolyData> append;
  append->AddInputConnection(glyph->GetOutputPort());
  append->AddInputConnection(sphere->GetOutputPort());
  append->Update();

  vtkNew<vtkCubeSource> cube;
  double bounds[6];
  append->GetOutput()->GetBounds(bounds);
  bounds[0] -= (bounds[1] - bounds[0]) * 0.25;
  bounds[1] += (bounds[1] - bounds[0]) * 0.25;
  bounds[2] -= (bounds[3] - bounds[2]) * 0.25;
  bounds[3] += (bounds[3] - bounds[2]) * 0.25;
  bounds[4] -= (bounds[5] - bounds[4]) * 0.25;
  bounds[5] += (bounds[5] - bounds[4]) * 0.25;
  bounds[0] = -1.0;
  bounds[1] = 1.0;
  bounds[2] = -1.0;
  bounds[3] = 1.0;
  bounds[4] = -1.0;
  bounds[5] = 1.0;
  cube->SetBounds(bounds);

  vtkNew<vtkMatrix4x4> affineMatrix;
  constexpr double m[] = { 1.0, 0.1, 0.2, 0.0, 0.1, 1.0, 0.1, 0.0, 0.2, 0.1, 1.0, 0.0, 0.0, 0.0,
    0.0, 1.0 };
  affineMatrix->DeepCopy(m);
  vtkNew<vtkMatrixToLinearTransform> transform;
  transform->SetInput(affineMatrix);
  transform->Update();
  vtkNew<vtkTransformFilter> transformFilter;
  transformFilter->SetTransform(transform);
  transformFilter->SetInputConnection(cube->GetOutputPort());
  transformFilter->Update();

  vtkNew<vtkPoints> parallelopipedPoints;
  parallelopipedPoints->DeepCopy(transformFilter->GetOutput()->GetPoints());

  transformFilter->SetInputConnection(append->GetOutputPort());
  transformFilter->Update();

  vtkNew<vtkPolyDataMapper> maceMapper;
  maceMapper->SetInputConnection(transformFilter->GetOutputPort());

  vtkNew<vtkActor> maceActor;
  maceActor->SetMapper(maceMapper);

  renderer->AddActor(maceActor);

  double parallelopipedPts[8][3];
  parallelopipedPoints->GetPoint(0, parallelopipedPts[0]);
  parallelopipedPoints->GetPoint(1, parallelopipedPts[1]);
  parallelopipedPoints->GetPoint(2, parallelopipedPts[3]);
  parallelopipedPoints->GetPoint(3, parallelopipedPts[2]);
  parallelopipedPoints->GetPoint(4, parallelopipedPts[4]);
  parallelopipedPoints->GetPoint(5, parallelopipedPts[5]);
  parallelopipedPoints->GetPoint(6, parallelopipedPts[7]);
  parallelopipedPoints->GetPoint(7, parallelopipedPts[6]);

  vtkNew<vtkParallelopipedWidget> widget;
  vtkNew<vtkParallelopipedRepresentation> rep;
  widget->SetRepresentation(rep);
  widget->SetInteractor(iren);
  rep->SetPlaceFactor(0.5);
  rep->PlaceWidget(parallelopipedPts);

  iren->Initialize();
  renWin->Render();

  widget->EnabledOn();

  vtkNew<vtkCubeAxesActor2D> axes;
  axes->SetInputConnection(transformFilter->GetOutputPort());
  axes->SetCamera(renderer->GetActiveCamera());
  axes->SetLabelFormat("{:6.1f}");
  axes->SetFlyModeToOuterEdges();
  axes->SetFontFactor(0.8);
  renderer->AddViewProp(axes);

  iren->Start();

  return EXIT_SUCCESS;
}
