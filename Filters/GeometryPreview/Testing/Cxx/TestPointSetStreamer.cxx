// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkPointSetStreamer.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"

int TestPointSetStreamer(int argc, char* argv[])
{
  // Create a sphere
  vtkNew<vtkSphereSource> sphere;
  sphere->SetCenter(0, 0, 0);
  sphere->SetRadius(0.5);
  sphere->SetPhiResolution(2000);
  sphere->SetThetaResolution(2000);

  // generate a streamer
  vtkNew<vtkPointSetStreamer> pointsStreamer;
  pointsStreamer->SetInputConnection(sphere->GetOutputPort());
  pointsStreamer->SetNumberOfPointsPerBucket(75000);
  pointsStreamer->Update();

  // get the first non empty bucket
  for (vtkIdType i = 0; i < pointsStreamer->GetNumberOfBuckets(); ++i)
  {
    pointsStreamer->SetBucketId(i);
    pointsStreamer->Update();
    if (pointsStreamer->GetOutput()->GetNumberOfPoints() > 0)
    {
      break;
    }
  }

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(pointsStreamer->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  vtkNew<vtkRenderer> ren;
  ren->SetBackground(0.2, 0.2, 0.5);
  ren->AddActor(actor);

  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(ren);
  renWin->SetSize(400, 400);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  ren->ResetCamera();
  renWin->Render();
  iren->Initialize();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return EXIT_SUCCESS;
}
