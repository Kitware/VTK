/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestLogoWidgetDepthPeeling.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// This example tests the vtkLogoWidget with depth peeling.
// The translucent sphere uses depth peeling. The logo image is translucent
// on the overlay. This test makes sure that depth peeling restore the
// blending state to render translucent geometry on the overlay.

// First include the required header files for the VTK classes we are using.
#include "vtkSmartPointer.h"

#include "vtkLogoWidget.h"
#include "vtkLogoRepresentation.h"
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
#include "vtkProperty.h"

int TestLogoWidgetDepthPeeling( int argc, char *argv[] )
{
  // Create the RenderWindow, Renderer and both Actors
  //
  vtkSmartPointer<vtkRenderer> ren1 =
    vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderWindow> renWin =
    vtkSmartPointer<vtkRenderWindow>::New();
  renWin->AddRenderer(ren1);
  renWin->SetMultiSamples(1);
  renWin->SetAlphaBitPlanes(1);
  
  ren1->SetUseDepthPeeling(1);
  ren1->SetMaximumNumberOfPeels(200);
  ren1->SetOcclusionRatio(0.1);
  
  vtkSmartPointer<vtkInteractorStyleTrackballCamera> style =
    vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);
  iren->SetInteractorStyle(style);

  // Create an image for the balloon widget
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/beach.tif");
  vtkSmartPointer<vtkTIFFReader> image1 =
    vtkSmartPointer<vtkTIFFReader>::New();
  image1->SetFileName(fname);
  /* 
  "beach.tif" image contains ORIENTATION tag which is 
  ORIENTATION_TOPLEFT (row 0 top, col 0 lhs) type. The TIFF 
  reader parses this tag and sets the internal TIFF image 
  orientation accordingly.  To overwrite this orientation with a vtk
  convention of ORIENTATION_BOTLEFT (row 0 bottom, col 0 lhs ), invoke
  SetOrientationType method with parameter value of 4.
  */
  image1->SetOrientationType( 4 );
  delete [] fname;

  // Create a test pipeline
  //
  vtkSmartPointer<vtkSphereSource> ss =
    vtkSmartPointer<vtkSphereSource>::New();
  vtkSmartPointer<vtkPolyDataMapper> mapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInput(ss->GetOutput());
  vtkSmartPointer<vtkActor> sph =
    vtkSmartPointer<vtkActor>::New();
  sph->SetMapper(mapper);

  vtkSmartPointer<vtkProperty> property=
    vtkSmartPointer<vtkProperty>::New();
  property->SetOpacity(0.2);
  property->SetColor(0.0,1.0,0.0);
  sph->SetProperty(property);
  
  vtkSmartPointer<vtkCylinderSource> cs =
    vtkSmartPointer<vtkCylinderSource>::New();
  vtkSmartPointer<vtkPolyDataMapper> csMapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  csMapper->SetInput(cs->GetOutput());
  vtkSmartPointer<vtkActor> cyl =
    vtkSmartPointer<vtkActor>::New();
  cyl->SetMapper(csMapper);
  cyl->AddPosition(5,0,0);
  
  vtkSmartPointer<vtkConeSource> coneSource =
    vtkSmartPointer<vtkConeSource>::New();
  vtkSmartPointer<vtkPolyDataMapper> coneMapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  coneMapper->SetInput(coneSource->GetOutput());
  vtkSmartPointer<vtkActor> cone =
    vtkSmartPointer<vtkActor>::New();
  cone->SetMapper(coneMapper);
  cone->AddPosition(0,5,0);

  // Create the widget
  vtkSmartPointer<vtkLogoRepresentation> rep =
    vtkSmartPointer<vtkLogoRepresentation>::New();
  rep->SetImage(image1->GetOutput());

  vtkSmartPointer<vtkLogoWidget> widget =
    vtkSmartPointer<vtkLogoWidget>::New();
  widget->SetInteractor(iren);
  widget->SetRepresentation(rep);

  // Add the actors to the renderer, set the background and size
  //
  ren1->AddActor(sph);
  ren1->AddActor(cyl);
  ren1->AddActor(cone);
  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(300, 300);

  // record events
  vtkSmartPointer<vtkInteractorEventRecorder> recorder =
    vtkSmartPointer<vtkInteractorEventRecorder>::New();
  recorder->SetInteractor(iren);
//  recorder->SetFileName("c:/record.log");
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
 
  return EXIT_SUCCESS;

}
