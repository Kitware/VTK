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
// This example tests the placement of vtkDistanceWidget, vtkAngleWidget,
// vtkBiDimensionalWidget

// First include the required header files for the VTK classes we are using.
#include "vtkDistanceWidget.h"
#include "vtkDistanceRepresentation2D.h"
#include "vtkDistanceRepresentation3D.h"
#include "vtkAngleWidget.h"
#include "vtkAngleRepresentation2D.h"
#include "vtkBiDimensionalWidget.h"
#include "vtkBiDimensionalRepresentation2D.h"
#include "vtkSphereSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkCommand.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkRegressionTestImage.h"
#include "vtkDebugLeaks.h"
#include "vtkCoordinate.h"
#include "vtkMath.h"
#include "vtkHandleWidget.h"
#include "vtkPointHandleRepresentation2D.h"
#include "vtkPointHandleRepresentation3D.h"
#include "vtkAxisActor2D.h"
#include "vtkProperty2D.h"
#include "vtkProperty.h"

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()


// The actual test function
int TestProgrammaticPlacement( int argc, char *argv[] )
{
  // Create the RenderWindow, Renderer and both Actors
  //
  VTK_CREATE(vtkRenderer, ren1);
  VTK_CREATE(vtkRenderWindow, renWin);
  renWin->AddRenderer(ren1);

  VTK_CREATE(vtkRenderWindowInteractor, iren);;
  iren->SetRenderWindow(renWin);

  // Create a test pipeline
  //
  VTK_CREATE(vtkSphereSource, ss);
  VTK_CREATE(vtkPolyDataMapper, mapper);
  mapper->SetInputConnection(ss->GetOutputPort());
  VTK_CREATE(vtkActor, actor);
  actor->SetMapper(mapper);

  // Create the widget and its representation
  VTK_CREATE(vtkPointHandleRepresentation2D, handle);
  handle->GetProperty()->SetColor(1,0,0);

  VTK_CREATE(vtkDistanceRepresentation2D, dRep);
  dRep->SetHandleRepresentation(handle);
  dRep->InstantiateHandleRepresentation();
  dRep->GetAxis()->SetNumberOfMinorTicks(4);
  dRep->GetAxis()->SetTickLength(9);
  dRep->GetAxis()->SetTitlePosition(0.2);
  dRep->RulerModeOn();
  dRep->SetRulerDistance(0.25);

  VTK_CREATE(vtkDistanceWidget, dWidget);
  dWidget->SetInteractor(iren);
  dWidget->SetRepresentation(dRep);
  dWidget->SetWidgetStateToManipulate();

  // Create the widget and its representation
  VTK_CREATE(vtkPointHandleRepresentation3D, handle2);
  handle2->GetProperty()->SetColor(1,1,0);

  VTK_CREATE(vtkDistanceRepresentation3D, dRep2);
  dRep2->SetHandleRepresentation(handle2);
  dRep2->InstantiateHandleRepresentation();
  dRep2->RulerModeOn();
  dRep2->SetRulerDistance(0.25);

  VTK_CREATE(vtkDistanceWidget, dWidget2);
  dWidget2->SetInteractor(iren);
  dWidget2->SetRepresentation(dRep2);
  dWidget2->SetWidgetStateToManipulate();

  // Add the actors to the renderer, set the background and size
  //
  ren1->AddActor(actor);
  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(300, 300);

  // render the image
  //
  iren->Initialize();
  renWin->Render();
  dWidget->On();
  dWidget2->On();

  double p[3]; p[0] = 25; p[1] = 50; p[2] = 0;
  dRep->SetPoint1DisplayPosition(p);
  p[0] = 275; p[1] = 250; p[2] = 0;
  dRep->SetPoint2DisplayPosition(p);

  p[0] = -0.75; p[1] = 0.75; p[2] = 0;
  dRep2->SetPoint1WorldPosition(p);
  p[0] = 0.75; p[1] = -0.75; p[2] = 0;
  dRep2->SetPoint2WorldPosition(p);

  renWin->Render();

  // Remove the observers so we can go interactive. Without this the "-I"
  // testing option fails.
  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }
  dWidget->Off();

  return !retVal;
}
