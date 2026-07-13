// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * Tests for vtkConvexHull covering 1D, 2D, and 3D hulls, the static API,
 * GeneratePolyData=false, and edge-case inputs.
 */

#include "vtkConvexHull.h"

#include "vtkCellArray.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"
#include "vtkVector.h"

#include <array>
#include <cmath>
#include <iostream>
#include <map>
#include <vector>

namespace
{

vtkSmartPointer<vtkPolyData> MakeInput(const std::vector<std::array<double, 3>>& coords)
{
  vtkNew<vtkPoints> pts;
  pts->SetDataTypeToDouble();
  for (const auto& p : coords)
  {
    pts->InsertNextPoint(p[0], p[1], p[2]);
  }
  vtkNew<vtkPolyData> pd;
  pd->SetPoints(pts);
  return pd;
}

int Check(bool condition, const char* msg)
{
  if (!condition)
  {
    std::cerr << "FAIL: " << msg << "\n";
    return 1;
  }
  return 0;
}

} // namespace

//------------------------------------------------------------------------------
// 1D: four points on the X-axis spanning [0, 5].
// The hull constrains x to [0, 5]; y and z are unconstrained.
static int Test1D()
{
  int errors = 0;

  vtkNew<vtkConvexHull> hull;
  hull->SetInputData(MakeInput({ { 0, 0, 0 }, { 5, 0, 0 }, { 2, 0, 0 }, { 3.5, 0, 0 } }));
  hull->SetDimension(1);
  hull->Update();

  // Interior / boundary
  errors += Check(hull->IsPointWithinConvexHull(2.5, 0, 0), "1D: midpoint should be inside");
  errors +=
    Check(hull->IsPointWithinConvexHull(0.0, 0, 0), "1D: left endpoint should be on boundary");
  errors +=
    Check(hull->IsPointWithinConvexHull(5.0, 0, 0), "1D: right endpoint should be on boundary");

  // Exterior
  errors += Check(!hull->IsPointWithinConvexHull(-0.1, 0, 0), "1D: x=-0.1 should be outside");
  errors += Check(!hull->IsPointWithinConvexHull(5.1, 0, 0), "1D: x=5.1 should be outside");

  return errors;
}

//------------------------------------------------------------------------------
// 2D: unit square in the XY plane (z=0).
// The hull constrains x and y to [0,1] and z to 0 (within tolerance).
static int Test2D()
{
  int errors = 0;

  vtkNew<vtkConvexHull> hull;
  hull->SetInputData(
    MakeInput({ { 0, 0, 0 }, { 1, 0, 0 }, { 1, 1, 0 }, { 0, 1, 0 }, { 0.5, 0.5, 0 } }));
  hull->SetDimension(2);
  hull->Update();

  // Interior
  errors += Check(hull->IsPointWithinConvexHull(0.5, 0.5, 0), "2D: center should be inside");
  errors += Check(hull->IsPointWithinConvexHull(0.1, 0.1, 0), "2D: near corner should be inside");

  // On boundary (edge midpoints)
  errors +=
    Check(hull->IsPointWithinConvexHull(0.0, 0.5, 0), "2D: left-edge midpoint should be inside");
  errors +=
    Check(hull->IsPointWithinConvexHull(1.0, 0.5, 0), "2D: right-edge midpoint should be inside");

  // Exterior (in-plane)
  errors += Check(!hull->IsPointWithinConvexHull(-0.1, 0.5, 0), "2D: x=-0.1 should be outside");
  errors += Check(!hull->IsPointWithinConvexHull(1.1, 0.5, 0), "2D: x=1.1 should be outside");
  errors += Check(!hull->IsPointWithinConvexHull(0.5, -0.1, 0), "2D: y=-0.1 should be outside");
  errors += Check(!hull->IsPointWithinConvexHull(0.5, 1.1, 0), "2D: y=1.1 should be outside");

  // Off-plane: the 2D hull adds a face-normal slab, so z!=0 is outside
  errors +=
    Check(!hull->IsPointWithinConvexHull(0.5, 0.5, 1.0), "2D: z=1.0 should be outside the plane");

  // Geometry: the square hull is a single polygon with 4 vertices
  auto* output = vtkPolyData::SafeDownCast(hull->GetOutput());
  errors += Check(
    output != nullptr && output->GetNumberOfCells() == 1, "2D: square hull should be one polygon");

  return errors;
}

