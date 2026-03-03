// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Description
// This test creates multiple random spheres with different region ids, and
// then produces sampled points from the volume. It compares the classic,
// volumetric surface nets to the generalized surface net.

#include "vtkActor.h"
#include "vtkCellData.h"
#include "vtkGeneralizedSurfaceNets3D.h"
#include "vtkIdTypeArray.h"
#include "vtkImageData.h"
#include "vtkIntArray.h"
#include "vtkLabeledImagePointSampler.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkOutlineFilter.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyDataWriter.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSMPTools.h"
#include "vtkSurfaceNets3D.h"
#include "vtkTesting.h"
#include "vtkTimerLog.h"

#include <iostream>

namespace
{

struct Sphere
{
  int RegionId;
  double Center[3];
  double Radius;
  Sphere(int rid, double x, double y, double z, double r)
    : RegionId(rid)
    , Center{ x, y, z }
    , Radius(r)
  {
  }

  // Label voxels within sphere
  void LabelVoxels(vtkImageData* volume)
  {
    int dims[3];
    volume->GetDimensions(dims);
    vtkIdType numPts = volume->GetNumberOfPoints();
    assert(numPts == (dims[0] * dims[1] * dims[2]));

    vtkDataArray* scalars = volume->GetPointData()->GetScalars();
    int* sPtr = vtkIntArray::FastDownCast(scalars)->GetPointer(0);

    double R2 = (this->Radius * this->Radius);
    // Make the sphere sampling a little faster
    vtkSMPTools::For(0, numPts,
      [this, &volume, &R2, &sPtr](vtkIdType ptId, vtkIdType endPtId)
      {
        double d2, x[3];
        int* s = sPtr + ptId;
        for (; ptId < endPtId; ++ptId, ++s)
        {
          volume->GetPoint(ptId, x);
          d2 = vtkMath::Distance2BetweenPoints(x, this->Center);
          if (d2 <= R2)
          {
            *s = this->RegionId;
          }
        }
      }); // end lambda
  }       // Label voxels
};        // Sphere

} // anonymous

