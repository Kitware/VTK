/*=========================================================================

  Program:   Visualization Toolkit
  Module:    BoxWidget2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSmartPointer.h"
#include "vtkAppendPolyData.h"
#include "vtkBoxWidget2.h"
#include "vtkBoxRepresentation.h"
#include "vtkCommand.h"
#include "vtkConeSource.h"
#include "vtkGlyph3D.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkTransformFilter.h"
#include "vtkTransform.h"
#include "vtkCamera.h"

// Callback for the interaction
class vtkSBWCallback2 : public vtkCommand
{
public:
  static vtkSBWCallback2 *New()
  { return new vtkSBWCallback2; }
  virtual void Execute(vtkObject *caller, unsigned long, void*)
  {
    vtkBoxWidget2 *boxWidget =
      reinterpret_cast<vtkBoxWidget2*>(caller);
    vtkBoxRepresentation *boxRep =
      reinterpret_cast<vtkBoxRepresentation*>(boxWidget->GetRepresentation());
    boxRep->GetTransform(this->Transform);

    vtkCamera *camera = boxRep->GetRenderer()->GetActiveCamera();
//    this->Actor->SetUserTransform(this->Transform);
  }
  vtkSBWCallback2():Transform(0),Actor(0) {}
  vtkTransform *Transform;
  vtkActor     *Actor;
};

char ScaledBoxWidgetEventLog2[] =
  "# StreamVersion 1\n"
  "CharEvent 187 242 0 0 105 1 i\n"
  "KeyReleaseEvent 187 242 0 0 105 1 i\n"
  ;

int ScaledBoxWidget2( int , char *[] )
{
  vtkSmartPointer<vtkRenderer> renderer =
    vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderWindow> renWin =
    vtkSmartPointer<vtkRenderWindow>::New();
  renWin->AddRenderer(renderer);
  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  vtkSmartPointer<vtkConeSource> cone =
    vtkSmartPointer<vtkConeSource>::New();
  cone->SetResolution(6);
  vtkSmartPointer<vtkSphereSource> sphere =
    vtkSmartPointer<vtkSphereSource>::New();
  sphere->SetThetaResolution(8); sphere->SetPhiResolution(8);
  vtkSmartPointer<vtkGlyph3D> glyph =
    vtkSmartPointer<vtkGlyph3D>::New();
  glyph->SetInputConnection(sphere->GetOutputPort());
  glyph->SetSource(cone->GetOutput());
  glyph->SetVectorModeToUseNormal();
  glyph->SetScaleModeToScaleByVector();
  glyph->SetScaleFactor(0.25);
  glyph->Update();

  vtkSmartPointer<vtkAppendPolyData> append =
    vtkSmartPointer<vtkAppendPolyData>::New();
  append->AddInput(glyph->GetOutput());
  append->AddInput(sphere->GetOutput());


  vtkSmartPointer<vtkTransform> dataTransform =
    vtkSmartPointer<vtkTransform>::New();
  dataTransform->Identity();
  dataTransform->Scale(1,2,1);

  vtkSmartPointer<vtkTransformFilter> tf =
    vtkSmartPointer<vtkTransformFilter>::New();
  tf->SetTransform(dataTransform);
  tf->SetInputConnection(append->GetOutputPort());
  tf->Update();

  vtkSmartPointer<vtkPolyDataMapper> maceMapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  maceMapper->SetInputConnection(tf->GetOutputPort());

  vtkSmartPointer<vtkActor> maceActor =
    vtkSmartPointer<vtkActor>::New();
  maceActor->SetMapper(maceMapper);

  // Configure the box widget including callbacks
  vtkSmartPointer<vtkTransform> t =
    vtkSmartPointer<vtkTransform>::New();
  vtkSmartPointer<vtkSBWCallback2> myCallback =
    vtkSmartPointer<vtkSBWCallback2>::New();
  myCallback->Transform = t;
  myCallback->Actor = maceActor;

  vtkSmartPointer<vtkBoxRepresentation> boxRep =
    vtkSmartPointer<vtkBoxRepresentation>::New();
  boxRep->SetPlaceFactor( 1.25 );
  boxRep->PlaceWidget(tf->GetOutput()->GetBounds());

  vtkSmartPointer<vtkBoxWidget2> boxWidget =
    vtkSmartPointer<vtkBoxWidget2>::New();
  boxWidget->SetInteractor( iren );
  boxWidget->SetRepresentation( boxRep );
  boxWidget->AddObserver(vtkCommand::InteractionEvent,myCallback);
  boxWidget->SetPriority(1);

  renderer->AddActor(maceActor);
  renderer->SetBackground(0,0,0);
  renWin->SetSize(1024,768);

  // Introduce scale to test out calculation of clipping range
  // by vtkRenderer.
  vtkSmartPointer<vtkTransform> scaleTransform =
    vtkSmartPointer<vtkTransform>::New();
  scaleTransform->SetInput(dataTransform);

  vtkCamera *camera = renderer->GetActiveCamera();

  int cameraScale = 1;
  if ( cameraScale == 0 )
    {
    maceActor->SetUserTransform(scaleTransform);
    boxRep->SetTransform(scaleTransform);
    }
  else if ( cameraScale == 1)
    {
    camera->SetModelTransformMatrix(scaleTransform->GetMatrix());
    }
  else if ( cameraScale == 2)
    {
    camera->SetUserTransform(scaleTransform);
    }
  else if ( cameraScale == 3 )
    {
    camera->SetUserViewTransform(scaleTransform);
    }


  // record events
  vtkSmartPointer<vtkInteractorEventRecorder> recorder =
    vtkSmartPointer<vtkInteractorEventRecorder>::New();
  recorder->SetInteractor(iren);
  recorder->ReadFromInputStringOn();
  recorder->SetInputString(ScaledBoxWidgetEventLog2);

  // interact with data
  // render the image
  //
  iren->Initialize();
  renWin->Render();
  recorder->Play();

  // Remove the observers so we can go interactive. Without this the "-I"
  // testing option fails.
  recorder->Off();

  boxRep->SetPlaceFactor( 1.0 );
  boxRep->HandlesOff();

  boxRep->SetPlaceFactor( 1.25 );
  boxRep->HandlesOn();

  renderer->ResetCamera();
  iren->Start();

  return EXIT_SUCCESS;
}
