/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestFreeTypeTextMapperWithColumns.cxx

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
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkStdString.h"
#include "vtkTextProperty.h"

//------------------------------------------------------------------------------
int TestFreeTypeTextMapperWithColumns(int argc, char* argv[])
{
  if (argc < 2)
  {
    cerr << "Missing font filename." << endl;
    return EXIT_FAILURE;
  }

  vtkStdString str = "Sample multiline\ntext rendered\nwith mathText $\\sum_{i=0}^\\infty x_i$";

  vtkNew<vtkTextMapper> mapper1;
  vtkNew<vtkActor2D> actor1;
  actor1->SetMapper(mapper1);
  mapper1->GetTextProperty()->SetFontSize(16);
  mapper1->GetTextProperty()->SetColor(1.0, 0.0, 0.0);
  mapper1->GetTextProperty()->SetBackgroundColor(0.0, 1.0, 0.0);
  mapper1->GetTextProperty()->SetBackgroundOpacity(0.5);
  mapper1->GetTextProperty()->SetJustificationToLeft();
  mapper1->GetTextProperty()->SetVerticalJustificationToTop();

  mapper1->GetTextProperty()->SetFontFamilyToTimes();
  mapper1->SetInput(str.c_str());
  actor1->SetPosition(10, 100);

  vtkNew<vtkTextMapper> mapper2;
  vtkNew<vtkActor2D> actor2;
  actor2->SetMapper(mapper2);
  mapper2->GetTextProperty()->SetFontSize(16);
  mapper2->GetTextProperty()->SetColor(0.0, 1.0, 0.0);
  mapper2->GetTextProperty()->SetBackgroundColor(0.4, 0.5, 0.0);
  mapper2->GetTextProperty()->SetBackgroundOpacity(0.5);
  mapper2->GetTextProperty()->SetJustificationToRight();
  mapper2->GetTextProperty()->SetVerticalJustificationToCentered();
  mapper2->GetTextProperty()->SetLineOffset(5.0);
  mapper2->GetTextProperty()->SetFrame(true);

  mapper2->GetTextProperty()->SetFontFamilyToTimes();
  mapper2->SetInput(str.c_str());
  actor2->SetPosition(350, 60);

  str = "Cell1 | Cell2 | $\\sum_{i=0}^\\infty x_i$\n12 | $\\sum_{i=0}^\\infty x_i$ | 2345";

  vtkNew<vtkTextMapper> mapper3;
  vtkNew<vtkActor2D> actor3;
  actor3->SetMapper(mapper3);
  mapper3->GetTextProperty()->SetFontSize(16);
  mapper3->GetTextProperty()->SetColor(0.0, 0.0, 1.0);
  mapper3->GetTextProperty()->SetBackgroundColor(0.2, 0.0, 0.5);
  mapper3->GetTextProperty()->SetBackgroundOpacity(0.5);
  mapper3->GetTextProperty()->SetFrame(true);
  mapper3->GetTextProperty()->SetVerticalJustificationToCentered();
  mapper3->GetTextProperty()->SetJustificationToCentered();
  mapper3->GetTextProperty()->SetFontFamilyToCourier();
  mapper3->SetInput(str.c_str());
  actor3->SetPosition(500, 60);

  vtkNew<vtkTextMapper> mapper4;
  vtkNew<vtkActor2D> actor4;
  actor4->SetMapper(mapper4);
  mapper4->GetTextProperty()->SetFontSize(16);
  mapper4->GetTextProperty()->SetColor(1.0, 0.2, 0.0);
  mapper4->GetTextProperty()->SetBackgroundColor(0.3, 0.1, 0.7);
  mapper4->GetTextProperty()->SetBackgroundOpacity(0.5);
  mapper4->GetTextProperty()->SetJustificationToCentered();
  mapper4->GetTextProperty()->SetVerticalJustificationToBottom();
  mapper4->GetTextProperty()->SetCellOffset(30.0);
  mapper4->GetTextProperty()->SetLineOffset(10.0);
  mapper4->GetTextProperty()->SetFrame(true);

  mapper4->SetInput(str.c_str());
  actor4->SetPosition(150, 150);

  vtkNew<vtkTextMapper> mapper5;
  vtkNew<vtkActor2D> actor5;
  actor5->SetMapper(mapper5);
  mapper5->GetTextProperty()->SetFontSize(16);
  mapper5->GetTextProperty()->SetColor(1.0, 0.2, 0.0);
  mapper5->GetTextProperty()->SetBackgroundColor(0.3, 0.1, 0.7);
  mapper5->GetTextProperty()->SetBackgroundOpacity(0.5);
  mapper5->GetTextProperty()->SetJustificationToCentered();
  mapper5->GetTextProperty()->SetVerticalJustificationToTop();
  mapper5->GetTextProperty()->SetCellOffset(30.0);
  mapper5->GetTextProperty()->SetLineOffset(10.0);
  mapper5->GetTextProperty()->SetLineSpacing(1.5);
  mapper5->GetTextProperty()->SetOrientation(50);

  mapper5->SetInput(str.c_str());
  actor5->SetPosition(400, 320);

  str = "1|2|3|4\n1|2|3";

  vtkNew<vtkTextMapper> mapper6;
  vtkNew<vtkActor2D> actor6;
  actor6->SetMapper(mapper6);
  mapper6->GetTextProperty()->SetFontSize(16);
  mapper6->GetTextProperty()->SetColor(0.3, 0.9, 0.5);
  mapper6->GetTextProperty()->SetBackgroundColor(0.0, 1.0, 0.0);
  mapper6->GetTextProperty()->SetBackgroundOpacity(0.5);
  mapper6->GetTextProperty()->SetJustificationToCentered();
  mapper6->GetTextProperty()->SetVerticalJustificationToCentered();
  mapper6->GetTextProperty()->SetCellOffset(41.0);

  mapper6->GetTextProperty()->SetFrame(true);
  mapper6->SetInput(str.c_str());
  actor6->SetPosition(120, 350);

  str = "|||\n";

  vtkNew<vtkTextMapper> mapper7;
  vtkNew<vtkActor2D> actor7;
  actor7->SetMapper(mapper7);
  mapper7->GetTextProperty()->SetFontSize(16);
  mapper7->GetTextProperty()->SetColor(0.6, 0.8, 0.2);
  mapper7->GetTextProperty()->SetBackgroundColor(0.0, 1.0, 0.0);
  mapper7->GetTextProperty()->SetBackgroundOpacity(0.5);
  mapper7->GetTextProperty()->SetJustificationToCentered();
  mapper7->GetTextProperty()->SetVerticalJustificationToCentered();
  mapper7->GetTextProperty()->SetCellOffset(41.0);

  mapper7->GetTextProperty()->SetFrame(true);
  mapper7->SetInput(str.c_str());
  actor7->SetPosition(120, 400);

  str = "TEST|||\n\n\n";

  vtkNew<vtkTextMapper> mapper8;
  vtkNew<vtkActor2D> actor8;
  actor8->SetMapper(mapper8);
  mapper8->GetTextProperty()->SetFontSize(20);
  mapper8->GetTextProperty()->SetColor(1.0, 0.5, 0.5);
  mapper8->GetTextProperty()->SetBackgroundColor(0.0, 0.5, 0.0);
  mapper8->GetTextProperty()->SetBackgroundOpacity(0.5);
  mapper8->GetTextProperty()->SetJustificationToCentered();
  mapper8->GetTextProperty()->SetVerticalJustificationToCentered();
  mapper8->GetTextProperty()->SetCellOffset(20.0);
  mapper8->GetTextProperty()->SetLineSpacing(1.5);

  mapper8->GetTextProperty()->SetFrame(true);
  mapper8->SetInput(str.c_str());
  actor8->SetPosition(120, 500);

  str = "TEST|||\n\n\n|Test";

  vtkNew<vtkTextMapper> mapper9;
  vtkNew<vtkActor2D> actor9;
  actor9->SetMapper(mapper9);
  mapper9->GetTextProperty()->SetFontSize(20);
  mapper9->GetTextProperty()->SetColor(0.2, 0.7, 0.4);
  mapper9->GetTextProperty()->SetBackgroundColor(0.0, 1.0, 0.0);
  mapper9->GetTextProperty()->SetBackgroundOpacity(0.5);
  mapper9->GetTextProperty()->SetJustificationToCentered();
  mapper9->GetTextProperty()->SetVerticalJustificationToCentered();
  mapper9->GetTextProperty()->SetCellOffset(41.0);

  mapper9->GetTextProperty()->SetFrame(true);
  mapper9->SetInput(str.c_str());
  actor9->SetPosition(150, 550);

  str = "1|2|3|4\n1|2|3";

  vtkNew<vtkTextMapper> mapper10;
  vtkNew<vtkActor2D> actor10;
  actor10->SetMapper(mapper10);
  mapper10->GetTextProperty()->SetFontSize(16);
  mapper10->GetTextProperty()->SetColor(0.3, 0.9, 0.5);
  mapper10->GetTextProperty()->SetBackgroundColor(0.0, 1.0, 0.0);
  mapper10->GetTextProperty()->SetBackgroundOpacity(0.5);
  mapper10->GetTextProperty()->SetJustificationToCentered();
  mapper10->GetTextProperty()->SetVerticalJustificationToCentered();
  mapper10->GetTextProperty()->SetCellOffset(41.0);
  mapper10->GetTextProperty()->SetOrientation(90);

  mapper10->GetTextProperty()->SetFrame(true);
  mapper10->SetInput(str.c_str());
  actor10->SetPosition(260, 390);

  str = "\\| Test FreeType escaped pipe \\pi \\|";

  vtkNew<vtkTextMapper> mapper11;
  vtkNew<vtkActor2D> actor11;
  actor11->SetMapper(mapper11);
  mapper11->GetTextProperty()->SetFontSize(16);
  mapper11->GetTextProperty()->SetColor(0.0, 0.0, 1.0);
  mapper11->GetTextProperty()->SetBackgroundColor(0.2, 1.0, 0.0);
  mapper11->GetTextProperty()->SetBackgroundOpacity(0.5);
  mapper11->GetTextProperty()->SetJustificationToCentered();
  mapper11->GetTextProperty()->SetVerticalJustificationToCentered();
  mapper11->SetInput(str.c_str());
  actor11->SetPosition(450, 430);

  str = "\\| Test MathText escaped pipe $\\pi$ \\| | column";

  vtkNew<vtkTextMapper> mapper12;
  vtkNew<vtkActor2D> actor12;
  actor12->SetMapper(mapper12);
  mapper12->GetTextProperty()->SetFontSize(16);
  mapper12->GetTextProperty()->SetColor(1.0, 0.0, 0.0);
  mapper12->GetTextProperty()->SetBackgroundColor(0.0, 0.3, 0.2);
  mapper12->GetTextProperty()->SetBackgroundOpacity(0.5);
  mapper12->GetTextProperty()->SetJustificationToCentered();
  mapper12->GetTextProperty()->SetVerticalJustificationToCentered();
  mapper12->SetInput(str.c_str());
  actor12->SetPosition(420, 510);

  // Boring rendering setup....

  vtkNew<vtkRenderer> ren;
  ren->SetBackground(0.1, 0.1, 0.1);
  vtkNew<vtkRenderWindow> win;
  win->SetSize(600, 600);
  win->AddRenderer(ren);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(win);

  ren->AddActor(actor1);
  ren->AddActor(actor2);
  ren->AddActor(actor3);
  ren->AddActor(actor4);
  ren->AddActor(actor5);
  ren->AddActor(actor6);
  ren->AddActor(actor7);
  ren->AddActor(actor8);
  ren->AddActor(actor9);
  ren->AddActor(actor10);
  ren->AddActor(actor11);
  ren->AddActor(actor12);

  win->SetMultiSamples(0);
  win->Render();
  win->GetInteractor()->Initialize();
  win->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
