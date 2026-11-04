// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkMathUtilities.h"
#include "vtkNew.h"
#include "vtkPoints.h"
#include "vtkPolyhedron.h"
#include "vtkTetra.h"
#include "vtkUnstructuredGrid.h"

#include <algorithm>
#include <cmath>
#include <iostream>

namespace
{

// Helper to build a polyhedron cell in an unstructured grid and return it.
vtkPolyhedron* BuildPolyhedron(
  vtkUnstructuredGrid* grid, vtkPoints* pts, const std::vector<std::vector<vtkIdType>>& faces)
{
  grid->SetPoints(pts);
  grid->Allocate(1);

  vtkNew<vtkIdList> faceStream;
  faceStream->InsertNextId(static_cast<vtkIdType>(faces.size()));
  for (const auto& face : faces)
  {
    faceStream->InsertNextId(static_cast<vtkIdType>(face.size()));
    for (vtkIdType pid : face)
      faceStream->InsertNextId(pid);
  }
  grid->InsertNextCell(VTK_POLYHEDRON, faceStream);
  return vtkPolyhedron::SafeDownCast(grid->GetCell(0));
}

bool TestVolume(const char* name, double computed, double expected, double tol = 1e-10)
{
  if (!vtkMathUtilities::FuzzyCompare(computed, expected, tol))
  {
    std::cerr << name << ": expected " << expected << ", got " << computed << std::endl;
    return false;
  }
  std::cout << name << ": " << computed << " (OK)" << std::endl;
  return true;
}

} // anonymous namespace

