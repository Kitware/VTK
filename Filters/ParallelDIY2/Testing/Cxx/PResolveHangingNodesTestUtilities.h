// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Helpers of the tests of vtkPResolveHangingNodes: the check of the output gathered from all the
// processes.
#ifndef PResolveHangingNodesTestUtilities_h
#define PResolveHangingNodesTestUtilities_h

#include "vtkAppendFilter.h"
#include "vtkCellData.h"
#include "vtkCellValidator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataSet.h"
#include "vtkFeatureEdges.h"
#include "vtkGeometryFilter.h"
#include "vtkIdList.h"
#include "vtkMath.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolygon.h"
#include "vtkSmartPointer.h"
#include "vtkUnstructuredGrid.h"

#include <array>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

// The functions are inline, so that a test that does not use one gets no warning.
namespace
{
constexpr double EPSILON = 1e-6;

inline vtkIdType FindPoint(vtkDataSet* ds, const std::array<double, 3>& p)
{
  for (vtkIdType i = 0; i < ds->GetNumberOfPoints(); ++i)
  {
    if (vtkMath::Distance2BetweenPoints(p.data(), ds->GetPoint(i)) < EPSILON)
    {
      return i;
    }
  }
  return -1;
}

// Total length (2D) or area (3D) of the edges or faces used by a single cell. It is the boundary
// measure of the domain if and only if the mesh is conforming: a remaining hanging node leaves both
// sides of an interface unmatched.
template <int Dim>
inline double BoundaryMeasure(vtkDataSet* ds)
{
  vtkNew<vtkGeometryFilter> geometry;
  geometry->SetInputData(ds);
  geometry->Update();
  vtkSmartPointer<vtkPolyData> boundary = geometry->GetOutput();
  if constexpr (Dim == 2)
  {
    vtkNew<vtkFeatureEdges> edges;
    edges->SetInputData(boundary);
    edges->BoundaryEdgesOn();
    edges->FeatureEdgesOff();
    edges->NonManifoldEdgesOff();
    edges->ManifoldEdgesOff();
    edges->Update();
    boundary = edges->GetOutput();
  }
  double measure = 0.0;
  vtkNew<vtkIdList> ids;
  for (vtkIdType c = 0; c < boundary->GetNumberOfCells(); ++c)
  {
    boundary->GetCellPoints(c, ids);
    if constexpr (Dim == 2)
    {
      double p0[3], p1[3];
      boundary->GetPoint(ids->GetId(0), p0);
      boundary->GetPoint(ids->GetId(1), p1);
      measure += std::sqrt(vtkMath::Distance2BetweenPoints(p0, p1));
    }
    else
    {
      double normal[3];
      measure += vtkPolygon::ComputeArea(
        boundary->GetPoints(), ids->GetNumberOfIds(), ids->GetPointer(0), normal);
    }
  }
  return measure;
}

// Gather the partitions of `output` on process 0, merge the points they share, and check that the
// whole mesh is conforming, valid in 3D, and covers the domain, of area (2D) or volume (3D)
// `expectedMeasure` and of boundary length (2D) or area (3D) `expectedBoundaryMeasure`.
template <int Dim>
inline bool CheckGlobalOutput(const std::string& name, vtkMultiProcessController* controller,
  vtkDataObject* output, double expectedMeasure, double expectedBoundaryMeasure)
{
  // The partitions of this process, in one dataset to gather, empty without partitions (since
  // vtkAppendFilter needs an input)
  auto local = vtkSmartPointer<vtkUnstructuredGrid>::New();
  const auto partitions = vtkCompositeDataSet::GetDataSets(output);
  if (!partitions.empty())
  {
    vtkNew<vtkAppendFilter> localAppend;
    for (vtkDataSet* partition : partitions)
    {
      localAppend->AddInputData(partition);
    }
    localAppend->Update();
    local = localAppend->GetOutput();
  }
  std::vector<vtkSmartPointer<vtkDataObject>> outputs;
  controller->Gather(local, outputs, 0);
  if (controller->GetLocalProcessId() != 0)
  {
    return true;
  }
  vtkNew<vtkAppendFilter> append;
  append->MergePointsOn();
  for (const auto& partition : outputs)
  {
    append->AddInputData(partition);
  }
  append->Update();
  vtkUnstructuredGrid* mesh = append->GetOutput();

  const double boundaryMeasure = BoundaryMeasure<Dim>(mesh);
  if (std::abs(boundaryMeasure - expectedBoundaryMeasure) > EPSILON)
  {
    std::cerr << name << ": the boundary " << (Dim == 2 ? "length" : "area") << " is "
              << boundaryMeasure << " instead of " << expectedBoundaryMeasure
              << ", the output is not conforming\n";
    return false;
  }

  if (Dim == 3)
  {
    vtkNew<vtkCellValidator> validator;
    validator->SetInputData(mesh);
    validator->Update();
    vtkDataArray* states =
      vtkDataSet::SafeDownCast(validator->GetOutput())->GetCellData()->GetArray("ValidityState");
    for (vtkIdType c = 0; c < mesh->GetNumberOfCells(); ++c)
    {
      if (states->GetTuple1(c) != static_cast<short>(vtkCellValidator::State::Valid))
      {
        std::cerr << name << ": cell " << c << " of type " << mesh->GetCellType(c)
                  << " is not valid\n";
        return false;
      }
    }
  }

  vtkDataArray* inverseMeasure =
    mesh->GetCellData()->GetArray(Dim == 2 ? "InverseArea" : "InverseVolume");
  double measure = 0.0;
  for (vtkIdType c = 0; inverseMeasure && c < mesh->GetNumberOfCells(); ++c)
  {
    measure += 1.0 / inverseMeasure->GetTuple1(c);
  }
  if (std::abs(measure - expectedMeasure) > EPSILON)
  {
    std::cerr << name << ": the cells cover " << (Dim == 2 ? "an area" : "a volume") << " of "
              << measure << " instead of " << expectedMeasure << "\n";
    return false;
  }
  return true;
}

} // namespace

#endif
