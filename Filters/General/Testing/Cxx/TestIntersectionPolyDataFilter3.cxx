/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestIntersectionPolyDataFilter3.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/

#include <vtkIntersectionPolyDataFilter.h>

#include <vtkActor.h>
#include <vtkConeSource.h>
#include <vtkCubeSource.h>
#include <vtkLinearSubdivisionFilter.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkSmartPointer.h>
#include <vtkSphereSource.h>
#include <vtkTriangleFilter.h>

int TestIntersectionPolyDataFilter3(int, char *[])
{
  vtkSmartPointer<vtkCubeSource> cubeSource =
    vtkSmartPointer<vtkCubeSource>::New();
  cubeSource->SetCenter(0.0, 0.0, 0.0);
  cubeSource->SetXLength(1.0);
  cubeSource->SetYLength(1.0);
  cubeSource->SetZLength(1.0);
  cubeSource->Update();
  vtkSmartPointer<vtkTriangleFilter> cubetriangulator =
    vtkSmartPointer<vtkTriangleFilter>::New();
  cubetriangulator->SetInputConnection(cubeSource->GetOutputPort());
  vtkSmartPointer<vtkLinearSubdivisionFilter> cubesubdivider =
    vtkSmartPointer<vtkLinearSubdivisionFilter>::New();
  cubesubdivider->SetInputConnection(cubetriangulator->GetOutputPort());
  cubesubdivider->SetNumberOfSubdivisions(3);
  vtkSmartPointer<vtkPolyDataMapper> cubeMapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  cubeMapper->SetInputConnection( cubesubdivider->GetOutputPort() );
  cubeMapper->ScalarVisibilityOff();
  vtkSmartPointer<vtkActor> cubeActor =
    vtkSmartPointer<vtkActor>::New();
  cubeActor->SetMapper( cubeMapper );
  cubeActor->GetProperty()->SetOpacity(.3);
  cubeActor->GetProperty()->SetColor(1,0,0);
  cubeActor->GetProperty()->SetInterpolationToFlat();

  vtkSmartPointer<vtkConeSource> coneSource =
    vtkSmartPointer<vtkConeSource>::New();
  coneSource->SetCenter(0.0, 0.0, 0.0);
  coneSource->SetRadius(0.5);
  coneSource->SetHeight(2.0);
  coneSource->SetResolution(10.0);
  coneSource->SetDirection(1,0,0);
  vtkSmartPointer<vtkTriangleFilter> conetriangulator =
    vtkSmartPointer<vtkTriangleFilter>::New();
  conetriangulator->SetInputConnection(coneSource->GetOutputPort());
  vtkSmartPointer<vtkLinearSubdivisionFilter> conesubdivider =
    vtkSmartPointer<vtkLinearSubdivisionFilter>::New();
  conesubdivider->SetInputConnection(conetriangulator->GetOutputPort());
  conesubdivider->SetNumberOfSubdivisions(3);
  vtkSmartPointer<vtkPolyDataMapper> coneMapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  coneMapper->SetInputConnection( conesubdivider->GetOutputPort() );
  coneMapper->ScalarVisibilityOff();
  vtkSmartPointer<vtkActor> coneActor =
    vtkSmartPointer<vtkActor>::New();
  coneActor->SetMapper( coneMapper );
  coneActor->GetProperty()->SetOpacity(.3);
  coneActor->GetProperty()->SetColor(0,1,0);
  coneActor->GetProperty()->SetInterpolationToFlat();

  vtkSmartPointer<vtkIntersectionPolyDataFilter> intersectionPolyDataFilter =
    vtkSmartPointer<vtkIntersectionPolyDataFilter>::New();
  intersectionPolyDataFilter->SetInputConnection( 0, cubesubdivider->GetOutputPort() );
  intersectionPolyDataFilter->SetInputConnection( 1, conesubdivider->GetOutputPort() );
  intersectionPolyDataFilter->SetSplitFirstOutput(0);
  intersectionPolyDataFilter->SetSplitSecondOutput(0);
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
  renderer->AddViewProp(cubeActor);
  renderer->AddViewProp(coneActor);
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
