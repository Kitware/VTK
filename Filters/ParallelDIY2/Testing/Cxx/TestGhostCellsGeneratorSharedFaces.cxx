// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// vtkGhostCellsGenerator on two partitions of polyhedra whose shared faces are stored once, the
// second cell of a face listing it as -(n+1) to traverse it in reverse. The ghost cells and their
// faces are those generated from the same partitions with every face stored per cell.

#include "vtkCellArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkGenericCell.h"
#include "vtkGhostCellsGenerator.h"
#include "vtkIdList.h"
#include "vtkMathUtilities.h"
#include "vtkNew.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPoints.h"
#include "vtkPolyhedron.h"
#include "vtkPolyhedronCellArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>

namespace
{
// nx * n * n hexahedra as polyhedra, from x = x0, each with its own copy of its faces.
vtkSmartPointer<vtkUnstructuredGrid> MakeDuplicated(int nx, int n, double x0)
{
  auto grid = vtkSmartPointer<vtkUnstructuredGrid>::New();
  vtkNew<vtkPoints> points;
  auto index = [nx, n](int i, int j, int k) { return i + (nx + 1) * (j + (n + 1) * k); };
  for (int k = 0; k <= n; ++k)
  {
    for (int j = 0; j <= n; ++j)
    {
      for (int i = 0; i <= nx; ++i)
      {
        points->InsertNextPoint(x0 + i, j, k);
      }
    }
  }
  grid->SetPoints(points);
  const int faces[6][4] = { { 0, 4, 7, 3 }, { 1, 2, 6, 5 }, { 0, 1, 5, 4 }, { 3, 7, 6, 2 },
    { 0, 3, 2, 1 }, { 4, 5, 6, 7 } };
  for (int k = 0; k < n; ++k)
  {
    for (int j = 0; j < n; ++j)
    {
      for (int i = 0; i < nx; ++i)
      {
        const vtkIdType c[8] = { index(i, j, k), index(i + 1, j, k), index(i + 1, j + 1, k),
          index(i, j + 1, k), index(i, j, k + 1), index(i + 1, j, k + 1),
          index(i + 1, j + 1, k + 1), index(i, j + 1, k + 1) };
        vtkNew<vtkIdList> stream;
        stream->InsertNextId(6);
        for (const auto& face : faces)
        {
          stream->InsertNextId(4);
          for (int v : face)
          {
            stream->InsertNextId(c[v]);
          }
        }
        grid->InsertNextCell(VTK_POLYHEDRON, stream);
      }
    }
  }
  return grid;
}

// The grid with every shared face stored once, as the first cell to list it winds it. A later
// cell lists it as -(n+1) when signed, as n otherwise. Faces match by their points.
vtkSmartPointer<vtkUnstructuredGrid> MakeShared(vtkUnstructuredGrid* duplicated, bool signedIds)
{
  vtkPolyhedronCellArray* polyhedra = duplicated->GetPolyhedronCellArray();
  std::map<std::vector<vtkIdType>, vtkIdType> storedFaces;
  vtkNew<vtkCellArray> faces;
  vtkNew<vtkCellArray> locations;
  vtkNew<vtkIdList> pts;
  for (vtkIdType cellId = 0; cellId < duplicated->GetNumberOfCells(); ++cellId)
  {
    const int numFaces = polyhedra->GetCellNumberOfFaces(cellId);
    locations->InsertNextCell(numFaces);
    for (int face = 0; face < numFaces; ++face)
    {
      polyhedra->GetFacePoints(cellId, face, pts);
      std::vector<vtkIdType> key(pts->begin(), pts->end());
      std::sort(key.begin(), key.end());
      auto found = storedFaces.find(key);
      if (found == storedFaces.end())
      {
        const vtkIdType faceId = faces->InsertNextCell(pts);
        storedFaces.emplace(key, faceId);
        locations->InsertCellPoint(faceId);
      }
      else
      {
        locations->InsertCellPoint(vtkPolyhedronCellArray::EncodeFaceId(found->second, signedIds));
      }
    }
  }
  auto shared = vtkSmartPointer<vtkUnstructuredGrid>::New();
  shared->SetPoints(duplicated->GetPoints());
  vtkNew<vtkUnsignedCharArray> types;
  types->DeepCopy(duplicated->GetCellTypes());
  shared->SetPolyhedralCells(types, duplicated->GetCells(), locations, faces);
  return shared;
}

vtkSmartPointer<vtkPartitionedDataSet> GenerateGhosts(
  vtkUnstructuredGrid* left, vtkUnstructuredGrid* right)
{
  vtkNew<vtkPartitionedDataSet> partitions;
  partitions->SetNumberOfPartitions(2);
  partitions->SetPartition(0, left);
  partitions->SetPartition(1, right);
  vtkNew<vtkGhostCellsGenerator> generator;
  generator->SetInputData(partitions);
  generator->SetNumberOfGhostLayers(1);
  generator->BuildIfRequiredOff();
  generator->Update();
  return vtkPartitionedDataSet::SafeDownCast(generator->GetOutputDataObject(0));
}

// The faces of every cell, each as a sorted list of its points, the faces sorted.
std::vector<std::vector<std::vector<vtkIdType>>> CellFaces(vtkUnstructuredGrid* grid)
{
  std::vector<std::vector<std::vector<vtkIdType>>> cells(grid->GetNumberOfCells());
  vtkPolyhedronCellArray* polyhedra = grid->GetPolyhedronCellArray();
  vtkNew<vtkIdList> pts;
  for (vtkIdType cellId = 0; cellId < grid->GetNumberOfCells(); ++cellId)
  {
    for (int face = 0; face < polyhedra->GetCellNumberOfFaces(cellId); ++face)
    {
      polyhedra->GetFacePoints(cellId, face, pts);
      std::vector<vtkIdType> ids(pts->begin(), pts->end());
      std::sort(ids.begin(), ids.end());
      cells[cellId].push_back(ids);
    }
    std::sort(cells[cellId].begin(), cells[cellId].end());
  }
  return cells;
}

double Volume(vtkUnstructuredGrid* grid)
{
  vtkNew<vtkGenericCell> cell;
  double volume = 0.0;
  for (vtkIdType cellId = 0; cellId < grid->GetNumberOfCells(); ++cellId)
  {
    grid->GetCell(cellId, cell);
    volume += vtkPolyhedron::SafeDownCast(cell->GetRepresentativeCell())->ComputeVolume();
  }
  return volume;
}
} // anonymous namespace

