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
int TestAxes4(int, char*[])
{
  int status = EXIT_SUCCESS;

  vtkNew<vtkContextView> view;
  view->GetRenderWindow()->SetSize(300, 200);

  vtkNew<vtkAxis> axis1;

  axis1->SetPoint1(vtkVector2f(16, 30));
  axis1->SetPoint2(vtkVector2f(184, 30));

  axis1->SetVerticalLabels(true);
  axis1->SetRange(100, 500);
  axis1->SetRangeLabelsVisible(true);
  axis1->SetPosition(vtkAxis::TOP);
  view->GetScene()->AddItem(axis1);
  axis1->Update();

  vtkNew<vtkAxis> axis2;

  axis2->SetPoint1(vtkVector2f(16, 120));
  axis2->SetPoint2(vtkVector2f(184, 120));

  axis2->SetVerticalLabels(true);
  axis2->SetRange(100, 500);
  axis2->SetRangeLabelsVisible(true);
  axis2->SetPosition(vtkAxis::BOTTOM);
  view->GetScene()->AddItem(axis2);

  axis2->Update();

  vtkNew<vtkAxis> axis3;

  axis3->SetPoint1(vtkVector2f(230, 16));
  axis3->SetPoint2(vtkVector2f(230, 184));

  axis3->SetVerticalLabels(true);
  axis3->SetRange(100, 500);
  axis3->SetRangeLabelsVisible(true);
  axis3->SetPosition(vtkAxis::LEFT);
  view->GetScene()->AddItem(axis3);
  axis3->Update();

  vtkNew<vtkAxis> axis4;

  axis4->SetPoint1(vtkVector2f(260, 16));
  axis4->SetPoint2(vtkVector2f(260, 184));

  axis4->SetVerticalLabels(true);
  axis4->SetRange(100, 500);
  axis4->SetRangeLabelsVisible(true);
  axis4->SetPosition(vtkAxis::RIGHT);
  view->GetScene()->AddItem(axis4);

  axis4->Update();

  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();

  return status;
}
