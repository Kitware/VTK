/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestBarGraph.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkAxis.h"
#include "vtkContextScene.h"
#include "vtkContextView.h"
#include "vtkDoubleArray.h"
#include "vtkNew.h"
#include "vtkSmartPointer.h"
#include "vtkVector.h"
#include "vtkStringArray.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"

#include <vector>

//----------------------------------------------------------------------------
int TestAxes(int , char * [])
{
  // Set up a 2D scene, add an XY chart to it
  vtkNew<vtkContextView> view;
  view->GetRenderWindow()->SetSize(500, 300);

  // Set up our custom label arrays for the axes;
  vtkNew<vtkDoubleArray> positions;
  vtkNew<vtkStringArray> labels;

  positions->InsertNextValue(0.0);
  labels->InsertNextValue("0.0");
  positions->InsertNextValue(42.0);
  labels->InsertNextValue("The Answer");
  positions->InsertNextValue(99.0);
  labels->InsertNextValue("99");

  // Let's create a few axes, and place them on the scene.
  std::vector<vtkSmartPointer<vtkAxis> > axesVertical(4);

  for (size_t i = 0; i < axesVertical.size(); ++i)
    {
    axesVertical[i] = vtkSmartPointer<vtkAxis>::New();
    vtkAxis *axis = axesVertical[i].GetPointer();
    axis->SetPoint1(vtkVector2f(i * 69 + 30, 10));
    axis->SetPoint2(vtkVector2f(i * 69 + 30, 290));
    axis->SetPosition(i % 2 ? vtkAxis::LEFT : vtkAxis::RIGHT);
    axis->SetRange(-1, 50);

    view->GetScene()->AddItem(axis);
    }

  // Exercise some of the API in the axis API.
  axesVertical[0]->AutoScale();

  axesVertical[1]->SetBehavior(vtkAxis::FIXED);
  axesVertical[1]->AutoScale();

  axesVertical[2]->SetNotation(vtkAxis::SCIENTIFIC_NOTATION);
  axesVertical[2]->SetPosition(vtkAxis::LEFT);
  axesVertical[2]->SetPrecision(0);

  axesVertical[3]->SetTitle("Custom vertical labels");
  axesVertical[3]->SetCustomTickPositions(positions.GetPointer(),
                                          labels.GetPointer());
  axesVertical[3]->SetPoint1(vtkVector2f(3 * 69 + 80, 10));
  axesVertical[3]->SetPoint2(vtkVector2f(3 * 69 + 80, 290));
  axesVertical[3]->AutoScale();

  for (size_t i = 0; i < axesVertical.size(); ++i)
    {
    axesVertical[i]->Update();
    }

  // Let's create a few axes, and place them on the scene.
  std::vector<vtkSmartPointer<vtkAxis> > axesHorizontal(6);

  for (size_t i = 0; i < axesHorizontal.size(); ++i)
    {
    axesHorizontal[i] = vtkSmartPointer<vtkAxis>::New();
    vtkAxis *axis = axesHorizontal[i].GetPointer();
    axis->SetPoint1(vtkVector2f(310, i * 50 + 30));
    axis->SetPoint2(vtkVector2f(490, i * 50 + 30));
    axis->SetPosition(i % 2 ? vtkAxis::TOP : vtkAxis::BOTTOM);
    axis->SetRange(-1, 50);

    view->GetScene()->AddItem(axis);
    axis->Update();
    }

  // Now to test some of the API in the horizontal axes.
  axesHorizontal[0]->AutoScale();

  axesHorizontal[1]->SetRange(10, -5);
  axesHorizontal[1]->AutoScale();

  axesHorizontal[2]->SetRange(10, -5);
  axesHorizontal[2]->SetBehavior(vtkAxis::FIXED);
  axesHorizontal[2]->AutoScale();
  axesHorizontal[2]->SetTitle("Test");

  axesHorizontal[3]->SetTickLabelAlgorithm(vtkAxis::TICK_WILKINSON_EXTENDED);
  axesHorizontal[3]->AutoScale();

  axesHorizontal[4]->SetNumberOfTicks(5);

  axesHorizontal[5]->SetTitle("Custom horizontal labels");
  axesHorizontal[5]->SetCustomTickPositions(positions.GetPointer(),
                                            labels.GetPointer());
  axesHorizontal[5]->SetPosition(vtkAxis::BOTTOM);

  for (size_t i = 0; i < axesHorizontal.size(); ++i)
    {
    axesHorizontal[i]->Update();
    }

  // Finally render the scene and compare the image to a reference image, or
  // start the main interactor loop if the test is interactive.
  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