int TestGhostCellsGeneratorSharedFaces(int, char*[])
{
  auto leftDuplicated = MakeDuplicated(2, 2, 0.0);
  auto rightDuplicated = MakeDuplicated(2, 2, 2.0);
  auto expected = GenerateGhosts(leftDuplicated, rightDuplicated);
  auto actual = GenerateGhosts(MakeShared(leftDuplicated, true), MakeShared(rightDuplicated, true));

  int status = EXIT_SUCCESS;
  for (unsigned int p = 0; p < 2; ++p)
  {
    auto* want = vtkUnstructuredGrid::SafeDownCast(expected->GetPartition(p));
    auto* got = vtkUnstructuredGrid::SafeDownCast(actual->GetPartition(p));
    if (!want || !got || want->GetNumberOfCells() != got->GetNumberOfCells() ||
      want->GetNumberOfCells() != 12)
    {
      std::cerr << "partition " << p << ": wrong number of cells with ghosts" << std::endl;
      status = EXIT_FAILURE;
      continue;
    }
    if (CellFaces(want) != CellFaces(got))
    {
      std::cerr << "partition " << p << ": the faces of the cells differ" << std::endl;
      status = EXIT_FAILURE;
    }
    if (!vtkMathUtilities::FuzzyCompare(Volume(got), Volume(want), 1e-12) ||
      !vtkMathUtilities::FuzzyCompare(Volume(got), 12.0, 1e-12))
    {
      std::cerr << "partition " << p << ": volume " << Volume(got) << " instead of " << Volume(want)
                << std::endl;
      status = EXIT_FAILURE;
    }
  }
  return status;
}
