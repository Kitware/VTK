/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPickTextActor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This tests picking of text actors.
//
#include "vtkActor2D.h"
#include "vtkNew.h"
#include "vtkPropPicker.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTextActor.h"

#include <cstdlib>

int TestPickTextActor(int, char*[])
{
  vtkNew<vtkTextActor> actor1;
  actor1->SetInput("One");
  actor1->SetPosition(140, 140);

  vtkNew<vtkTextActor> actor2;
  actor2->SetInput("Two");
  actor2->SetPosition(160, 170);

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor1);
  renderer->AddActor(actor2);

  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);
  renWin->Render();

  vtkNew<vtkPropPicker> picker;
  picker->Pick(145, 145, 0.0, renderer);
  vtkActor2D* pickedActor = picker->GetActor2D();
  if (pickedActor != actor1)
  {
    std::cout << "Incorrect actor picked!" << std::endl;
    std::cout << "Should have been " << actor1 << ", but was " << pickedActor << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
