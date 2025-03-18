// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef TestAxisActorInternal_h
#define TestAxisActorInternal_h

#include "vtkAxisActor.h"
#include "vtkCamera.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkStringArray.h"
#include "vtkTextProperty.h"

namespace
{
// ----------------------------------------------------------------------------
inline void InitializeAxis(vtkAxisActor* axis)
{
  axis->GetProperty()->SetAmbient(1);
  axis->GetProperty()->SetDiffuse(0);
  axis->SetPoint1(0, 0, 0);
  axis->SetExponent("+00");
  axis->SetExponentVisibility(true);
  axis->SetTitleScale(0.8);
  axis->SetLabelScale(0.5);

  vtkNew<vtkStringArray> labels;
  labels->SetNumberOfTuples(6);
  labels->SetValue(0, "0");
  labels->SetValue(1, "2");
  labels->SetValue(2, "4");
  labels->SetValue(3, "6");
  labels->SetValue(4, "8");
  labels->SetValue(5, "10");
  axis->SetLabels(labels);

  vtkNew<vtkCamera> camera;
  axis->SetCamera(camera);
}

// ----------------------------------------------------------------------------
inline void InitializeXAxis(vtkAxisActor* axis)
{
  ::InitializeAxis(axis);
  axis->SetPoint2(10, 0, 0);
  axis->SetTitle("X Axis");
  axis->SetBounds(0, 10, 0, 0, 0, 0);
  axis->SetTickLocationToBoth();
  axis->SetAxisTypeToX();
  axis->SetRange(0, 10);
  axis->SetLabelOffset(5);
  axis->SetDeltaRangeMajor(2);
  axis->SetDeltaRangeMinor(0.5);
  axis->SetExponentOffset(30);
  axis->SetTitleOffset(0, 30);

  vtkNew<vtkTextProperty> textProp1;
  textProp1->SetColor(0., 0., 1.);
  textProp1->SetOpacity(0.9);
  textProp1->SetFontSize(36);
  axis->SetTitleTextProperty(textProp1);

  vtkNew<vtkTextProperty> textProp2;
  textProp2->SetColor(1., 0., 0.);
  textProp2->SetOpacity(0.6);
  textProp2->SetFontSize(24);
  axis->SetLabelTextProperty(textProp2);

  vtkNew<vtkProperty> prop1;
  prop1->SetColor(1., 0., 1.);
  axis->SetAxisMainLineProperty(prop1);

  vtkNew<vtkProperty> prop2;
  prop2->SetColor(1., 1., 0.);
  axis->SetAxisMajorTicksProperty(prop2);

  vtkNew<vtkProperty> prop3;
  prop3->SetColor(0., 1., 1.);
  axis->SetAxisMinorTicksProperty(prop3);
}

// ----------------------------------------------------------------------------
inline void InitializeYAxis(vtkAxisActor* axis)
{
  ::InitializeAxis(axis);
  axis->SetPoint2(0, 10, 0);
  axis->SetTitle("Y Axis");
  axis->SetBounds(0, 0, 0, 10, 0, 0);
  axis->SetTickLocationToInside();
  axis->SetAxisTypeToY();
  axis->SetRange(0.1, 4000);
  axis->SetMajorRangeStart(0.1);
  axis->SetMinorRangeStart(0.1);
  axis->SetMinorTicksVisible(true);
  axis->SetTitleAlignLocation(vtkAxisActor::VTK_ALIGN_TOP);
  axis->SetTitleOffset(0, 3);
  axis->SetExponentLocation(vtkAxisActor::VTK_ALIGN_TOP);
  axis->SetExponentOffset(20);
  axis->SetLog(true);

  axis->GetCamera()->SetViewUp(1, 0, 0);

  vtkNew<vtkTextProperty> textProp2;
  textProp2->SetColor(1., 0., 0.);
  textProp2->SetOpacity(0.6);
  axis->SetTitleTextProperty(textProp2);

  vtkNew<vtkProperty> prop1;
  prop1->SetColor(1., 0., 1.);
  axis->SetAxisLinesProperty(prop1);
}

// ----------------------------------------------------------------------------
inline void InitializeZAxis(vtkAxisActor* axis)
{
  ::InitializeAxis(axis);

  axis->SetPoint2(0, 0, 10);
  axis->SetTitle("Z Axis");
  axis->SetBounds(0, 0, 0, 0, 0, 10);
  axis->SetTickLocationToOutside();
  axis->SetAxisTypeToZ();
  axis->SetRange(0, 10);
  axis->SetTitleAlignLocation(vtkAxisActor::VTK_ALIGN_POINT2);
  axis->SetExponentLocation(vtkAxisActor::VTK_ALIGN_POINT1);
  axis->SetTitleOffset(-80, -150);
  axis->SetExponentOffset(-150);
  axis->SetMajorTickSize(3);
  axis->SetMinorTickSize(1);
  axis->SetDeltaRangeMajor(2);
  axis->SetDeltaRangeMinor(0.5);

  axis->GetCamera()->SetPosition(0, 10, 0);
  axis->GetCamera()->SetViewUp(1, 0, 0);

  vtkNew<vtkTextProperty> textProp3;
  textProp3->SetColor(0., 1., 0.);
  textProp3->SetOpacity(1);
  axis->SetTitleTextProperty(textProp3);
}
}

// ----------------------------------------------------------------------------
inline int TestAxisActorInternal(vtkAxisActor* axis)
{
  vtkNew<vtkRenderer> renderer;
  renderer->SetActiveCamera(axis->GetCamera());
  renderer->AddActor(axis);
  renderer->SetBackground(.5, .5, .5);

  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer);

  renderWindow->SetSize(300, 300);
  renderWindow->SetMultiSamples(0);

  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow);

  renderWindow->Render();
  renderer->ResetCameraScreenSpace(0.8);
  renderWindow->Render();
  renderWindowInteractor->Start();

  return EXIT_SUCCESS;
}

#endif
