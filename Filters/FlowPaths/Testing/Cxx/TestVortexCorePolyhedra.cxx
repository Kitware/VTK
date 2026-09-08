// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Covers vtkVortexCore and vtkParallelVectors on a polyhedral mesh, which the
// existing tests do not: TestVortexCore and TestParallelVectors build only
// VTK_TETRA and VTK_HEXAHEDRON grids, and those take their face points from
// static local-id tables, so the defect below stayed invisible.
//
// vtkPolyhedron::GetFacePoints returned global ids, so
// vtkParallelVectors::surfaceTessellationForCell indexed its per-cell point
// array out of range. Reverting that fix makes this test crash.
//
// One cell has a face with a collinear mid-edge vertex, so the non-quad
// triangulation branch is exercised as well. Note that this does NOT reproduce
// the nPoints - 2 buffer overrun: for this pentagon TriangulateLocalIds still
// returns nPoints - 2 triangles, one of them degenerate, so nothing overruns.
// Reverting the sizing fix alone leaves this test passing. A face whose ear cut
// yields strictly fewer triangles is still needed to cover that.

#include <vtkCellArray.h>
#include <vtkDoubleArray.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkPointLocator.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkUnstructuredGrid.h>
#include <vtkVortexCore.h>

#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace
{
// The cube's six quad faces, as offsets into the eight corner points.
constexpr vtkIdType CubeQuads[6][4] = { { 0, 3, 2, 1 }, { 4, 5, 6, 7 }, { 0, 1, 5, 4 },
  { 1, 2, 6, 5 }, { 2, 3, 7, 6 }, { 3, 0, 4, 7 } };

// A grid of cubes, every cell a VTK_POLYHEDRON. The cell at the origin end
// gets one face subdivided with a mid-edge point, giving that face five
// vertices of which three are collinear.
vtkSmartPointer<vtkUnstructuredGrid> BuildPolyhedralGrid(int nX, int nY, int nZ)
{
  vtkNew<vtkPoints> pointArray;
  vtkNew<vtkPointLocator> locator;
  double bounds[6] = { -1., 1., -1., 1., -1., 1. };
  locator->InitPointInsertion(pointArray, bounds);

  vtkNew<vtkUnstructuredGrid> grid;
  grid->SetPoints(pointArray);

  const double dx = (bounds[1] - bounds[0]) / nX;
  const double dy = (bounds[3] - bounds[2]) / nY;
  const double dz = (bounds[5] - bounds[4]) / nZ;

  bool collinearDone = false;
  for (int i = 0; i < nX; ++i)
  {
    for (int j = 0; j < nY; ++j)
    {
      for (int k = 0; k < nZ; ++k)
      {
        const double x0 = bounds[0] + i * dx, x1 = x0 + dx;
        const double y0 = bounds[2] + j * dy, y1 = y0 + dy;
        const double z0 = bounds[4] + k * dz, z1 = z0 + dz;
        const double corner[8][3] = { { x0, y0, z0 }, { x1, y0, z0 }, { x1, y1, z0 },
          { x0, y1, z0 }, { x0, y0, z1 }, { x1, y0, z1 }, { x1, y1, z1 }, { x0, y1, z1 } };

        vtkIdType gid[8];
        for (int c = 0; c < 8; ++c)
        {
          locator->InsertUniquePoint(corner[c], gid[c]);
        }

        std::vector<vtkIdType> cellPts(gid, gid + 8);
        vtkNew<vtkCellArray> faces;

        // Subdivide the -z face of exactly one cell by inserting the midpoint
        // of its first edge. The face stays planar and still covers the same
        // area, but now carries a vertex collinear with its neighbours.
        const bool addCollinear = !collinearDone;
        vtkIdType midId = -1;
        if (addCollinear)
        {
          const double mid[3] = { 0.5 * (corner[0][0] + corner[3][0]),
            0.5 * (corner[0][1] + corner[3][1]), 0.5 * (corner[0][2] + corner[3][2]) };
          locator->InsertUniquePoint(mid, midId);
          cellPts.push_back(midId);
          collinearDone = true;
        }

        for (int f = 0; f < 6; ++f)
        {
          if (addCollinear && f == 0)
          {
            // { 0, 3, 2, 1 } becomes { 0, mid, 3, 2, 1 }
            const vtkIdType penta[5] = { gid[0], midId, gid[3], gid[2], gid[1] };
            faces->InsertNextCell(5, penta);
          }
          else
          {
            const vtkIdType quad[4] = { gid[CubeQuads[f][0]], gid[CubeQuads[f][1]],
              gid[CubeQuads[f][2]], gid[CubeQuads[f][3]] };
            faces->InsertNextCell(4, quad);
          }
        }

        grid->InsertNextCell(
          VTK_POLYHEDRON, static_cast<vtkIdType>(cellPts.size()), cellPts.data(), faces);
      }
    }
  }
  return grid;
}

// The swirling field TestVortexCore uses. Double here: array precision is a
// separate axis, exercised by TestVortexCore's float case.
void AddVelocity(vtkUnstructuredGrid* grid)
{
  vtkNew<vtkDoubleArray> velocity;
  velocity->SetName("velocity");
  velocity->SetNumberOfComponents(3);
  velocity->SetNumberOfTuples(grid->GetNumberOfPoints());

  for (vtkIdType i = 0; i < grid->GetNumberOfPoints(); ++i)
  {
    double p[3];
    grid->GetPoint(i, p);
    constexpr double s = .5, r = .5, kk = .1;
    const double v[3] = { -s * p[1] + r * s * std::sin(kk * p[2]),
      s * p[0] - r * s * std::cos(kk * p[2]), 1. };
    velocity->SetTypedTuple(i, v);
  }
  grid->GetPointData()->AddArray(velocity);
  grid->GetPointData()->SetActiveVectors("velocity");
}
}

int TestVortexCorePolyhedra(int, char*[])
{
  auto grid = BuildPolyhedralGrid(3, 3, 3);
  if (grid->GetNumberOfCells() != 27)
  {
    std::cerr << "Expected 27 polyhedral cells, got " << grid->GetNumberOfCells() << "."
              << std::endl;
    return EXIT_FAILURE;
  }
  AddVelocity(grid);

  // The tessellation sweeps every cell before any core is identified, so this
  // exercises the polyhedral face path whether or not cores are found.
  vtkNew<vtkVortexCore> vortexCore;
  vortexCore->SetInputData(grid);
  vortexCore->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "velocity");
  vortexCore->Update();

  vtkPolyData* output = vtkPolyData::SafeDownCast(vortexCore->GetOutput());
  if (!output)
  {
    std::cerr << "No output." << std::endl;
    return EXIT_FAILURE;
  }
  if (output->CheckAttributes())
  {
    std::cerr << "Attribute check failed." << std::endl;
    return EXIT_FAILURE;
  }
  std::cout << output->GetNumberOfCells() << " output cells, " << output->GetNumberOfPoints()
            << " points." << std::endl;

  return EXIT_SUCCESS;
}
