/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// This test ensure that when all translucent fragments are in front of opaque fragments, the
// occlusion query check does not exit too early
#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCubeSource.h"
#include "vtkNew.h"
#include "vtkOpenGLRenderer.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSphereSource.h"
#include "vtkTestUtilities.h"

int TestDepthPeelingOcclusionQuery(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  vtkNew<vtkRenderWindowInteractor> iren;
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  renWin->SetAlphaBitPlanes(1);
  iren->SetRenderWindow(renWin);
  vtkNew<vtkRenderer> renderer;
  renWin->AddRenderer(renderer);

  vtkNew<vtkPolyDataMapper> mapperBox;
  vtkNew<vtkCubeSource> box;
  box->SetXLength(3.0);
  box->SetYLength(3.0);
  mapperBox->SetInputConnection(box->GetOutputPort());

  vtkNew<vtkPolyDataMapper> mapperSphere;
  vtkNew<vtkSphereSource> sphere;
  mapperSphere->SetInputConnection(sphere->GetOutputPort());

  vtkNew<vtkActor> actorBox;
  actorBox->GetProperty()->SetColor(0.1, 0.1, 0.1);
  actorBox->SetMapper(mapperBox);
  renderer->AddActor(actorBox);

  vtkNew<vtkActor> actorSphere1;
  actorSphere1->GetProperty()->SetColor(1.0, 0.0, 0.0);
  actorSphere1->GetProperty()->SetOpacity(0.2);
  actorSphere1->SetPosition(0.0, 0.0, 1.0);
  actorSphere1->SetMapper(mapperSphere);
  renderer->AddActor(actorSphere1);

  vtkNew<vtkActor> actorSphere2;
  actorSphere2->GetProperty()->SetColor(0.0, 1.0, 0.0);
  actorSphere2->GetProperty()->SetOpacity(0.2);
  actorSphere2->SetPosition(0.0, 0.0, 2.0);
  actorSphere2->SetMapper(mapperSphere);
  renderer->AddActor(actorSphere2);

  renderer->SetUseDepthPeeling(1);
  renderer->SetOcclusionRatio(0.0);
  renderer->SetMaximumNumberOfPeels(20);

  renWin->SetSize(500, 500);
  renderer->ResetCamera();

  renWin->Render();
  iren->Start();

  return EXIT_SUCCESS;
}
