// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Exercises vtkUnstructuredGridSideMap, the half-side side map: a dense side
// numbering over a vtkUnstructuredGrid that stores no side-to-point
// connectivity of its own.
//
// The invariant that carries everything else is that VTK's canonical face
// and edge templates are outward-wound, so the two cells sharing a side
// traverse it in opposite directions. Every test here that involves a shared side ends
// up leaning on that.

#include "vtkCallbackCommand.h"
#include "vtkCell.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkCharArray.h"
#include "vtkDataObject.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkGenericCell.h"
#include "vtkHexahedron.h"
#include "vtkHigherOrderHexahedron.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkIntArray.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkQuad.h"
#include "vtkShortArray.h"
#include "vtkSideData.h"
#include "vtkSmartPointer.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridSideMap.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <map>
#include <utility>
#include <vector>

namespace
{
// The points of a side, with scratch objects of its own.
void SidePoints(vtkUnstructuredGridSideMap* sides, vtkIdType sideId, vtkIdList* ptIds)
{
  vtkNew<vtkIdList> cellPointIds;
  vtkNew<vtkGenericCell> cell;
  sides->GetSidePoints(sideId, ptIds, cellPointIds, cell);
}

int Failures = 0;

#define CHECK(condition, message)                                                                  \
  do                                                                                               \
  {                                                                                                \
    if (!(condition))                                                                              \
    {                                                                                              \
      std::cerr << "FAIL (" << __LINE__ << "): " << message << std::endl;                          \
      ++Failures;                                                                                  \
    }                                                                                              \
  } while (false)

std::vector<vtkIdType> ToVector(vtkIdList* ids)
{
  std::vector<vtkIdType> out;
  out.reserve(ids->GetNumberOfIds());
  for (vtkIdType i = 0; i < ids->GetNumberOfIds(); ++i)
  {
    out.push_back(ids->GetId(i));
  }
  return out;
}

// Whether the first k entries of b are a rotation of the first k of a, i.e.
// the same loop walked the same way round from a possibly different corner.
// Only the corners take part: a quadratic facet lists its mid-edge and centre
// nodes after them, and the two cells sharing it order those their own way.
bool IsRotationOf(const std::vector<vtkIdType>& a, const std::vector<vtkIdType>& b, int corners)
{
  const std::size_t k = static_cast<std::size_t>(corners);
  if (a.size() != b.size() || k < 2 || k > a.size())
  {
    return false;
  }
  for (std::size_t shift = 0; shift < k; ++shift)
  {
    bool match = true;
    for (std::size_t i = 0; i < k; ++i)
    {
      if (a[(i + shift) % k] != b[i])
      {
        match = false;
        break;
      }
    }
    if (match)
    {
      return true;
    }
  }
  return false;
}

// Whether b walks the same loop the other way round.
bool IsReversedRotationOf(
  const std::vector<vtkIdType>& a, const std::vector<vtkIdType>& b, int corners)
{
  const std::size_t k = static_cast<std::size_t>(corners);
  if (a.size() != b.size() || k < 2 || k > a.size())
  {
    return false;
  }
  std::vector<vtkIdType> reversed(a.begin(), a.begin() + k);
  std::reverse(reversed.begin(), reversed.end());
  reversed.insert(reversed.end(), a.begin() + k, a.end());
  return IsRotationOf(reversed, b, corners);
}

// Whether two facets are made of the same points, mid-edge nodes included.
bool SamePointSet(std::vector<vtkIdType> a, std::vector<vtkIdType> b)
{
  std::sort(a.begin(), a.end());
  std::sort(b.begin(), b.end());
  return a == b;
}

vtkSmartPointer<vtkUnstructuredGrid> MakeQuadSheet(int nx, int ny, bool flipOne);

// The points of a half-side, straight from the cell rather than through the
// map, so that the map can be checked against something independent.
std::vector<vtkIdType> HalfSidePointsFromCell(
  vtkUnstructuredGrid* grid, vtkIdType cellId, vtkIdType localSide, int* corners = nullptr)
{
  vtkCell* cell = grid->GetCell(cellId);

  // A voxel is a hexahedron and a pixel a quad, with the points numbered
  // differently, so ask the equivalent cell. That keeps this check coming from
  // VTK's own conventions rather than from the code under test, and makes the
  // equivalence itself the thing being checked.
  vtkSmartPointer<vtkCell> equivalent;
  const int cellType = grid->GetCellType(cellId);
  if (cellType == VTK_VOXEL || cellType == VTK_PIXEL)
  {
    static const int voxelToHexahedron[8] = { 0, 1, 3, 2, 4, 5, 7, 6 };
    static const int pixelToQuad[4] = { 0, 1, 3, 2 };
    const bool isVoxel = cellType == VTK_VOXEL;
    const int* order = isVoxel ? voxelToHexahedron : pixelToQuad;
    const int count = isVoxel ? 8 : 4;
    if (isVoxel)
    {
      equivalent = vtkSmartPointer<vtkHexahedron>::New();
    }
    else
    {
      equivalent = vtkSmartPointer<vtkQuad>::New();
    }
    equivalent->PointIds->SetNumberOfIds(count);
    equivalent->Points->SetNumberOfPoints(count);
    for (int i = 0; i < count; ++i)
    {
      equivalent->PointIds->SetId(i, cell->PointIds->GetId(order[i]));
      equivalent->Points->SetPoint(i, cell->Points->GetPoint(order[i]));
    }
    cell = equivalent;
  }

  // The codimension-one boundary: a 3D cell's faces, a 2D cell's edges. Asking
  // a 2D cell for a side gets nullptr, which is the whole distinction.
  vtkCell* facet = cell->GetCellDimension() == 2 ? cell->GetEdge(static_cast<int>(localSide))
                                                 : cell->GetFace(static_cast<int>(localSide));
  std::vector<vtkIdType> out;
  if (!facet)
  {
    return out;
  }
  for (vtkIdType i = 0; i < facet->GetNumberOfPoints(); ++i)
  {
    out.push_back(facet->GetPointId(i));
  }
  // The edges of a higher-order quadrilateral follow its parametric axes, so
  // that edges 2 and 3 run against its boundary; the map reverses them.
  if ((cellType == VTK_LAGRANGE_QUADRILATERAL || cellType == VTK_BEZIER_QUADRILATERAL) &&
    (localSide == 2 || localSide == 3) && out.size() >= 2)
  {
    std::swap(out[0], out[1]);
    std::reverse(out.begin() + 2, out.end());
  }
  if (corners)
  {
    // A facet's corners are the points its own edges run between; an edge has
    // two and no edges of its own to count.
    *corners = facet->GetCellDimension() == 1 ? 2 : facet->GetNumberOfEdges();
  }
  return out;
}

//------------------------------------------------------------------------------
// A nx by ny by nz block of hexahedra.
vtkSmartPointer<vtkUnstructuredGrid> MakeHexGrid(int nx, int ny, int nz)
{
  vtkNew<vtkPoints> points;
  auto pointId = [&](int i, int j, int k)
  { return static_cast<vtkIdType>(i + j * (nx + 1) + k * (nx + 1) * (ny + 1)); };

  for (int k = 0; k <= nz; ++k)
  {
    for (int j = 0; j <= ny; ++j)
    {
      for (int i = 0; i <= nx; ++i)
      {
        points->InsertNextPoint(i, j, k);
      }
    }
  }

  auto grid = vtkSmartPointer<vtkUnstructuredGrid>::New();
  grid->SetPoints(points);
  for (int k = 0; k < nz; ++k)
  {
    for (int j = 0; j < ny; ++j)
    {
      for (int i = 0; i < nx; ++i)
      {
        const vtkIdType hex[8] = { pointId(i, j, k), pointId(i + 1, j, k), pointId(i + 1, j + 1, k),
          pointId(i, j + 1, k), pointId(i, j, k + 1), pointId(i + 1, j, k + 1),
          pointId(i + 1, j + 1, k + 1), pointId(i, j + 1, k + 1) };
        grid->InsertNextCell(VTK_HEXAHEDRON, 8, hex);
      }
    }
  }
  return grid;
}

//------------------------------------------------------------------------------
// Two tetrahedra sharing the side (0,1,2).
vtkSmartPointer<vtkUnstructuredGrid> MakeTetPair()
{
  vtkNew<vtkPoints> points;
  points->InsertNextPoint(0, 0, 0);
  points->InsertNextPoint(1, 0, 0);
  points->InsertNextPoint(0, 1, 0);
  points->InsertNextPoint(0, 0, 1);
  points->InsertNextPoint(0, 0, -1);

  auto grid = vtkSmartPointer<vtkUnstructuredGrid>::New();
  grid->SetPoints(points);
  const vtkIdType above[4] = { 0, 1, 2, 3 };
  const vtkIdType below[4] = { 0, 2, 1, 4 };
  grid->InsertNextCell(VTK_TETRA, 4, above);
  grid->InsertNextCell(VTK_TETRA, 4, below);
  return grid;
}

//------------------------------------------------------------------------------
// A unit cube as a hexahedron, with the cube above it as a polyhedron, so the
// two share a quad side across the fixed/polyhedron divide.
vtkSmartPointer<vtkUnstructuredGrid> MakeHexPolyhedronPair()
{
  vtkNew<vtkPoints> points;
  for (int k = 0; k <= 2; ++k)
  {
    for (int j = 0; j <= 1; ++j)
    {
      for (int i = 0; i <= 1; ++i)
      {
        points->InsertNextPoint(i, j, k);
      }
    }
  }
  auto pointId = [](int i, int j, int k) { return static_cast<vtkIdType>(i + 2 * j + 4 * k); };

  auto grid = vtkSmartPointer<vtkUnstructuredGrid>::New();
  grid->SetPoints(points);

  const vtkIdType hex[8] = { pointId(0, 0, 0), pointId(1, 0, 0), pointId(1, 1, 0), pointId(0, 1, 0),
    pointId(0, 0, 1), pointId(1, 0, 1), pointId(1, 1, 1), pointId(0, 1, 1) };
  grid->InsertNextCell(VTK_HEXAHEDRON, 8, hex);

  const vtkIdType poly[8] = { pointId(0, 0, 1), pointId(1, 0, 1), pointId(1, 1, 1),
    pointId(0, 1, 1), pointId(0, 0, 2), pointId(1, 0, 2), pointId(1, 1, 2), pointId(0, 1, 2) };
  // Outward-wound faces of the upper cube, as a face stream.
  const vtkIdType faceStream[] = {
    4, poly[0], poly[3], poly[2], poly[1], // bottom, shared with the hexahedron
    4, poly[4], poly[5], poly[6], poly[7], // top
    4, poly[0], poly[1], poly[5], poly[4], //
    4, poly[1], poly[2], poly[6], poly[5], //
    4, poly[2], poly[3], poly[7], poly[6], //
    4, poly[3], poly[0], poly[4], poly[7], //
  };
  grid->InsertNextCell(VTK_POLYHEDRON, 8, poly, 6, faceStream);
  return grid;
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkUnstructuredGridSideMap> MapOf(vtkUnstructuredGrid* grid)
{
  auto sides = vtkSmartPointer<vtkUnstructuredGridSideMap>::New();
  sides->SetDataSet(grid);
  return sides;
}

//------------------------------------------------------------------------------
// The checks that should hold for any mesh: half-side addressing is
// arithmetic, every side is reachable from its cells and back, and the cells
// sharing a side disagree about its winding by exactly a reversal.
void CheckInvariants(vtkUnstructuredGrid* grid, vtkUnstructuredGridSideMap* sides, const char* what)
{
  std::cerr << "--- " << what << ": " << grid->GetNumberOfCells() << " cells, "
            << sides->GetNumberOfSides() << " sides, " << sides->GetNumberOfHalfSides()
            << " half-sides" << std::endl;

  vtkNew<vtkIdList> sidePoints;
  vtkNew<vtkIdList> sideCells;
  vtkNew<vtkIdList> cellSides;

  vtkIdType seenHalfSides = 0;
  for (vtkIdType cellId = 0; cellId < grid->GetNumberOfCells(); ++cellId)
  {
    sides->GetCellSides(cellId, cellSides);
    const vtkIdType count = sides->GetNumberOfCellSides(cellId);
    CHECK(cellSides->GetNumberOfIds() == count, "GetCellSides disagrees with GetNumberOfCellSides");
    seenHalfSides += count;

    for (vtkIdType localSide = 0; localSide < count; ++localSide)
    {
      const vtkIdType sideId = sides->GetCellSide(cellId, localSide);
      CHECK(sideId >= 0 && sideId < sides->GetNumberOfSides(), "side id out of range");
      CHECK(cellSides->GetId(localSide) == sideId, "GetCellSides disagrees with GetCellSide");

      // Half-side addressing is arithmetic off the cell's offset.
      const vtkIdType halfSide = sides->GetHalfSideId(cellId, localSide);
      CHECK(halfSide ==
          static_cast<vtkIdType>(sides->GetCellSideOffsets()->GetTuple1(cellId)) + localSide,
        "half-side id is not the cell's offset plus the local side");
      CHECK(static_cast<vtkIdType>(sides->GetHalfSideToSide()->GetTuple1(halfSide)) == sideId,
        "HalfSideToSide disagrees with GetCellSide");

      // This cell is one of the side's cells.
      sides->GetSideCells(sideId, sideCells);
      CHECK(sideCells->IsId(cellId) >= 0, "side does not list a cell that claims it");

      // The map's idea of the side's points, against the cell's own.
      SidePoints(sides, sideId, sidePoints);
      const std::vector<vtkIdType> fromMap = ToVector(sidePoints);
      int corners = 0;
      const std::vector<vtkIdType> fromCell =
        HalfSidePointsFromCell(grid, cellId, localSide, &corners);

      // Every point of the facet takes part in its identity, mid-edge nodes
      // included; only the corners take part in its winding.
      CHECK(SamePointSet(fromMap, fromCell), "the map and the cell disagree about a side's points");

      const int orientation = sides->GetSideOrientation(cellId, localSide);
      if (orientation > 0)
      {
        CHECK(IsRotationOf(fromMap, fromCell, corners),
          "orientation says +1 but the cell walks the side the other way");
      }
      else
      {
        CHECK(IsReversedRotationOf(fromMap, fromCell, corners),
          "orientation says -1 but the cell walks the side the same way");
      }
    }
  }
  CHECK(seenHalfSides == sides->GetNumberOfHalfSides(), "half-sides do not add up");

  // Every side is claimed, and an interior side is claimed by exactly two
  // cells which disagree about its winding.
  vtkIdType interior = 0;
  vtkIdType boundary = 0;
  for (vtkIdType sideId = 0; sideId < sides->GetNumberOfSides(); ++sideId)
  {
    sides->GetSideCells(sideId, sideCells);
    CHECK(sideCells->GetNumberOfIds() >= 1, "a side with no cells");
    if (sideCells->GetNumberOfIds() == 1)
    {
      ++boundary;
    }
    else if (sideCells->GetNumberOfIds() == 2)
    {
      ++interior;
      int firstCorners = 0;
      const std::vector<vtkIdType> first = HalfSidePointsFromCell(grid, sideCells->GetId(0),
        static_cast<vtkIdType>(sides->GetSideLocalSidesArray()->GetTuple1(
          static_cast<vtkIdType>(sides->GetSideOffsets()->GetTuple1(sideId)))),
        &firstCorners);
      const std::vector<vtkIdType> second = HalfSidePointsFromCell(grid, sideCells->GetId(1),
        static_cast<vtkIdType>(sides->GetSideLocalSidesArray()->GetTuple1(
          static_cast<vtkIdType>(sides->GetSideOffsets()->GetTuple1(sideId)) + 1)));
      CHECK(IsReversedRotationOf(first, second, firstCorners),
        "the two cells of an interior side agree about its winding, which they should not");
    }
  }
  std::cerr << "    " << interior << " interior, " << boundary << " boundary" << std::endl;

  // Exactly one representative per side, so exactly one +1 per side.
  vtkIdType owned = 0;
  for (vtkIdType cellId = 0; cellId < grid->GetNumberOfCells(); ++cellId)
  {
    for (vtkIdType localSide = 0; localSide < sides->GetNumberOfCellSides(cellId); ++localSide)
    {
      if (sides->GetSideOrientation(cellId, localSide) > 0)
      {
        ++owned;
      }
    }
  }
  CHECK(owned == sides->GetNumberOfSides(), "the number of owned half-sides is not the side count");
}

//------------------------------------------------------------------------------
void TestHexBlock()
{
  const int nx = 3, ny = 2, nz = 2;
  auto grid = MakeHexGrid(nx, ny, nz);

  vtkNew<vtkUnstructuredGridSideMap> sides;
  sides->SetDataSet(grid);

  const vtkIdType expected = static_cast<vtkIdType>((nx + 1) * ny * nz) +
    static_cast<vtkIdType>(nx * (ny + 1) * nz) + static_cast<vtkIdType>(nx * ny * (nz + 1));
  CHECK(sides->GetNumberOfSides() == expected,
    "hex block side count is " << sides->GetNumberOfSides() << ", expected " << expected);
  CHECK(sides->GetNumberOfHalfSides() == 6 * nx * ny * nz, "hex block half-side count");

  CheckInvariants(grid, sides, "hex block");

  // Neighbours across sides: an interior cell of a 3x2x2 block has no
  // six-neighbour cell, so just check the count is the number of interior
  // sides of that cell.
  vtkNew<vtkIdList> neighbors;
  sides->GetCellNeighbors(0, neighbors);
  CHECK(neighbors->GetNumberOfIds() == 3, "corner hexahedron should have three side neighbours");
}

//------------------------------------------------------------------------------
void TestTetPair()
{
  auto grid = MakeTetPair();

  vtkNew<vtkUnstructuredGridSideMap> sides;
  sides->SetDataSet(grid);

  CHECK(sides->GetNumberOfSides() == 7,
    "two tets sharing a side have seven sides, not " << sides->GetNumberOfSides());
  CHECK(sides->GetNumberOfHalfSides() == 8, "two tets have eight half-sides");

  CheckInvariants(grid, sides, "tet pair");

  // The shared side is the one with two cells.
  vtkIdType shared = -1;
  vtkNew<vtkIdList> cellIds;
  for (vtkIdType sideId = 0; sideId < sides->GetNumberOfSides(); ++sideId)
  {
    sides->GetSideCells(sideId, cellIds);
    if (cellIds->GetNumberOfIds() == 2)
    {
      CHECK(shared < 0, "more than one shared side between two tets");
      shared = sideId;
    }
  }
  CHECK(shared >= 0, "no shared side found between two tets");

  vtkNew<vtkIdList> ptIds;
  SidePoints(sides, shared, ptIds);
  std::vector<vtkIdType> sorted = ToVector(ptIds);
  std::sort(sorted.begin(), sorted.end());
  const std::vector<vtkIdType> want = { 0, 1, 2 };
  CHECK(sorted == want, "the shared side is not (0,1,2)");
}

//------------------------------------------------------------------------------
void TestHexPolyhedronPair()
{
  auto grid = MakeHexPolyhedronPair();

  vtkNew<vtkUnstructuredGridSideMap> sides;
  sides->SetDataSet(grid);

  CHECK(sides->GetNumberOfSides() == 11,
    "a hexahedron and a polyhedron sharing a side have eleven sides, not "
      << sides->GetNumberOfSides());
  CheckInvariants(grid, sides, "hexahedron and polyhedron");

  vtkNew<vtkIdList> neighbors;
  sides->GetCellNeighbors(0, neighbors);
  CHECK(neighbors->GetNumberOfIds() == 1 && neighbors->GetId(0) == 1,
    "the hexahedron should have the polyhedron as its only neighbour");
}

//------------------------------------------------------------------------------
// A supplied numbering is honoured as given: no matching pass runs, and the
// side ids come back exactly as handed over. This is the path a reader takes
// when it wants the file's own side numbering.
void TestAuthoredNumbering()
{
  auto grid = MakeTetPair();

  vtkNew<vtkUnstructuredGridSideMap> derived;
  derived->SetDataSet(grid);
  const vtkIdType numberOfSides = derived->GetNumberOfSides();
  vtkDataArray* derivedIds = derived->GetHalfSideToSide();

  // Hand back the derived numbering with the side ids reversed, which is a
  // different numbering of the same sides.
  vtkNew<vtkIdTypeArray> authoredIds;
  authoredIds->SetNumberOfTuples(derivedIds->GetNumberOfTuples());
  for (vtkIdType i = 0; i < derivedIds->GetNumberOfTuples(); ++i)
  {
    authoredIds->SetValue(i, numberOfSides - 1 - static_cast<vtkIdType>(derivedIds->GetTuple1(i)));
  }

  vtkNew<vtkUnstructuredGridSideMap> authored;
  authored->SetDataSet(grid);
  authored->SetSideIds(authoredIds, numberOfSides);

  CHECK(authored->GetAuthored(), "SetSideIds should mark the numbering as authored");
  CHECK(authored->GetNumberOfSides() == numberOfSides, "authored side count");

  for (vtkIdType i = 0; i < derivedIds->GetNumberOfTuples(); ++i)
  {
    CHECK(static_cast<vtkIdType>(authored->GetHalfSideToSide()->GetTuple1(i)) ==
        numberOfSides - 1 - static_cast<vtkIdType>(derivedIds->GetTuple1(i)),
      "the supplied numbering was not kept");
  }

  CheckInvariants(grid, authored, "authored numbering");
}

//------------------------------------------------------------------------------
// A reader may supply the side ids in an array of any type.
template <typename ArrayT>
void CheckSideIdsOfType(const char* name)
{
  auto grid = MakeTetPair();
  vtkNew<vtkUnstructuredGridSideMap> derived;
  derived->SetDataSet(grid);
  const vtkIdType numberOfSides = derived->GetNumberOfSides();
  const vtkIdType numberOfHalfSides = derived->GetNumberOfHalfSides();

  vtkNew<ArrayT> ids;
  ids->SetNumberOfTuples(numberOfHalfSides);
  for (vtkIdType i = 0; i < numberOfHalfSides; ++i)
  {
    ids->SetTuple1(i, numberOfSides - 1 - derived->GetHalfSideToSide()->GetTuple1(i));
  }
  vtkNew<vtkUnstructuredGridSideMap> authored;
  authored->SetDataSet(grid);
  authored->SetSideIds(ids, numberOfSides);

  vtkIdType mismatches = 0;
  for (vtkIdType cellId = 0; cellId < grid->GetNumberOfCells(); ++cellId)
  {
    for (vtkIdType localSide = 0; localSide < authored->GetNumberOfCellSides(cellId); ++localSide)
    {
      mismatches += authored->GetCellSide(cellId, localSide) ==
          numberOfSides - 1 - derived->GetCellSide(cellId, localSide)
        ? 0
        : 1;
    }
  }
  CHECK(mismatches == 0, name << ": " << mismatches << " supplied side ids read back wrong");
}

void TestSideIdArrayTypes()
{
  CheckSideIdsOfType<vtkIdTypeArray>("vtkIdTypeArray");
  CheckSideIdsOfType<vtkIntArray>("vtkIntArray");
  CheckSideIdsOfType<vtkShortArray>("vtkShortArray");
  CheckSideIdsOfType<vtkCharArray>("vtkCharArray");
  CheckSideIdsOfType<vtkFloatArray>("vtkFloatArray");
  CheckSideIdsOfType<vtkDoubleArray>("vtkDoubleArray");
}

//------------------------------------------------------------------------------
// Nothing is allocated until something is asked for, and side data pins the
// numbering once it exists.
void TestLaziness()
{
  auto grid = MakeHexGrid(2, 2, 2);

  vtkNew<vtkUnstructuredGridSideMap> sides;
  sides->SetDataSet(grid);
  CHECK(!sides->IsMapBuilt(), "the structure should not be built until it is asked for");
  CHECK(sides->GetActualMemorySize() == 0, "an unbuilt structure should hold nothing");

  const vtkIdType sideCount = sides->GetNumberOfSides();
  CHECK(sides->IsMapBuilt(), "asking for the side count should have built the structure");
  CHECK(sides->GetActualMemorySize() > 0, "a built structure should hold something");

  // A derived numbering with no data on it is a cache and may be dropped.
  sides->Initialize();
  CHECK(!sides->IsMapBuilt(), "Initialize should have dropped the structure");
  CHECK(sides->GetNumberOfSides() == sideCount, "rebuilding should give the same side count");
}

//------------------------------------------------------------------------------
// What the structure costs, next to what storing side-to-point connectivity
// would have cost. Reported rather than asserted.
void ReportMemory()
{
  auto grid = MakeHexGrid(20, 20, 20);
  vtkNew<vtkUnstructuredGridSideMap> sides;
  sides->SetDataSet(grid);

  const vtkIdType numberOfSides = sides->GetNumberOfSides();
  const vtkIdType numberOfHalfSides = sides->GetNumberOfHalfSides();

  // What it would cost to store the sides outright instead: cell to side ids,
  // plus side to point ids, each with offsets.
  const double explicitIds = static_cast<double>(numberOfHalfSides) + // cell -> side
    static_cast<double>(grid->GetNumberOfCells() + 1) +               //   offsets
    static_cast<double>(numberOfSides) * 4.0 +                        // side -> point
    static_cast<double>(numberOfSides + 1);                           //   offsets
  const double explicitBytes = explicitIds * sizeof(vtkIdType) / 1024.0;

  std::cerr << "--- memory, 20x20x20 hexahedra" << std::endl;
  std::cerr << "    cells " << grid->GetNumberOfCells() << ", sides " << numberOfSides
            << ", half-sides " << numberOfHalfSides << std::endl;
  std::cerr << "    half-side map: " << sides->GetActualMemorySize() << " KiB" << std::endl;
  std::cerr << "    stored side connectivity would be about " << static_cast<int>(explicitBytes)
            << " KiB" << std::endl;
  std::cerr << "    cell-to-point connectivity, which the map leaves alone: "
            << grid->GetCells()->GetActualMemorySize() << " KiB" << std::endl;
}

//------------------------------------------------------------------------------
// The same block again as voxels, whose points run along scanlines rather than
// round the boundary. vtkThreshold keeps its input's cell type, so this is
// what an unstructured grid made from image data actually looks like.
vtkSmartPointer<vtkUnstructuredGrid> MakeVoxelGrid(int nx, int ny, int nz)
{
  vtkNew<vtkPoints> points;
  auto pointId = [&](int i, int j, int k)
  { return static_cast<vtkIdType>(i + j * (nx + 1) + k * (nx + 1) * (ny + 1)); };

  for (int k = 0; k <= nz; ++k)
  {
    for (int j = 0; j <= ny; ++j)
    {
      for (int i = 0; i <= nx; ++i)
      {
        points->InsertNextPoint(i, j, k);
      }
    }
  }

  auto grid = vtkSmartPointer<vtkUnstructuredGrid>::New();
  grid->SetPoints(points);
  for (int k = 0; k < nz; ++k)
  {
    for (int j = 0; j < ny; ++j)
    {
      for (int i = 0; i < nx; ++i)
      {
        const vtkIdType voxel[8] = { pointId(i, j, k), pointId(i + 1, j, k), pointId(i, j + 1, k),
          pointId(i + 1, j + 1, k), pointId(i, j, k + 1), pointId(i + 1, j, k + 1),
          pointId(i, j + 1, k + 1), pointId(i + 1, j + 1, k + 1) };
        grid->InsertNextCell(VTK_VOXEL, 8, voxel);
      }
    }
  }
  return grid;
}

//------------------------------------------------------------------------------
// A sheet of pixels, the 2D counterpart.
vtkSmartPointer<vtkUnstructuredGrid> MakePixelSheet(int nx, int ny)
{
  vtkNew<vtkPoints> points;
  for (int j = 0; j <= ny; ++j)
  {
    for (int i = 0; i <= nx; ++i)
    {
      points->InsertNextPoint(i, j, 0);
    }
  }
  auto pointId = [&](int i, int j) { return static_cast<vtkIdType>(i + j * (nx + 1)); };

  auto grid = vtkSmartPointer<vtkUnstructuredGrid>::New();
  grid->SetPoints(points);
  for (int j = 0; j < ny; ++j)
  {
    for (int i = 0; i < nx; ++i)
    {
      const vtkIdType pixel[4] = { pointId(i, j), pointId(i + 1, j), pointId(i, j + 1),
        pointId(i + 1, j + 1) };
      grid->InsertNextCell(VTK_PIXEL, 4, pixel);
    }
  }
  return grid;
}

//------------------------------------------------------------------------------
// A hexahedron with a voxel stacked on it, sharing a quad side. The two number
// their points differently, so this is what says the reordering really does
// give the same side, seen opposite ways.
vtkSmartPointer<vtkUnstructuredGrid> MakeHexVoxelPair()
{
  vtkNew<vtkPoints> points;
  for (int k = 0; k <= 2; ++k)
  {
    for (int j = 0; j <= 1; ++j)
    {
      for (int i = 0; i <= 1; ++i)
      {
        points->InsertNextPoint(i, j, k);
      }
    }
  }
  auto pointId = [](int i, int j, int k) { return static_cast<vtkIdType>(i + 2 * j + 4 * k); };

  auto grid = vtkSmartPointer<vtkUnstructuredGrid>::New();
  grid->SetPoints(points);

  const vtkIdType hex[8] = { pointId(0, 0, 0), pointId(1, 0, 0), pointId(1, 1, 0), pointId(0, 1, 0),
    pointId(0, 0, 1), pointId(1, 0, 1), pointId(1, 1, 1), pointId(0, 1, 1) };
  grid->InsertNextCell(VTK_HEXAHEDRON, 8, hex);

  const vtkIdType voxel[8] = { pointId(0, 0, 1), pointId(1, 0, 1), pointId(0, 1, 1),
    pointId(1, 1, 1), pointId(0, 0, 2), pointId(1, 0, 2), pointId(0, 1, 2), pointId(1, 1, 2) };
  grid->InsertNextCell(VTK_VOXEL, 8, voxel);
  return grid;
}

//------------------------------------------------------------------------------
// A voxel block should behave exactly as the hexahedral block does: same
// sides, same counts, same invariants.
void TestVoxelBlock()
{
  const int nx = 3, ny = 2, nz = 2;
  auto grid = MakeVoxelGrid(nx, ny, nz);

  vtkNew<vtkUnstructuredGridSideMap> sides;
  sides->SetDataSet(grid);

  const vtkIdType expected = static_cast<vtkIdType>((nx + 1) * ny * nz) +
    static_cast<vtkIdType>(nx * (ny + 1) * nz) + static_cast<vtkIdType>(nx * ny * (nz + 1));
  CHECK(sides->GetNumberOfSides() == expected,
    "voxel block side count is " << sides->GetNumberOfSides() << ", expected " << expected);
  CHECK(sides->GetNumberOfHalfSides() == 6 * nx * ny * nz, "voxel block half-side count");

  CheckInvariants(grid, sides, "voxel block");
  CHECK(sides->CheckWindingConsistency() == 0, "a voxel block is consistently wound");

  vtkNew<vtkIdList> neighbors;
  sides->GetCellNeighbors(0, neighbors);
  CHECK(neighbors->GetNumberOfIds() == 3, "corner voxel should have three side neighbours");
}

//------------------------------------------------------------------------------
void TestPixelSheet()
{
  const int nx = 4, ny = 3;
  auto grid = MakePixelSheet(nx, ny);

  vtkNew<vtkUnstructuredGridSideMap> sides;
  sides->SetDataSet(grid);

  const vtkIdType expected = static_cast<vtkIdType>((nx + 1) * ny + nx * (ny + 1));
  CHECK(sides->GetNumberOfSides() == expected,
    "pixel sheet edge count is " << sides->GetNumberOfSides() << ", expected " << expected);

  CheckInvariants(grid, sides, "pixel sheet");
  CHECK(sides->CheckWindingConsistency() == 0, "a pixel sheet is consistently wound");

  vtkNew<vtkIdList> ptIds;
  for (vtkIdType sideId = 0; sideId < sides->GetNumberOfSides(); ++sideId)
  {
    SidePoints(sides, sideId, ptIds);
    CHECK(ptIds->GetNumberOfIds() == 2, "a pixel's side should be an edge");
  }
}

//------------------------------------------------------------------------------
void TestHexVoxelPair()
{
  auto grid = MakeHexVoxelPair();

  vtkNew<vtkUnstructuredGridSideMap> sides;
  sides->SetDataSet(grid);

  CHECK(sides->GetNumberOfSides() == 11,
    "a hexahedron and a voxel sharing a side have eleven sides, not " << sides->GetNumberOfSides());
  CheckInvariants(grid, sides, "hexahedron and voxel");

  // The shared side is the whole point: the two cells number their points
  // differently and still have to agree which side it is, and disagree about
  // its winding.
  CHECK(sides->CheckWindingConsistency() == 0,
    "the hexahedron and the voxel should see their shared side opposite ways");

  vtkNew<vtkIdList> neighbors;
  sides->GetCellNeighbors(0, neighbors);
  CHECK(neighbors->GetNumberOfIds() == 1 && neighbors->GetId(0) == 1,
    "the hexahedron should have the voxel as its only neighbour");
}

//------------------------------------------------------------------------------
// Two quadratic tetrahedra sharing a side, mid-edge nodes and all. The shared
// side has six points, not three, and both cells have to name the same six.
vtkSmartPointer<vtkUnstructuredGrid> MakeQuadraticTetPair()
{
  vtkNew<vtkPoints> points;
  points->InsertNextPoint(0, 0, 0);      // 0  corner
  points->InsertNextPoint(1, 0, 0);      // 1  corner
  points->InsertNextPoint(0, 1, 0);      // 2  corner
  points->InsertNextPoint(0, 0, 1);      // 3  corner, above
  points->InsertNextPoint(0.5, 0, 0);    // 4  mid 0-1
  points->InsertNextPoint(0.5, 0.5, 0);  // 5  mid 1-2
  points->InsertNextPoint(0, 0.5, 0);    // 6  mid 2-0
  points->InsertNextPoint(0, 0, 0.5);    // 7  mid 0-3
  points->InsertNextPoint(0.5, 0, 0.5);  // 8  mid 1-3
  points->InsertNextPoint(0, 0.5, 0.5);  // 9  mid 2-3
  points->InsertNextPoint(0, 0, -1);     // 10 corner, below
  points->InsertNextPoint(0, 0, -0.5);   // 11 mid 0-10
  points->InsertNextPoint(0, 0.5, -0.5); // 12 mid 2-10
  points->InsertNextPoint(0.5, 0, -0.5); // 13 mid 1-10

  auto grid = vtkSmartPointer<vtkUnstructuredGrid>::New();
  grid->SetPoints(points);

  // vtkQuadraticTetra orders its points as four corners then the mid-edge
  // nodes of (0,1) (1,2) (2,0) (0,3) (1,3) (2,3).
  const vtkIdType above[10] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
  // The lower one takes the base the other way round, so the two share the
  // whole of side (0,1,2) including its mid-edge nodes 4, 5 and 6.
  const vtkIdType below[10] = { 0, 2, 1, 10, 6, 5, 4, 11, 12, 13 };
  grid->InsertNextCell(VTK_QUADRATIC_TETRA, 10, above);
  grid->InsertNextCell(VTK_QUADRATIC_TETRA, 10, below);
  return grid;
}

//------------------------------------------------------------------------------
void TestQuadraticTetPair()
{
  auto grid = MakeQuadraticTetPair();

  vtkNew<vtkUnstructuredGridSideMap> sides;
  sides->SetDataSet(grid);

  CHECK(sides->GetNumberOfSides() == 7,
    "two quadratic tetrahedra sharing a side have seven sides, not " << sides->GetNumberOfSides());
  CheckInvariants(grid, sides, "quadratic tetrahedra");
  CHECK(sides->CheckWindingConsistency() == 0,
    "the two quadratic tetrahedra should see their shared side opposite ways");

  // The shared side carries its mid-edge nodes, so it has six points and not
  // the three its corners would give.
  vtkIdType shared = -1;
  vtkNew<vtkIdList> cellIds;
  for (vtkIdType sideId = 0; sideId < sides->GetNumberOfSides(); ++sideId)
  {
    sides->GetSideCells(sideId, cellIds);
    if (cellIds->GetNumberOfIds() == 2)
    {
      shared = sideId;
    }
  }
  CHECK(shared >= 0, "no shared side between the quadratic tetrahedra");

  vtkNew<vtkIdList> ptIds;
  SidePoints(sides, shared, ptIds);
  CHECK(ptIds->GetNumberOfIds() == 6,
    "a quadratic triangle side should have six points, not " << ptIds->GetNumberOfIds());
  std::vector<vtkIdType> sorted = ToVector(ptIds);
  std::sort(sorted.begin(), sorted.end());
  const std::vector<vtkIdType> want = { 0, 1, 2, 4, 5, 6 };
  CHECK(sorted == want, "the shared quadratic side is not (0,1,2) with its mid-edge nodes");
}

//------------------------------------------------------------------------------
// Every 3D type this supports has to wind its sides outward, or the sign from
// GetSideOrientation() means nothing. Rather than read the tables, build one
// cell of each on its own reference element and check the geometry: the corner
// loop of every side must turn away from the middle of the cell.
//
// Corners come first in a facet's point list, so the first three points are
// three consecutive corners whatever the type.
void CheckSidesTurnOutward(int cellType, const char* name)
{
  vtkNew<vtkGenericCell> prototype;
  prototype->SetCellType(cellType);
  const double* parametric = prototype->GetParametricCoords();
  if (!parametric)
  {
    CHECK(false, name << " has no parametric coordinates to build from");
    return;
  }
  const vtkIdType numberOfPoints = prototype->GetNumberOfPoints();

  vtkNew<vtkPoints> points;
  vtkNew<vtkIdList> ids;
  double centre[3] = { 0.0, 0.0, 0.0 };
  for (vtkIdType i = 0; i < numberOfPoints; ++i)
  {
    points->InsertNextPoint(parametric + 3 * i);
    ids->InsertNextId(i);
    for (int c = 0; c < 3; ++c)
    {
      centre[c] += parametric[3 * i + c] / static_cast<double>(numberOfPoints);
    }
  }

  vtkNew<vtkUnstructuredGrid> grid;
  grid->SetPoints(points);
  grid->InsertNextCell(cellType, ids);

  vtkNew<vtkUnstructuredGridSideMap> sides;
  sides->SetDataSet(grid);
  CHECK(sides->GetNumberOfSides() == sides->GetNumberOfHalfSides(),
    name << ": a lone cell should have every side to itself");
  CHECK(sides->GetNumberOfSides() > 0, name << " contributed no sides");

  vtkNew<vtkIdList> sidePoints;
  for (vtkIdType sideId = 0; sideId < sides->GetNumberOfSides(); ++sideId)
  {
    SidePoints(sides, sideId, sidePoints);
    if (sidePoints->GetNumberOfIds() < 3)
    {
      CHECK(false, name << " side " << sideId << " has fewer than three points");
      continue;
    }
    double p0[3], p1[3], p2[3];
    points->GetPoint(sidePoints->GetId(0), p0);
    points->GetPoint(sidePoints->GetId(1), p1);
    points->GetPoint(sidePoints->GetId(2), p2);

    double u[3], v[3], normal[3];
    for (int c = 0; c < 3; ++c)
    {
      u[c] = p1[c] - p0[c];
      v[c] = p2[c] - p0[c];
    }
    vtkMath::Cross(u, v, normal);

    double outward[3];
    for (int c = 0; c < 3; ++c)
    {
      outward[c] = p0[c] - centre[c];
    }
    CHECK(vtkMath::Dot(normal, outward) > 0.0, name << " side " << sideId << " is wound inward");
  }
}

void TestSidesTurnOutward()
{
  CheckSidesTurnOutward(VTK_TETRA, "tetrahedron");
  CheckSidesTurnOutward(VTK_PYRAMID, "pyramid");
  CheckSidesTurnOutward(VTK_WEDGE, "wedge");
  CheckSidesTurnOutward(VTK_HEXAHEDRON, "hexahedron");
  CheckSidesTurnOutward(VTK_VOXEL, "voxel");
  CheckSidesTurnOutward(VTK_PENTAGONAL_PRISM, "pentagonal prism");
  CheckSidesTurnOutward(VTK_HEXAGONAL_PRISM, "hexagonal prism");
  CheckSidesTurnOutward(VTK_QUADRATIC_TETRA, "quadratic tetrahedron");
  CheckSidesTurnOutward(VTK_QUADRATIC_PYRAMID, "quadratic pyramid");
  CheckSidesTurnOutward(VTK_QUADRATIC_WEDGE, "quadratic wedge");
  CheckSidesTurnOutward(VTK_QUADRATIC_HEXAHEDRON, "quadratic hexahedron");
  CheckSidesTurnOutward(VTK_TRIQUADRATIC_HEXAHEDRON, "triquadratic hexahedron");
  CheckSidesTurnOutward(VTK_TRIQUADRATIC_PYRAMID, "triquadratic pyramid");
  CheckSidesTurnOutward(VTK_QUADRATIC_LINEAR_WEDGE, "quadratic linear wedge");
  CheckSidesTurnOutward(VTK_BIQUADRATIC_QUADRATIC_WEDGE, "biquadratic quadratic wedge");
  CheckSidesTurnOutward(VTK_BIQUADRATIC_QUADRATIC_HEXAHEDRON, "biquadratic quadratic hexahedron");
  std::cerr << "--- every supported 3D type winds its sides outward" << std::endl;
}

//------------------------------------------------------------------------------
// A block of Lagrange hexahedra of the given order, sharing their sides. The
// points of a Lagrange hexahedron run corners, then edge interiors, then face
// interiors, then the volume, and every one of them on a shared side has to be
// shared for the two cells to agree about that side.
vtkSmartPointer<vtkUnstructuredGrid> MakeHigherOrderHexPair(int cellType, int order)
{
  // One lattice spanning two cells stacked in z, so the shared side's nodes
  // are the same points for both.
  const int nx = order, ny = order, nz = 2 * order;
  auto latticeId = [&](int i, int j, int k)
  { return static_cast<vtkIdType>(i + j * (nx + 1) + k * (nx + 1) * (ny + 1)); };

  vtkNew<vtkPoints> points;
  for (int k = 0; k <= nz; ++k)
  {
    for (int j = 0; j <= ny; ++j)
    {
      for (int i = 0; i <= nx; ++i)
      {
        points->InsertNextPoint(i / static_cast<double>(order), j / static_cast<double>(order),
          k / static_cast<double>(order));
      }
    }
  }

  auto grid = vtkSmartPointer<vtkUnstructuredGrid>::New();
  grid->SetPoints(points);

  // vtkHigherOrderHexahedron numbers its points by a fixed rule from the
  // (i,j,k) position within the cell, and hands that rule out, so the cell can
  // be filled without transcribing an ordering.
  const int orders[3] = { order, order, order };
  for (int cell = 0; cell < 2; ++cell)
  {
    const int base = cell * order;
    vtkNew<vtkIdList> ids;
    ids->SetNumberOfIds((order + 1) * (order + 1) * (order + 1));
    for (int k = 0; k <= order; ++k)
    {
      for (int j = 0; j <= order; ++j)
      {
        for (int i = 0; i <= order; ++i)
        {
          const int node = vtkHigherOrderHexahedron::PointIndexFromIJK(i, j, k, orders);
          ids->SetId(node, latticeId(i, j, base + k));
        }
      }
    }
    grid->InsertNextCell(cellType, ids);
  }
  return grid;
}

//------------------------------------------------------------------------------
void CheckHigherOrderHexPair(int cellType, int order, const char* name)
{
  auto grid = MakeHigherOrderHexPair(cellType, order);

  vtkNew<vtkUnstructuredGridSideMap> sides;
  sides->SetDataSet(grid);

  CHECK(sides->GetNumberOfSides() == 11,
    name << " order " << order << " hexahedra sharing a side should give eleven sides, not "
         << sides->GetNumberOfSides());
  CHECK(sides->GetNumberOfHalfSides() == 12, "two hexahedra have twelve half-sides");

  CheckInvariants(grid, sides, name);
  CHECK(sides->CheckWindingConsistency() == 0,
    name << " order " << order << " hexahedra should see their shared side opposite ways");

  // The shared side carries every node on it, not just the corners: an order n
  // quadrilateral side has (n+1)^2 of them.
  vtkIdType shared = -1;
  vtkNew<vtkIdList> cellIds;
  for (vtkIdType sideId = 0; sideId < sides->GetNumberOfSides(); ++sideId)
  {
    sides->GetSideCells(sideId, cellIds);
    if (cellIds->GetNumberOfIds() == 2)
    {
      shared = sideId;
    }
  }
  CHECK(shared >= 0, "no shared side between the higher-order hexahedra");

  vtkNew<vtkIdList> ptIds;
  SidePoints(sides, shared, ptIds);
  const vtkIdType expected = (order + 1) * (order + 1);
  CHECK(ptIds->GetNumberOfIds() == expected,
    "an order " << order << " side should have " << expected << " points, not "
                << ptIds->GetNumberOfIds());
}

void TestHigherOrderHexahedra()
{
  // Lagrange and Bezier cells of the same shape lay their points out the same
  // way and differ only in how they interpolate, so both go through the same
  // template.
  CheckHigherOrderHexPair(VTK_LAGRANGE_HEXAHEDRON, 1, "Lagrange hexahedra");
  CheckHigherOrderHexPair(VTK_LAGRANGE_HEXAHEDRON, 2, "Lagrange hexahedra");
  CheckHigherOrderHexPair(VTK_LAGRANGE_HEXAHEDRON, 3, "Lagrange hexahedra");
  CheckHigherOrderHexPair(VTK_BEZIER_HEXAHEDRON, 2, "Bezier hexahedra");
  CheckHigherOrderHexPair(VTK_BEZIER_HEXAHEDRON, 3, "Bezier hexahedra");
}

//------------------------------------------------------------------------------
// A nx by ny sheet of quads in the z = 0 plane, every cell wound the same way.
vtkSmartPointer<vtkUnstructuredGrid> MakeQuadSheet(int nx, int ny, bool flipOne)
{
  vtkNew<vtkPoints> points;
  for (int j = 0; j <= ny; ++j)
  {
    for (int i = 0; i <= nx; ++i)
    {
      points->InsertNextPoint(i, j, 0);
    }
  }
  auto pointId = [&](int i, int j) { return static_cast<vtkIdType>(i + j * (nx + 1)); };

  auto grid = vtkSmartPointer<vtkUnstructuredGrid>::New();
  grid->SetPoints(points);
  for (int j = 0; j < ny; ++j)
  {
    for (int i = 0; i < nx; ++i)
    {
      vtkIdType quad[4] = { pointId(i, j), pointId(i + 1, j), pointId(i + 1, j + 1),
        pointId(i, j + 1) };
      // One cell wound the other way is the defect CheckWindingConsistency()
      // is for: nothing about the cell itself is wrong, only its relation to
      // its neighbours.
      if (flipOne && i == 1 && j == 1)
      {
        std::reverse(quad, quad + 4);
      }
      grid->InsertNextCell(VTK_QUAD, 4, quad);
    }
  }
  return grid;
}

//------------------------------------------------------------------------------
// A 2D cell's sides are its edges. Everything else should behave exactly as it
// does in 3D, which is why this leans on the same invariant checker.
void TestQuadSheet()
{
  const int nx = 4, ny = 3;
  auto grid = MakeQuadSheet(nx, ny, false);

  auto sides = MapOf(grid);

  // Edges of a quad grid: the verticals plus the horizontals.
  const vtkIdType expected = static_cast<vtkIdType>((nx + 1) * ny + nx * (ny + 1));
  CHECK(sides->GetNumberOfSides() == expected,
    "quad sheet edge count is " << sides->GetNumberOfSides() << ", expected " << expected);
  CHECK(sides->GetNumberOfHalfSides() == 4 * nx * ny, "quad sheet half-side count");

  CheckInvariants(grid, sides, "quad sheet");

  CHECK(sides->CheckWindingConsistency() == 0,
    "a consistently wound sheet should have no winding complaints");

  // Every side here is an edge, so two points.
  vtkNew<vtkIdList> ptIds;
  for (vtkIdType sideId = 0; sideId < sides->GetNumberOfSides(); ++sideId)
  {
    SidePoints(sides, sideId, ptIds);
    CHECK(ptIds->GetNumberOfIds() == 2, "a 2D cell's side should be an edge");
  }

  // An interior cell has four neighbours across its edges.
  vtkNew<vtkIdList> neighbors;
  sides->GetCellNeighbors(nx + 1, neighbors);
  CHECK(neighbors->GetNumberOfIds() == 4,
    "an interior quad should have four edge neighbours, not " << neighbors->GetNumberOfIds());
}

//------------------------------------------------------------------------------
// Triangles and polygons, mixed, so the loop rule is exercised on cells whose
// point counts differ.
void TestMixedPolygons()
{
  vtkNew<vtkPoints> points;
  points->InsertNextPoint(0, 0, 0);
  points->InsertNextPoint(1, 0, 0);
  points->InsertNextPoint(2, 0, 0);
  points->InsertNextPoint(0, 1, 0);
  points->InsertNextPoint(1, 1, 0);
  points->InsertNextPoint(2, 1, 0);
  points->InsertNextPoint(0.5, 2, 0);

  vtkNew<vtkUnstructuredGrid> grid;
  grid->SetPoints(points);
  const vtkIdType triangle[3] = { 0, 1, 4 };
  const vtkIdType quad[4] = { 1, 2, 5, 4 };
  const vtkIdType pentagon[5] = { 0, 4, 5, 6, 3 };
  grid->InsertNextCell(VTK_TRIANGLE, 3, triangle);
  grid->InsertNextCell(VTK_QUAD, 4, quad);
  grid->InsertNextCell(VTK_POLYGON, 5, pentagon);

  auto sides = MapOf(grid);
  CHECK(sides->GetNumberOfHalfSides() == 12, "3 + 4 + 5 edges");
  // Shared: (1,4) between triangle and quad, (0,4) between triangle and
  // pentagon, (4,5) between quad and pentagon. So 12 - 3 = 9.
  CHECK(sides->GetNumberOfSides() == 9,
    "mixed polygons should share three edges, leaving 9, not " << sides->GetNumberOfSides());

  CheckInvariants(grid, sides, "triangle, quad and polygon");
  CHECK(sides->CheckWindingConsistency() == 0, "these polygons are consistently wound");
}

//------------------------------------------------------------------------------
// The diagnostic has to actually catch a mis-wound cell, or it is worth
// nothing.
void TestWindingDiagnostic()
{
  auto grid = MakeQuadSheet(4, 3, /*flipOne=*/true);
  auto sides = MapOf(grid);

  vtkNew<vtkIdList> offenders;
  sides->SetGlobalWarningDisplay(0);
  const vtkIdType inconsistent = sides->CheckWindingConsistency(offenders);
  sides->SetGlobalWarningDisplay(1);

  // The flipped cell is interior, so all four of its edges are shared and all
  // four now run the same way as their neighbour's.
  CHECK(
    inconsistent == 4, "one flipped interior quad should spoil four edges, not " << inconsistent);
  CHECK(offenders->GetNumberOfIds() == inconsistent, "the offender list should match the count");

  // And the mesh is otherwise fine: the flipped cell is the only one involved.
  vtkNew<vtkIdList> cellIds;
  for (vtkIdType i = 0; i < offenders->GetNumberOfIds(); ++i)
  {
    sides->GetSideCells(offenders->GetId(i), cellIds);
    CHECK(cellIds->IsId(1 + 1 * 4) >= 0, "every spoiled edge should touch the flipped cell");
  }

  std::cerr << "--- winding diagnostic: caught " << inconsistent << " edges" << std::endl;
}

//------------------------------------------------------------------------------
// Two higher-order quadrilaterals of order 2 side by side, wound the same way.
// Their edges follow the parametric axes, so the shared edge, edge 1 of the
// first and edge 3 of the second, runs the same way in both until edge 3 is
// reversed.
void CheckHigherOrderQuadPair(int cellType, const char* name)
{
  // A 5 by 3 lattice of points, (0.5 i, 0.5 j), point i + 5 j.
  vtkNew<vtkPoints> points;
  for (int j = 0; j <= 2; ++j)
  {
    for (int i = 0; i <= 4; ++i)
    {
      points->InsertNextPoint(0.5 * i, 0.5 * j, 0);
    }
  }
  // Corners, then the nodes of the edges (0,1), (1,2), (3,2), (0,3), then the
  // centre.
  const vtkIdType first[9] = { 0, 2, 12, 10, 1, 7, 11, 5, 6 };
  const vtkIdType second[9] = { 2, 4, 14, 12, 3, 9, 13, 7, 8 };
  vtkNew<vtkUnstructuredGrid> grid;
  grid->SetPoints(points);
  grid->InsertNextCell(cellType, 9, first);
  grid->InsertNextCell(cellType, 9, second);

  auto sides = MapOf(grid);
  CHECK(sides->GetNumberOfHalfSides() == 8, name << ": 4 + 4 edges");
  CHECK(sides->GetNumberOfSides() == 7, name << ": one shared edge, leaving 7");
  CheckInvariants(grid, sides, name);
  sides->SetGlobalWarningDisplay(0);
  const vtkIdType inconsistent = sides->CheckWindingConsistency();
  sides->SetGlobalWarningDisplay(1);
  CHECK(inconsistent == 0,
    name << ": consistently wound quadrilaterals reported " << inconsistent
         << " inconsistent edges");
}

void TestHigherOrderQuadrilaterals()
{
  CheckHigherOrderQuadPair(VTK_LAGRANGE_QUADRILATERAL, "Lagrange quadrilaterals");
  CheckHigherOrderQuadPair(VTK_BEZIER_QUADRILATERAL, "Bezier quadrilaterals");
}

//------------------------------------------------------------------------------
// Two unit cubes as polyhedra sharing the side x = 1, point x + 3 y + 6 z. The
// second lists the shared side reversed, as outward winding has it, in the
// same direction as the first, or twisted, in an order that is neither, so
// that it is another quad on the same points.
enum class SharedSide
{
  Reversed,
  Same,
  Twisted
};

vtkSmartPointer<vtkUnstructuredGrid> MakePolyhedronPair(SharedSide shared)
{
  vtkNew<vtkPoints> points;
  for (int z = 0; z <= 1; ++z)
  {
    for (int y = 0; y <= 1; ++y)
    {
      for (int x = 0; x <= 2; ++x)
      {
        points->InsertNextPoint(x, y, z);
      }
    }
  }
  auto grid = vtkSmartPointer<vtkUnstructuredGrid>::New();
  grid->SetPoints(points);

  const vtkIdType firstPoints[8] = { 0, 1, 3, 4, 6, 7, 9, 10 };
  const vtkIdType firstSides[] = { 4, 0, 6, 9, 3, 4, 1, 4, 10, 7, 4, 0, 1, 7, 6, 4, 3, 9, 10, 4, 4,
    0, 3, 4, 1, 4, 6, 7, 10, 9 };
  grid->InsertNextCell(VTK_POLYHEDRON, 8, firstPoints, 6, firstSides);

  const vtkIdType secondPoints[8] = { 1, 2, 4, 5, 7, 8, 10, 11 };
  vtkIdType secondSides[] = { 4, 1, 7, 10, 4, 4, 2, 5, 11, 8, 4, 1, 2, 8, 7, 4, 4, 10, 11, 5, 4, 1,
    4, 5, 2, 4, 7, 8, 11, 10 };
  if (shared == SharedSide::Same)
  {
    // (1, 4, 10, 7), as the first cube lists it.
    secondSides[2] = 4;
    secondSides[3] = 10;
    secondSides[4] = 7;
  }
  else if (shared == SharedSide::Twisted)
  {
    // (1, 10, 7, 4) instead of (1, 7, 10, 4).
    secondSides[2] = 10;
    secondSides[3] = 7;
  }
  grid->InsertNextCell(VTK_POLYHEDRON, 8, secondPoints, 6, secondSides);
  return grid;
}

void TestPolyhedronWinding()
{
  auto grid = MakePolyhedronPair(SharedSide::Reversed);
  auto sides = MapOf(grid);
  CHECK(sides->GetNumberOfSides() == 11, "two cubes sharing a side have 11 sides");
  CheckInvariants(grid, sides, "polyhedron pair");
  CHECK(sides->CheckWindingConsistency() == 0, "the polyhedron pair is consistently wound");

  auto same = MakePolyhedronPair(SharedSide::Same);
  auto sameSides = MapOf(same);
  CHECK(sameSides->GetNumberOfSides() == 11, "a side listed in the same direction is shared");
  sameSides->SetGlobalWarningDisplay(0);
  const vtkIdType inconsistent = sameSides->CheckWindingConsistency();
  sameSides->SetGlobalWarningDisplay(1);
  CHECK(inconsistent == 1,
    "a shared side listed in the same direction should be reported, got " << inconsistent);

  // A twisted quad on the points of the shared side is another side.
  auto twisted = MakePolyhedronPair(SharedSide::Twisted);
  CHECK(MapOf(twisted)->GetNumberOfSides() == 12, "a twisted side is not shared");
}

//------------------------------------------------------------------------------
// The structure follows the topology: moving the points keeps an authored
// numbering, while a change of the cells drops it even when the number of
// half-sides is the same.
void TestTopologyChanges()
{
  auto grid = MakeTetPair();
  auto sides = MapOf(grid);
  const vtkIdType numberOfSides = sides->GetNumberOfSides();
  vtkNew<vtkIdTypeArray> ids;
  ids->DeepCopy(sides->GetHalfSideToSide());
  sides->SetSideIds(ids, numberOfSides);

  double x[3];
  grid->GetPoints()->GetPoint(0, x);
  grid->GetPoints()->SetPoint(0, x[0] + 0.1, x[1], x[2]);
  grid->GetPoints()->Modified();
  CHECK(sides->IsMapBuilt(), "moving the points should not invalidate the sides");
  CHECK(sides->GetAuthored(), "moving the points should keep the supplied numbering");

  // The same two cells in the other order: the same number of half-sides, but
  // the supplied ids now name other sides.
  vtkNew<vtkCellArray> swapped;
  vtkNew<vtkIdList> cellPoints;
  grid->GetCellPoints(1, cellPoints);
  swapped->InsertNextCell(cellPoints);
  grid->GetCellPoints(0, cellPoints);
  swapped->InsertNextCell(cellPoints);
  vtkNew<vtkUnsignedCharArray> types;
  types->SetNumberOfValues(2);
  types->SetValue(0, static_cast<unsigned char>(grid->GetCellType(1)));
  types->SetValue(1, static_cast<unsigned char>(grid->GetCellType(0)));
  grid->SetCells(types, swapped);

  sides->SetGlobalWarningDisplay(0);
  CHECK(sides->GetNumberOfSides() == numberOfSides, "the sides of the reordered cells");
  sides->SetGlobalWarningDisplay(1);
  CHECK(!sides->GetAuthored(), "a change of the cells should drop the supplied numbering");
  CheckInvariants(grid, sides, "reordered tetrahedra");
}

//------------------------------------------------------------------------------
// A higher-order cell inserted after the distinct cell types were cached is
// still found, since InsertNextCell() invalidates them.
void TestCellsInsertedAfterTypesCached()
{
  vtkNew<vtkPoints> points;
  for (int z = 0; z <= 1; ++z)
  {
    for (int y = 0; y <= 1; ++y)
    {
      for (int x = 0; x <= 2; ++x)
      {
        points->InsertNextPoint(x, y, z);
      }
    }
  }
  vtkNew<vtkUnstructuredGrid> grid;
  grid->SetPoints(points);
  const vtkIdType hexahedron[8] = { 0, 1, 4, 3, 6, 7, 10, 9 };
  grid->InsertNextCell(VTK_HEXAHEDRON, 8, hexahedron);
  grid->GetDistinctCellTypesArray();
  const vtkIdType lagrange[8] = { 1, 2, 5, 4, 7, 8, 11, 10 };
  grid->InsertNextCell(VTK_LAGRANGE_HEXAHEDRON, 8, lagrange);

  auto sides = MapOf(grid);
  CHECK(sides->GetNumberOfHalfSides() == 12,
    "the sides of a cell inserted after the cell types were cached, "
      << sides->GetNumberOfHalfSides() << " half-sides");
  CHECK(sides->GetNumberOfSides() == 11, "a hexahedron and a Lagrange hexahedron share a side");
}

//------------------------------------------------------------------------------
// Hidden and refined ghost cells still have sides, shared with their
// neighbors like any other cell's.
void TestGhostCells()
{
  for (unsigned char ghost : { vtkDataSetAttributes::HIDDENCELL, vtkDataSetAttributes::REFINEDCELL,
         vtkDataSetAttributes::DUPLICATECELL })
  {
    auto grid = MakeTetPair();
    vtkNew<vtkUnsignedCharArray> ghosts;
    ghosts->SetName(vtkDataSetAttributes::GhostArrayName());
    ghosts->SetNumberOfValues(2);
    ghosts->SetValue(0, 0);
    ghosts->SetValue(1, ghost);
    grid->GetCellData()->AddArray(ghosts);

    auto sides = MapOf(grid);
    CHECK(sides->GetNumberOfHalfSides() == 8, "ghost type " << int(ghost) << ": 4 + 4 sides");
    CHECK(sides->GetNumberOfSides() == 7,
      "ghost type " << int(ghost) << ": the ghost cell shares a side with the other");
    CheckInvariants(grid, sides, "tetrahedra with a ghost cell");
  }
}

//------------------------------------------------------------------------------
// The derived numbering is the same from one build to the next, whatever the
// threads.
void TestReproducibleNumbering()
{
  auto grid = MakeHexGrid(16, 16, 16);
  std::vector<vtkIdType> first;
  vtkIdType differing = 0;
  for (int run = 0; run < 5; ++run)
  {
    vtkNew<vtkUnstructuredGridSideMap> sides;
    sides->SetDataSet(grid);
    vtkDataArray* ids = sides->GetHalfSideToSide();
    std::vector<vtkIdType> numbering(ids->GetNumberOfTuples());
    for (vtkIdType i = 0; i < ids->GetNumberOfTuples(); ++i)
    {
      numbering[i] = static_cast<vtkIdType>(ids->GetTuple1(i));
    }
    if (run == 0)
    {
      first = numbering;
    }
    else if (numbering != first)
    {
      ++differing;
    }
  }
  CHECK(differing == 0, differing << " of 4 builds numbered the sides differently");
}

//------------------------------------------------------------------------------
// Side data reached through the grid, the way point and cell data are.
void TestGridSideData()
{
  auto grid = MakeHexGrid(3, 3, 3);
  const int SIDE = vtkDataObject::SIDE;

  // A grid has side data the way it has cell data, an empty collection until
  // arrays are added, and asking about it must not number the sides: a generic
  // walk over attribute types should not set off an O(n) build.
  vtkSideData* sideData = grid->GetSideData();
  CHECK(sideData != nullptr, "GetSideData should never return null");
  CHECK(grid->GetAttributes(SIDE) == sideData, "GetAttributes should return the side data");
  CHECK(grid->GetAttributesAsFieldData(SIDE) == sideData,
    "GetAttributesAsFieldData should return the side data");
  CHECK(
    grid->GetNumberOfElements(SIDE) == 0, "no side map and no side arrays, so no sides counted");
  CHECK(!grid->HasSideMap(), "asking about side attributes should not have built the map");

  const vtkIdType numberOfSides = grid->GetNumberOfSides();
  CHECK(numberOfSides == 108, "a 3x3x3 hex block has 108 sides, not " << numberOfSides);
  CHECK(grid->GetNumberOfElements(SIDE) == numberOfSides,
    "GetNumberOfElements disagrees with GetNumberOfSides");

  // Side data is a vtkDataSetAttributes like any other, so it takes arrays the
  // same way point and cell data do.
  vtkNew<vtkDoubleArray> flux;
  flux->SetName("flux");
  flux->SetNumberOfTuples(numberOfSides);
  for (vtkIdType i = 0; i < numberOfSides; ++i)
  {
    flux->SetValue(i, static_cast<double>(i));
  }
  sideData->AddArray(flux);
  sideData->SetActiveScalars("flux");

  CHECK(sideData->GetScalars() == flux.Get(), "active scalars should work on side data");
  CHECK(grid->GetAttributeTypeForArray(flux) == SIDE,
    "an array in side data should report the side attribute type");

  // And the existing attribute types still answer for themselves.
  vtkNew<vtkDoubleArray> height;
  height->SetName("height");
  height->SetNumberOfTuples(grid->GetNumberOfPoints());
  height->FillValue(0.0);
  grid->GetPointData()->AddArray(height);
  CHECK(grid->GetAttributeTypeForArray(height) == vtkDataObject::POINT,
    "a point array should still report POINT");

  // Initialize drops it all, as it does the rest of the grid.
  grid->Initialize();
  CHECK(!grid->HasSideMap(), "Initialize should have dropped the side map");
  CHECK(
    grid->GetSideData()->GetNumberOfArrays() == 0, "Initialize should have dropped the side data");
}

//------------------------------------------------------------------------------
// The numbering and the data indexed by it have to travel together.
void TestGridCopies()
{
  auto grid = MakeHexGrid(2, 2, 2);
  const vtkIdType numberOfSides = grid->GetNumberOfSides();

  vtkNew<vtkDoubleArray> flux;
  flux->SetName("flux");
  flux->SetNumberOfTuples(numberOfSides);
  for (vtkIdType i = 0; i < numberOfSides; ++i)
  {
    flux->SetValue(i, static_cast<double>(i) * 0.5);
  }
  grid->GetSideData()->AddArray(flux);

  // Shallow: the copy has the same sides, in the same order, sharing arrays.
  vtkNew<vtkUnstructuredGrid> shallow;
  shallow->ShallowCopy(grid);
  CHECK(shallow->HasSideMap(), "ShallowCopy should have carried the side map");
  // Carried, not rebuilt. A rebuild would produce the same numbering on this
  // mesh and so would go unnoticed by the checks below, but on an authored
  // numbering it would quietly transpose every value.
  CHECK(shallow->GetSideMap()->GetHalfSideToSide() == grid->GetSideMap()->GetHalfSideToSide(),
    "a shallow copy should share the numbering rather than rebuild it");
  CHECK(shallow->GetNumberOfSides() == numberOfSides, "shallow copy side count");
  CHECK(shallow->GetSideMap()->GetDataSet() == shallow.Get(),
    "the copied map should point at the copy, not the original");
  CHECK(shallow->GetSideData()->GetArray("flux") == flux.Get(),
    "a shallow copy should share the side array");

  // Deep: its own arrays, same values.
  vtkNew<vtkUnstructuredGrid> deep;
  deep->DeepCopy(grid);
  CHECK(deep->HasSideMap(), "DeepCopy should have carried the side map");
  CHECK(deep->GetNumberOfSides() == numberOfSides, "deep copy side count");
  vtkDataArray* deepFlux = deep->GetSideData()->GetArray("flux");
  CHECK(deepFlux != nullptr && deepFlux != flux.Get(), "a deep copy should own its side array");
  CHECK(deep->GetSideMap()->GetHalfSideToSide() != grid->GetSideMap()->GetHalfSideToSide(),
    "a deep copy should own its side map arrays");

  // The numbering survived, so side i still means the same side and still
  // carries the same value on every copy.
  vtkNew<vtkIdList> original;
  vtkNew<vtkIdList> copied;
  vtkIdType mismatches = 0;
  for (vtkIdType sideId = 0; sideId < numberOfSides; ++sideId)
  {
    SidePoints(grid->GetSideMap(), sideId, original);
    SidePoints(deep->GetSideMap(), sideId, copied);
    if (ToVector(original) != ToVector(copied) ||
      deepFlux->GetComponent(sideId, 0) != flux->GetValue(sideId))
    {
      ++mismatches;
    }
  }
  CHECK(mismatches == 0, mismatches << " sides changed meaning across a deep copy");

  // A grid with no side map copies as a grid with no side map.
  auto plain = MakeHexGrid(1, 1, 1);
  vtkNew<vtkUnstructuredGrid> plainCopy;
  plainCopy->ShallowCopy(plain);
  CHECK(!plainCopy->HasSideMap(), "copying a grid with no side map should not make one");

  // CopyStructure carries the numbering, which belongs to the topology, and
  // CopyAttributes the side data, as they do for the cells and cell data.
  vtkNew<vtkUnstructuredGrid> structure;
  structure->CopyStructure(grid);
  CHECK(structure->HasSideMap(), "CopyStructure should have carried the side map");
  CHECK(structure->GetSideMap()->GetHalfSideToSide() == grid->GetSideMap()->GetHalfSideToSide(),
    "a copied structure should share the numbering rather than rebuild it");
  CHECK(structure->GetSideData()->GetNumberOfArrays() == 0,
    "CopyStructure should leave the side data to CopyAttributes");
  structure->CopyAttributes(grid);
  CHECK(structure->GetSideData()->GetArray("flux") == grid->GetSideData()->GetArray("flux"),
    "CopyAttributes should have passed the side data");
  CHECK(structure->GetSideData() != grid->GetSideData(),
    "the copy's side data should be its own collection");
}

//------------------------------------------------------------------------------
// The identity a finite-volume loop rests on. Summing an outward-signed
// quantity over every cell's sides must leave only the boundary: each interior
// side is counted once as +f by the cell that owns it and once as -f by the
// other, and cancels. If the orientation convention were wrong anywhere, the
// two would not cancel and the total would drift.
void CheckFluxTelescoping(vtkUnstructuredGrid* grid, const char* what)
{
  const vtkIdType numberOfSides = grid->GetNumberOfSides();

  vtkNew<vtkDoubleArray> flux;
  flux->SetName("flux");
  flux->SetNumberOfTuples(numberOfSides);
  for (vtkIdType sideId = 0; sideId < numberOfSides; ++sideId)
  {
    // Something that varies, so cancellation cannot happen by accident.
    flux->SetValue(sideId, 1.0 + 0.37 * static_cast<double>(sideId));
  }
  grid->GetSideData()->AddArray(flux);

  vtkUnstructuredGridSideMap* sides = grid->GetSideMap();

  double summedOverCells = 0.0;
  for (vtkIdType cellId = 0; cellId < grid->GetNumberOfCells(); ++cellId)
  {
    for (vtkIdType localSide = 0; localSide < sides->GetNumberOfCellSides(cellId); ++localSide)
    {
      const vtkIdType sideId = sides->GetCellSide(cellId, localSide);
      summedOverCells += sides->GetSideOrientation(cellId, localSide) * flux->GetValue(sideId);
    }
  }

  double summedOverBoundary = 0.0;
  vtkNew<vtkIdList> cellIds;
  for (vtkIdType sideId = 0; sideId < numberOfSides; ++sideId)
  {
    sides->GetSideCells(sideId, cellIds);
    if (cellIds->GetNumberOfIds() == 1)
    {
      summedOverBoundary += flux->GetValue(sideId);
    }
  }

  CHECK(std::abs(summedOverCells - summedOverBoundary) < 1e-9,
    "outward-signed flux did not telescope over " << what << ": cells gave " << summedOverCells
                                                  << ", boundary gave " << summedOverBoundary);
  std::cerr << "--- flux telescoping, " << what << ": " << summedOverCells << " over cells, "
            << summedOverBoundary << " over boundary" << std::endl;
}

void TestFluxTelescoping()
{
  // In 3D this is the divergence theorem over sides. In 2D it is the same
  // statement over edges, which is the shape of a staggered-grid model
  // carrying the normal component of velocity on each edge.
  CheckFluxTelescoping(MakeHexGrid(4, 3, 2), "hexahedra");
  CheckFluxTelescoping(MakeQuadSheet(5, 4, false), "quad sheet");
  CheckFluxTelescoping(MakeTetPair(), "tetrahedra");
  CheckFluxTelescoping(MakeVoxelGrid(3, 3, 2), "voxels");
  CheckFluxTelescoping(MakePixelSheet(4, 4), "pixel sheet");
}

//------------------------------------------------------------------------------
// A copy of an up-to-date map is not built again, so the points of the sides
// of its higher-order cells have to come out the same.
void TestHigherOrderCopy()
{
  auto grid = MakeHigherOrderHexPair(VTK_LAGRANGE_HEXAHEDRON, 2);
  const vtkIdType numberOfSides = grid->GetNumberOfSides();
  vtkNew<vtkUnstructuredGrid> deep;
  deep->DeepCopy(grid);
  vtkNew<vtkUnstructuredGrid> shallow;
  shallow->ShallowCopy(grid);
  vtkNew<vtkIdList> original;
  vtkNew<vtkIdList> copied;
  vtkIdType mismatches = 0;
  for (vtkIdType sideId = 0; sideId < numberOfSides; ++sideId)
  {
    SidePoints(grid->GetSideMap(), sideId, original);
    SidePoints(deep->GetSideMap(), sideId, copied);
    mismatches += ToVector(original) != ToVector(copied) ? 1 : 0;
    SidePoints(shallow->GetSideMap(), sideId, copied);
    mismatches += ToVector(original) != ToVector(copied) ? 1 : 0;
  }
  CHECK(mismatches == 0, mismatches << " sides of higher-order cells lost their points in a copy");
}

//------------------------------------------------------------------------------
// A staggered-grid ocean or atmosphere model, as a reader would bring it in:
// columns of hexagonal prisms over a patch of a honeycomb, two layers deep,
// with the normal velocity on every side in the file's own side numbering and
// measured from the file's own cell of each side. The reader supplies the
// numbering, adds the velocity as side data, and negates it where the file's
// cell is not the side's representative. A constant velocity field then has no
// net flux out of any cell.
void TestStaggeredColumns()
{
  // The corners of the hexagons of a honeycomb around the origin, merged.
  vtkNew<vtkPoints> points;
  std::map<std::pair<long, long>, vtkIdType> merged;
  const double radius = 1.0;
  auto corner = [&](double x, double y)
  {
    const auto key = std::make_pair(std::lround(x * 1e6), std::lround(y * 1e6));
    auto found = merged.find(key);
    if (found != merged.end())
    {
      return found->second;
    }
    const vtkIdType id = points->InsertNextPoint(x, y, 0.0);
    merged.emplace(key, id);
    return id;
  };
  struct Hexagon
  {
    double X, Y;
  };
  std::vector<Hexagon> hexagons = { { 0, 0 } };
  for (int k = 0; k < 6; ++k)
  {
    const double angle = vtkMath::Pi() / 3.0 * k + vtkMath::Pi() / 6.0;
    hexagons.push_back(
      { std::sqrt(3.0) * radius * std::cos(angle), std::sqrt(3.0) * radius * std::sin(angle) });
  }
  // The planar corners, counterclockwise seen from above, of every hexagon.
  std::vector<std::array<vtkIdType, 6>> rings;
  for (const Hexagon& h : hexagons)
  {
    std::array<vtkIdType, 6> ring;
    for (int k = 0; k < 6; ++k)
    {
      const double angle = vtkMath::Pi() / 3.0 * k;
      ring[k] = corner(h.X + radius * std::cos(angle), h.Y + radius * std::sin(angle));
    }
    rings.push_back(ring);
  }
  // Three levels of the planar points.
  const vtkIdType numberOfPlanar = points->GetNumberOfPoints();
  vtkNew<vtkPoints> levels;
  for (int level = 0; level < 3; ++level)
  {
    for (vtkIdType i = 0; i < numberOfPlanar; ++i)
    {
      double x[3];
      points->GetPoint(i, x);
      levels->InsertNextPoint(x[0], x[1], static_cast<double>(level));
    }
  }
  vtkNew<vtkUnstructuredGrid> grid;
  grid->SetPoints(levels);
  for (int layer = 0; layer < 2; ++layer)
  {
    for (const auto& ring : rings)
    {
      vtkIdType prism[12];
      for (int k = 0; k < 6; ++k)
      {
        prism[k] = ring[k] + layer * numberOfPlanar;
        prism[6 + k] = ring[k] + (layer + 1) * numberOfPlanar;
      }
      grid->InsertNextCell(VTK_HEXAGONAL_PRISM, 12, prism);
    }
  }

  // The file: its own numbering of the sides, here the derived one reversed,
  // and its own cell of every side, here the last one that has it, whose
  // outward normal the file measures the velocity along.
  vtkNew<vtkUnstructuredGridSideMap> derived;
  derived->SetDataSet(grid);
  const vtkIdType numberOfSides = derived->GetNumberOfSides();
  const double velocity[3] = { 0.3, -1.1, 0.7 };
  std::vector<double> fileVelocity(numberOfSides);
  std::vector<std::pair<vtkIdType, vtkIdType>> fileCell(numberOfSides);
  vtkNew<vtkIdList> sidePoints;
  vtkNew<vtkIdList> cellPoints;
  vtkNew<vtkGenericCell> cell;
  for (vtkIdType sideId = 0; sideId < numberOfSides; ++sideId)
  {
    const vtkIdType fileId = numberOfSides - 1 - sideId;
    vtkDataArray* cells = derived->GetSideCellsArray();
    vtkDataArray* localSides = derived->GetSideLocalSidesArray();
    const vtkIdType last =
      static_cast<vtkIdType>(derived->GetSideOffsets()->GetTuple1(sideId + 1)) - 1;
    const vtkIdType cellId = static_cast<vtkIdType>(cells->GetTuple1(last));
    const vtkIdType localSide = static_cast<vtkIdType>(localSides->GetTuple1(last));
    fileCell[fileId] = { cellId, localSide };
    unsigned char faceType = VTK_EMPTY_CELL;
    grid->GetCellFacePoints(
      cellId, static_cast<int>(localSide), sidePoints, faceType, cellPoints, cell);
    // The area vector of the side, outward from the file's cell (Newell).
    double area[3] = { 0.0, 0.0, 0.0 };
    const vtkIdType n = sidePoints->GetNumberOfIds();
    for (vtkIdType i = 0; i < n; ++i)
    {
      double a[3], b[3];
      grid->GetPoint(sidePoints->GetId(i), a);
      grid->GetPoint(sidePoints->GetId((i + 1) % n), b);
      area[0] += 0.5 * (a[1] * b[2] - a[2] * b[1]);
      area[1] += 0.5 * (a[2] * b[0] - a[0] * b[2]);
      area[2] += 0.5 * (a[0] * b[1] - a[1] * b[0]);
    }
    fileVelocity[fileId] = vtkMath::Dot(velocity, area);
  }

  // The reader.
  vtkNew<vtkIdTypeArray> ids;
  ids->SetNumberOfTuples(derived->GetNumberOfHalfSides());
  for (vtkIdType halfSide = 0; halfSide < derived->GetNumberOfHalfSides(); ++halfSide)
  {
    ids->SetValue(halfSide,
      numberOfSides - 1 -
        static_cast<vtkIdType>(derived->GetHalfSideToSide()->GetTuple1(halfSide)));
  }
  vtkUnstructuredGridSideMap* sides = grid->GetSideMap();
  sides->SetSideIds(ids, numberOfSides);
  vtkNew<vtkDoubleArray> normalVelocity;
  normalVelocity->SetName("normalVelocity");
  normalVelocity->SetNumberOfTuples(numberOfSides);
  for (vtkIdType fileId = 0; fileId < numberOfSides; ++fileId)
  {
    const auto& [cellId, localSide] = fileCell[fileId];
    normalVelocity->SetValue(
      fileId, sides->GetSideOrientation(cellId, localSide) * fileVelocity[fileId]);
  }
  grid->GetSideData()->AddArray(normalVelocity);

  // No net flux out of any cell.
  double worst = 0.0;
  for (vtkIdType cellId = 0; cellId < grid->GetNumberOfCells(); ++cellId)
  {
    double net = 0.0;
    for (vtkIdType localSide = 0; localSide < sides->GetNumberOfCellSides(cellId); ++localSide)
    {
      net += sides->GetSideOrientation(cellId, localSide) *
        normalVelocity->GetValue(sides->GetCellSide(cellId, localSide));
    }
    worst = std::max(worst, std::abs(net));
  }
  CHECK(sides->GetAuthored(), "the file's numbering is kept");
  // 7 hexagons on each of 3 levels, and 30 distinct hexagon edges in each of 2 layers.
  CHECK(numberOfSides == 7 * 3 + 30 * 2, "sides of the columns: " << numberOfSides);
  CHECK(worst < 1e-9, "a constant velocity has a net flux of " << worst << " out of a cell");
}

//------------------------------------------------------------------------------
// Once the grid has side data the numbering it is indexed by is pinned: Reset
// is refused, moving the points keeps both, and a change of the cells drops
// both, since the values no longer name the sides they were attached to.
void TestPinnedNumbering()
{
  auto grid = MakeTetPair();
  vtkUnstructuredGridSideMap* sides = grid->GetSideMap();
  vtkNew<vtkDoubleArray> flux;
  flux->SetName("flux");
  flux->SetNumberOfTuples(grid->GetNumberOfSides());
  flux->FillValue(1.0);
  grid->GetSideData()->AddArray(flux);

  sides->SetGlobalWarningDisplay(0);
  sides->Initialize();
  sides->SetGlobalWarningDisplay(1);
  CHECK(sides->IsMapBuilt(), "Initialize should be refused while side data is attached");

  double x[3];
  grid->GetPoints()->GetPoint(0, x);
  grid->GetPoints()->SetPoint(0, x[0] + 0.1, x[1], x[2]);
  grid->GetPoints()->Modified();
  CHECK(sides->IsMapBuilt(), "moving the points should not invalidate the sides");
  CHECK(grid->GetSideData()->GetArray("flux") != nullptr,
    "moving the points should keep the side data");

  vtkNew<vtkCellArray> swapped;
  vtkNew<vtkIdList> cellPoints;
  grid->GetCellPoints(1, cellPoints);
  swapped->InsertNextCell(cellPoints);
  grid->GetCellPoints(0, cellPoints);
  swapped->InsertNextCell(cellPoints);
  vtkNew<vtkUnsignedCharArray> types;
  types->SetNumberOfValues(2);
  types->SetValue(0, static_cast<unsigned char>(grid->GetCellType(1)));
  types->SetValue(1, static_cast<unsigned char>(grid->GetCellType(0)));
  grid->SetCells(types, swapped);
  sides->SetGlobalWarningDisplay(0);
  sides->BuildMap();
  sides->SetGlobalWarningDisplay(1);
  CHECK(grid->GetSideData()->GetNumberOfArrays() == 0,
    "a change of the cells should drop the side data with the numbering");
}

//------------------------------------------------------------------------------
// The grid builds its map when asked for it, and the grid and its map, which
// refer to each other, are collected together.
void TestGridOwnsSideMap()
{
  bool deleted = false;
  vtkNew<vtkCallbackCommand> onDelete;
  onDelete->SetClientData(&deleted);
  onDelete->SetCallback([](vtkObject*, unsigned long, void* clientData, void*)
    { *static_cast<bool*>(clientData) = true; });
  {
    auto grid = MakeTetPair();
    vtkUnstructuredGridSideMap* sides = grid->GetSideMap();
    CHECK(sides->IsMapBuilt(), "GetSideMap should return a built map");
    CHECK(sides->GetDataSet() == grid.Get(), "the map should refer to its grid");
    sides->AddObserver(vtkCommand::DeleteEvent, onDelete);
  }
  CHECK(deleted, "the grid and its side map should be collected together");
}

} // anonymous namespace
int TestUnstructuredGridSideMap(int, char*[])
{
  TestHexBlock();
  TestTetPair();
  TestHexPolyhedronPair();
  TestAuthoredNumbering();
  TestSideIdArrayTypes();
  TestLaziness();
  TestGridSideData();
  TestGridCopies();
  TestPinnedNumbering();
  TestGridOwnsSideMap();
  TestFluxTelescoping();
  TestQuadSheet();
  TestMixedPolygons();
  TestWindingDiagnostic();
  TestVoxelBlock();
  TestPixelSheet();
  TestHexVoxelPair();
  TestQuadraticTetPair();
  TestSidesTurnOutward();
  TestHigherOrderHexahedra();
  TestHigherOrderQuadrilaterals();
  TestPolyhedronWinding();
  TestTopologyChanges();
  TestHigherOrderCopy();
  TestCellsInsertedAfterTypesCached();
  TestGhostCells();
  TestStaggeredColumns();
  TestReproducibleNumbering();
  ReportMemory();

  if (Failures > 0)
  {
    std::cerr << Failures << " failure(s)" << std::endl;
    return EXIT_FAILURE;
  }
  std::cerr << "all checks passed" << std::endl;
  return EXIT_SUCCESS;
}