//------------------------------------------------------------------------------
// 3D: unit cube with all 8 corner vertices.
// Euler: V=8, E=18, F=12 triangles (every rectangular face split into 2).
static int Test3D()
{
  int errors = 0;

  vtkNew<vtkConvexHull> hull;
  hull->SetInputData(MakeInput({ { 0, 0, 0 }, { 1, 0, 0 }, { 0, 1, 0 }, { 1, 1, 0 }, { 0, 0, 1 },
    { 1, 0, 1 }, { 0, 1, 1 }, { 1, 1, 1 } }));
  hull->SetDimension(3);
  hull->Update();

  // Interior
  errors += Check(hull->IsPointWithinConvexHull(0.5, 0.5, 0.5), "3D: cube center should be inside");
  errors += Check(hull->IsPointWithinConvexHull(0.1, 0.1, 0.1), "3D: near corner should be inside");

  // Vertices on boundary
  errors +=
    Check(hull->IsPointWithinConvexHull(0, 0, 0), "3D: corner (0,0,0) should be on boundary");
  errors +=
    Check(hull->IsPointWithinConvexHull(1, 1, 1), "3D: corner (1,1,1) should be on boundary");

  // Exterior
  errors += Check(!hull->IsPointWithinConvexHull(-0.1, 0.5, 0.5), "3D: x=-0.1 should be outside");
  errors += Check(!hull->IsPointWithinConvexHull(1.1, 0.5, 0.5), "3D: x=1.1 should be outside");
  errors += Check(!hull->IsPointWithinConvexHull(0.5, 0.5, 1.1), "3D: z=1.1 should be outside");
  errors += Check(!hull->IsPointWithinConvexHull(0.5, -0.1, 0.5), "3D: y=-0.1 should be outside");

  // Geometry: a cube triangulated into 12 faces (F = 12 by Euler: V-E+F=2, 8-18+12=2)
  auto* output = vtkPolyData::SafeDownCast(hull->GetOutput());
  errors += Check(output != nullptr && output->GetNumberOfCells() == 12,
    "3D: cube hull should have 12 triangular faces");

  return errors;
}

//------------------------------------------------------------------------------
// 3D regression: the same unit cube in VTK hexahedron corner order.
// This ordering used to leave QuickHull with degenerate non-supporting planes
// (from coplanar sliver faces) that wrongly classified interior points as
// outside the hull.
static int Test3DHexCornerOrder()
{
  int errors = 0;

  vtkNew<vtkConvexHull> hull;
  hull->SetInputData(MakeInput({ { 0, 0, 0 }, { 1, 0, 0 }, { 1, 1, 0 }, { 0, 1, 0 }, { 0, 0, 1 },
    { 1, 0, 1 }, { 1, 1, 1 }, { 0, 1, 1 } }));
  hull->SetDimension(3);
  hull->Update();

  // Interior points that the degenerate planes used to reject.
  errors += Check(
    hull->IsPointWithinConvexHull(0.3, 0.4, 0.65), "3D-hex-order: (0.3,0.4,0.65) should be inside");
  errors += Check(hull->IsPointWithinConvexHull(0.625, 0.7, 0.35),
    "3D-hex-order: (0.625,0.7,0.35) should be inside");
  errors += Check(
    hull->IsPointWithinConvexHull(0.5, 0.5, 0.5), "3D-hex-order: cube center should be inside");

  // Exterior points must still be rejected.
  errors +=
    Check(!hull->IsPointWithinConvexHull(-0.1, 0.5, 0.5), "3D-hex-order: x=-0.1 should be outside");
  errors +=
    Check(!hull->IsPointWithinConvexHull(0.5, 0.5, 1.1), "3D-hex-order: z=1.1 should be outside");

  // All 8 cube corners must be hull vertices.
  auto* output = vtkPolyData::SafeDownCast(hull->GetOutput());
  errors += Check(output != nullptr && output->GetNumberOfPoints() == 8,
    "3D-hex-order: all 8 cube corners should be hull vertices");

  return errors;
}

