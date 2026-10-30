// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// A grid of polyhedra whose shared faces are stored once, the second cell of a face listing it as
// -(n+1) to traverse it in reverse, gives the filters the results of the same grid with every
// face stored per cell. Stored once without the sign, a shared face is wound inward for one of
// its cells; the filters that rely on the winding then differ, and vtkAppendFilter must not
// overflow on face locations that outnumber the faces.

#include "vtkAppendFilter.h"
#include "vtkCellArray.h"
#include "vtkContourFilter.h"
#include "vtkCutter.h"
#include "vtkDoubleArray.h"
#include "vtkExtractCells.h"
#include "vtkGenericCell.h"
#include "vtkIdList.h"
#include "vtkMassProperties.h"
#include "vtkMathUtilities.h"
#include "vtkNew.h"
#include "vtkPlane.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyhedron.h"
#include "vtkPolyhedronCellArray.h"
#include "vtkStaticCleanUnstructuredGrid.h"
#include "vtkTriangleFilter.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace
{
int Failures = 0;

void Check(bool condition, const std::string& message)
{
  if (!condition)
  {
    std::cerr << message << std::endl;
    ++Failures;
  }
}

void CheckClose(double value, double expected, const std::string& message)
{
  Check(vtkMathUtilities::FuzzyCompare(value, expected, 1e-9 * std::max(1.0, std::abs(expected))),
    message + ": " + std::to_string(value) + " instead of " + std::to_string(expected));
}

// n^3 hexahedra as polyhedra filling the unit cube, each with its own copy of its faces, on
// unevenly spaced planes so that the faces are planar and the cells unlike.
vtkSmartPointer<vtkUnstructuredGrid> MakeDuplicated(int n)
{
  auto grid = vtkSmartPointer<vtkUnstructuredGrid>::New();
  vtkNew<vtkPoints> points;
  auto index = [n](int i, int j, int k) { return i + (n + 1) * (j + (n + 1) * k); };
  auto plane = [n](int i, double phase)
  { return (i + (i > 0 && i < n ? 0.2 * std::sin(phase + 2.0 * i) : 0.0)) / n; };
  for (int k = 0; k <= n; ++k)
  {
    for (int j = 0; j <= n; ++j)
    {
      for (int i = 0; i <= n; ++i)
      {
        points->InsertNextPoint(plane(i, 1.0), plane(j, 2.0), plane(k, 3.0));
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
      for (int i = 0; i < n; ++i)
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

double Area(vtkPolyData* surface)
{
  vtkNew<vtkTriangleFilter> triangles;
  triangles->SetInputData(surface);
  vtkNew<vtkMassProperties> mass;
  mass->SetInputConnection(triangles->GetOutputPort());
  mass->Update();
  return mass->GetSurfaceArea();
}

double ContourArea(vtkUnstructuredGrid* grid)
{
  vtkNew<vtkUnstructuredGrid> input;
  input->ShallowCopy(grid);
  vtkNew<vtkDoubleArray> scalars;
  scalars->SetName("f");
  for (vtkIdType p = 0; p < input->GetNumberOfPoints(); ++p)
  {
    double x[3];
    input->GetPoint(p, x);
    scalars->InsertNextValue(std::sin(3.0 * x[0]) + x[1] * x[1] - 0.5 * x[2]);
  }
  input->GetPointData()->SetScalars(scalars);
  vtkNew<vtkContourFilter> contour;
  contour->SetInputData(input);
  contour->SetValue(0, 0.6);
  contour->Update();
  return Area(contour->GetOutput());
}

double CutArea(vtkUnstructuredGrid* grid)
{
  vtkNew<vtkPlane> plane;
  plane->SetOrigin(0.37, 0.41, 0.43);
  plane->SetNormal(1, 2, 3);
  vtkNew<vtkCutter> cutter;
  cutter->SetInputData(grid);
  cutter->SetCutFunction(plane);
  cutter->Update();
  return Area(cutter->GetOutput());
}

double ExtractedVolume(vtkUnstructuredGrid* grid)
{
  vtkNew<vtkIdList> cellIds;
  for (vtkIdType cellId = 0; cellId < grid->GetNumberOfCells(); cellId += 2)
  {
    cellIds->InsertNextId(cellId);
  }
  vtkNew<vtkExtractCells> extract;
  extract->SetInputData(grid);
  extract->SetCellList(cellIds);
  extract->Update();
  return Volume(extract->GetOutput());
}

vtkSmartPointer<vtkUnstructuredGrid> AppendTwice(vtkUnstructuredGrid* grid)
{
  vtkNew<vtkAppendFilter> append;
  append->AddInputData(grid);
  append->AddInputData(grid);
  append->Update();
  return append->GetOutput();
}

double CleanedVolume(vtkUnstructuredGrid* grid)
{
  vtkNew<vtkStaticCleanUnstructuredGrid> clean;
  clean->SetInputData(grid);
  clean->Update();
  return Volume(clean->GetOutput());
}

} // anonymous namespace

int TestPolyhedronSharedFaces(int, char*[])
{
  auto duplicated = MakeDuplicated(4);
  auto shared = MakeShared(duplicated, false);
  auto signedShared = MakeShared(duplicated, true);

  // 4^3 cells: 3 * 4 * 4 * 5 faces, 3 * 4 * 4 * 3 of them shared.
  const vtkIdType numSides = 240;
  Check(shared->GetPolyhedronCellArray()->GetNumberOfFaces() == numSides &&
      signedShared->GetPolyhedronCellArray()->GetNumberOfFaces() == numSides,
    "every shared face should be stored once");

  const double volume = Volume(duplicated);
  const double contourArea = ContourArea(duplicated);
  const double cutArea = CutArea(duplicated);
  const double extracted = ExtractedVolume(duplicated);
  CheckClose(volume, 1.0, "the polyhedra should fill the unit cube");

  CheckClose(Volume(signedShared), volume, "signed: volume");
  CheckClose(ContourArea(signedShared), contourArea, "signed: contour area");
  CheckClose(CutArea(signedShared), cutArea, "signed: cut area");
  CheckClose(ExtractedVolume(signedShared), extracted, "signed: extracted volume");
  CheckClose(CleanedVolume(signedShared), volume, "signed: cleaned volume");
  auto signedTwice = AppendTwice(signedShared);
  CheckClose(Volume(signedTwice), 2.0 * volume, "signed: appended volume");
  CheckClose(ContourArea(signedTwice), 2.0 * contourArea, "signed: appended contour area");

  // Without the sign, a shared face is inward for one of its cells, so the divergence theorem
  // volume and the contour both come out wrong.
  Check(std::abs(Volume(shared) - volume) > 1e-3, "unsigned: the volume was expected to differ");
  Check(std::abs(ContourArea(shared) - contourArea) > 1e-3,
    "unsigned: the contour was expected to differ");
  auto sharedTwice = AppendTwice(shared);
  Check(sharedTwice->GetNumberOfCells() == 2 * shared->GetNumberOfCells(),
    "unsigned: wrong number of appended cells");
  Check(sharedTwice->GetPolyhedronCellArray()->GetNumberOfFaces() == 2 * numSides,
    "unsigned: the appended faces should stay shared");

  if (Failures > 0)
  {
    std::cerr << Failures << " failure(s)" << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