int TestLabeledImagePointSampler2(int argc, char* argv[])
{
  // Create a volume
  int numSpheres = 20;
  const int DIM = 51;
  int dims[3] = { DIM, 2 * DIM, 3 * DIM };

  double xRange[2] = { -2, 2 };
  double yRange[2] = { -4, 4 };
  double zRange[2] = { -6, 6 };
  double rRange[2] = { 0.5, 2.0 };

  double spacing[3];
  spacing[0] = (xRange[1] - xRange[0]) / static_cast<double>(dims[0] - 1);
  spacing[1] = (yRange[1] - yRange[0]) / static_cast<double>(dims[1] - 1);
  spacing[2] = (zRange[1] - zRange[0]) / static_cast<double>(dims[2] - 1);

  vtkNew<vtkImageData> volume;
  volume->SetDimensions(dims);
  volume->SetOrigin(xRange[0], yRange[0], zRange[0]);
  volume->SetSpacing(spacing);
  volume->AllocateScalars(VTK_INT, 1);
  vtkIntArray* scalars = vtkIntArray::FastDownCast(volume->GetPointData()->GetScalars());
  int* sPtr = scalars->GetPointer(0);
  vtkSMPTools::Fill(sPtr, sPtr + volume->GetNumberOfPoints(), VTK_INT_MAX);

  // Generate the spheres
  std::vector<Sphere> spheres;
  for (int rid = 0; rid < numSpheres; ++rid)
  {
    double x = vtkMath::Random(xRange[0], xRange[1]);
    double y = vtkMath::Random(yRange[0], yRange[1]);
    double z = vtkMath::Random(zRange[0], zRange[1]);
    double r = vtkMath::Random(rRange[0], rRange[1]);
    spheres.emplace_back(rid + 1, x, y, z, r);
  }

  // Populate the volume. This sets region ids within the spheres.
  for (auto& sItr : spheres)
  {
    sItr.LabelVoxels(volume);
  }

  // Time processing
  vtkNew<vtkTimerLog> timer;

  // Compare to volumetric surface net for context
  vtkNew<vtkSurfaceNets3D> snet;
  snet->SetInputData(volume);
  snet->GenerateLabels(numSpheres, 1, numSpheres);

  timer->StartTimer();
  //  snet->Update();
  timer->StopTimer();
  auto time = timer->GetElapsedTime();
  std::cout << "Time to classic surface nets data: " << time << "\n";
  std::cout << "\tNumber of primitives produced: " << snet->GetOutput()->GetNumberOfCells() << "\n";

  vtkNew<vtkPolyDataMapper> snetMapper;
  snetMapper->SetInputConnection(snet->GetOutputPort());
  snetMapper->ScalarVisibilityOn();
  snetMapper->SetScalarRange(1, numSpheres + 1);
  snetMapper->SetScalarModeToUseCellData();

  vtkNew<vtkActor> snetActor;
  snetActor->SetMapper(snetMapper);
  snetActor->GetProperty()->SetColor(0.5, 0.5, 0.5);
  //  snetActor->GetProperty()->SetOpacity(0.4);

  // Sampled points
  vtkNew<vtkLabeledImagePointSampler> sampler;
  sampler->SetInputData(volume);
  sampler->GenerateLabels(numSpheres, 1, numSpheres);
  sampler->SetDensityDistributionToExponential();
  //  sampler->SetDensityDistributionToLinear();
  sampler->SetN(2);
  sampler->SetOutputTypeToLabeledPoints();
  sampler->SetOutputTypeToBackgroundPoints();
  sampler->SetOutputTypeToAllPoints();
  sampler->GenerateVertsOn();
  sampler->RandomizeOn();
  sampler->JoggleOn(); // turn off to test degeneracies

  timer->StartTimer();
  sampler->Update();
  timer->StopTimer();
  time = timer->GetElapsedTime();
  std::cout << "Time to sample data: " << time << "\n";

  // Voronoi-based surface net
  vtkNew<vtkGeneralizedSurfaceNets3D> vsn;
  vsn->SetInputConnection(sampler->GetOutputPort());
  vsn->GenerateLabels(numSpheres, 1, numSpheres);
  vsn->SetOutputMeshTypeToTriangles();
  vsn->BoundaryCappingOn();
  vsn->SmoothingOn();
  vsn->ValidateOn();
  vsn->SetNumberOfIterations(50);
  vsn->SetConstraintDistance(1);
  vsn->GenerateSmoothingStencilsOn();

  timer->StartTimer();
  vsn->Update();
  timer->StopTimer();
  time = timer->GetElapsedTime();
  std::cout << "Time to generalized surface data: " << time << "\n";
  std::cout << "\tNumber of threads used: " << vsn->GetNumberOfThreads() << "\n";

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(vsn->GetOutputPort());
  mapper->SetScalarRange(1, numSpheres + 1);
  mapper->SetScalarModeToUseCellData();
  mapper->ScalarVisibilityOn();

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->SetColor(1, 1, 1);
  actor->GetProperty()->SetPointSize(2);

  // Report some stats
  sampler->Update();
  std::cout << "\tNumber of input Voroni points: " << sampler->GetOutput()->GetNumberOfPoints()
            << "\n";
  std::cout << "\tNumber of output Voronoi points: " << vsn->GetOutput()->GetNumberOfPoints()
            << "\n";
  std::cout << "\tNumber of output Voronoi cells: " << vsn->GetOutput()->GetNumberOfCells() << "\n";

  // Bounding box
  vtkNew<vtkOutlineFilter> outline;
  outline->SetInputData(volume);

  vtkNew<vtkPolyDataMapper> outlineMapper;
  outlineMapper->SetInputConnection(outline->GetOutputPort());

  vtkNew<vtkActor> outlineActor;
  outlineActor->SetMapper(outlineMapper);

  vtkNew<vtkRenderer> ren;
  // ren->AddActor(snetActor);
  ren->AddActor(actor);
  ren->AddActor(outlineActor);

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