//------------------------------------------------------------------------------
// 3D regression: a lat/long point grid on the unit sphere. Every point is a
// hull vertex, which used to expose inconsistent face winding in QuickHull's
// horizon search: the hull came out with missing vertices and an open surface.
static int Test3DSphere()
{
  int errors = 0;

  // Poles plus 18 interior latitude rings of 20 points each (matches
  // vtkSphereSource with theta/phi resolution 20): N = 2 + 18 * 20 = 362.
  const double pi = vtkMath::Pi();
  std::vector<std::array<double, 3>> coords;
  coords.push_back({ 0.0, 0.0, 1.0 });
  coords.push_back({ 0.0, 0.0, -1.0 });
  for (int i = 1; i <= 18; ++i)
  {
    const double phi = pi * i / 19.0;
    for (int j = 0; j < 20; ++j)
    {
      const double theta = 2.0 * pi * j / 20.0;
      coords.push_back(
        { std::sin(phi) * std::cos(theta), std::sin(phi) * std::sin(theta), std::cos(phi) });
    }
  }
  const vtkIdType numPoints = static_cast<vtkIdType>(coords.size());

  vtkNew<vtkConvexHull> hull;
  hull->SetInputData(MakeInput(coords));
  hull->SetDimension(3);
  hull->Update();

  // Every input point is a hull vertex; a simplicial hull has 2V-4 facets.
  auto* output = vtkPolyData::SafeDownCast(hull->GetOutput());
  errors += Check(output != nullptr && output->GetNumberOfPoints() == numPoints,
    "3D-sphere: every input point should be a hull vertex");
  errors += Check(output != nullptr && output->GetNumberOfCells() == 2 * numPoints - 4,
    "3D-sphere: hull should have 2V-4 triangles");

  // All input points are inside (on the boundary of) the half-plane set, and
  // pushed-out copies are rejected.
  for (const auto& p : coords)
  {
    errors +=
      Check(hull->IsPointWithinConvexHull(p.data()), "3D-sphere: input point should be inside");
    errors += Check(!hull->IsPointWithinConvexHull(1.05 * p[0], 1.05 * p[1], 1.05 * p[2]),
      "3D-sphere: scaled-out point should be outside");
  }

  // The surface is closed and consistently oriented: every directed edge
  // appears exactly once, and so does its reverse.
  if (output)
  {
    std::map<std::pair<vtkIdType, vtkIdType>, int> edgeCount;
    auto* polys = output->GetPolys();
    vtkIdType npts;
    const vtkIdType* cellPts;
    for (polys->InitTraversal(); polys->GetNextCell(npts, cellPts);)
    {
      for (vtkIdType e = 0; e < npts; ++e)
      {
        ++edgeCount[{ cellPts[e], cellPts[(e + 1) % npts] }];
      }
    }
    bool closed = !edgeCount.empty();
    for (const auto& entry : edgeCount)
    {
      const auto reverse = std::make_pair(entry.first.second, entry.first.first);
      const auto rIt = edgeCount.find(reverse);
      closed = closed && entry.second == 1 && rIt != edgeCount.end() && rIt->second == 1;
    }
    errors += Check(closed, "3D-sphere: hull surface should be closed and consistently oriented");
  }

  return errors;
}

