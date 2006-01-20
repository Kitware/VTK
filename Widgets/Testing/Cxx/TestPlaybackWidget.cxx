/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPlaybackWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// This example tests the vtkPlaybackWidget.

// First include the required header files for the VTK classes we are using.
#include "vtkPlaybackWidget.h"
#include "vtkPlaybackRepresentation.h"
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

class vtkSubclassPlaybackRepresentation : public vtkPlaybackRepresentation
{
public:
  static vtkSubclassPlaybackRepresentation *New() {return new vtkSubclassPlaybackRepresentation;}
  virtual void Play() {cout << "play\n";}
  virtual void Stop() {cout << "stop\n";}
  virtual void ForwardOneFrame() {cout << "forward one frame\n";}
  virtual void BackwardOneFrame() {cout << "backward one frame\n";}
  virtual void JumpToBeginning() {cout << "jump to beginning\n";}
  virtual void JumpToEnd() {cout << "jump to end\n";}
};


int TestPlaybackWidget( int argc, char *argv[] )
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

  // Create the widget
  vtkSubclassPlaybackRepresentation *rep = vtkSubclassPlaybackRepresentation::New();

  vtkPlaybackWidget *widget = vtkPlaybackWidget::New();
  widget->SetInteractor(iren);
  widget->SetRepresentation(rep);

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
  widget->Off();
  widget->Delete();
  rep->Delete();
  iren->Delete();
  renWin->Delete();
  ren1->Delete();
  recorder->Delete();
  
  return !retVal;

}


