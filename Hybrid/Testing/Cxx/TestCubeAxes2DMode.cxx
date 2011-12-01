/*=========================================================================

Program:   Visualization Toolkit
Module:    TestCubeAxesWithGridlines.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .SECTION Thanks
// This test was written by Philippe Pebay, Kitware SAS 2011

#include "vtkSmartPointer.h"
#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkProperty2D.h"
#include "vtkCoordinate.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkCubeAxesActor.h"
#include "vtkAxisActor.h"

#include "vtkPlaneSource.h"
#include "vtkStructuredGrid.h"
#include "vtkProperty.h"
#include "vtkPolyDataMapper.h"

#include "vtkCubeAxesActor.h"

int TestCubeAxes2DMode(int, char *[])
{
  // Create plane source
  vtkSmartPointer<vtkPlaneSource> plane
    = vtkSmartPointer<vtkPlaneSource>::New();

  // Create plane mapper
  vtkSmartPointer<vtkPolyDataMapper> planeMapper
    = vtkSmartPointer<vtkPolyDataMapper>::New();
  planeMapper->SetInputConnection( plane->GetOutputPort() );

  // Create plane actor
  vtkSmartPointer<vtkActor> planeActor
    = vtkSmartPointer<vtkActor>::New();
  planeActor->SetMapper( planeMapper );
  planeActor->GetProperty()->SetColor( .5, .5, .5 );

  // Create edge mapper and actor
  vtkSmartPointer<vtkPolyDataMapper> edgeMapper
    = vtkSmartPointer<vtkPolyDataMapper>::New();
  edgeMapper->SetInputConnection( plane->GetOutputPort() );

  // Create edge actor
  vtkSmartPointer<vtkActor> edgeActor
    = vtkSmartPointer<vtkActor>::New();
  edgeActor->SetMapper( edgeMapper );
  edgeActor->GetProperty()->SetColor( .0, .0, .0 );
  edgeActor->GetProperty()->SetRepresentationToWireframe();

  // Create cube axes actor
  vtkSmartPointer<vtkCubeAxesActor> axes = vtkSmartPointer<vtkCubeAxesActor>::New();
  axes->SetCamera (renderer->GetActiveCamera());
  axes->SetCornerOffset( .0 );
  axes->SetXAxisVisibility( 1 );
  axes->SetYAxisVisibility( 1 );
  axes->SetZAxisVisibility( 0 );
  //axes->SetUse2DMode(1);
  axes->SetBounds( -.5, .5, .5, .5, 0., 0. );

  // Create renderer
  vtkSmartPointer<vtkRenderer> renderer
    = vtkSmartPointer<vtkRenderer>::New();
  renderer->AddActor( planeActor );
  renderer->AddActor( edgeActor );
  renderer->AddActor( axes );
  renderer->SetBackground( .3, .6, .3 );
  renderer->GetActiveCamera()->SetFocalPoint( .0, .0, .0 );
  renderer->GetActiveCamera()->SetPosition( .0, .0, 2.5 );

  // Create render window and interactor
  vtkSmartPointer<vtkRenderWindow> renderWindow
    = vtkSmartPointer<vtkRenderWindow>::New();
  renderWindow->AddRenderer( renderer );
  renderWindow->SetSize( 800, 600 );
  vtkSmartPointer<vtkRenderWindowInteractor> interactor
    = vtkSmartPointer<vtkRenderWindowInteractor>::New();
  interactor->SetRenderWindow( renderWindow );

  // Render and possibly interact
  renderWindow->Render();
  int retVal = vtkRegressionTestImage( renWin.GetPointer() );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
      iren->Start();
    }
   
  return !retVal;
}
