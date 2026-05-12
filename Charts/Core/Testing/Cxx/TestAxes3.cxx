// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAxis.h"
#include "vtkContextScene.h"
#include "vtkContextView.h"
#include "vtkNew.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTextProperty.h"

//------------------------------------------------------------------------------
int TestAxes3(int, char*[])
{
  int status = EXIT_SUCCESS;

  vtkNew<vtkContextView> view;
  view->GetRenderWindow()->SetSize(450, 100);

  vtkNew<vtkAxis> axisWithOverlap;

  axisWithOverlap->SetPoint1(vtkVector2f(26, 30));
  axisWithOverlap->SetPoint2(vtkVector2f(394, 30));

  axisWithOverlap->SetRange(0.0001, 0.0005);
  axisWithOverlap->SetRangeLabelsVisible(true);
  axisWithOverlap->SetPosition(vtkAxis::TOP);
  axisWithOverlap->GetLabelProperties()->SetFontSize(16);
  view->GetScene()->AddItem(axisWithOverlap);
  axisWithOverlap->Update();

  vtkNew<vtkAxis> axisWithoutOverlap;

  axisWithoutOverlap->SetPoint1(vtkVector2f(26, 60));
  axisWithoutOverlap->SetPoint2(vtkVector2f(394, 60));

  axisWithoutOverlap->SetOverlappingLabels(false);
  axisWithoutOverlap->SetRange(0.0001, 0.0005);
  axisWithoutOverlap->SetRangeLabelsVisible(true);
  axisWithoutOverlap->SetPosition(vtkAxis::TOP);
  axisWithoutOverlap->GetLabelProperties()->SetFontSize(16);

  view->GetScene()->AddItem(axisWithoutOverlap);

  axisWithoutOverlap->Update();

  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();

  return status;
}
