/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestProgressBarWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// This example tests the vtkProgressBarWidget.

// First include the required header files for the VTK classes we are using.
#include "vtkActor.h"
#include "vtkCommand.h"
#include "vtkConeSource.h"
#include "vtkCylinderSource.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkProgressBarRepresentation.h"
#include "vtkProgressBarWidget.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkTestUtilities.h"

int TestProgressBarWidget(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  // Create the RenderWindow, Renderer and both Actors
  //
  vtkNew<vtkRenderer> ren1;
  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(ren1.Get());

  vtkNew<vtkInteractorStyleTrackballCamera> style;
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.Get());
  iren->SetInteractorStyle(style.Get());

  // Create a test pipeline
  //
  vtkNew<vtkSphereSource> ss;
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(ss->GetOutputPort());
  vtkNew<vtkActor> sph;
  sph->SetMapper(mapper.Get());

  vtkNew<vtkCylinderSource> cs;
  vtkNew<vtkPolyDataMapper> csMapper;
  csMapper->SetInputConnection(cs->GetOutputPort());
  vtkNew<vtkActor> cyl;
  cyl->SetMapper(csMapper.Get());
  cyl->AddPosition(5, 0, 0);

  vtkNew<vtkConeSource> coneSource;
  vtkNew<vtkPolyDataMapper> coneMapper;
  coneMapper->SetInputConnection(coneSource->GetOutputPort());
  vtkNew<vtkActor> cone;
  cone->SetMapper(coneMapper.Get());
  cone->AddPosition(0, 5, 0);

  // Create the widget
  vtkNew<vtkProgressBarRepresentation> rep;

  vtkNew<vtkProgressBarWidget> widget;
  widget->SetInteractor(iren.Get());
  widget->SetRepresentation(rep.Get());

  // Create the widget
  vtkNew<vtkProgressBarWidget> widget2;
  widget2->SetInteractor(iren.Get());
  widget2->CreateDefaultRepresentation();
  vtkProgressBarRepresentation* rep2 = vtkProgressBarRepresentation::SafeDownCast(widget2->GetRepresentation());

  // Add the actors to the renderer, set the background and size
  //
  ren1->AddActor(sph.Get());
  ren1->AddActor(cyl.Get());
  ren1->AddActor(cone.Get());
  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(300, 300);

  // render the image
  //
  iren->Initialize();
  rep->SetProgressRate(0.4);
  rep->SetPosition(0.4, 0.4);
  rep->SetProgressBarColor(0.2, 0.4, 0);
  rep->SetBackgroundColor(1, 1, 0.5);
  rep->DrawBackgroundOff();

  rep2->SetProgressRate(0.8);
  rep2->SetProgressBarColor(0.1, 0.8, 0);
  rep2->SetBackgroundColor(1, 1, 0.5);
  rep2->DrawBackgroundOn();

  renWin->Render();
  widget->On();
  widget2->On();
  renWin->Render();

  iren->Start();

  return EXIT_SUCCESS;

}
