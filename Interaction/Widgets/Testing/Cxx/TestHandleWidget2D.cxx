/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestHandleWidget2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// This example tests the vtkHandleWidget with a 2D representation

// First include the required header files for the VTK classes we are using.
#include "vtkSmartPointer.h"

#include "vtkHandleWidget.h"
#include "vtkPointHandleRepresentation2D.h"
#include "vtkCursor2D.h"
#include "vtkCoordinate.h"
#include "vtkDiskSource.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkActor2D.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkCommand.h"
#include "vtkInteractorEventRecorder.h"

// This does the actual work: updates the probe.
// Callback for the interaction
class vtkHandle2Callback : public vtkCommand
{
public:
  static vtkHandle2Callback *New()
  { return new vtkHandle2Callback; }
  void Execute(vtkObject *caller, unsigned long, void*) VTK_OVERRIDE
  {
    vtkHandleWidget *handleWidget =
      reinterpret_cast<vtkHandleWidget*>(caller);
    double pos[3];
    static_cast<vtkHandleRepresentation *>(handleWidget->GetRepresentation())->GetDisplayPosition(pos);
    this->Actor->SetPosition(pos[0],pos[1]);
  }
  vtkHandle2Callback():Actor(0) {}
  vtkActor2D *Actor;
};

int TestHandleWidget2D(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  // Create two widgets
  //
  vtkSmartPointer<vtkDiskSource> diskSource =
    vtkSmartPointer<vtkDiskSource>::New();
  diskSource->SetInnerRadius(0.0);
  diskSource->SetOuterRadius(2);

  vtkSmartPointer<vtkPolyDataMapper2D> diskMapper =
    vtkSmartPointer<vtkPolyDataMapper2D>::New();
  diskMapper->SetInputConnection(diskSource->GetOutputPort());

  vtkSmartPointer<vtkActor2D> diskActor =
    vtkSmartPointer<vtkActor2D>::New();
  diskActor->SetMapper(diskMapper);
  diskActor->SetPosition(165,180);

  vtkSmartPointer<vtkDiskSource> diskSource2 =
    vtkSmartPointer<vtkDiskSource>::New();
  diskSource2->SetInnerRadius(0.0);
  diskSource2->SetOuterRadius(2);

  vtkSmartPointer<vtkPolyDataMapper2D> diskMapper2 =
    vtkSmartPointer<vtkPolyDataMapper2D>::New();
  diskMapper2->SetInputConnection(diskSource2->GetOutputPort());

  vtkSmartPointer<vtkActor2D> diskActor2 =
    vtkSmartPointer<vtkActor2D>::New();
  diskActor2->SetMapper(diskMapper2);
  diskActor2->SetPosition(50,50);

  // Create the RenderWindow, Renderer and both Actors
  //
  vtkSmartPointer<vtkRenderer> ren1 =
    vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderWindow> renWin =
    vtkSmartPointer<vtkRenderWindow>::New();
  renWin->AddRenderer(ren1);

  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  // The cursor shape can be defined externally. Here we use a default.
  vtkSmartPointer<vtkCursor2D> cursor2D =
    vtkSmartPointer<vtkCursor2D>::New();
  cursor2D->AllOff();
  cursor2D->AxesOn();
  cursor2D->OutlineOn();
  cursor2D->SetRadius(4);
  cursor2D->Update();

  vtkSmartPointer<vtkPointHandleRepresentation2D> handleRep =
    vtkSmartPointer<vtkPointHandleRepresentation2D>::New();
  handleRep->SetDisplayPosition(diskActor->GetPosition());
  handleRep->ActiveRepresentationOn();
  handleRep->SetCursorShape(cursor2D->GetOutput());

  vtkSmartPointer<vtkHandleWidget> handleWidget =
    vtkSmartPointer<vtkHandleWidget>::New();
  handleWidget->SetInteractor(iren);
  handleWidget->SetRepresentation(handleRep);

  vtkSmartPointer<vtkHandle2Callback> callback =
    vtkSmartPointer<vtkHandle2Callback>::New();
  callback->Actor = diskActor;
  handleWidget->AddObserver(vtkCommand::InteractionEvent,callback);

  vtkSmartPointer<vtkPointHandleRepresentation2D> handleRep2 =
    vtkSmartPointer<vtkPointHandleRepresentation2D>::New();
  handleRep2->SetDisplayPosition(diskActor2->GetPosition());
//  handleRep2->ActiveRepresentationOn();
  handleRep2->SetCursorShape(cursor2D->GetOutput());

  vtkSmartPointer<vtkHandleWidget> handleWidget2 =
    vtkSmartPointer<vtkHandleWidget>::New();
  handleWidget2->SetInteractor(iren);
  handleWidget2->SetRepresentation(handleRep2);

  vtkSmartPointer<vtkHandle2Callback> callback2 =
    vtkSmartPointer<vtkHandle2Callback>::New();
  callback2->Actor = diskActor2;
  handleWidget2->AddObserver(vtkCommand::InteractionEvent,callback2);

  ren1->AddActor(diskActor);
  ren1->AddActor(diskActor2);

  // Add the actors to the renderer, set the background and size
  //
  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(300, 300);

  // record events
  vtkSmartPointer<vtkInteractorEventRecorder> recorder =
    vtkSmartPointer<vtkInteractorEventRecorder>::New();
  recorder->SetInteractor(iren);
  recorder->SetFileName("c:/record.log");
//  recorder->Record();
//  recorder->ReadFromInputStringOn();
//  recorder->SetInputString(eventLog);

  // render the image
  //
  iren->Initialize();
  renWin->Render();
//  recorder->Play();
  handleWidget->On();
  handleWidget2->On();

  // Remove the observers so we can go interactive. Without this the "-I"
  // testing option fails.
  recorder->Off();

  iren->Start();

  return EXIT_SUCCESS;

}
