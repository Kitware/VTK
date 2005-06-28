/*=========================================================================

  Program:   Visualization Toolkit
  Module:    CellLocator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkActor.h"
#include "vtkCellLocator.h"
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

int CellLocator( int argc, char *argv[] )
{
#if !defined(VTK_LEGACY_REMOVE) && defined(VTK_LEGACY_SILENT)
  vtkDebugLeaks::PromptUserOff();
#endif

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
    sphereMapper->SetInput(sphere->GetOutput());
  vtkActor *sphereActor = vtkActor::New();
    sphereActor->SetMapper(sphereMapper);
    
  vtkSphereSource *spot = vtkSphereSource::New();
    spot->SetPhiResolution(6);
    spot->SetThetaResolution(6);
    spot->SetRadius(0.1);

  vtkPolyDataMapper *spotMapper = vtkPolyDataMapper::New();
    spotMapper->SetInput(spot->GetOutput());

  // Build a locator 
  vtkCellLocator *cellLocator = vtkCellLocator::New();
  cellLocator->SetDataSet(sphere->GetOutput());
  cellLocator->BuildLocator();

  // Intersect with line
  double p1[] = {2.0, 1.0, 3.0};
  double p2[] = {0.0, 0.0, 0.0};
  double t, ptline[3], pcoords[3];
  int subId;
  cellLocator->IntersectWithLine(p1, p2, 0.001, t, ptline, pcoords, subId);

  vtkActor *intersectLineActor = vtkActor::New();
    intersectLineActor->SetMapper(spotMapper);
    intersectLineActor->SetPosition(ptline[0],ptline[1],ptline[2]);
    intersectLineActor->GetProperty()->SetColor(1.0, 0.0, 0.0);

  // Find closest point
  vtkIdType cellId;
  double dist;
  p1[0] = -2.4; p1[1] = -0.9;
  cellLocator->FindClosestPoint(p1, ptline, cellId, subId, dist);
  vtkActor *closestPointActor = vtkActor::New();
    closestPointActor->SetMapper(spotMapper);
    closestPointActor->SetPosition(ptline[0],ptline[1],ptline[2]);
    closestPointActor->GetProperty()->SetColor(0.0, 1.0, 0.0);

  // Find closest point within radius
  float radius = 5.0;
  p1[0] = .2; p1[1] = 1.0; p1[2] = 1.0;
  cellLocator->FindClosestPointWithinRadius(p1, radius, ptline, cellId, subId, dist);
  vtkActor *closestPointActor2 = vtkActor::New();
    closestPointActor2->SetMapper(spotMapper);
    closestPointActor2->SetPosition(ptline[0],ptline[1],ptline[2]);
    closestPointActor2->GetProperty()->SetColor(0.0, 1.0, 0.0);
  
  renderer->AddActor(sphereActor);
  renderer->AddActor(intersectLineActor);
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
  intersectLineActor->Delete();
  closestPointActor->Delete();
  closestPointActor2->Delete();
  cellLocator->FreeSearchStructure();
  cellLocator->Delete();

  return !retVal;
}
