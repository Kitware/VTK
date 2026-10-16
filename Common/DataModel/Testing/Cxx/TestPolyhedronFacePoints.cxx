// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// vtkCell3D::GetFacePoints is documented to return point ids local to the
// cell, in [0, GetNumberOfPoints()). vtkPolyhedron used to return ids in the
// mesh's global id space instead, so any caller that indexed its own per-cell
// arrays with the result read the wrong points or ran off the end.
//
// The polyhedron here deliberately does NOT use the first points of the grid:
// its global ids start at PointOffset, so local and global numbering differ
// and the distinction is observable. With ids 0..n-1 the bug is invisible.

#include <vtkCellArray.h>
#include <vtkGenericCell.h>
#include <vtkIdList.h>
#include <vtkNew.h>
#include <vtkPoints.h>
#include <vtkPolyhedron.h>
#include <vtkUnstructuredGrid.h>

#include <cstdlib>
#include <iostream>
#include <set>

namespace
{
constexpr vtkIdType PointOffset = 4;

// A unit cube as a polyhedron, offset in the grid's point list.
vtkSmartPointer<vtkUnstructuredGrid> BuildOffsetCube()
{
  vtkNew<vtkPoints> points;
  // Padding points, never referenced by the cell. They exist only so the
  // cube's global ids do not start at zero.
  for (vtkIdType i = 0; i < PointOffset; ++i)
  {
    points->InsertNextPoint(-10.0 - i, -10.0, -10.0);
  }
  const double cube[8][3] = { { 0, 0, 0 }, { 1, 0, 0 }, { 1, 1, 0 }, { 0, 1, 0 }, { 0, 0, 1 },
    { 1, 0, 1 }, { 1, 1, 1 }, { 0, 1, 1 } };
  for (const auto& p : cube)
  {
    points->InsertNextPoint(p);
  }

  // Cell point ids and the face stream are both in global id space, which is
  // what InsertNextCell expects.
  vtkIdType cellPts[8];
  for (vtkIdType i = 0; i < 8; ++i)
  {
    cellPts[i] = PointOffset + i;
  }

  const vtkIdType quads[6][4] = {
    { 0, 3, 2, 1 }, // -z
    { 4, 5, 6, 7 }, // +z
    { 0, 1, 5, 4 }, // -y
    { 1, 2, 6, 5 }, // +x
    { 2, 3, 7, 6 }, // +y
    { 3, 0, 4, 7 }  // -x
  };
  vtkNew<vtkCellArray> faces;
  for (const auto& q : quads)
  {
    vtkIdType face[4] = { PointOffset + q[0], PointOffset + q[1], PointOffset + q[2],
      PointOffset + q[3] };
    faces->InsertNextCell(4, face);
  }

  vtkNew<vtkUnstructuredGrid> grid;
  grid->SetPoints(points);
  grid->InsertNextCell(VTK_POLYHEDRON, 8, cellPts, faces);
  return grid;
}
}

int TestPolyhedronFacePoints(int, char*[])
{
  auto grid = BuildOffsetCube();

  vtkNew<vtkGenericCell> genericCell;
  grid->GetCell(0, genericCell);
  vtkPolyhedron* polyhedron = vtkPolyhedron::SafeDownCast(genericCell->GetRepresentativeCell());
  if (!polyhedron)
  {
    std::cerr << "Cell 0 is not a vtkPolyhedron." << std::endl;
    return EXIT_FAILURE;
  }

  const vtkIdType numPoints = polyhedron->GetNumberOfPoints();
  const int numFaces = polyhedron->GetNumberOfFaces();
  if (numPoints != 8 || numFaces != 6)
  {
    std::cerr << "Expected 8 points and 6 faces, got " << numPoints << " and " << numFaces << "."
              << std::endl;
    return EXIT_FAILURE;
  }

  int status = EXIT_SUCCESS;
  std::set<vtkIdType> seen;
  for (int faceId = 0; faceId < numFaces; ++faceId)
  {
    const vtkIdType* facePts = nullptr;
    const vtkIdType faceSize = polyhedron->GetFacePoints(faceId, facePts);
    if (faceSize != 4 || !facePts)
    {
      std::cerr << "Face " << faceId << ": expected 4 points, got " << faceSize << "." << std::endl;
      status = EXIT_FAILURE;
      continue;
    }

    for (vtkIdType i = 0; i < faceSize; ++i)
    {
      const vtkIdType local = facePts[i];

      // The contract: ids are local to the cell.
      if (local < 0 || local >= numPoints)
      {
        std::cerr << "Face " << faceId << " point " << i << ": id " << local << " is outside [0, "
                  << numPoints << "). This is the global-id regression." << std::endl;
        status = EXIT_FAILURE;
        continue;
      }
      seen.insert(local);

      // A local id must index the cell's own point list, and PointIds must map
      // it back to a global id belonging to this cell.
      const vtkIdType global = polyhedron->GetPointIds()->GetId(local);
      if (global < PointOffset || global >= PointOffset + numPoints)
      {
        std::cerr << "Face " << faceId << " point " << i << ": local id " << local
                  << " maps to global id " << global << ", outside the cell's points." << std::endl;
        status = EXIT_FAILURE;
      }

      // The coordinates reached through the local id must match those reached
      // through the global id. This is what a caller indexing its own per-cell
      // array actually depends on.
      double viaLocal[3], viaGlobal[3];
      polyhedron->GetPoints()->GetPoint(local, viaLocal);
      grid->GetPoints()->GetPoint(global, viaGlobal);
      for (int c = 0; c < 3; ++c)
      {
        if (viaLocal[c] != viaGlobal[c])
        {
          std::cerr << "Face " << faceId << " point " << i << ": coordinate " << c
                    << " differs between local and global lookup (" << viaLocal[c] << " vs "
                    << viaGlobal[c] << ")." << std::endl;
          status = EXIT_FAILURE;
          break;
        }
      }
    }
  }

  // Every point of a closed cube is used by some face.
  if (static_cast<vtkIdType>(seen.size()) != numPoints)
  {
    std::cerr << "Faces referenced " << seen.size() << " distinct local ids, expected " << numPoints
              << "." << std::endl;
    status = EXIT_FAILURE;
  }

  return status;
}
