// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include <vtkDataSet.h>
#include <vtkImageData.h>
#include <vtkImplicitArray.h>
#include <vtkPointData.h>
#include <vtkSphereSource.h>
#include <vtkStreamTracer.h>

#include <cstdlib>

namespace
{

struct VortexBackend
{
  VortexBackend(vtkDataSet* geometry)
    : Geometry(geometry)
  {
  }
  double operator()(int idx) const
  {
    int iComp = idx % 3;
    int iTuple = idx / 3;
    double pt[3];
    this->Geometry->GetPoint(iTuple, pt);
    switch (iComp)
    {
      case (0):
        return -0.2 * pt[1];
      case (1):
        return 0.08 * pt[0];
      case (2):
        return 0.02 * pt[2];
      default:
        return 0.0;
    }
  }
  vtkDataSet* Geometry;
};

}

int TestStreamTracerImplicitArray(int, char*[])
{
  vtkNew<vtkImageData> baseGrid;
  int nPix = 100;
  int halfCells = nPix / 2 - 1;
  baseGrid->SetExtent(-halfCells, halfCells, -halfCells, halfCells, -halfCells, halfCells);
  baseGrid->SetSpacing(1.0 / nPix, 1.0 / nPix, 1.0 / nPix);
  vtkNew<vtkImplicitArray<VortexBackend>> vortex;
  vortex->SetName("Vortex");
  vortex->SetBackend(std::make_shared<VortexBackend>(baseGrid));
  vortex->SetNumberOfComponents(3);
  vortex->SetNumberOfTuples(std::pow(nPix, 3));
  baseGrid->GetPointData()->AddArray(vortex);
  baseGrid->GetPointData()->SetActiveVectors("Vortex");

  vtkNew<vtkSphereSource> sphere;
  sphere->SetRadius(0.1);
  sphere->SetThetaResolution(10);
  sphere->SetPhiResolution(10);

  vtkNew<vtkStreamTracer> streams;
  streams->SetInputData(baseGrid);
  streams->SetSourceConnection(sphere->GetOutputPort());
  streams->SetIntegratorTypeToRungeKutta45();
  streams->SetMaximumPropagation(20);
  streams->SetIntegrationDirectionToBoth();
  streams->SetComputeVorticity(true);

  streams->Update();

  return (streams->GetOutput(0) && streams->GetOutput(0)->GetNumberOfPoints() != 0 &&
           streams->GetOutput(0)->GetPointData()->HasArray("Vorticity"))
    ? EXIT_SUCCESS
    : EXIT_FAILURE;
}
