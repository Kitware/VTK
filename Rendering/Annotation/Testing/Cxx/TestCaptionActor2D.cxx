/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCaptionActor2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkCaptionActor2D.h>
#include <vtkNew.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkTextActor.h>
#include <vtkTextProperty.h>

int TestCaptionActor2D(int, char *[])
{
  // Draw text with diameter measure
  vtkNew<vtkCaptionActor2D> captionActor;
  captionActor->SetAttachmentPoint(0, 0, 0);
  captionActor->SetCaption("(2) 2.27");
  captionActor->BorderOff();
  captionActor->LeaderOff();
  captionActor->SetPadding(0);
  captionActor->GetCaptionTextProperty()->SetJustificationToLeft();
  captionActor->GetCaptionTextProperty()->ShadowOff();
  captionActor->GetCaptionTextProperty()->ItalicOff();
  captionActor->GetCaptionTextProperty()->SetFontFamilyToCourier();
  captionActor->GetCaptionTextProperty()->SetFontSize( 24 );
  captionActor->GetTextActor()->SetTextScaleModeToNone();

  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(0,0,0);
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer.GetPointer());
  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow.GetPointer());

  renderer->AddActor(captionActor.GetPointer());

  renderWindow->SetMultiSamples(0);
  renderWindow->Render();
  renderWindow->GetInteractor()->Initialize();
  renderWindow->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
