// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Verifies that vtkOpenGLIndexBufferObject::AppendTriangleIndexBuffer produces a
// correct triangulation of non-convex polygons. A naive fan triangulation from
// the first vertex emits triangles that fall outside a non-convex polygon; this
// test fails against such an implementation and passes with an ear-clip that
// respects the polygon boundary.

#include "vtkCellArray.h"
#include "vtkNew.h"
#include "vtkOpenGLIndexBufferObject.h"
#include "vtkPoints.h"

#include <array>
#include <cmath>
#include <iostream>
#include <vector>

namespace
{

struct Polygon
{
  const char* name;
  std::vector<std::array<double, 3>> verts;
};

// All test polygons are planar in z = 0, so areas use the 2D shoelace form.
double TriangleArea(
  const std::array<double, 3>& a, const std::array<double, 3>& b, const std::array<double, 3>& c)
{
  const double ux = b[0] - a[0], uy = b[1] - a[1];
  const double vx = c[0] - a[0], vy = c[1] - a[1];
  return 0.5 * std::abs(ux * vy - uy * vx);
}

double PolygonArea(const std::vector<std::array<double, 3>>& v)
{
  double a = 0.0;
  const std::size_t n = v.size();
  for (std::size_t i = 0; i < n; ++i)
  {
    const auto& p = v[i];
    const auto& q = v[(i + 1) % n];
    a += p[0] * q[1] - q[0] * p[1];
  }
  return std::abs(a) * 0.5;
}

bool PointInPolygon(const std::array<double, 3>& pt, const std::vector<std::array<double, 3>>& v)
{
  bool inside = false;
  const std::size_t n = v.size();
  for (std::size_t i = 0, j = n - 1; i < n; j = i++)
  {
    const double xi = v[i][0], yi = v[i][1];
    const double xj = v[j][0], yj = v[j][1];
    if (((yi > pt[1]) != (yj > pt[1])) && (pt[0] < (xj - xi) * (pt[1] - yi) / (yj - yi) + xi))
    {
      inside = !inside;
    }
  }
  return inside;
}

bool CheckPolygon(const Polygon& poly)
{
  const auto& v = poly.verts;
  const auto n = static_cast<vtkIdType>(v.size());

  vtkNew<vtkPoints> points;
  for (const auto& p : v)
  {
    points->InsertNextPoint(p[0], p[1], p[2]);
  }
  vtkNew<vtkCellArray> cells;
  cells->InsertNextCell(n);
  for (vtkIdType i = 0; i < n; ++i)
  {
    cells->InsertCellPoint(i);
  }

  std::vector<unsigned int> indexArray;
  vtkOpenGLIndexBufferObject::AppendTriangleIndexBuffer(
    indexArray, cells, points, 0, nullptr, nullptr);

  bool ok = true;

  // A simple polygon of n vertices must triangulate into exactly n - 2 triangles.
  const std::size_t expectedTriangles = static_cast<std::size_t>(n - 2);
  if (indexArray.size() != expectedTriangles * 3)
  {
    std::cerr << poly.name << ": expected " << expectedTriangles << " triangles ("
              << expectedTriangles * 3 << " indices), got " << indexArray.size() << " indices\n";
    ok = false;
  }

  // The triangulation must cover the polygon exactly: summed triangle areas equal
  // the polygon area, and every triangle lies inside the polygon. A fan
  // triangulation of a non-convex polygon violates both: it emits triangles that
  // overlap and spill outside the boundary across a reentrant vertex.
  const double polyArea = PolygonArea(v);
  double triArea = 0.0;
  for (std::size_t i = 0; i + 2 < indexArray.size(); i += 3)
  {
    const auto& a = v[indexArray[i]];
    const auto& b = v[indexArray[i + 1]];
    const auto& c = v[indexArray[i + 2]];
    triArea += TriangleArea(a, b, c);
    const std::array<double, 3> centroid = { (a[0] + b[0] + c[0]) / 3.0, (a[1] + b[1] + c[1]) / 3.0,
      0.0 };
    if (!PointInPolygon(centroid, v))
    {
      std::cerr << poly.name << ": triangle (" << indexArray[i] << "," << indexArray[i + 1] << ","
                << indexArray[i + 2] << ") lies outside the polygon\n";
      ok = false;
    }
  }
  if (std::abs(triArea - polyArea) > 1e-6 * std::max(1.0, polyArea))
  {
    std::cerr << poly.name << ": triangulated area " << triArea << " != polygon area " << polyArea
              << "\n";
    ok = false;
  }

  return ok;
}

}

int TestIndexBufferObjectTriangulation(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  std::vector<Polygon> polygons = {
    // Convex pentagon: any correct triangulation (fan or ear-clip) passes. Acts
    // as a control so the test is not vacuously satisfied only by rejection.
    { "convex_pentagon",
      { { 1.0, 0.0, 0.0 }, { 0.31, 0.95, 0.0 }, { -0.81, 0.59, 0.0 }, { -0.81, -0.59, 0.0 },
        { 0.31, -0.95, 0.0 } } },
    // Non-convex "dart" pentagon: fan-from-vertex-0 emits triangle (0,2,3), whose
    // centroid lies outside the reentrant vertex 3, inflating the area from 10 to
    // 14. Ear clipping yields three triangles that tile the polygon exactly.
    { "dart_pentagon",
      { { 0.0, 0.0, 0.0 }, { 4.0, 0.0, 0.0 }, { 4.0, 4.0, 0.0 }, { 2.0, 1.0, 0.0 },
        { 0.0, 4.0, 0.0 } } },
    // Non-convex U / comb shape: fan-from-vertex-0 spills across the notch in two
    // triangles (area 31 vs the true 22).
    { "u_shape",
      { { 0.0, 0.0, 0.0 }, { 5.0, 0.0, 0.0 }, { 5.0, 5.0, 0.0 }, { 3.0, 5.0, 0.0 },
        { 3.0, 2.0, 0.0 }, { 2.0, 2.0, 0.0 }, { 2.0, 5.0, 0.0 }, { 0.0, 5.0, 0.0 } } },
  };

  bool ok = true;
  for (const auto& poly : polygons)
  {
    ok &= CheckPolygon(poly);
  }

  if (!ok)
  {
    std::cerr << "TestIndexBufferObjectTriangulation FAILED\n";
    return EXIT_FAILURE;
  }
  std::cout << "TestIndexBufferObjectTriangulation passed\n";
  return EXIT_SUCCESS;
}
