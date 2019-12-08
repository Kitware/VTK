/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestButtonWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Test vtkProp3DButtonRepresentation

#include "vtkButtonWidget.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkProp3DButtonRepresentation.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

// ----------------------------------------------------------------------------
bool TestUnMapped();

// ----------------------------------------------------------------------------
int TestProp3DButtonRepresentation(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  vtkObjectFactory::SetAllEnableFlags(false, "vtkRenderWindowInteractor", "vtkTestingInteractor");
  bool res = true;
  res = TestUnMapped() && res;
  return res ? EXIT_SUCCESS : EXIT_FAILURE;
}

// ----------------------------------------------------------------------------
bool TestUnMapped()
{
  vtkNew<vtkRenderWindow> renderWindow;
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow);

  vtkNew<vtkProp3DButtonRepresentation> representation1;
  vtkNew<vtkButtonWidget> buttonWidget1;
  buttonWidget1->SetInteractor(iren);
  buttonWidget1->SetRepresentation(representation1);
  buttonWidget1->SetEnabled(1);

  vtkNew<vtkRenderer> renderer;
  renderWindow->AddRenderer(renderer);

  vtkNew<vtkProp3DButtonRepresentation> representation2;
  vtkNew<vtkButtonWidget> buttonWidget2;
  buttonWidget2->SetInteractor(iren);
  buttonWidget2->SetRepresentation(representation2);
  buttonWidget2->SetEnabled(1);

  iren->Initialize();

  vtkNew<vtkProp3DButtonRepresentation> representation3;
  vtkNew<vtkButtonWidget> buttonWidget3;
  buttonWidget3->SetInteractor(iren);
  buttonWidget3->SetRepresentation(representation3);
  buttonWidget3->SetEnabled(1);

  iren->Start();

  vtkNew<vtkProp3DButtonRepresentation> representation4;
  vtkNew<vtkButtonWidget> buttonWidget4;
  buttonWidget4->SetInteractor(iren);
  buttonWidget4->SetRepresentation(representation4);
  buttonWidget4->SetEnabled(1);

  return true;
}
