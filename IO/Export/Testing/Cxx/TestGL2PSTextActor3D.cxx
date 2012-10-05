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

#include "vtkTextActor3D.h"

#include "vtkCamera.h"
#include "vtkGL2PSExporter.h"
#include "vtkNew.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestingInteractor.h"
#include "vtkTextProperty.h"

//----------------------------------------------------------------------------
int TestGL2PSTextActor3D(int, char *[])
{
  vtkNew<vtkTextActor3D> actor1;
  actor1->SetInput("Some text!");
  actor1->GetTextProperty()->SetFontSize(36);
  actor1->GetTextProperty()->SetOrientation(45);
  // These should be ignored by both the actor and exporter:
  actor1->GetTextProperty()->SetVerticalJustificationToCentered();
  actor1->GetTextProperty()->SetJustificationToCentered();
  actor1->SetPosition(-100, 25, -100);
  actor1->RotateWXYZ(50, 1, .5, -.2);
  actor1->GetTextProperty()->SetColor(0.6, 0.5, 0.8);

  vtkNew<vtkTextActor3D> actor2;
  actor2->SetInput("Some more text!");
  actor2->GetTextProperty()->SetFontSize(40);
  actor2->SetPosition(-50, 0, -200);
  actor2->RotateWXYZ(-70, 0, 1, 0);
  actor2->GetTextProperty()->SetColor(0.7, 0.3, 0.2);

  vtkNew<vtkTextActor3D> actor3;
  actor3->SetInput("More text!");
  actor3->GetTextProperty()->SetFontSize(36);
  actor3->GetTextProperty()->SetColor(0.8, 0.8, 0.6);
  actor3->SetPosition(-100, -25, 0);
  actor3->RotateWXYZ(70, 0, 1, 0);

  vtkNew<vtkTextActor3D> actor4;
  actor4->SetInput("Testing...");
  actor4->GetTextProperty()->SetFontSize(22);
  actor4->SetPosition(-75, -75, 25);
  actor4->RotateWXYZ(40, -.2, 1, .3);
  actor4->GetTextProperty()->SetColor(0.2, 0.6, 0.4);

  vtkNew<vtkTextActor3D> actor5;
  actor5->SetInput("A somewhat longer string of text!");
  actor5->GetTextProperty()->SetFontSize(26);
  actor5->GetTextProperty()->SetColor(1, 1, 1);
  actor5->SetPosition(-240, -110, -500);
  actor5->RotateWXYZ(-25, 1, 0, 1);

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
  ren->GetActiveCamera()->SetPosition(0, 0, 400);
  ren->GetActiveCamera()->SetFocalPoint(0, 0, 0);
  ren->GetActiveCamera()->SetViewUp(0, 1, 0);
  win->SetSize(600, 600);
  win->Render();

  vtkNew<vtkGL2PSExporter> exp;
  exp->SetRenderWindow(win.GetPointer());
  exp->SetFileFormatToPS();
  exp->CompressOff();
  exp->SetSortToSimple();
  exp->DrawBackgroundOn();

  std::string fileprefix = vtkTestingInteractor::TempDirectory +
      std::string("/TestGL2PSTextActor3D");

  exp->SetFilePrefix(fileprefix.c_str());
  exp->Write();

  // Finally render the scene and compare the image to a reference image
  win->SetMultiSamples(0);
  win->GetInteractor()->Initialize();
  win->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
