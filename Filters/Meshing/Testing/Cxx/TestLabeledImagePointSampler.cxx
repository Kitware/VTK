// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Description
// This test creates a single sphere inside a volume and fills the sphere
// with a segmented regions id = 1. It then produces sampled points from the
// volume.

#include "vtkActor.h"
#include "vtkImageData.h"
#include "vtkIntArray.h"
#include "vtkLabeledImagePointSampler.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTesting.h"
#include "vtkTimerLog.h"

#include <iostream>

namespace
{

// Sphere of radius R centered at (0,0,0)
void LabelVoxels(vtkImageData* volume, double R)
{
  int dims[3];
  volume->GetDimensions(dims);
  vtkIdType numPts = volume->GetNumberOfPoints();
  assert(numPts == (dims[0] * dims[1] * dims[2]));

  vtkDataArray* scalars = volume->GetPointData()->GetScalars();
  int* s = vtkIntArray::FastDownCast(scalars)->GetPointer(0);

  double d2, x[3], R2 = R * R;
  double origin[3] = { 0, 0, 0 };
  for (vtkIdType ptId = 0; ptId < numPts; ++ptId)
  {
    volume->GetPoint(ptId, x);
    d2 = vtkMath::Distance2BetweenPoints(x, origin);
    *s++ = (d2 <= R2 ? 1 : 0);
  }
}

} // anonymous

int TestLabeledImagePointSampler(int argc, char* argv[])
{
  // Create a volume
  int DIM = 51;
  double RADIUS = DIM / 2.5;
  vtkNew<vtkImageData> volume;
  volume->SetDimensions(DIM, DIM, DIM);
  volume->SetOrigin(-DIM / 2, -DIM / 2, -DIM / 2);
  volume->SetSpacing(1, 1, 1);
  volume->AllocateScalars(VTK_INT, 1);

  // Label the sphere region
  LabelVoxels(volume, RADIUS);

  // Time processing
  vtkNew<vtkTimerLog> timer;

  // Sampled points
  vtkNew<vtkLabeledImagePointSampler> sampler;
  sampler->SetInputData(volume);
  sampler->SetLabel(0, 1);
  sampler->SetDensityDistributionToExponential();
  sampler->SetN(2);
  sampler->SetOutputTypeToAllPoints();
  sampler->GenerateVertsOn();
  sampler->RandomizeOff();
  sampler->JoggleOff();

  timer->StartTimer();
  sampler->Update();
  timer->StopTimer();
  auto time = timer->GetElapsedTime();
  std::cout << "Time to sample data: " << time << "\n";

  // Report some stats
  sampler->Update();
  std::cout << "Number of sampled points: " << sampler->GetOutput()->GetNumberOfPoints() << "\n";

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(sampler->GetOutputPort());
  mapper->ScalarVisibilityOff();

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->SetColor(1, 1, 1);
  actor->GetProperty()->SetPointSize(2);

  vtkNew<vtkRenderer> ren;
  ren->AddActor(actor);

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(400, 400);
  renWin->AddRenderer(ren);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  ren->ResetCamera();
  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
