/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"

#include "vtkActor.h"
#include "vtkMatrix4x4.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSphereSource.h"

int TestHomogeneousTransformOfActor(int argc, char *argv[])
{
  vtkSphereSource *sphere = vtkSphereSource::New();
  sphere->SetThetaResolution(10);
  sphere->SetPhiResolution(10);
  vtkPolyDataMapper *sphereMapper = vtkPolyDataMapper::New();
  sphereMapper->SetInputConnection( sphere->GetOutputPort() );
  sphere->FastDelete();

  vtkActor *sphereActor = vtkActor::New();
  sphereActor->SetMapper(sphereMapper);
  sphereMapper->FastDelete();

  vtkActor *referenceSphereActor = vtkActor::New();
  referenceSphereActor->SetMapper(sphereMapper);
  referenceSphereActor->SetPosition(6, 0, 0);

  // the crux of the test, set w to be not equal to 1
  vtkMatrix4x4 *matrix = vtkMatrix4x4::New();
  matrix->SetElement(3, 3, 0.25);
  sphereActor->SetUserMatrix( matrix );
  matrix->FastDelete();

  //Create the rendering stuff
  vtkRenderer *renderer = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->AddRenderer(renderer);
  renderer->FastDelete();
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);
  renWin->FastDelete();

  renderer->AddActor(referenceSphereActor);
  referenceSphereActor->Delete();
  renderer->AddActor(sphereActor);
  sphereActor->Delete();
  renderer->SetBackground(0.5,0.5,0.5);
  renWin->SetSize(450,450);
  renWin->Render();

  renderer->ResetCamera();

  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if ( retVal == vtkRegressionTester::DO_INTERACTOR )
  {
    iren->Start();
  }
  iren->Delete();

  return !retVal;
}
