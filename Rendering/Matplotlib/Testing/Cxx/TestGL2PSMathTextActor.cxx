/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestStringToPath.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMathTextActor.h"

#include "vtkCamera.h"
#include "vtkGL2PSExporter.h"
#include "vtkNew.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestingInteractor.h"
#include "vtkTextProperty.h"

//----------------------------------------------------------------------------
int TestGL2PSMathTextActor(int, char *[])
{
  vtkNew<vtkMathTextActor> actor1;
  actor1->SetInput("$\\langle\\psi_i\\mid\\psi_j\\rangle = \\delta_{ij}$");
  actor1->GetTextProperty()->SetFontSize(36);
  actor1->GetTextProperty()->SetOrientation(0.0);
  actor1->GetTextProperty()->SetColor(0.8, 0.8, 0.6);
  actor1->SetPosition(0, 0);
  actor1->GetTextProperty()->SetVerticalJustificationToBottom();
  actor1->GetTextProperty()->SetJustificationToLeft();

  vtkNew<vtkMathTextActor> actor2;
  actor2->SetInput("$\\langle\\psi_i\\mid\\psi_j\\rangle = \\delta_{ij}$");
  actor2->GetTextProperty()->SetFontSize(36);
  actor2->SetPosition(300, 300);
  actor2->GetTextProperty()->SetColor(0.7, 0.3, 0.2);
  actor2->GetTextProperty()->SetVerticalJustificationToCentered();
  actor2->GetTextProperty()->SetJustificationToCentered();
  actor2->GetTextProperty()->SetOrientation(90.0);

  vtkNew<vtkMathTextActor> actor3;
  actor3->SetInput("$\\langle\\psi_i\\mid\\psi_j\\rangle = \\delta_{ij}$");
  actor3->GetTextProperty()->SetFontSize(36);
  actor3->SetPosition(600, 600);
  actor3->GetTextProperty()->SetColor(0.6, 0.5, 0.8);
  actor3->GetTextProperty()->SetVerticalJustificationToTop();
  actor3->GetTextProperty()->SetJustificationToRight();

  vtkNew<vtkMathTextActor> actor4;
  actor4->SetInput("$\\langle\\psi_i\\mid\\psi_j\\rangle = \\delta_{ij}$");
  actor4->GetTextProperty()->SetFontSize(22);
  actor4->SetPosition(150, 300);
  actor4->GetTextProperty()->SetColor(0.2, 0.6, 0.4);
  actor4->GetTextProperty()->SetVerticalJustificationToCentered();
  actor4->GetTextProperty()->SetJustificationToCentered();
  actor4->GetTextProperty()->SetOrientation(45.0);

  vtkNew<vtkMathTextActor> actor5;
  actor5->ShallowCopy(actor4.GetPointer());
  actor5->SetPosition(450, 300);

  vtkNew<vtkRenderer> ren;
  vtkNew<vtkRenderWindow> win;
  win->AddRenderer(ren.GetPointer());
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(win.GetPointer());

  ren->AddActor(actor1.GetPointer());
  ren->AddActor(actor2.GetPointer());
  ren->AddActor(actor3.GetPointer());
  ren->AddActor(actor4.GetPointer());
  ren->AddActor(actor5.GetPointer());

  ren->SetBackground(0.0, 0.0, 0.0);
  win->SetSize(600, 600);
  win->Render();

  vtkNew<vtkGL2PSExporter> exp;
  exp->SetRenderWindow(win.GetPointer());
  exp->SetFileFormatToPS();
  exp->CompressOff();
  exp->SetSortToSimple();
  exp->DrawBackgroundOn();

  std::string fileprefix = vtkTestingInteractor::TempDirectory +
      std::string("/TestGL2PSMathTextActor");

  exp->SetFilePrefix(fileprefix.c_str());
  exp->Write();

  // Finally render the scene and compare the image to a reference image
  win->SetMultiSamples(0);
  win->GetInteractor()->Initialize();
  win->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
