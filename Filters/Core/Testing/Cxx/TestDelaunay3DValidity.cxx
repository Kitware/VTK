// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
//
// Validates that vtkDelaunay3D produces a correct Delaunay triangulation:
//   1. Every tetrahedron has strictly positive volume.
//   2. The Delaunay property holds: no input point lies strictly inside the
//      circumsphere of any tetrahedron it does not belong to.
//   3. Basic sanity: the output is non-empty for non-trivial input.

#include <vtkDelaunay3D.h>
#include <vtkMath.h>
#include <vtkMinimalStandardRandomSequence.h>
#include <vtkNew.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkTetra.h>
#include <vtkUnstructuredGrid.h>

#include <iostream>
#include <vector>

namespace
{

//------------------------------------------------------------------------------
// Check that every tet has positive volume and the Delaunay property holds.
// Returns true if valid, false otherwise.
bool ValidateDelaunay(vtkUnstructuredGrid* mesh, vtkPoints* inputPoints)
{
  vtkIdType numCells = mesh->GetNumberOfCells();
  vtkIdType numInputPoints = inputPoints->GetNumberOfPoints();

  if (numCells == 0)
  {
    std::cerr << "  FAIL: output has zero cells" << std::endl;
    return false;
  }

  // Collect circumspheres and check volumes.
  std::vector<double> cx(numCells), cy(numCells), cz(numCells), cr2(numCells);
  std::vector<const vtkIdType*> cellPts(numCells);
  std::vector<vtkIdType> cellNpts(numCells);

  vtkPoints* meshPoints = mesh->GetPoints();
  int negativeVolumes = 0;

  for (vtkIdType i = 0; i < numCells; i++)
  {
    vtkIdType npts;
    const vtkIdType* pts;
    mesh->GetCellPoints(i, npts, pts);
    cellPts[i] = pts;
    cellNpts[i] = npts;

    if (npts != 4)
    {
      continue; // skip non-tet cells (alpha shapes may produce triangles etc.)
    }

    double p0[3], p1[3], p2[3], p3[3];
    meshPoints->GetPoint(pts[0], p0);
    meshPoints->GetPoint(pts[1], p1);
    meshPoints->GetPoint(pts[2], p2);
    meshPoints->GetPoint(pts[3], p3);

    // Check positive volume via determinant
    double a[3] = { p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2] };
    double b[3] = { p2[0] - p0[0], p2[1] - p0[1], p2[2] - p0[2] };
    double c[3] = { p3[0] - p0[0], p3[1] - p0[1], p3[2] - p0[2] };
    double det = a[0] * (b[1] * c[2] - b[2] * c[1]) - a[1] * (b[0] * c[2] - b[2] * c[0]) +
      a[2] * (b[0] * c[1] - b[1] * c[0]);

    if (det <= 0.0)
    {
      negativeVolumes++;
    }

    // Compute circumsphere
    double center[3];
    cr2[i] = vtkTetra::Circumsphere(p0, p1, p2, p3, center);
    cx[i] = center[0];
    cy[i] = center[1];
    cz[i] = center[2];
  }

  if (negativeVolumes > 0)
  {
    std::cerr << "  FAIL: " << negativeVolumes << " tetrahedra have non-positive volume"
              << std::endl;
    return false;
  }

  // Check Delaunay property: no input point strictly inside any circumsphere.
  // Use a small tolerance to account for floating-point imprecision.
  int violations = 0;
  for (vtkIdType ci = 0; ci < numCells; ci++)
  {
    if (cellNpts[ci] != 4)
    {
      continue;
    }
    const vtkIdType* pts = cellPts[ci];

    for (vtkIdType pi = 0; pi < numInputPoints; pi++)
    {
      // Skip vertices of this tet
      if (pi == pts[0] || pi == pts[1] || pi == pts[2] || pi == pts[3])
      {
        continue;
      }

      double p[3];
      inputPoints->GetPoint(pi, p);
      double dx = p[0] - cx[ci];
      double dy = p[1] - cy[ci];
      double dz = p[2] - cz[ci];
      double dist2 = dx * dx + dy * dy + dz * dz;

      // Point is strictly inside if dist2 < r2 with some tolerance
      if (dist2 < 0.999 * cr2[ci])
      {
        violations++;
        if (violations <= 5)
        {
          std::cerr << "  Delaunay violation: point " << pi << " inside circumsphere of tet " << ci
                    << " (dist2=" << dist2 << ", r2=" << cr2[ci] << ")" << std::endl;
        }
      }
    }
  }

  if (violations > 0)
  {
    std::cerr << "  FAIL: " << violations << " Delaunay violations found" << std::endl;
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
// Run Delaunay3D on a point cloud and validate the result.
bool RunAndValidate(vtkIdType numPoints, int seed, const char* label)
{
  vtkNew<vtkMinimalStandardRandomSequence> rng;
  rng->SetSeed(seed);

  vtkNew<vtkPoints> points;
  points->SetDataType(VTK_DOUBLE);
  points->SetNumberOfPoints(numPoints);
  for (vtkIdType i = 0; i < numPoints; i++)
  {
    double p[3];
    for (int d = 0; d < 3; d++)
    {
      rng->Next();
      p[d] = rng->GetValue();
    }
    points->SetPoint(i, p);
  }

  vtkNew<vtkPolyData> input;
  input->SetPoints(points);

  vtkNew<vtkDelaunay3D> delaunay;
  delaunay->SetInputData(input);
  // Use Tolerance=0 so that no near-duplicate points are skipped;
  // this lets us verify the pure Delaunay property without being
  // affected by tolerance-based point merging.
  delaunay->SetTolerance(0.0);
  delaunay->Update();

  vtkUnstructuredGrid* output = delaunay->GetOutput();
  if (!output || output->GetNumberOfCells() == 0)
  {
    std::cerr << "[" << label << "] FAIL: no output" << std::endl;
    return false;
  }

  std::cout << "[" << label << "] " << numPoints << " pts -> " << output->GetNumberOfCells()
            << " tets ... ";

  bool valid = ValidateDelaunay(output, points);
  std::cout << (valid ? "PASS" : "FAIL") << std::endl;
  return valid;
}

} // anonymous namespace

int TestDelaunay3DValidity(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  bool allPassed = true;

  // Small case – easy to debug if something is wrong
  allPassed &= ::RunAndValidate(100, 42, "100 random points");

  // Medium case
  allPassed &= ::RunAndValidate(1000, 123, "1000 random points");

  // Larger case – exercises the full optimised path
  allPassed &= ::RunAndValidate(5000, 12345, "5000 random points");

  return allPassed ? EXIT_SUCCESS : EXIT_FAILURE;
}
