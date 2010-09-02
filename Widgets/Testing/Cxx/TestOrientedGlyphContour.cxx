/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOrientedGlyphContour.cxx

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

#include "vtkContourWidget.h"
#include "vtkOrientedGlyphContourRepresentation.h"
#include "vtkCoordinate.h"
#include "vtkActor2D.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkCommand.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkImageActor.h"
#include "vtkVolume16Reader.h"
#include "vtkImageShiftScale.h"
#include "vtkTestUtilities.h"
#include "vtkImageData.h"
#include "vtkProperty2D.h"
#include "vtkCamera.h"
#include "vtkPlane.h"
#include "vtkBoundedPlanePointPlacer.h"
#include "vtkWidgetEventTranslator.h"
#include "vtkWidgetEvent.h"
#include "vtkEvent.h"

int TestOrientedGlyphContour( int argc, char *argv[] )
{
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/headsq/quarter");
   
  vtkSmartPointer<vtkVolume16Reader> v16 =
    vtkSmartPointer<vtkVolume16Reader>::New();
  v16->SetDataDimensions(64, 64);
  v16->SetDataByteOrderToLittleEndian();
  v16->SetImageRange(1, 93);
  v16->SetDataSpacing(3.2, 3.2, 1.5);
  v16->SetFilePrefix(fname);
  v16->ReleaseDataFlagOn();
  v16->SetDataMask(0x7fff);
  v16->Update();
  delete[] fname;
    
  double range[2];
  v16->GetOutput()->GetScalarRange(range);

  vtkSmartPointer<vtkImageShiftScale> shifter =
    vtkSmartPointer<vtkImageShiftScale>::New();
  shifter->SetShift(-1.0*range[0]);
  shifter->SetScale(255.0/(range[1]-range[0]));
  shifter->SetOutputScalarTypeToUnsignedChar();
  shifter->SetInputConnection(v16->GetOutputPort());
  shifter->ReleaseDataFlagOff();
  shifter->Update();
  
  vtkSmartPointer<vtkImageActor> imageActor =
    vtkSmartPointer<vtkImageActor>::New();
  imageActor->SetInput(shifter->GetOutput());
  imageActor->VisibilityOn();
  imageActor->SetDisplayExtent(0, 63, 0, 63, 46, 46);
  imageActor->InterpolateOn();

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

  // Add the actors to the renderer, set the background and size
  //
  ren1->SetBackground(0.1, 0.2, 0.4);
  ren1->AddActor(imageActor);
  renWin->SetSize(600, 600);
  
  // render the image
  //
  ren1->GetActiveCamera()->SetPosition(0,0,0);
  ren1->GetActiveCamera()->SetFocalPoint(0,0,1);  
  ren1->GetActiveCamera()->SetViewUp(0,1,0);
  ren1->ResetCamera();
  renWin->Render();

  double bounds[6];
  imageActor->GetBounds( bounds );
  
  vtkSmartPointer<vtkPlane> p1 =
    vtkSmartPointer<vtkPlane>::New();
  p1->SetOrigin( bounds[0], bounds[2], bounds[4] );
  p1->SetNormal( 1.0, 0.0, 0.0 );

  vtkSmartPointer<vtkPlane> p2 =
    vtkSmartPointer<vtkPlane>::New();
  p2->SetOrigin( bounds[0], bounds[2], bounds[4] );
  p2->SetNormal( 0.0, 1.0, 0.0 );

  vtkSmartPointer<vtkPlane> p3 =
    vtkSmartPointer<vtkPlane>::New();
  p3->SetOrigin( bounds[1], bounds[3], bounds[5] );
  p3->SetNormal( -1.0, 0.0, 0.0 );

  vtkSmartPointer<vtkPlane> p4 =
    vtkSmartPointer<vtkPlane>::New();
  p4->SetOrigin( bounds[1], bounds[3], bounds[5] );
  p4->SetNormal( 0.0, -1.0, 0.0 );

  vtkSmartPointer<vtkOrientedGlyphContourRepresentation> contourRep =
    vtkSmartPointer<vtkOrientedGlyphContourRepresentation>::New();
  vtkSmartPointer<vtkContourWidget> contourWidget =
    vtkSmartPointer<vtkContourWidget>::New();
  vtkSmartPointer<vtkBoundedPlanePointPlacer> placer =
    vtkSmartPointer<vtkBoundedPlanePointPlacer>::New();
  
  contourWidget->SetInteractor(iren);
  contourWidget->SetRepresentation(contourRep);

  // Change bindings.
  vtkWidgetEventTranslator *eventTranslator = contourWidget->GetEventTranslator();
  eventTranslator->RemoveTranslation( vtkCommand::RightButtonPressEvent );
  eventTranslator->SetTranslation( 
    vtkCommand::KeyPressEvent,
    vtkEvent::NoModifier, 103, 0, "g",
    vtkWidgetEvent::AddFinalPoint );

  contourWidget->On();

  contourRep->SetPointPlacer( placer );
  
  placer->SetProjectionNormalToZAxis();
  placer->SetProjectionPosition(imageActor->GetCenter()[2]);
   
  placer->AddBoundingPlane(p1);
  placer->AddBoundingPlane(p2);
  placer->AddBoundingPlane(p3);
  placer->AddBoundingPlane(p4);
    
  iren->Initialize();
  
  iren->Start();

  return EXIT_SUCCESS;

}
