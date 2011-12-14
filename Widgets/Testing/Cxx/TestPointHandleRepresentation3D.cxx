/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPointHandleRepresentation3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// This example tests the vtkPointHandleRepresentation3D::PlaceWidget
// through vtkSeedWidget while changing the translation mode.
// If TranslationMode is set to False from outside, and PlaceWidget is called,
// the crosshair should be placed at the center of the bounds.

#include "vtkSmartPointer.h"
#include "vtkSeedWidget.h"
#include "vtkSeedRepresentation.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkHandleWidget.h"
#include "vtkProperty.h"
#include "vtkPointHandleRepresentation3D.h"

int TestPointHandleRepresentation3D(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  // Create the RenderWindow, Renderer and both Actors
  //
  vtkSmartPointer<vtkRenderer> render =
    vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderWindow> renWin =
    vtkSmartPointer<vtkRenderWindow>::New();
  renWin->AddRenderer(render);

  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  // Create the widget and its representation
  vtkSmartPointer<vtkPointHandleRepresentation3D> handlePointRep3D =
    vtkSmartPointer<vtkPointHandleRepresentation3D>::New();
  handlePointRep3D->AllOn();
  handlePointRep3D->GetProperty()->SetColor(1.,0.,1.);

  vtkSmartPointer<vtkSeedRepresentation> seedRep =
    vtkSmartPointer<vtkSeedRepresentation>::New();
  seedRep->SetHandleRepresentation(handlePointRep3D);

  vtkSmartPointer<vtkSeedWidget> seedWidget =
    vtkSmartPointer<vtkSeedWidget>::New();

  seedWidget->SetRepresentation(seedRep);
  seedWidget->SetInteractor(iren);
  seedWidget->On();
  seedWidget->ProcessEventsOff();

  // Place two different points in different translation mode.
  double bounds[6] = {0, 0.05, 0, 0.05, 0, 0.05};
  double bounds2[6] = {-0.05, 0, -0.05, 0, -0.05, 0};

  vtkHandleWidget *currentHandle = seedWidget->CreateNewHandle();
  currentHandle->SetEnabled(1);
  vtkPointHandleRepresentation3D* handleRep =
    vtkPointHandleRepresentation3D::SafeDownCast(currentHandle->GetRepresentation());
  handleRep->PlaceWidget(bounds);

  currentHandle = seedWidget->CreateNewHandle();
  currentHandle->SetEnabled(1);
  handleRep =
    vtkPointHandleRepresentation3D::SafeDownCast(currentHandle->GetRepresentation());
  handleRep->TranslationModeOff();
  handleRep->PlaceWidget(bounds2);

  // Add the actors to the renderer, set the background and size
  //
  render->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(300, 300);

  // render the image
  iren->Initialize();
  renWin->Render();
  iren->Start();

  return EXIT_SUCCESS;
}
