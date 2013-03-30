/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestMathTextFreeTypeTextRendererNoMath.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkTextRenderer.h"

#include "vtkNew.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkStdString.h"
#include "vtkTextActor.h"
#include "vtkTextProperty.h"

#include <iostream>

//----------------------------------------------------------------------------
int TestMathTextFreeTypeTextRendererNoMath(int , char *[])
{
  vtkNew<vtkTextRenderer> tren;
  if (tren.GetPointer() == NULL)
    {
    std::cerr << "Object factory cannot find vtkTextRenderer override.\n";
    return EXIT_FAILURE;
    }

  if (strcmp(tren->GetClassName(), "vtkMathTextFreeTypeTextRenderer") != 0)
    {
    std::cerr << "Object factory returning unrecognized vtkTextRenderer "
                 "override: " << tren->GetClassName() << std::endl;
    return EXIT_FAILURE;
    }

  vtkStdString str = "Sample multiline\ntext rendered\nusing FreeTypeTools.";

  vtkNew<vtkTextActor> actor1;
  actor1->GetTextProperty()->SetFontSize(20);
  actor1->GetTextProperty()->SetColor(1.0, 0.0, 0.0);
  actor1->GetTextProperty()->SetJustificationToLeft();
  actor1->GetTextProperty()->SetVerticalJustificationToTop();
  actor1->GetTextProperty()->SetFontFamilyToTimes();
  actor1->SetInput(str.c_str());
  actor1->SetPosition(10, 590);

  vtkNew<vtkTextActor> actor2;
  actor2->GetTextProperty()->SetFontSize(20);
  actor2->GetTextProperty()->SetColor(0.0, 1.0, 0.0);
  actor2->GetTextProperty()->SetJustificationToRight();
  actor2->GetTextProperty()->SetVerticalJustificationToTop();
  actor2->GetTextProperty()->SetFontFamilyToCourier();
  actor2->SetInput(str.c_str());
  actor2->SetPosition(590, 590);

  vtkNew<vtkTextActor> actor3;
  actor3->GetTextProperty()->SetFontSize(20);
  actor3->GetTextProperty()->SetColor(0.0, 0.0, 1.0);
  actor3->GetTextProperty()->SetJustificationToLeft();
  actor3->GetTextProperty()->SetVerticalJustificationToBottom();
  actor3->GetTextProperty()->SetItalic(1);
  actor3->SetInput(str.c_str());
  actor3->SetPosition(10, 10);

  vtkNew<vtkTextActor> actor4;
  actor4->GetTextProperty()->SetFontSize(20);
  actor4->GetTextProperty()->SetColor(0.3, 0.4, 0.5);
  actor4->GetTextProperty()->SetJustificationToRight();
  actor4->GetTextProperty()->SetVerticalJustificationToBottom();
  actor4->GetTextProperty()->SetBold(1);
  actor4->GetTextProperty()->SetShadow(1);
  actor4->GetTextProperty()->SetShadowOffset(-3, 2);
  actor4->SetInput(str.c_str());
  actor4->SetPosition(590, 10);

  vtkNew<vtkTextActor> actor5;
  actor5->GetTextProperty()->SetFontSize(20);
  actor5->GetTextProperty()->SetColor(1.0, 1.0, 0.0);
  actor5->GetTextProperty()->SetJustificationToCentered();
  actor5->GetTextProperty()->SetVerticalJustificationToCentered();
  actor5->GetTextProperty()->SetBold(1);
  actor5->GetTextProperty()->SetItalic(1);
  actor5->GetTextProperty()->SetShadow(1);
  actor5->GetTextProperty()->SetShadowOffset(5, -8);
  actor5->SetInput(str.c_str());
  actor5->SetPosition(300, 300);

  vtkNew<vtkTextActor> actor6;
  actor6->GetTextProperty()->SetFontSize(16);
  actor6->GetTextProperty()->SetColor(1.0, 0.5, 0.2);
  actor6->GetTextProperty()->SetJustificationToCentered();
  actor6->GetTextProperty()->SetVerticalJustificationToCentered();
  actor6->GetTextProperty()->SetOrientation(45);
  actor6->SetInput(str.c_str());
  actor6->SetPosition(300, 450);

  vtkNew<vtkTextActor> actor7;
  actor7->GetTextProperty()->SetFontSize(16);
  actor7->GetTextProperty()->SetColor(0.5, 0.2, 1.0);
  actor7->GetTextProperty()->SetJustificationToLeft();
  actor7->GetTextProperty()->SetVerticalJustificationToCentered();
  actor7->GetTextProperty()->SetOrientation(45);
  actor7->SetInput(str.c_str());
  actor7->SetPosition(100, 200);

  vtkNew<vtkTextActor> actor8;
  actor8->GetTextProperty()->SetFontSize(16);
  actor8->GetTextProperty()->SetColor(0.8, 1.0, 0.3);
  actor8->GetTextProperty()->SetJustificationToRight();
  actor8->GetTextProperty()->SetVerticalJustificationToCentered();
  actor8->GetTextProperty()->SetOrientation(45);
  actor8->SetInput(str.c_str());
  actor8->SetPosition(500, 200);

  // Numbers, using courier, Text that gets 'cut off'
  vtkNew<vtkTextActor> actor9;
  actor9->GetTextProperty()->SetFontSize(21);
  actor9->GetTextProperty()->SetColor(1.0, 0.0, 0.0);
  actor9->GetTextProperty()->SetJustificationToCentered();
  actor9->GetTextProperty()->SetVerticalJustificationToCentered();
  actor9->GetTextProperty()->SetBold(1);
  actor9->GetTextProperty()->SetItalic(1);
  actor9->GetTextProperty()->SetFontFamilyToCourier();
  actor9->SetInput("4.0");
  actor9->SetPosition(500, 400);

  // Boring rendering setup....

  vtkNew<vtkRenderer> ren;
  ren->SetBackground(0.1, 0.1, 0.1);
  vtkNew<vtkRenderWindow> win;
  win->SetSize(600, 600);
  win->AddRenderer(ren.GetPointer());
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(win.GetPointer());

  ren->AddActor(actor1.GetPointer());
  ren->AddActor(actor2.GetPointer());
  ren->AddActor(actor3.GetPointer());
  ren->AddActor(actor4.GetPointer());
  ren->AddActor(actor5.GetPointer());
  ren->AddActor(actor6.GetPointer());
  ren->AddActor(actor7.GetPointer());
  ren->AddActor(actor8.GetPointer());
  ren->AddActor(actor9.GetPointer());

  win->SetMultiSamples(0);
  win->Render();
  win->GetInteractor()->Initialize();
  win->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
