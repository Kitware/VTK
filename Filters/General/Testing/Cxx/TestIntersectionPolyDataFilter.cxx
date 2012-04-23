/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestIntersectionPolyDataFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/

#include <vtkIntersectionPolyDataFilter.h>

#include <vtkActor.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkSmartPointer.h>
#include <vtkSphereSource.h>

int TestIntersectionPolyDataFilter(int, char *[])
{
  vtkSmartPointer<vtkSphereSource> sphereSource1 =
    vtkSmartPointer<vtkSphereSource>::New();
  sphereSource1->SetCenter(0.0, 0.0, 0.0);
  sphereSource1->SetRadius(2.0);
  sphereSource1->SetPhiResolution(11);
  sphereSource1->SetThetaResolution(21);
  sphereSource1->Update();
  vtkSmartPointer<vtkPolyDataMapper> sphere1Mapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  sphere1Mapper->SetInputConnection( sphereSource1->GetOutputPort() );
  sphere1Mapper->ScalarVisibilityOff();
  vtkSmartPointer<vtkActor> sphere1Actor =
    vtkSmartPointer<vtkActor>::New();
  sphere1Actor->SetMapper( sphere1Mapper );
  sphere1Actor->GetProperty()->SetOpacity(.3);
  sphere1Actor->GetProperty()->SetColor(1,0,0);
  sphere1Actor->GetProperty()->SetInterpolationToFlat();

  vtkSmartPointer<vtkSphereSource> sphereSource2 =
    vtkSmartPointer<vtkSphereSource>::New();
  sphereSource2->SetCenter(1.0, 0.0, 0.0);
  sphereSource2->SetRadius(2.0);
  vtkSmartPointer<vtkPolyDataMapper> sphere2Mapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  sphere2Mapper->SetInputConnection( sphereSource2->GetOutputPort() );
  sphere2Mapper->ScalarVisibilityOff();
  vtkSmartPointer<vtkActor> sphere2Actor =
    vtkSmartPointer<vtkActor>::New();
  sphere2Actor->SetMapper( sphere2Mapper );
  sphere2Actor->GetProperty()->SetOpacity(.3);
  sphere2Actor->GetProperty()->SetColor(0,1,0);
  sphere2Actor->GetProperty()->SetInterpolationToFlat();

  vtkSmartPointer<vtkIntersectionPolyDataFilter> intersectionPolyDataFilter =
    vtkSmartPointer<vtkIntersectionPolyDataFilter>::New();
  intersectionPolyDataFilter->SetInputConnection( 0, sphereSource1->GetOutputPort() );
  intersectionPolyDataFilter->SetInputConnection( 1, sphereSource2->GetOutputPort() );
  intersectionPolyDataFilter->Update();

  vtkSmartPointer<vtkPolyDataMapper> intersectionMapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  intersectionMapper->SetInputConnection( intersectionPolyDataFilter->GetOutputPort() );
  intersectionMapper->ScalarVisibilityOff();

  vtkSmartPointer<vtkActor> intersectionActor =
    vtkSmartPointer<vtkActor>::New();
  intersectionActor->SetMapper( intersectionMapper );

  vtkSmartPointer<vtkRenderer> renderer =
    vtkSmartPointer<vtkRenderer>::New();
  renderer->AddViewProp(sphere1Actor);
  renderer->AddViewProp(sphere2Actor);
  renderer->AddViewProp(intersectionActor);
  renderer->SetBackground(.1, .2, .3);

  vtkSmartPointer<vtkRenderWindow> renderWindow
    = vtkSmartPointer<vtkRenderWindow>::New();
  renderWindow->AddRenderer( renderer );

  vtkSmartPointer<vtkRenderWindowInteractor> renWinInteractor =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  renWinInteractor->SetRenderWindow( renderWindow );

  intersectionPolyDataFilter->Print(std::cout);

  renderWindow->Render();
  renWinInteractor->Start();

  return EXIT_SUCCESS;
}
