/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTextActorScaleModeProp.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkTextActor.h"

#include "vtkNew.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTextProperty.h"

//----------------------------------------------------------------------------
int TestTextActorScaleModeProp(int, char *[])
{
  // Test PROP scale mode actor with a text property that's instantiated first
  // See VTK bug 15412
  vtkNew<vtkTextProperty> textProperty;
  textProperty->SetBold(1);
  textProperty->SetItalic(1);
  textProperty->SetShadow(0);
  textProperty->SetFontFamily(VTK_ARIAL);
  textProperty->SetJustification(VTK_TEXT_LEFT);
  textProperty->SetVerticalJustification(VTK_TEXT_BOTTOM);

  vtkNew<vtkTextActor> textActor;
  textActor->GetPositionCoordinate()->SetCoordinateSystemToDisplay();
  textActor->GetPositionCoordinate()->SetReferenceCoordinate(NULL);
  textActor->GetPosition2Coordinate()->SetCoordinateSystemToDisplay();
  textActor->GetPosition2Coordinate()->SetReferenceCoordinate(NULL);
  textActor->SetTextScaleModeToProp();
  textActor->SetTextProperty(textProperty.GetPointer());
  textActor->SetInput("15412");

  textActor->GetPositionCoordinate()->SetValue(20.0, 20.0, 0.0);
  textActor->GetPosition2Coordinate()->SetValue(280.0, 80.0, 0.0);

  vtkNew<vtkRenderer> ren;
  vtkNew<vtkRenderWindow> win;
  win->AddRenderer(ren.GetPointer());
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(win.GetPointer());

  ren->SetBackground(0.1, 0.1, 0.1);
  win->SetSize(300, 300);

  ren->AddActor2D(textActor.GetPointer());

  win->SetMultiSamples(0);
  win->GetInteractor()->Initialize();
  win->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
