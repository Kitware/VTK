/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestLogoWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// This example tests the vtkLogoWidget.

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

int TestLogoWidget( int argc, char *argv[] )
{
  // Create the RenderWindow, Renderer and both Actors
  //
  vtkRenderer *ren1 = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->AddRenderer(ren1);

  vtkInteractorStyleTrackballCamera *style = vtkInteractorStyleTrackballCamera::New();
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);
  iren->SetInteractorStyle(style);

  // Create an image for the balloon widget
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/beach.tif");
  vtkTIFFReader *image1 = vtkTIFFReader::New();
  image1->SetFileName(fname);
  image1->SetOrientation( 4 );     

  // Create a test pipeline
  //
  vtkSphereSource *ss = vtkSphereSource::New();
  vtkPolyDataMapper *mapper = vtkPolyDataMapper::New();
  mapper->SetInput(ss->GetOutput());
  vtkActor *sph = vtkActor::New();
  sph->SetMapper(mapper);

  vtkCylinderSource *cs = vtkCylinderSource::New();
  vtkPolyDataMapper *csMapper = vtkPolyDataMapper::New();
  csMapper->SetInput(cs->GetOutput());
  vtkActor *cyl = vtkActor::New();
  cyl->SetMapper(csMapper);
  cyl->AddPosition(5,0,0);
  
  vtkConeSource *coneSource = vtkConeSource::New();
  vtkPolyDataMapper *coneMapper = vtkPolyDataMapper::New();
  coneMapper->SetInput(coneSource->GetOutput());
  vtkActor *cone = vtkActor::New();
  cone->SetMapper(coneMapper);
  cone->AddPosition(0,5,0);

  // Create the widget
  vtkLogoRepresentation *rep = vtkLogoRepresentation::New();
  rep->SetImage(image1->GetOutput());

  vtkLogoWidget *widget = vtkLogoWidget::New();
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
  sph->Delete();
  widget->Off();
  widget->Delete();
  iren->Delete();
  renWin->Delete();
  ren1->Delete();
  recorder->Delete();
  delete [] fname;

  return !retVal;

}


