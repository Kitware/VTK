/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDistancePolyDataFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include <vtkSmartPointer.h>

#include <vtkActor.h>
#include <vtkDistancePolyDataFilter.h>
#include <vtkPolyDataMapper.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkSphereSource.h>
#include <vtkPointData.h>
#include <vtkScalarBarActor.h>

int TestDistancePolyDataFilter(int, char*[])
{
  vtkSmartPointer<vtkSphereSource> model1 =
    vtkSmartPointer<vtkSphereSource>::New();
  model1->SetPhiResolution(11);
  model1->SetThetaResolution(11);
  model1->SetCenter (0.0, 0.0, 0.0);

  vtkSmartPointer<vtkSphereSource> model2 =
    vtkSmartPointer<vtkSphereSource>::New();
  model2->SetPhiResolution(11);
  model2->SetThetaResolution(11);
  model2->SetCenter (0.2, 0.3, 0.0);

  vtkSmartPointer<vtkDistancePolyDataFilter> distanceFilter =
    vtkSmartPointer<vtkDistancePolyDataFilter>::New();

  distanceFilter->SetInputConnection( 0, model1->GetOutputPort() );
  distanceFilter->SetInputConnection( 1, model2->GetOutputPort() );
  distanceFilter->Update();

  vtkSmartPointer<vtkPolyDataMapper> mapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputConnection( distanceFilter->GetOutputPort() );
  mapper->SetScalarRange(
    distanceFilter->GetOutput()->GetPointData()->GetScalars()->GetRange()[0],
    distanceFilter->GetOutput()->GetPointData()->GetScalars()->GetRange()[1]);

  vtkSmartPointer<vtkActor> actor =
    vtkSmartPointer<vtkActor>::New();
  actor->SetMapper( mapper );

  vtkSmartPointer<vtkPolyDataMapper> mapper2 =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper2->SetInputConnection( distanceFilter->GetOutputPort(1) );
  mapper2->SetScalarRange(
    distanceFilter->GetSecondDistanceOutput()->GetPointData()->GetScalars()->GetRange()[0],
    distanceFilter->GetSecondDistanceOutput()->GetPointData()->GetScalars()->GetRange()[1]);

  vtkSmartPointer<vtkActor> actor2 =
    vtkSmartPointer<vtkActor>::New();
  actor2->SetMapper( mapper2 );

  vtkSmartPointer<vtkScalarBarActor> scalarBar =
    vtkSmartPointer<vtkScalarBarActor>::New();
  scalarBar->SetLookupTable(mapper->GetLookupTable());
  scalarBar->SetTitle("Distance");
  scalarBar->SetNumberOfLabels(5);
  scalarBar->SetTextPad(4);
  vtkSmartPointer<vtkRenderer> renderer =
    vtkSmartPointer<vtkRenderer>::New();

  vtkSmartPointer<vtkRenderWindow> renWin =
    vtkSmartPointer<vtkRenderWindow>::New();
  renWin->AddRenderer( renderer );

  vtkSmartPointer<vtkRenderWindowInteractor> renWinInteractor =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  renWinInteractor->SetRenderWindow( renWin );

  renderer->AddActor( actor );
  renderer->AddActor( actor2 );
  renderer->AddActor2D(scalarBar);

  renWin->Render();
  distanceFilter->Print(std::cout);

  renWinInteractor->Start();

  return EXIT_SUCCESS;
}
