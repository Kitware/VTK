/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestBalloonWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// This example tests the vtkHoverWidget and vtkBalloonWidget.

// First include the required header files for the VTK classes we are using.
#include "vtkSmartPointer.h"
#include "vtkBalloonWidget.h"
#include "vtkBalloonRepresentation.h"
#include "vtkSphereSource.h"
#include "vtkCylinderSource.h"
#include "vtkConeSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkCommand.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkTestUtilities.h"
#include "vtkTIFFReader.h"
#include "vtkPropPicker.h"

class vtkBalloonCallback : public vtkCommand
{
public:
  static vtkBalloonCallback *New()
    { return new vtkBalloonCallback; }
  void Execute(vtkObject *caller, unsigned long, void*) VTK_OVERRIDE
  {
      vtkBalloonWidget *balloonWidget = reinterpret_cast<vtkBalloonWidget*>(caller);
      if ( balloonWidget->GetCurrentProp() != NULL )
      {
        std::cout << "Prop selected\n";
      }
  }

  vtkActor *PickedActor;

};

class vtkBalloonPickCallback : public vtkCommand
{
public:
  static vtkBalloonPickCallback *New()
    { return new vtkBalloonPickCallback; }
  void Execute(vtkObject *caller, unsigned long, void*) VTK_OVERRIDE
  {
      vtkPropPicker *picker = reinterpret_cast<vtkPropPicker*>(caller);
      vtkProp *prop = picker->GetViewProp();
      if ( prop != NULL )
      {
        this->BalloonWidget->UpdateBalloonString(prop,"Picked");
      }
  }
  vtkBalloonWidget *BalloonWidget;
};

int TestBalloonWidget( int argc, char *argv[] )
{
  // Create the RenderWindow, Renderer and both Actors
  //
  vtkSmartPointer<vtkRenderer> ren1 = vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderWindow> renWin = vtkSmartPointer<vtkRenderWindow>::New();
  renWin->AddRenderer(ren1);

  vtkSmartPointer<vtkRenderWindowInteractor> iren = vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  vtkSmartPointer<vtkPropPicker> picker = vtkSmartPointer<vtkPropPicker>::New();
  vtkSmartPointer<vtkBalloonPickCallback> pcbk = vtkSmartPointer<vtkBalloonPickCallback>::New();
  picker->AddObserver(vtkCommand::PickEvent,pcbk);
  iren->SetPicker(picker);

  // Create an image for the balloon widget
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/beach.tif");
  vtkSmartPointer<vtkTIFFReader> image1 = vtkSmartPointer<vtkTIFFReader>::New();
  image1->SetFileName(fname);
  image1->SetOrientationType( 4 );

  // Create a test pipeline
  //
  vtkSmartPointer<vtkSphereSource> ss = vtkSmartPointer<vtkSphereSource>::New();
  vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputConnection(ss->GetOutputPort());
  vtkSmartPointer<vtkActor> sph = vtkSmartPointer<vtkActor>::New();
  sph->SetMapper(mapper);

  vtkSmartPointer<vtkCylinderSource> cs = vtkSmartPointer<vtkCylinderSource>::New();
  vtkSmartPointer<vtkPolyDataMapper> csMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  csMapper->SetInputConnection(cs->GetOutputPort());
  vtkSmartPointer<vtkActor> cyl = vtkSmartPointer<vtkActor>::New();
  cyl->SetMapper(csMapper);
  cyl->AddPosition(5,0,0);

  vtkSmartPointer<vtkConeSource> coneSource = vtkSmartPointer<vtkConeSource>::New();
  vtkSmartPointer<vtkPolyDataMapper> coneMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  coneMapper->SetInputConnection(coneSource->GetOutputPort());
  vtkSmartPointer<vtkActor> cone = vtkSmartPointer<vtkActor>::New();
  cone->SetMapper(coneMapper);
  cone->AddPosition(0,5,0);

  // Create the widget
  vtkSmartPointer<vtkBalloonRepresentation> rep = vtkSmartPointer<vtkBalloonRepresentation>::New();
  rep->SetBalloonLayoutToImageRight();

  vtkSmartPointer<vtkBalloonWidget> widget = vtkSmartPointer<vtkBalloonWidget>::New();
  widget->SetInteractor(iren);
  widget->SetRepresentation(rep);
  widget->AddBalloon(sph,"This is a sphere",NULL);
  widget->AddBalloon(cyl,"This is a\ncylinder",image1->GetOutput());
  widget->AddBalloon(cone,"This is a\ncone,\na really big cone,\nyou wouldn't believe how big",image1->GetOutput());
  pcbk->BalloonWidget = widget;

  vtkSmartPointer<vtkBalloonCallback> cbk = vtkSmartPointer<vtkBalloonCallback>::New();
  widget->AddObserver(vtkCommand::WidgetActivateEvent,cbk);

  // Add the actors to the renderer, set the background and size
  //
  ren1->AddActor(sph);
  ren1->AddActor(cyl);
  ren1->AddActor(cone);
  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(300, 300);

  // record events
  vtkSmartPointer<vtkInteractorEventRecorder> recorder = vtkSmartPointer<vtkInteractorEventRecorder>::New();
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

  iren->Start();

  delete [] fname;

  return EXIT_SUCCESS;

}


