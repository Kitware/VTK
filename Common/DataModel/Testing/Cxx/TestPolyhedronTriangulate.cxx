// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// The tetrahedra of a polyhedron make it up, concave or not: they lie inside it and their
// volume is its volume. A cell that cannot be tetrahedralized on its own points, the
// Schoenhardt prism, gets none.

#include "vtkCellType.h"
#include "vtkGenericCell.h"
#include "vtkIdList.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkPoints.h"
#include "vtkPolyhedron.h"
#include "vtkSmartPointer.h"
#include "vtkStringFormatter.h"
#include "vtkTetra.h"
#include "vtkUnstructuredGrid.h"

#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace
{
using Point = std::array<double, 3>;
using Faces = std::vector<std::vector<vtkIdType>>;

int Failures = 0;

void Check(bool condition, const std::string& message)
{
  if (!condition)
  {
    std::cerr << message << std::endl;
    ++Failures;
  }
}

vtkSmartPointer<vtkUnstructuredGrid> MakeCell(const std::vector<Point>& points, const Faces& faces)
{
  vtkNew<vtkPoints> pts;
  pts->SetDataTypeToDouble();
  for (const auto& p : points)
  {
    pts->InsertNextPoint(p.data());
  }
  vtkNew<vtkIdList> stream;
  stream->InsertNextId(static_cast<vtkIdType>(faces.size()));
  for (const auto& face : faces)
  {
    stream->InsertNextId(static_cast<vtkIdType>(face.size()));
    for (vtkIdType id : face)
    {
      stream->InsertNextId(id);
    }
  }
  auto grid = vtkSmartPointer<vtkUnstructuredGrid>::New();
  grid->SetPoints(pts);
  grid->InsertNextCell(VTK_POLYHEDRON, stream);
  return grid;
}

// A cube with its top pushed in to a point at height 0.6.
vtkSmartPointer<vtkUnstructuredGrid> DentedCube()
{
  return MakeCell({ { 0, 0, 0 }, { 1, 0, 0 }, { 1, 1, 0 }, { 0, 1, 0 }, { 0, 0, 1 }, { 1, 0, 1 },
                    { 1, 1, 1 }, { 0, 1, 1 }, { 0.5, 0.5, 0.6 } },
    { { 0, 3, 2, 1 }, { 0, 1, 5, 4 }, { 1, 2, 6, 5 }, { 2, 3, 7, 6 }, { 3, 0, 4, 7 }, { 4, 5, 8 },
      { 5, 6, 8 }, { 6, 7, 8 }, { 7, 4, 8 } });
}

// A planar ring, extruded from z = 0 to 1.
vtkSmartPointer<vtkUnstructuredGrid> Prism(const std::vector<std::array<double, 2>>& ring)
{
  const auto n = static_cast<vtkIdType>(ring.size());
  std::vector<Point> points;
  for (double z : { 0.0, 1.0 })
  {
    for (const auto& p : ring)
    {
      points.push_back({ p[0], p[1], z });
    }
  }
  Faces faces(2);
  for (vtkIdType i = 0; i < n; ++i)
  {
    faces[0].push_back(n - 1 - i);
    faces[1].push_back(n + i);
    faces.push_back({ i, (i + 1) % n, n + (i + 1) % n, n + i });
  }
  return MakeCell(points, faces);
}

// A disk with a 60 degree wedge cut out.
vtkSmartPointer<vtkUnstructuredGrid> PacMan()
{
  std::vector<std::array<double, 2>> ring = { { 0, 0 } };
  for (int k = 0; k <= 12; ++k)
  {
    const double a = vtkMath::RadiansFromDegrees(30.0 + 300.0 * k / 12);
    ring.push_back({ std::cos(a), std::sin(a) });
  }
  return Prism(ring);
}

// A ring of radii 0.5 and 1 over the given angle: no point sees all of it past 180 degrees.
vtkSmartPointer<vtkUnstructuredGrid> ThickArc(double degrees)
{
  std::vector<std::array<double, 2>> ring;
  for (int k = 0; k <= 12; ++k)
  {
    const double a = vtkMath::RadiansFromDegrees(degrees * k / 12);
    ring.push_back({ std::cos(a), std::sin(a) });
  }
  for (int k = 12; k >= 0; --k)
  {
    const double a = vtkMath::RadiansFromDegrees(degrees * k / 12);
    ring.push_back({ 0.5 * std::cos(a), 0.5 * std::sin(a) });
  }
  return Prism(ring);
}

// A triangular prism whose top is turned by 30 degrees, each side folded inward along a
// diagonal. Every tetrahedron on its points crosses its faces.
vtkSmartPointer<vtkUnstructuredGrid> Schoenhardt()
{
  std::vector<Point> points;
  for (int k = 0; k < 3; ++k)
  {
    const double a = vtkMath::RadiansFromDegrees(120.0 * k);
    points.push_back({ std::cos(a), std::sin(a), 0.0 });
  }
  for (int k = 0; k < 3; ++k)
  {
    const double a = vtkMath::RadiansFromDegrees(120.0 * k + 30.0);
    points.push_back({ std::cos(a), std::sin(a), 1.0 });
  }
  Faces faces = { { 0, 2, 1 }, { 3, 4, 5 } };
  for (vtkIdType i = 0; i < 3; ++i)
  {
    const vtkIdType j = (i + 1) % 3;
    faces.push_back({ i, j, 3 + j });
    faces.push_back({ i, 3 + j, 3 + i });
  }
  return MakeCell(points, faces);
}

void CheckTetrahedra(vtkUnstructuredGrid* grid, const std::string& name)
{
  vtkNew<vtkGenericCell> cell;
  grid->GetCell(0, cell);
  auto* polyhedron = vtkPolyhedron::SafeDownCast(cell->GetRepresentativeCell());
  vtkNew<vtkIdList> ptIds;
  vtkNew<vtkPoints> pts;
  pts->SetDataTypeToDouble();
  Check(polyhedron->Triangulate(0, ptIds, pts) == 1, name + ": Triangulate() should succeed");
  const vtkIdType numTets = ptIds->GetNumberOfIds() / 4;
  double volume = 0.0;
  for (vtkIdType tet = 0; tet < numTets; ++tet)
  {
    double x[4][3], center[3] = { 0, 0, 0 };
    for (int k = 0; k < 4; ++k)
    {
      pts->GetPoint(4 * tet + k, x[k]);
      for (int i = 0; i < 3; ++i)
      {
        center[i] += 0.25 * x[k][i];
      }
    }
    volume += std::abs(vtkTetra::ComputeVolume(x[0], x[1], x[2], x[3]));
    Check(polyhedron->IsInside(center, 1e-6) == 1,
      name + ": tetrahedron " + vtk::to_string(tet) + " is outside the cell");
  }
  const double expected = polyhedron->ComputeVolume();
  Check(numTets > 0 && std::abs(volume - expected) <= 1e-9 * expected,
    name + ": the tetrahedra hold " + vtk::to_string(volume) + " instead of " +
      vtk::to_string(expected));
}
} // anonymous namespace

int TestPolyhedronTriangulate(int, char*[])
{
  CheckTetrahedra(DentedCube(), "dented cube");
  CheckTetrahedra(PacMan(), "pac-man");
  for (double degrees : { 90.0, 270.0, 340.0 })
  {
    CheckTetrahedra(ThickArc(degrees), "thick arc of " + vtk::to_string(degrees) + " degrees");
  }

  vtkNew<vtkGenericCell> cell;
  Schoenhardt()->GetCell(0, cell);
  vtkNew<vtkIdList> ptIds;
  vtkNew<vtkPoints> pts;
  Check(cell->Triangulate(0, ptIds, pts) == 0 && ptIds->GetNumberOfIds() == 0,
    "the Schoenhardt prism should get no tetrahedra");

  if (Failures > 0)
  {
    std::cerr << Failures << " failure(s)" << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
