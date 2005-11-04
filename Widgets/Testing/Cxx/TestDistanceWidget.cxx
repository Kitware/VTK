/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDistanceWidget.cxx

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
#include "vtkDistanceWidget.h"
#include "vtkDistanceRepresentation2D.h"
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


// This callback is responsible for adjusting the point position.
// It looks in the region around the point and finds the maximum or
// minimum value.
class vtkDistanceCallback : public vtkCommand
{
public:
  static vtkDistanceCallback *New() 
    { return new vtkDistanceCallback; }
  virtual void Execute(vtkObject *caller, unsigned long, void*);
  vtkDistanceCallback():Renderer(0),RenderWindow(0),DistanceWidget(0),Distance(0) {}
  vtkRenderer *Renderer;
  vtkRenderWindow *RenderWindow;
  vtkDistanceWidget *DistanceWidget;
  vtkDistanceRepresentation2D *Distance;
};


// Method re-positions the points using random perturbation
void vtkDistanceCallback::Execute(vtkObject*, unsigned long eid, void* callData)
{
  if ( eid == vtkCommand::InteractionEvent ||
       eid == vtkCommand::EndInteractionEvent )
    {
        double pos1[3], pos2[3];
    // Modify the measure axis
    this->Distance->GetPoint1WorldPosition(pos1);
    this->Distance->GetPoint2WorldPosition(pos2);
    double dist=sqrt(vtkMath::Distance2BetweenPoints(pos1,pos2));

    char title[256];
    this->Distance->GetAxis()->SetRange(0.0,dist);
    sprintf(title,"%-#6.3g",dist);
    this->Distance->GetAxis()->SetTitle(title);
    }
  else
    {
    int pid = *(reinterpret_cast<int*>(callData));
    
    //From the point id, get the display coordinates
    double pos1[3], pos2[3], *pos;
    this->Distance->GetPoint1DisplayPosition(pos1);
    this->Distance->GetPoint2DisplayPosition(pos2);
    if ( pid == 0 )
      {
      pos = pos1;
      }
    else
      {
      pos = pos2;
      }

    // Okay, render without the widget, and get the color buffer
    int enabled = this->DistanceWidget->GetEnabled();
    if ( enabled )
      {
      this->DistanceWidget->SetEnabled(0); //does a Render() as a side effect
      }

    // Pretend we are doing something serious....just randomly bump the
    // location of the point.
    double p[3];
    p[0] = pos[0] + static_cast<int>(vtkMath::Random(-5.5,5.5));
    p[1] = pos[1] + static_cast<int>(vtkMath::Random(-5.5,5.5));
    p[2] = 0.0;

    // Set the new position
    if ( pid == 0 )
      {
      this->Distance->SetPoint1DisplayPosition(p);
      }
    else
      {
      this->Distance->SetPoint2DisplayPosition(p);
      }

    // Side effect of a render here
    if ( enabled )
      {
      this->DistanceWidget->SetEnabled(1);
      }
    }
}

// The actual test function
int TestDistanceWidget( int argc, char *argv[] )
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
  vtkDistanceRepresentation2D *rep = vtkDistanceRepresentation2D::New();
  rep->SetHandleRepresentation(handle);

  vtkDistanceWidget *widget = vtkDistanceWidget::New();
  widget->SetInteractor(iren);
  widget->SetRepresentation(rep);

  vtkDistanceCallback *mcbk = vtkDistanceCallback::New();
  mcbk->Renderer = ren1;
  mcbk->RenderWindow = renWin;
  mcbk->Distance = rep;
  mcbk->DistanceWidget = widget;
//  widget->AddObserver(vtkCommand::PlacePointEvent,mcbk);
//  widget->AddObserver(vtkCommand::InteractionEvent,mcbk);
//  widget->AddObserver(vtkCommand::EndInteractionEvent,mcbk);

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
