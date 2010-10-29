/*=========================================================================

  Program:   Visualization Toolkit
  Module:    GraphItem.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkContextView.h"
#include "vtkContextScene.h"
#include "vtkPiecewiseFunctionItem.h"
#include "vtkPiecewiseControlPointsItem.h"
#include "vtkPiecewiseFunction.h"

//----------------------------------------------------------------------------
int main(int, char *[])
{
  // Set up a 2D context view, context test object and add it to the scene
  vtkSmartPointer<vtkContextView> view = vtkSmartPointer<vtkContextView>::New();
  view->GetRenderer()->SetBackground(1.0, 1.0, 1.0);
  view->GetRenderWindow()->SetSize(800, 600);
  view->GetRenderWindow()->SetMultiSamples(0);

  vtkSmartPointer<vtkPiecewiseFunction> source = vtkSmartPointer<vtkPiecewiseFunction>::New();
  source->AddPoint(0,0);
  source->AddPoint(200,200);
  source->AddPoint(400,500);
  source->AddPoint(700,500);
  source->Update();
  vtkSmartPointer<vtkPiecewiseControlPointsItem> item = vtkSmartPointer<vtkPiecewiseControlPointsItem>::New();
  item->SetPiecewiseFunction(source);
  view->GetScene()->AddItem(item);

  // NOT WORKING...
  //vtkSmartPointer<vtkPiecewiseFunctionItem> item2 = vtkSmartPointer<vtkPiecewiseFunctionItem>::New();
  //item2->SetPiecewiseFunction(source);
  //view->GetScene()->AddItem(item2);

  view->GetRenderWindow()->GetInteractor()->Initialize();
  view->GetRenderWindow()->GetInteractor()->CreateOneShotTimer(10);

  view->GetRenderWindow()->GetInteractor()->Start();
}