int TestPolyhedronVolume(int, char*[])
{
  bool ok = true;

  // --- Unit cube: vertices at (0,0,0) to (1,1,1), outward normals ---
  {
    vtkNew<vtkPoints> pts;
    pts->InsertNextPoint(0, 0, 0); // 0
    pts->InsertNextPoint(1, 0, 0); // 1
    pts->InsertNextPoint(1, 1, 0); // 2
    pts->InsertNextPoint(0, 1, 0); // 3
    pts->InsertNextPoint(0, 0, 1); // 4
    pts->InsertNextPoint(1, 0, 1); // 5
    pts->InsertNextPoint(1, 1, 1); // 6
    pts->InsertNextPoint(0, 1, 1); // 7

    // Faces with outward normals (CCW when viewed from outside)
    std::vector<std::vector<vtkIdType>> faces = {
      { 0, 3, 2, 1 }, // bottom (-z)
      { 4, 5, 6, 7 }, // top (+z)
      { 0, 1, 5, 4 }, // front (-y)
      { 2, 3, 7, 6 }, // back (+y)
      { 0, 4, 7, 3 }, // left (-x)
      { 1, 2, 6, 5 }, // right (+x)
    };

    vtkNew<vtkUnstructuredGrid> grid;
    vtkPolyhedron* cell = BuildPolyhedron(grid, pts, faces);
    ok &= TestVolume("Unit cube", cell->ComputeVolume(), 1.0);

    double centroid[3];
    cell->GetCentroid(centroid);
    ok &= TestVolume("Unit cube centroid X", centroid[0], 0.5);
    ok &= TestVolume("Unit cube centroid Y", centroid[1], 0.5);
    ok &= TestVolume("Unit cube centroid Z", centroid[2], 0.5);
  }

  // --- 2x3x4 box ---
  {
    vtkNew<vtkPoints> pts;
    pts->InsertNextPoint(0, 0, 0);
    pts->InsertNextPoint(2, 0, 0);
    pts->InsertNextPoint(2, 3, 0);
    pts->InsertNextPoint(0, 3, 0);
    pts->InsertNextPoint(0, 0, 4);
    pts->InsertNextPoint(2, 0, 4);
    pts->InsertNextPoint(2, 3, 4);
    pts->InsertNextPoint(0, 3, 4);

    std::vector<std::vector<vtkIdType>> faces = {
      { 0, 3, 2, 1 },
      { 4, 5, 6, 7 },
      { 0, 1, 5, 4 },
      { 2, 3, 7, 6 },
      { 0, 4, 7, 3 },
      { 1, 2, 6, 5 },
    };

    vtkNew<vtkUnstructuredGrid> grid;
    vtkPolyhedron* cell = BuildPolyhedron(grid, pts, faces);
    ok &= TestVolume("2x3x4 box", cell->ComputeVolume(), 24.0);

    double centroid[3];
    cell->GetCentroid(centroid);
    ok &= TestVolume("2x3x4 box centroid X", centroid[0], 1.0);
    ok &= TestVolume("2x3x4 box centroid Y", centroid[1], 1.5);
    ok &= TestVolume("2x3x4 box centroid Z", centroid[2], 2.0);
  }

  // --- Tetrahedron as polyhedron: V = 1/6 for unit tet ---
  // Vertices: (0,0,0), (1,0,0), (0,1,0), (0,0,1)
  {
    vtkNew<vtkPoints> pts;
    pts->InsertNextPoint(0, 0, 0); // 0
    pts->InsertNextPoint(1, 0, 0); // 1
    pts->InsertNextPoint(0, 1, 0); // 2
    pts->InsertNextPoint(0, 0, 1); // 3

    // 4 triangular faces, outward normals
    std::vector<std::vector<vtkIdType>> faces = {
      { 0, 2, 1 }, // base (-z face, normal points down)
      { 0, 1, 3 }, // front
      { 1, 2, 3 }, // hypotenuse
      { 0, 3, 2 }, // left
    };

    vtkNew<vtkUnstructuredGrid> grid;
    vtkPolyhedron* cell = BuildPolyhedron(grid, pts, faces);
    ok &= TestVolume("Tetrahedron", cell->ComputeVolume(), 1.0 / 6.0);

    // Tet centroid = average of vertices = (0.25, 0.25, 0.25)
    double centroid[3];
    cell->GetCentroid(centroid);
    ok &= TestVolume("Tet centroid X", centroid[0], 0.25);
    ok &= TestVolume("Tet centroid Y", centroid[1], 0.25);
    ok &= TestVolume("Tet centroid Z", centroid[2], 0.25);
  }

  // --- Offset cube: same unit cube but translated to (10, 20, 30) ---
  // Tests that the divergence theorem works correctly away from the origin
  {
    vtkNew<vtkPoints> pts;
    pts->InsertNextPoint(10, 20, 30);
    pts->InsertNextPoint(11, 20, 30);
    pts->InsertNextPoint(11, 21, 30);
    pts->InsertNextPoint(10, 21, 30);
    pts->InsertNextPoint(10, 20, 31);
    pts->InsertNextPoint(11, 20, 31);
    pts->InsertNextPoint(11, 21, 31);
    pts->InsertNextPoint(10, 21, 31);

    std::vector<std::vector<vtkIdType>> faces = {
      { 0, 3, 2, 1 },
      { 4, 5, 6, 7 },
      { 0, 1, 5, 4 },
      { 2, 3, 7, 6 },
      { 0, 4, 7, 3 },
      { 1, 2, 6, 5 },
    };

    vtkNew<vtkUnstructuredGrid> grid;
    vtkPolyhedron* cell = BuildPolyhedron(grid, pts, faces);
    ok &= TestVolume("Offset cube volume", cell->ComputeVolume(), 1.0);

    double centroid[3];
    cell->GetCentroid(centroid);
    ok &= TestVolume("Offset cube centroid X", centroid[0], 10.5);
    ok &= TestVolume("Offset cube centroid Y", centroid[1], 20.5);
    ok &= TestVolume("Offset cube centroid Z", centroid[2], 30.5);
  }

  // --- Thin tetrahedron as polyhedron, from a Delaunay mesh of random points ---
  // Its circumsphere takes in the points that bound the Delaunay triangulation
  // of TriangulateIds(), which then found no tetrahedron inside the cell.
  {
    const double x[4][3] = {
      { 0.99868357181549072, 0.67447972297668457, 0.18184348940849304 },
      { 0.9945252537727356, 0.23247383534908295, 0.44469746947288513 },
      { 0.99774801731109619, 0.14650104939937592, 0.41676804423332214 },
      { 0.99735122919082642, 0.39230278134346008, 0.4737933874130249 },
    };
    vtkNew<vtkPoints> pts;
    for (const auto& p : x)
    {
      pts->InsertNextPoint(p);
    }
    const double exact = vtkTetra::ComputeVolume(const_cast<double*>(x[0]),
      const_cast<double*>(x[1]), const_cast<double*>(x[2]), const_cast<double*>(x[3]));
    // Outward faces for a positively oriented tetrahedron, reversed otherwise.
    std::vector<std::vector<vtkIdType>> faces = { { 0, 2, 1 }, { 0, 1, 3 }, { 1, 2, 3 },
      { 0, 3, 2 } };
    if (exact < 0)
    {
      for (auto& face : faces)
      {
        std::reverse(face.begin(), face.end());
      }
    }

    vtkNew<vtkUnstructuredGrid> grid;
    vtkPolyhedron* cell = BuildPolyhedron(grid, pts, faces);
    ok &= TestVolume("Thin tetrahedron", cell->ComputeVolume(), std::abs(exact), 1e-12);

    vtkNew<vtkIdList> tetIds;
    cell->TriangulateIds(0, tetIds);
    double triangulated = 0.0;
    for (vtkIdType t = 0; t + 3 < tetIds->GetNumberOfIds(); t += 4)
    {
      double p[4][3];
      for (int k = 0; k < 4; ++k)
      {
        grid->GetPoint(tetIds->GetId(t + k), p[k]);
      }
      triangulated += std::abs(vtkTetra::ComputeVolume(p[0], p[1], p[2], p[3]));
    }
    ok &= TestVolume("Thin tetrahedron, triangulated", triangulated, std::abs(exact), 1e-12);
  }

  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
