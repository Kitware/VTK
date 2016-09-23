/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PointLocator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkActor.h"
#include "vtkPointLocator.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"

#include "vtkRegressionTestImage.h"
#include "vtkDebugLeaks.h"

int PointLocator( int argc, char *argv[] )
{
  vtkRenderer *renderer = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
    renWin->AddRenderer(renderer);
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
    iren->SetRenderWindow(renWin);

  vtkSphereSource *sphere = vtkSphereSource::New();
    sphere->SetThetaResolution(8); sphere->SetPhiResolution(8);
    sphere->SetRadius(1.0);
    sphere->Update();
  vtkPolyDataMapper *sphereMapper = vtkPolyDataMapper::New();
    sphereMapper->SetInputConnection(sphere->GetOutputPort());
  vtkActor *sphereActor = vtkActor::New();
    sphereActor->SetMapper(sphereMapper);

  vtkSphereSource *spot = vtkSphereSource::New();
    spot->SetPhiResolution(6);
    spot->SetThetaResolution(6);
    spot->SetRadius(0.1);

  vtkPolyDataMapper *spotMapper = vtkPolyDataMapper::New();
    spotMapper->SetInputConnection(spot->GetOutputPort());

  // Build a locator
  vtkPointLocator *pointLocator = vtkPointLocator::New();
  pointLocator->SetDataSet(sphere->GetOutput());
  pointLocator->BuildLocator();

  //
  double p1[] = {2.0, 1.0, 3.0};

  // Find closest point
  vtkIdType ptId;
  double dist;
  p1[0] = 0.1; p1[1] = -0.2; p1[2] = 0.2;
  ptId = pointLocator->FindClosestPoint(p1);
  vtkActor *closestPointActor = vtkActor::New();
    closestPointActor->SetMapper(spotMapper);
    // TODO cleanupo
    closestPointActor->SetPosition(
      sphere->GetOutput()->GetPoints()->GetPoint(ptId)[0],
      sphere->GetOutput()->GetPoints()->GetPoint(ptId)[1],
      sphere->GetOutput()->GetPoints()->GetPoint(ptId)[2]);
    closestPointActor->GetProperty()->SetColor(0.0, 1.0, 0.0);

  // Find closest point within radius
  float radius = 5.0;
  p1[0] = .2; p1[1] = 1.0; p1[2] = 1.0;
  ptId = pointLocator->FindClosestPointWithinRadius(radius, p1, dist);
  vtkActor *closestPointActor2 = vtkActor::New();
    closestPointActor2->SetMapper(spotMapper);
    // TODO cleanup
    closestPointActor2->SetPosition(
      sphere->GetOutput()->GetPoints()->GetPoint(ptId)[0],
      sphere->GetOutput()->GetPoints()->GetPoint(ptId)[1],
      sphere->GetOutput()->GetPoints()->GetPoint(ptId)[2]);
    closestPointActor2->GetProperty()->SetColor(0.0, 1.0, 0.0);

  renderer->AddActor(sphereActor);
  renderer->AddActor(closestPointActor);
  renderer->AddActor(closestPointActor2);
  renderer->SetBackground(1,1,1);
  renWin->SetSize(300,300);

  // interact with data
  renWin->Render();

  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  // Clean up
  renderer->Delete();
  renWin->Delete();
  iren->Delete();
  sphere->Delete();
  sphereMapper->Delete();
  sphereActor->Delete();
  spot->Delete();
  spotMapper->Delete();
  closestPointActor->Delete();
  closestPointActor2->Delete();
  pointLocator->FreeSearchStructure();
  pointLocator->Delete();

  return !retVal;
}