//------------------------------------------------------------------------------
// Static API: ComputeConvexHull with a raw vtkVector3d array, then IsPointInside.
static int TestStaticAPI()
{
  int errors = 0;

  // Unit tetrahedron: (0,0,0), (1,0,0), (0,1,0), (0,0,1)
  std::vector<vtkVector3d> pts = {
    { 0, 0, 0 },
    { 1, 0, 0 },
    { 0, 1, 0 },
    { 0, 0, 1 },
  };

  std::vector<vtkConvexHull::Plane> planes;
  vtkConvexHull::ComputeConvexHull(pts.data(), static_cast<int>(pts.size()), 3, planes);

  errors += Check(!planes.empty(), "StaticAPI: tetrahedron should produce planes");

  // Centroid (0.25, 0.25, 0.25) is strictly inside
  errors += Check(vtkConvexHull::IsPointInside({ 0.25, 0.25, 0.25 }, planes),
    "StaticAPI: tetrahedron centroid should be inside");

  // Clearly exterior
  errors += Check(!vtkConvexHull::IsPointInside({ 1, 1, 1 }, planes),
    "StaticAPI: (1,1,1) should be outside the tetrahedron");
  errors += Check(!vtkConvexHull::IsPointInside({ -0.5, 0, 0 }, planes),
    "StaticAPI: (-0.5,0,0) should be outside the tetrahedron");

  // Capacity is preserved across calls (re-use the planes vector)
  const auto prevCapacity = planes.capacity();
  vtkConvexHull::ComputeConvexHull(pts.data(), static_cast<int>(pts.size()), 3, planes);
  errors += Check(planes.capacity() >= prevCapacity,
    "StaticAPI: planes vector capacity should not shrink on reuse");

  return errors;
}

//------------------------------------------------------------------------------
// GeneratePolyData=false: IsPointWithinConvexHull still works, output is empty.
static int TestNoGeometry()
{
  int errors = 0;

  vtkNew<vtkConvexHull> hull;
  hull->SetInputData(MakeInput({ { 0, 0, 0 }, { 1, 0, 0 }, { 0, 1, 0 }, { 1, 1, 0 }, { 0, 0, 1 },
    { 1, 0, 1 }, { 0, 1, 1 }, { 1, 1, 1 } }));
  hull->SetDimension(3);
  hull->GeneratePolyDataOff();
  hull->Update();

  errors += Check(
    hull->IsPointWithinConvexHull(0.5, 0.5, 0.5), "NoGeom: cube center should still be inside");
  errors +=
    Check(!hull->IsPointWithinConvexHull(2, 2, 2), "NoGeom: (2,2,2) should still be outside");

  auto* output = vtkPolyData::SafeDownCast(hull->GetOutput());
  errors +=
    Check(output != nullptr && output->GetNumberOfCells() == 0, "NoGeom: output should be empty");

  return errors;
}

//------------------------------------------------------------------------------
// Edge cases: empty input and single-point input should not crash.
static int TestEdgeCases()
{
  int errors = 0;

  // Empty input
  {
    vtkNew<vtkConvexHull> hull;
    hull->SetInputData(MakeInput({}));
    hull->SetDimension(3);
    hull->Update();
    errors +=
      Check(hull->GetOutput() != nullptr, "EdgeCase: empty input should not crash the filter");
  }

  // Single point: not enough vertices for any hull
  {
    vtkNew<vtkConvexHull> hull;
    hull->SetInputData(MakeInput({ { 1, 2, 3 } }));
    hull->SetDimension(3);
    hull->Update();
    errors += Check(hull->GetOutput() != nullptr, "EdgeCase: single-point input should not crash");
  }

  // Collinear 3D input: Compute3D falls back through Compute2D to Compute1D
  {
    vtkNew<vtkConvexHull> hull;
    hull->SetInputData(
      MakeInput({ { 0, 0, 0 }, { 1, 0, 0 }, { 2, 0, 0 }, { 3, 0, 0 }, { 4, 0, 0 } }));
    hull->SetDimension(3);
    hull->Update();
    errors += Check(hull->IsPointWithinConvexHull(2, 0, 0),
      "EdgeCase: collinear midpoint should be inside after fallback");
    errors += Check(!hull->IsPointWithinConvexHull(5, 0, 0),
      "EdgeCase: collinear point beyond range should be outside after fallback");
  }

  return errors;
}

//------------------------------------------------------------------------------
int TestConvexHull(int, char*[])
{
  int errors = 0;
  errors += Test1D();
  errors += Test2D();
  errors += Test3D();
  errors += Test3DHexCornerOrder();
  errors += Test3DSphere();
  errors += TestStaticAPI();
  errors += TestNoGeometry();
  errors += TestEdgeCases();
  return errors == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
