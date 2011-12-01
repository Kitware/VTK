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

  // --------------------------------------------------
   vtkSmartPointer<vtkPlaneSource> plane= vtkSmartPointer<vtkPlaneSource>::New();
   
   vtkSmartPointer<vtkPolyDataMapper> surfaceMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
   surfaceMapper->SetInputConnection( plane->GetOutputPort() );
   vtkSmartPointer<vtkActor> surfaceActor = vtkSmartPointer<vtkActor>::New();
   surfaceActor->SetMapper( surfaceMapper );
   surfaceActor->GetProperty()->SetColor(0.50, 0.50, 0.50);

   vtkSmartPointer<vtkPolyDataMapper> edgeMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
   edgeMapper->SetInputConnection( plane->GetOutputPort() );
   vtkSmartPointer<vtkActor> edgeActor = vtkSmartPointer<vtkActor>::New();
   edgeActor->SetMapper( edgeMapper );
   edgeActor->GetProperty()->SetColor(0.0, 0.0, 0.0);
   edgeActor->GetProperty()->SetRepresentationToWireframe();

  // --------------------------------------------------
   vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
   vtkSmartPointer<vtkRenderWindow> renderWindow = vtkSmartPointer<vtkRenderWindow>::New();
   renderWindow->AddRenderer(renderer);
   vtkSmartPointer<vtkRenderWindowInteractor> interactor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
   interactor->SetRenderWindow(renderWindow);

  // --------------------------------------------------
  // --- Les axes
  vtkSmartPointer<vtkCubeAxesActor> axes = vtkSmartPointer<vtkCubeAxesActor>::New();
  axes->SetCamera (renderer->GetActiveCamera());
  axes->SetCornerOffset(0.0);
  axes->SetXAxisVisibility(1);
  axes->SetYAxisVisibility(1);
  axes->SetZAxisVisibility(0);
            
 // pour avoir des axes 2D (textactor au lieu de follower)
  //axes->SetUse2DMode(1);
  axes->SetBounds(-0.5,0.5,-0.5,0.5,0.0,0.0);


  // --------------------------------------------------
   renderer->AddActor( surfaceActor );
   renderer->AddActor( edgeActor );
   renderer->AddActor( axes );
   
   renderer->SetBackground(.3,.6,.3);
   renderer->GetActiveCamera()->SetFocalPoint(0.,0.,0.);
   renderer->GetActiveCamera()->SetPosition(0.,0.,2.5);
   renderWindow->SetSize(800,600);

   renderWindow->Render();
   interactor->Start();
   
   return EXIT_SUCCESS;
}
