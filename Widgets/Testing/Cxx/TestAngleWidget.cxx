/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestAngleWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// This example tests the vtkBorderWidget.

// First include the required header files for the VTK classes we are using.
#include "vtkAngleWidget.h"
#include "vtkAngleRepresentation2D.h"
#include "vtkSphereSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkCommand.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkRegressionTestImage.h"
#include "vtkDebugLeaks.h"
#include "vtkCoordinate.h"
#include "vtkMath.h"
#include "vtkHandleWidget.h"
#include "vtkPointHandleRepresentation2D.h"
#include "vtkAxisActor2D.h"
#include "vtkProperty2D.h"


// This callback is responsible for setting the angle label.
class vtkAngleCallback : public vtkCommand
{
public:
  static vtkAngleCallback *New() 
    { return new vtkAngleCallback; }
  virtual void Execute(vtkObject*, unsigned long eid, void*)
    {
      if ( eid == vtkCommand::PlacePointEvent )
        {
        cout << "point placed\n";
        }
      else //if ( eid == vtkCommand::InteractionEvent )
        {
        cout << "Angle: " << this->Rep->GetAngle() << "\n";
        }
    }
  vtkAngleRepresentation2D *Rep;
  vtkAngleCallback():Rep(0) {}
};


// The actual test function
int TestAngleWidget( int argc, char *argv[] )
{
  // Create the RenderWindow, Renderer and both Actors
  //
  vtkRenderer *ren1 = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->AddRenderer(ren1);

  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  // Create a test pipeline
  //
  vtkSphereSource *ss = vtkSphereSource::New();
  vtkPolyDataMapper *mapper = vtkPolyDataMapper::New();
  mapper->SetInput(ss->GetOutput());
  vtkActor *actor = vtkActor::New();
  actor->SetMapper(mapper);

  // Create the widget and its representation
  vtkPointHandleRepresentation2D *handle = vtkPointHandleRepresentation2D::New();
  handle->GetProperty()->SetColor(1,0,0);
  vtkAngleRepresentation2D *rep = vtkAngleRepresentation2D::New();
  rep->SetHandleRepresentation(handle);

  vtkAngleWidget *widget = vtkAngleWidget::New();
  widget->SetInteractor(iren);
  widget->SetRepresentation(rep);

  vtkAngleCallback *mcbk = vtkAngleCallback::New();
  mcbk->Rep = rep;
  widget->AddObserver(vtkCommand::PlacePointEvent,mcbk);
//  widget->AddObserver(vtkCommand::InteractionEvent,mcbk);

  // Add the actors to the renderer, set the background and size
  //
  ren1->AddActor(actor);
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
  widget->On();
//  recorder->Play();

  // Remove the observers so we can go interactive. Without this the "-I"
  // testing option fails.
  recorder->Off();

  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  ss->Delete();
  mapper->Delete();
  actor->Delete();
  handle->Delete();
  rep->Delete();
  widget->RemoveObserver(mcbk);
  mcbk->Delete();
  widget->Off();
  widget->Delete();
  iren->Delete();
  renWin->Delete();
  ren1->Delete();
  recorder->Delete();
  
  return !retVal;
}
