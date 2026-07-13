// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Verify that vtkExtractCellsAlongPolyLine preserves the face topology of
// VTK_POLYHEDRON cells.
//
// A polyhedron's faces are not carried by its point list: they live in the
// grid's separate PolyhedronFaces / PolyhedronFaceLocations arrays. A filter
// that copies only the point list and calls the two argument
// vtkUnstructuredGrid::SetCells() will have that point list reinterpreted as a
// legacy face stream, producing invalid topology.

#include "vtkCellArray.h"
#include "vtkCellType.h"
#include "vtkExtractCellsAlongPolyLine.h"
#include "vtkIdList.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPoints.h"
#include "vtkPolyLineSource.h"
#include "vtkSmartPointer.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

#include <array>
#include <cstdlib>
#include <set>

namespace
{
// Two unit hexahedra side by side along x, expressed as VTK_POLYHEDRON.
// Points form a 3 x 2 x 2 lattice, so cell c spans x in [c, c + 1].
constexpr int NUMBER_OF_INPUT_CELLS = 2;
constexpr int FACES_PER_CELL = 6;
constexpr int POINTS_PER_FACE = 4;
constexpr int POINTS_PER_CELL = 8;

//------------------------------------------------------------------------------
vtkIdType PointId(int i, int j, int k)
{
  return i + 3 * j + 6 * k;
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkUnstructuredGrid> BuildPolyhedralGrid()
{
  vtkNew<vtkPoints> points;
  points->SetNumberOfPoints(12);
  for (int k = 0; k < 2; ++k)
  {
    for (int j = 0; j < 2; ++j)
    {
      for (int i = 0; i < 3; ++i)
      {
        points->SetPoint(PointId(i, j, k), i, j, k);
      }
    }
  }

  vtkNew<vtkCellArray> cells;         // one point list per cell
  vtkNew<vtkCellArray> faces;         // every face of every cell
  vtkNew<vtkCellArray> faceLocations; // global face ids, per cell
  vtkNew<vtkUnsignedCharArray> types;

  // Faces given against the hexahedron's local corner ordering, outward facing.
  static const int localFace[FACES_PER_CELL][POINTS_PER_FACE] = { { 0, 3, 2, 1 }, { 4, 5, 6, 7 },
    { 0, 1, 5, 4 }, { 1, 2, 6, 5 }, { 2, 3, 7, 6 }, { 3, 0, 4, 7 } };

  for (int c = 0; c < NUMBER_OF_INPUT_CELLS; ++c)
  {
    const std::array<vtkIdType, POINTS_PER_CELL> corner = { PointId(c, 0, 0), PointId(c + 1, 0, 0),
      PointId(c + 1, 1, 0), PointId(c, 1, 0), PointId(c, 0, 1), PointId(c + 1, 0, 1),
      PointId(c + 1, 1, 1), PointId(c, 1, 1) };

    cells->InsertNextCell(POINTS_PER_CELL, corner.data());
    types->InsertNextValue(VTK_POLYHEDRON);

    std::array<vtkIdType, FACES_PER_CELL> globalFaceIds;
    for (int f = 0; f < FACES_PER_CELL; ++f)
    {
      std::array<vtkIdType, POINTS_PER_FACE> facePts;
      for (int p = 0; p < POINTS_PER_FACE; ++p)
      {
        facePts[p] = corner[localFace[f][p]];
      }
      globalFaceIds[f] = faces->InsertNextCell(POINTS_PER_FACE, facePts.data());
    }
    faceLocations->InsertNextCell(FACES_PER_CELL, globalFaceIds.data());
  }

  auto grid = vtkSmartPointer<vtkUnstructuredGrid>::New();
  grid->SetPoints(points);
  grid->SetPolyhedralCells(types, cells, faceLocations, faces);
  return grid;
}

//------------------------------------------------------------------------------
bool CheckOutput(vtkUnstructuredGrid* output)
{
  if (!output)
  {
    vtkLog(ERROR, "No output produced.");
    return false;
  }

  const vtkIdType numberOfCells = output->GetNumberOfCells();
  if (numberOfCells != NUMBER_OF_INPUT_CELLS)
  {
    vtkLog(ERROR,
      "Expected " << NUMBER_OF_INPUT_CELLS << " extracted cells, got " << numberOfCells << ".");
    return false;
  }

  if (!output->GetPolyhedronFaces() || !output->GetPolyhedronFaceLocations())
  {
    vtkLog(ERROR, "Output has no polyhedron face arrays: face topology was dropped.");
    return false;
  }

  const vtkIdType numberOfPoints = output->GetNumberOfPoints();
  bool ok = true;

  for (vtkIdType cellId = 0; cellId < numberOfCells; ++cellId)
  {
    if (output->GetCellType(cellId) != VTK_POLYHEDRON)
    {
      vtkLog(ERROR, "Cell " << cellId << " is not a VTK_POLYHEDRON.");
      ok = false;
      continue;
    }

    vtkNew<vtkCellArray> cellFaces;
    output->GetPolyhedronFaces(cellId, cellFaces);

    if (cellFaces->GetNumberOfCells() != FACES_PER_CELL)
    {
      vtkLog(ERROR,
        "Cell " << cellId << " has " << cellFaces->GetNumberOfCells() << " faces, expected "
                << FACES_PER_CELL << ".");
      ok = false;
      continue;
    }

    std::set<vtkIdType> facePointIds;
    for (vtkIdType f = 0; f < cellFaces->GetNumberOfCells(); ++f)
    {
      vtkIdType numberOfFacePoints;
      const vtkIdType* facePoints;
      cellFaces->GetCellAtId(f, numberOfFacePoints, facePoints);

      if (numberOfFacePoints != POINTS_PER_FACE)
      {
        vtkLog(ERROR,
          "Cell " << cellId << " face " << f << " has " << numberOfFacePoints
                  << " points, expected " << POINTS_PER_FACE << ".");
        ok = false;
        continue;
      }

      for (vtkIdType p = 0; p < numberOfFacePoints; ++p)
      {
        const vtkIdType pointId = facePoints[p];
        if (pointId < 0 || pointId >= numberOfPoints)
        {
          vtkLog(ERROR,
            "Cell " << cellId << " face " << f << " references out of range output point id "
                    << pointId << " (output has " << numberOfPoints << " points).");
          ok = false;
          continue;
        }
        facePointIds.insert(pointId);
      }
    }

    // The points named by the faces must be exactly the cell's point list.
    vtkNew<vtkIdList> cellPointIds;
    output->GetCellPoints(cellId, cellPointIds);
    std::set<vtkIdType> connectivityPointIds;
    for (vtkIdType p = 0; p < cellPointIds->GetNumberOfIds(); ++p)
    {
      connectivityPointIds.insert(cellPointIds->GetId(p));
    }

    if (connectivityPointIds.size() != POINTS_PER_CELL)
    {
      vtkLog(ERROR,
        "Cell " << cellId << " has " << connectivityPointIds.size() << " distinct points, expected "
                << POINTS_PER_CELL << ".");
      ok = false;
    }

    if (facePointIds != connectivityPointIds)
    {
      vtkLog(ERROR,
        "Cell " << cellId << ": the points referenced by its faces do not match its point list.");
      ok = false;
    }
  }

  return ok;
}
} // anonymous namespace

//------------------------------------------------------------------------------
int TestExtractCellsAlongPolyLinePolyhedron(int, char*[])
{
  auto input = BuildPolyhedralGrid();

  // A straight line through the middle of both cells, entering and leaving the
  // grid, so both polyhedra are extracted.
  vtkNew<vtkPolyLineSource> polyLine;
  polyLine->SetNumberOfPoints(2);
  polyLine->GetPoints()->SetPoint(0, -0.5, 0.5, 0.5);
  polyLine->GetPoints()->SetPoint(1, 2.5, 0.5, 0.5);

  vtkNew<vtkExtractCellsAlongPolyLine> extractor;
  extractor->SetInputData(input);
  extractor->SetSourceConnection(polyLine->GetOutputPort());
  extractor->Update();

  vtkLog(INFO, "Testing polyhedral vtkUnstructuredGrid input...");

  if (!CheckOutput(extractor->GetOutput()))
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
