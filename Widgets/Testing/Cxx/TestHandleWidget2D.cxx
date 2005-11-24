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
#include "vtkRegressionTestImage.h"
#include "vtkDebugLeaks.h"

// This does the actual work: updates the probe.
// Callback for the interaction
class vtkHandle2Callback : public vtkCommand
{
public:
  static vtkHandle2Callback *New() 
    { return new vtkHandle2Callback; }
  virtual void Execute(vtkObject *caller, unsigned long, void*)
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

int TestHandleWidget2D( int argc, char *argv[] )
{
  // Create two widgets
  //
  vtkDiskSource *diskSource = vtkDiskSource::New();
  diskSource->SetInnerRadius(0.0);
  diskSource->SetOuterRadius(2);

  vtkPolyDataMapper2D *diskMapper = vtkPolyDataMapper2D::New();
  diskMapper->SetInput(diskSource->GetOutput());

  vtkActor2D *diskActor = vtkActor2D::New();
  diskActor->SetMapper(diskMapper);
  diskActor->SetPosition(165,180);

  vtkDiskSource *diskSource2 = vtkDiskSource::New();
  diskSource2->SetInnerRadius(0.0);
  diskSource2->SetOuterRadius(2);

  vtkPolyDataMapper2D *diskMapper2 = vtkPolyDataMapper2D::New();
  diskMapper2->SetInput(diskSource2->GetOutput());

  vtkActor2D *diskActor2 = vtkActor2D::New();
  diskActor2->SetMapper(diskMapper2);
  diskActor2->SetPosition(50,50);

  // Create the RenderWindow, Renderer and both Actors
  //
  vtkRenderer *ren1 = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->AddRenderer(ren1);

  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  // The cursor shape can be defined externally. Here we use a default.
  vtkCursor2D *cursor2D = vtkCursor2D::New();
  cursor2D->AllOff();
  cursor2D->AxesOn();
  cursor2D->OutlineOn();
  cursor2D->SetRadius(4);

  vtkPointHandleRepresentation2D *handleRep = vtkPointHandleRepresentation2D::New();
  handleRep->SetDisplayPosition(diskActor->GetPosition());
  handleRep->ActiveRepresentationOn();
  handleRep->SetCursorShape(cursor2D->GetOutput());

  vtkHandleWidget *handleWidget = vtkHandleWidget::New();
  handleWidget->SetInteractor(iren);
  handleWidget->SetRepresentation(handleRep);

  vtkHandle2Callback *callback = vtkHandle2Callback::New();
  callback->Actor = diskActor;
  handleWidget->AddObserver(vtkCommand::InteractionEvent,callback);

  vtkPointHandleRepresentation2D *handleRep2 = vtkPointHandleRepresentation2D::New();
  handleRep2->SetDisplayPosition(diskActor2->GetPosition());
//  handleRep2->ActiveRepresentationOn();
  handleRep2->SetCursorShape(cursor2D->GetOutput());

  vtkHandleWidget *handleWidget2 = vtkHandleWidget::New();
  handleWidget2->SetInteractor(iren);
  handleWidget2->SetRepresentation(handleRep2);

  vtkHandle2Callback *callback2 = vtkHandle2Callback::New();
  callback2->Actor = diskActor2;
  handleWidget2->AddObserver(vtkCommand::InteractionEvent,callback2);

  ren1->AddActor(diskActor);
  ren1->AddActor(diskActor2);

  // Add the actors to the renderer, set the background and size
  //
  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(300, 300);

  // record events
  vtkInteractorEventRecorder *recorder = vtkInteractorEventRecorder::New();
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

  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  diskSource->Delete();
  diskSource2->Delete();
  diskMapper->Delete();
  diskMapper2->Delete();
  diskActor->Delete();
  diskActor2->Delete();
  cursor2D->Delete();
  handleWidget->RemoveObserver(callback);
  handleWidget->Off();
  handleWidget->Delete();
  handleRep->Delete();
  callback->Delete();
  handleWidget2->RemoveObserver(callback2);
  handleWidget2->Off();
  handleWidget2->Delete();
  handleRep2->Delete();
  callback2->Delete();
  iren->Delete();
  renWin->Delete();
  ren1->Delete();
  recorder->Delete();

  return !retVal;

}


