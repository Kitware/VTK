/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestFreeTypeTextMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkTextMapper.h"

#include "vtkActor2D.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOverrideInformation.h"
#include "vtkOverrideInformationCollection.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkStdString.h"
#include "vtkTextProperty.h"

//----------------------------------------------------------------------------
int TestFreeTypeTextMapperNoMath(int argc, char *argv[])
{
  if (argc < 2)
  {
    cerr << "Missing font filename." << endl;
    return EXIT_FAILURE;
  }

  vtkStdString uncodeFontFile(argv[1]);
  vtkStdString str = "Sample multiline\ntext rendered\nusing FreeTypeTools.";

  vtkNew<vtkTextMapper> mapper1;
  vtkNew<vtkActor2D> actor1;
  actor1->SetMapper(mapper1.GetPointer());
  mapper1->GetTextProperty()->SetFontSize(20);
  mapper1->GetTextProperty()->SetColor(1.0, 0.0, 0.0);
  mapper1->GetTextProperty()->SetJustificationToLeft();
  mapper1->GetTextProperty()->SetVerticalJustificationToTop();
  mapper1->GetTextProperty()->SetFontFamilyToTimes();
  mapper1->SetInput(str.c_str());
  actor1->SetPosition(10, 590);

  vtkNew<vtkTextMapper> mapper2;
  vtkNew<vtkActor2D> actor2;
  actor2->SetMapper(mapper2.GetPointer());
  mapper2->GetTextProperty()->SetFontSize(20);
  mapper2->GetTextProperty()->SetColor(0.0, 1.0, 0.0);
  mapper2->GetTextProperty()->SetJustificationToRight();
  mapper2->GetTextProperty()->SetVerticalJustificationToTop();
  mapper2->GetTextProperty()->SetFontFamilyToCourier();
  mapper2->SetInput(str.c_str());
  actor2->SetPosition(590, 590);

  vtkNew<vtkTextMapper> mapper3;
  vtkNew<vtkActor2D> actor3;
  actor3->SetMapper(mapper3.GetPointer());
  mapper3->GetTextProperty()->SetFontSize(20);
  mapper3->GetTextProperty()->SetColor(0.0, 0.0, 1.0);
  mapper3->GetTextProperty()->SetJustificationToLeft();
  mapper3->GetTextProperty()->SetVerticalJustificationToBottom();
  mapper3->GetTextProperty()->SetItalic(1);
  mapper3->SetInput(str.c_str());
  actor3->SetPosition(10, 10);

  vtkNew<vtkTextMapper> mapper4;
  vtkNew<vtkActor2D> actor4;
  actor4->SetMapper(mapper4.GetPointer());
  mapper4->GetTextProperty()->SetFontSize(20);
  mapper4->GetTextProperty()->SetColor(0.3, 0.4, 0.5);
  mapper4->GetTextProperty()->SetJustificationToRight();
  mapper4->GetTextProperty()->SetVerticalJustificationToBottom();
  mapper4->GetTextProperty()->SetBold(1);
  mapper4->GetTextProperty()->SetShadow(1);
  mapper4->GetTextProperty()->SetShadowOffset(-3, 2);
  mapper4->SetInput(str.c_str());
  actor4->SetPosition(590, 10);

  vtkNew<vtkTextMapper> mapper5;
  vtkNew<vtkActor2D> actor5;
  actor5->SetMapper(mapper5.GetPointer());
  mapper5->GetTextProperty()->SetFontSize(20);
  mapper5->GetTextProperty()->SetColor(1.0, 1.0, 0.0);
  mapper5->GetTextProperty()->SetJustificationToCentered();
  mapper5->GetTextProperty()->SetVerticalJustificationToCentered();
  mapper5->GetTextProperty()->SetBold(1);
  mapper5->GetTextProperty()->SetItalic(1);
  mapper5->GetTextProperty()->SetShadow(1);
  mapper5->GetTextProperty()->SetShadowOffset(5, -8);
  mapper5->SetInput(str.c_str());
  actor5->SetPosition(300, 300);

  vtkNew<vtkTextMapper> mapper6;
  vtkNew<vtkActor2D> actor6;
  actor6->SetMapper(mapper6.GetPointer());
  mapper6->GetTextProperty()->SetFontSize(16);
  mapper6->GetTextProperty()->SetColor(1.0, 0.5, 0.2);
  mapper6->GetTextProperty()->SetJustificationToCentered();
  mapper6->GetTextProperty()->SetVerticalJustificationToCentered();
  mapper6->GetTextProperty()->SetOrientation(45);
  mapper6->SetInput(str.c_str());
  actor6->SetPosition(300, 450);

  vtkNew<vtkTextMapper> mapper7;
  vtkNew<vtkActor2D> actor7;
  actor7->SetMapper(mapper7.GetPointer());
  mapper7->GetTextProperty()->SetFontSize(16);
  mapper7->GetTextProperty()->SetColor(0.5, 0.2, 1.0);
  mapper7->GetTextProperty()->SetJustificationToLeft();
  mapper7->GetTextProperty()->SetVerticalJustificationToCentered();
  mapper7->GetTextProperty()->SetOrientation(45);
  mapper7->SetInput(str.c_str());
  actor7->SetPosition(100, 200);

  vtkNew<vtkTextMapper> mapper8;
  vtkNew<vtkActor2D> actor8;
  actor8->SetMapper(mapper8.GetPointer());
  mapper8->GetTextProperty()->SetFontSize(16);
  mapper8->GetTextProperty()->SetColor(0.8, 1.0, 0.3);
  mapper8->GetTextProperty()->SetJustificationToRight();
  mapper8->GetTextProperty()->SetVerticalJustificationToCentered();
  mapper8->GetTextProperty()->SetOrientation(45);
  mapper8->SetInput(str.c_str());
  actor8->SetPosition(500, 200);

  // Numbers, using courier, Text that gets 'cut off'
  vtkNew<vtkTextMapper> mapper9;
  vtkNew<vtkActor2D> actor9;
  actor9->SetMapper(mapper9.GetPointer());
  mapper9->GetTextProperty()->SetFontSize(21);
  mapper9->GetTextProperty()->SetColor(1.0, 0.0, 0.0);
  mapper9->GetTextProperty()->SetJustificationToCentered();
  mapper9->GetTextProperty()->SetVerticalJustificationToCentered();
  mapper9->GetTextProperty()->SetBold(1);
  mapper9->GetTextProperty()->SetItalic(1);
  mapper9->GetTextProperty()->SetFontFamilyToCourier();
  mapper9->SetInput("4.0");
  actor9->SetPosition(500, 400);

  // UTF-8 freetype handling:
  vtkNew<vtkTextMapper> mapper10;
  vtkNew<vtkActor2D> actor10;
  actor10->SetMapper(mapper10.GetPointer());
  mapper10->GetTextProperty()->SetFontFile(uncodeFontFile.c_str());
  mapper10->GetTextProperty()->SetFontFamily(VTK_FONT_FILE);
  mapper10->GetTextProperty()->SetJustificationToCentered();
  mapper10->GetTextProperty()->SetVerticalJustificationToCentered();
  mapper10->GetTextProperty()->SetFontSize(18);
  mapper10->GetTextProperty()->SetColor(0.0, 1.0, 0.7);
  mapper10->SetInput("UTF-8 FreeType: \xce\xa8\xd2\x94\xd2\x96\xd1\x84\xd2\xbe");
  actor10->SetPosition(300, 110);

  // Test for rotated kerning (PR#15301)
  vtkNew<vtkTextMapper> mapper11;
  vtkNew<vtkActor2D> actor11;
  actor11->SetMapper(mapper11.GetPointer());
  mapper11->GetTextProperty()->SetFontFile(uncodeFontFile.c_str());
  mapper11->GetTextProperty()->SetFontFamily(VTK_FONT_FILE);
  mapper11->GetTextProperty()->SetJustificationToCentered();
  mapper11->GetTextProperty()->SetVerticalJustificationToCentered();
  mapper11->GetTextProperty()->SetFontSize(18);
  mapper11->GetTextProperty()->SetOrientation(90);
  mapper11->GetTextProperty()->SetColor(0.0, 1.0, 0.7);
  mapper11->SetInput("oTeVaVoVAW");
  actor11->SetPosition(300, 200);

  // Empty string, solid background: should not render
  vtkNew<vtkTextMapper> mapper12;
  vtkNew<vtkActor2D> actor12;
  actor12->SetMapper(mapper12.GetPointer());
  mapper12->GetTextProperty()->SetFontSize(16);
  mapper12->GetTextProperty()->SetColor(1.0, 0.0, 0.0);
  mapper12->GetTextProperty()->SetBackgroundColor(1.0, 0.5, 1.0);
  mapper12->GetTextProperty()->SetBackgroundOpacity(1.0);
  mapper12->GetTextProperty()->SetJustificationToRight();
  mapper12->GetTextProperty()->SetVerticalJustificationToCentered();
  mapper12->SetInput("");
  actor12->SetPosition(0, 0);

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
  ren->AddActor(actor10.GetPointer());
  ren->AddActor(actor11.GetPointer());
  ren->AddActor(actor12.GetPointer());

  win->SetMultiSamples(0);
  win->Render();
  win->GetInteractor()->Initialize();
  win->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
