/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestAxes2.cxx

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
#include "vtkTextProperty.h"

#include <vector>

//----------------------------------------------------------------------------
int TestAxes2(int , char * [])
{
  int status = EXIT_SUCCESS;

  // Set up a 2D scene, add an XY chart to it
  vtkNew<vtkContextView> view;
  view->GetRenderWindow()->SetSize(200, 200);

  vtkNew<vtkAxis> axisVertical;

  axisVertical->LogScaleOn();
  axisVertical->SetPoint1(vtkVector2f(180, 16));
  axisVertical->SetPoint2(vtkVector2f(180, 184));
  axisVertical->SetPosition(vtkAxis::LEFT);

  // Exercise some of the API in the axis API.

  axisVertical->SetNotation(vtkAxis::SCIENTIFIC_NOTATION);
  axisVertical->SetPosition(vtkAxis::LEFT);
  axisVertical->SetPrecision(0);
  axisVertical->SetRange(0.1, 1000000.);
  axisVertical->SetRangeLabelsVisible(true);
  axisVertical->GetLabelProperties()->SetFontSize(24);

  view->GetScene()->AddItem(axisVertical);

  axisVertical->Update();


  // Let's create a few axes, and place them on the scene.

  // Finally render the scene and compare the image to a reference image, or
  // start the main interactor loop if the test is interactive.
  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();

  return status;
}
