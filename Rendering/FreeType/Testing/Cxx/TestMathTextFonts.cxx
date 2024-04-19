// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkTextRenderer.h"

#include "vtkNew.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTextActor.h"
#include "vtkTextProperty.h"

#include <iostream>
#include <string>

//------------------------------------------------------------------------------
int TestMathTextFonts(int argc, char* argv[])
{
  if (argc < 2)
  {
    cerr << "Missing font filename." << endl;
    return EXIT_FAILURE;
  }

  std::string unciodeFontFile(argv[1]);

  vtkNew<vtkTextRenderer> tren;
  if (tren == nullptr)
  {
    std::cerr << "Object factory cannot find vtkTextRenderer override.\n";
    return EXIT_FAILURE;
  }

  if (strcmp(tren->GetClassName(), "vtkMathTextFreeTypeTextRenderer") != 0)
  {
    std::cerr << "Object factory returning unrecognized vtkTextRenderer "
                 "override: "
              << tren->GetClassName() << std::endl;
    return EXIT_FAILURE;
  }

  std::string str = "$TextMath=\\int_0^\\infty\\frac{2\\pi}{x - \\frac{z}{4}}\\,dx$";

  vtkNew<vtkTextActor> actor1;
  actor1->GetTextProperty()->SetFontSize(20);
  actor1->GetTextProperty()->SetColor(1.0, 0.0, 0.0);
  actor1->GetTextProperty()->SetFontFamilyToTimes();
  actor1->SetInput(("Times " + str).c_str());
  actor1->SetPosition(10, 500);

  vtkNew<vtkTextActor> actor2;
  actor2->GetTextProperty()->SetFontSize(20);
  actor2->GetTextProperty()->SetColor(0.0, 1.0, 0.0);
  actor2->GetTextProperty()->SetFontFamilyToCourier();
  actor2->SetInput(("Courier " + str).c_str());
  actor2->SetPosition(10, 400);

  vtkNew<vtkTextActor> actor3;
  actor3->GetTextProperty()->SetFontSize(20);
  actor3->GetTextProperty()->SetColor(0.0, 0.0, 1.0);
  actor3->GetTextProperty()->SetItalic(1);
  actor3->SetInput(("Italic " + str).c_str());
  actor3->SetPosition(10, 10);

  vtkNew<vtkTextActor> actor4;
  actor4->GetTextProperty()->SetFontSize(20);
  actor4->GetTextProperty()->SetColor(0.3, 0.4, 0.5);
  actor4->GetTextProperty()->SetBold(1);
  actor4->SetInput(("Bold " + str).c_str());
  actor4->SetPosition(10, 60);

  vtkNew<vtkTextActor> actor5;
  actor5->GetTextProperty()->SetFontSize(20);
  actor5->GetTextProperty()->SetColor(1.0, 1.0, 0.0);
  actor5->GetTextProperty()->SetBold(1);
  actor5->GetTextProperty()->SetItalic(1);
  actor5->SetInput(("ItalicBold " + str).c_str());
  actor5->SetPosition(10, 300);

  vtkNew<vtkTextActor> actor6;
  actor6->GetTextProperty()->SetFontSize(16);
  actor6->GetTextProperty()->SetColor(1.0, 0.5, 0.2);
  actor6->GetTextProperty()->SetOrientation(45);
  actor6->SetInput(("Oriented " + str).c_str());
  actor6->SetPosition(400, 300);

  vtkNew<vtkTextActor> actor7;
  actor7->GetTextProperty()->SetFontFamily(VTK_FONT_FILE);
  actor7->GetTextProperty()->SetFontFile(unciodeFontFile.c_str());
  actor7->GetTextProperty()->SetFontSize(16);
  actor7->GetTextProperty()->SetColor(0.5, 0.2, 1.0);
  actor7->SetInput(("FontFile " + str).c_str());
  actor7->SetPosition(10, 130);

  str = "$\\mathit{TextMathItalic}$ | $\\mathbf{TextMathBold}$\n$\\mathcal{TextMathCallihraphy}$ | "
        "$\\mathtt{TextMathTypewriter}$";

  vtkNew<vtkTextActor> actor8;
  actor8->GetTextProperty()->SetFontSize(20);
  actor8->GetTextProperty()->SetColor(1.0, 0.5, 0.2);
  actor8->SetInput(str.c_str());
  actor8->SetPosition(10, 200);

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

  win->SetMultiSamples(0);
  win->Render();
  win->GetInteractor()->Initialize();
  win->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
