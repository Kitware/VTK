/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestLogoWidgetAlphaBlending.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// This example tests the vtkLogoWidget with alpha blending.
// The translucent sphere uses alpha blending. The logo image is translucent
// on the overlay. This test makes sure that rendering translucent geometry
// with alpha blending on the main layer restores the
// blending state to render translucent geometry on the overlay.

// First include the required header files for the VTK classes we are using.
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
#include "vtkRegressionTestImage.h"
#include "vtkDebugLeaks.h"
#include "vtkTestUtilities.h"
#include "vtkTIFFReader.h"
#include "vtkProperty.h"

int TestLogoWidgetAlphaBlending( int argc, char *argv[] )
{
  // Create the RenderWindow, Renderer and both Actors
  //
  vtkRenderer *ren1 = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->AddRenderer(ren1);
  ren1->Delete();
  renWin->SetMultiSamples(1);
  renWin->SetAlphaBitPlanes(1);
  
  ren1->SetUseDepthPeeling(0);
  ren1->SetMaximumNumberOfPeels(200);
  ren1->SetOcclusionRatio(0.1);
  
  vtkInteractorStyleTrackballCamera *style = vtkInteractorStyleTrackballCamera::New();
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);
  renWin->Delete();
  iren->SetInteractorStyle(style);
  style->Delete();

  // Create an image for the balloon widget
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/beach.tif");
  vtkTIFFReader *image1 = vtkTIFFReader::New();
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
  vtkSphereSource *ss = vtkSphereSource::New();
  vtkPolyDataMapper *mapper = vtkPolyDataMapper::New();
  mapper->SetInput(ss->GetOutput());
  ss->Delete();
  vtkActor *sph = vtkActor::New();
  sph->SetMapper(mapper);
  mapper->Delete();

  vtkProperty *property=vtkProperty::New();
  property->SetOpacity(0.2);
  property->SetColor(0.0,1.0,0.0);
  sph->SetProperty(property);
  property->Delete();
  
  vtkCylinderSource *cs = vtkCylinderSource::New();
  vtkPolyDataMapper *csMapper = vtkPolyDataMapper::New();
  csMapper->SetInput(cs->GetOutput());
  cs->Delete();
  vtkActor *cyl = vtkActor::New();
  cyl->SetMapper(csMapper);
  csMapper->Delete();
  cyl->AddPosition(5,0,0);
  
  vtkConeSource *coneSource = vtkConeSource::New();
  vtkPolyDataMapper *coneMapper = vtkPolyDataMapper::New();
  coneMapper->SetInput(coneSource->GetOutput());
  coneSource->Delete();
  vtkActor *cone = vtkActor::New();
  cone->SetMapper(coneMapper);
  coneMapper->Delete();
  cone->AddPosition(0,5,0);

  // Create the widget
  vtkLogoRepresentation *rep = vtkLogoRepresentation::New();
  rep->SetImage(image1->GetOutput());
  image1->Delete();

  vtkLogoWidget *widget = vtkLogoWidget::New();
  widget->SetInteractor(iren);
  widget->SetRepresentation(rep);
  rep->Delete();

  // Add the actors to the renderer, set the background and size
  //
  ren1->AddActor(sph);
  sph->Delete();
  ren1->AddActor(cyl);
  cyl->Delete();
  ren1->AddActor(cone);
  cone->Delete();
  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(300, 300);

  // record events
  vtkInteractorEventRecorder *recorder = vtkInteractorEventRecorder::New();
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

  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }
  
  widget->Off();
  widget->Delete();
  iren->Delete();
  recorder->Delete();
 
  return !retVal;

}
