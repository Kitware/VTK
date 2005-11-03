/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestRotationWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// This example tests the vtkSliderWidget.

// First include the required header files for the VTK classes we are using.
#include "vtkRotationWidget.h"
#include "vtkRotationRepresentation.h"
#include "vtkCoordinate.h"
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

// This does the actual work: updates the probe.
// Callback for the interaction
class vtkRotationCallback : public vtkCommand
{
public:
  static vtkRotationCallback *New() 
    { return new vtkRotationCallback; }
  virtual void Execute(vtkObject *caller, unsigned long, void*)
    {
      vtkRotationWidget *rotationWidget = 
        reinterpret_cast<vtkRotationWidget*>(caller);
    }
  vtkRotationCallback():Sphere(0) {}
  vtkSphereSource *Sphere;
};

int TestRotationWidget( int argc, char *argv[] )
{
  // Create a mace out of filters.
  //
  vtkSphereSource *sphereSource = vtkSphereSource::New();
  sphereSource->SetCenter(1,1,1);
  sphereSource->SetThetaResolution(16);
  sphereSource->SetPhiResolution(8);
  sphereSource->SetRadius(2.2);

  vtkPolyDataMapper *sphereMapper = vtkPolyDataMapper::New();
  sphereMapper->SetInput(sphereSource->GetOutput());

  vtkActor *sphereActor = vtkActor::New();
  sphereActor->SetMapper(sphereMapper);
  sphereActor->VisibilityOn();

  // Create the RenderWindow, Renderer and both Actors
  //
  vtkRenderer *ren1 = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->AddRenderer(ren1);

  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  // VTK widgets consist of two parts: the widget part that rotations event processing;
  // and the widget representation that defines how the widget appears in the scene 
  // (i.e., matters pertaining to geometry).
  vtkRotationRepresentation *rotationRep = vtkRotationRepresentation::New();
  rotationRep->PlaceWidget(sphereActor->GetBounds());

  vtkRotationWidget *rotationWidget = vtkRotationWidget::New();
  rotationWidget->SetInteractor(iren);
  rotationWidget->SetRepresentation(rotationRep);

  vtkRotationCallback *callback = vtkRotationCallback::New();
  callback->Sphere = sphereSource;
  rotationWidget->AddObserver(vtkCommand::InteractionEvent,callback);

  ren1->AddActor(sphereActor);

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

  // Remove the observers so we can go interactive. Without this the "-I"
  // testing option fails.
  recorder->Off();

  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  sphereSource->Delete();
  sphereMapper->Delete();
  sphereActor->Delete();
  iren->Delete();
  renWin->Delete();
  ren1->Delete();
  rotationRep->Delete();
  rotationWidget->Delete();
  callback->Delete();
  recorder->Delete();

  return !retVal;

}


