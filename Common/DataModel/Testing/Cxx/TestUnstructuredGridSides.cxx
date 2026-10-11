// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Tests vtkUnstructuredGridSides, with its 4 instantiations, see Check(): the sides of every
// dimension, the summary strategies, the scattered sides and their rebuild, the renderable inputs,
// a face shared by 3 cells, the masking of cells, 32-bit cell arrays, polyhedra, the types of the
// sides of every cell type, degenerate sides, sides matched by all their points, the rules of
// Boundary with the sides of several dimensions, and the grouped sides.

#include "vtkCell.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkCellTypeUtilities.h"
#include "vtkDataSetAttributes.h"
#include "vtkGenericCell.h"
#include "vtkHexahedron.h"
#include "vtkIdList.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkPoints.h"
#include "vtkSmartPointer.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridSides.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <initializer_list>
#include <iostream>
#include <numeric>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace
{
using Sides = vtkUnstructuredGridSides<>;
using F = Sides::SideFlags;
using S = Sides::SummaryStrategy;

// The settings of vtkUnstructuredGridSides, but its input, flags and strategy. The ghost types of
// the cells are those of the input, see SetCellGhosts().
struct Parameters
{
  bool PreserveRenderableInputs = false;
  bool OmitSidesForRenderableInputs = false;
  bool PointClipping = false;
  bool CellClipping = false;
  bool ExtentClipping = false;
  vtkIdType PointMaximum = VTK_ID_MAX;
  vtkIdType CellMaximum = VTK_ID_MAX;
  std::array<double, 6> Extent = { -VTK_DOUBLE_MAX, VTK_DOUBLE_MAX, -VTK_DOUBLE_MAX, VTK_DOUBLE_MAX,
    -VTK_DOUBLE_MAX, VTK_DOUBLE_MAX };
  bool RemoveGhostInterfaces = true;
  unsigned char MaskedGhostTypes =
    vtkDataSetAttributes::HIDDENCELL | vtkDataSetAttributes::REFINEDCELL;
  bool MatchSidePoints = true;
};

template <typename SidesT>
void Build(SidesT& sides, vtkUnstructuredGrid* input, F flags, S strategy,
  const Parameters& parameters = Parameters())
{
  sides.SetDataSet(input);
  sides.SetOutputDimensionControl(flags);
  sides.SetStrategy(strategy);
  sides.SetPreserveRenderableInputs(parameters.PreserveRenderableInputs);
  sides.SetOmitSidesForRenderableInputs(parameters.OmitSidesForRenderableInputs);
  sides.SetPointClipping(parameters.PointClipping);
  sides.SetCellClipping(parameters.CellClipping);
  sides.SetExtentClipping(parameters.ExtentClipping);
  sides.SetPointMaximum(parameters.PointMaximum);
  sides.SetCellMaximum(parameters.CellMaximum);
  sides.SetExtent(parameters.Extent.data());
  sides.SetRemoveGhostInterfaces(parameters.RemoveGhostInterfaces);
  sides.SetMaskedGhostTypes(parameters.MaskedGhostTypes);
  sides.SetMatchSidePoints(parameters.MatchSidePoints);
  sides.BuildReducedSides();
}

// Whether the n points `ids` are those of the edge edgeId of the built cell `cell`, in order, or,
// for a polyhedron, those of one of its edges, in either order: the edges of a polyhedron are those
// of vtkPolyhedron in another order, see vtkCellTypeSides::PolyhedronCellSides.
bool IsEdgeOfBuiltCell(vtkGenericCell* cell, int edgeId, const vtkIdType* ids, vtkIdType n)
{
  if (cell->GetCellType() != VTK_POLYHEDRON)
  {
    vtkIdList* edgeIds = cell->GetEdge(edgeId)->GetPointIds();
    return n == edgeIds->GetNumberOfIds() && std::equal(ids, ids + n, edgeIds->begin());
  }
  for (int e = 0; n == 2 && e < cell->GetNumberOfEdges(); ++e)
  {
    vtkIdList* edgeIds = cell->GetEdge(e)->GetPointIds();
    if ((edgeIds->GetId(0) == ids[0] && edgeIds->GetId(1) == ids[1]) ||
      (edgeIds->GetId(0) == ids[1] && edgeIds->GetId(1) == ids[0]))
    {
      return true;
    }
  }
  return false;
}

// Whether the side k of the key id keyId, of points sidePoints, has the information given by its
// getters, see GetSideInformation(), and the type and the points of the side of its built cell, but
// for the faces of a voxel, which are quads in the order of a polygon, or the type of its cell if
// it is preserved.
template <typename SidesT>
bool HasSideOfBuiltCell(
  SidesT* sides, vtkUnstructuredGrid* input, vtkIdType keyId, vtkIdType k, vtkIdList* sidePoints)
{
  vtkNew<vtkGenericCell> cell;
  const auto [cellId, cellType, sideId, sideType, sideDim, sideSize] =
    sides->GetSideInformation(keyId, k, cell);
  if (cellId != sides->GetCellIdOfSide(keyId, k) || cellType != input->GetCellType(cellId) ||
    sideId != sides->GetSideIdOfSide(keyId, k) || sideDim != sides->GetDimensionOfSide(keyId, k) ||
    sideSize != sidePoints->GetNumberOfIds())
  {
    return false;
  }
  if (sideId < 0)
  {
    return sideType == cellType;
  }
  if (sideDim == 0)
  {
    return sideType == VTK_VERTEX;
  }
  if (cellType == VTK_VOXEL && sideDim == 2)
  {
    return sideType == VTK_QUAD;
  }
  input->GetCell(cellId, cell);
  if (sideDim == 1)
  {
    return sideType == cell->GetEdge(sideId)->GetCellType() &&
      IsEdgeOfBuiltCell(cell, sideId, sidePoints->GetPointer(0), sidePoints->GetNumberOfIds());
  }
  vtkCell* side = cell->GetFace(sideId);
  vtkIdList* sideIds = side->GetPointIds();
  return sideType == side->GetCellType() &&
    sidePoints->GetNumberOfIds() == sideIds->GetNumberOfIds() &&
    std::equal(sideIds->begin(), sideIds->end(), sidePoints->begin());
}

template <typename SidesT>
bool CheckSides(const std::string& name, vtkUnstructuredGrid* input, F flags, S strategy,
  const Parameters& parameters, vtkIdType expected)
{
  vtkNew<SidesT> sides;
  Build(*sides, input, flags, strategy, parameters);
  if (sides->GetNumberOfSides() != expected)
  {
    std::cerr << name << ": expected " << expected << " sides, got " << sides->GetNumberOfSides()
              << "\n";
    return false;
  }
  // The sides of the keys add up to the sides, and their smallest point id is their key, except
  // for the preserved cells, in the last key id, whose points are those of their cell.
  vtkIdType count = 0;
  vtkNew<vtkIdList> sidePoints;
  vtkNew<vtkIdList> cellPoints;
  vtkNew<vtkIdList> tempCellPoints;
  vtkNew<vtkGenericCell> tempCell;
  for (vtkIdType keyId = 0; keyId < sides->GetNumberOfKeys(); ++keyId)
  {
    if (sides->GetSideOffsetOfKey(keyId) != count)
    {
      std::cerr << name << ": wrong number of sides before key " << keyId << "\n";
      return false;
    }
    for (vtkIdType k = 0; k < sides->GetNumberOfSidesOfKey(keyId); ++k)
    {
      const vtkIdType sideId = sides->GetSideIdOfSide(keyId, k);
      if (sides->GetCellIdsOfKey(keyId)[k] != sides->GetCellIdOfSide(keyId, k) ||
        sides->GetSideIdsOfKey(keyId)[k] != sideId ||
        (sideId < 0) != (keyId == sides->GetNumberOfKeys() - 1))
      {
        std::cerr << name << ": wrong side " << k << " of key " << keyId << "\n";
        return false;
      }
      unsigned char cellType = VTK_EMPTY_CELL;
      sides->GetSidePoints(keyId, k, sidePoints, cellType, tempCellPoints, tempCell);
      const vtkIdType numSidePts = sidePoints->GetNumberOfIds();
      const vtkIdType* ids = sidePoints->GetPointer(0);
      bool pointsOk =
        numSidePts > 0 && cellType == input->GetCellType(sides->GetCellIdOfSide(keyId, k));
      if (pointsOk && sideId < 0)
      {
        // The preserved cells are in the order of their ids, with their own points.
        input->GetCellPoints(sides->GetCellIdOfSide(keyId, k), cellPoints);
        pointsOk = numSidePts == cellPoints->GetNumberOfIds() &&
          std::equal(ids, ids + numSidePts, cellPoints->GetPointer(0)) &&
          (k == 0 || sides->GetCellIdOfSide(keyId, k - 1) < sides->GetCellIdOfSide(keyId, k)) &&
          sides->GetDimensionOfSide(keyId, k) == vtkCellTypeUtilities::GetDimension(cellType);
      }
      else if (pointsOk)
      {
        // The smallest point of a side is its key. The faces are those of
        // vtkUnstructuredGrid::GetCellFacePoints(), e.keyId., of a polyhedron, and the edges those
        // of vtkCell::GetEdge(), see IsEdgeOfBuiltCell().
        const int sideDim = sides->GetDimensionOfSide(keyId, k);
        const vtkIdType cellId = sides->GetCellIdOfSide(keyId, k);
        pointsOk = *std::min_element(ids, ids + numSidePts) == keyId;
        if (sideDim == 2)
        {
          unsigned char faceCellType = VTK_EMPTY_CELL;
          input->GetCellFacePoints(
            cellId, static_cast<int>(sideId), cellPoints, faceCellType, tempCellPoints, tempCell);
          pointsOk &= numSidePts == cellPoints->GetNumberOfIds() &&
            std::equal(ids, ids + numSidePts, cellPoints->GetPointer(0));
        }
        else if (sideDim == 1)
        {
          input->GetCell(cellId, tempCell);
          pointsOk &= IsEdgeOfBuiltCell(tempCell, static_cast<int>(sideId), ids, numSidePts);
        }
      }
      if (!pointsOk)
      {
        std::cerr << name << ": wrong points of side " << k << " of key " << keyId << "\n";
        return false;
      }
      if (!HasSideOfBuiltCell(sides.Get(), input, keyId, k, sidePoints))
      {
        const auto side = sides->GetSideInformation(keyId, k, tempCell);
        std::cerr << name << ": wrong information of side " << k << " of key " << keyId << " (cell "
                  << vtkCellTypeUtilities::GetClassNameFromTypeId(side.CellType) << ", side "
                  << side.SideId << ", type " << side.SideType << ", size " << side.SideSize
                  << ")\n";
        return false;
      }
    }
    count += sides->GetNumberOfSidesOfKey(keyId);
  }
  if (count != expected || sides->GetSideOffsetOfKey(sides->GetNumberOfKeys()) != count)
  {
    std::cerr << name << ": the keys have " << count << " sides\n";
    return false;
  }
  return true;
}

// CheckSides() with the 4 instantiations of vtkUnstructuredGridSides, whose types of ids and of
// local side ids differ.
bool Check(const std::string& name, vtkUnstructuredGrid* input, F flags, S strategy,
  const Parameters& parameters, vtkIdType expected)
{
  return CheckSides<vtkUnstructuredGridSides<vtkTypeInt32, vtkTypeInt8>>(
           name + " <Int32, Int8>", input, flags, strategy, parameters, expected) &&
    CheckSides<vtkUnstructuredGridSides<vtkTypeInt32, vtkTypeInt32>>(
      name + " <Int32, Int32>", input, flags, strategy, parameters, expected) &&
    CheckSides<vtkUnstructuredGridSides<vtkTypeInt64, vtkTypeInt8>>(
      name + " <Int64, Int8>", input, flags, strategy, parameters, expected) &&
    CheckSides<vtkUnstructuredGridSides<vtkTypeInt64, vtkTypeInt32>>(
      name + " <Int64, Int32>", input, flags, strategy, parameters, expected);
}

// A grid of nx * ny * nz unit hexahedra.
vtkSmartPointer<vtkUnstructuredGrid> CreateHexGrid(int nx, int ny, int nz)
{
  auto ug = vtkSmartPointer<vtkUnstructuredGrid>::New();
  vtkNew<vtkPoints> points;
  auto id = [&](int i, int j, int k) { return i + (nx + 1) * (j + (ny + 1) * k); };
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
  ug->SetPoints(points);
  for (int k = 0; k < nz; ++k)
  {
    for (int j = 0; j < ny; ++j)
    {
      for (int i = 0; i < nx; ++i)
      {
        const vtkIdType hex[8] = {
          id(i, j, k),
          id(i + 1, j, k),
          id(i + 1, j + 1, k),
          id(i, j + 1, k),
          id(i, j, k + 1),
          id(i + 1, j, k + 1),
          id(i + 1, j + 1, k + 1),
          id(i, j + 1, k + 1),
        };
        ug->InsertNextCell(VTK_HEXAHEDRON, 8, hex);
      }
    }
  }
  return ug;
}

//------------------------------------------------------------------------------
// The faces, edges and vertices of 2 hexahedra sharing a face, and the boundary of a block.
bool TestHexahedra()
{
  auto two = CreateHexGrid(2, 1, 1);
  bool ok = Check("Faces/Boundary", two, F::SurfacesOfVolumes, S::Boundary, Parameters(), 10);
  ok &= Check("Faces/Winding", two, F::SurfacesOfVolumes, S::Winding, Parameters(), 10);
  ok &= Check("Faces/AnyOccurrence", two, F::SurfacesOfVolumes, S::AnyOccurrence, Parameters(), 11);
  ok &= Check("Edges/AnyOccurrence", two, F::EdgesOfVolumes, S::AnyOccurrence, Parameters(), 20);
  // Only the sides of dimension d - 1 are matched by Boundary, the edges are kept once.
  ok &= Check("Edges/Boundary", two, F::EdgesOfVolumes, S::Boundary, Parameters(), 20);
  ok &=
    Check("Vertices/AnyOccurrence", two, F::VerticesOfVolumes, S::AnyOccurrence, Parameters(), 12);
  ok &= Check("AllSides/Boundary", two, F::AllSides, S::Boundary, Parameters(), 10 + 20 + 12);

  // The shared face (cell 0, face 1) and (cell 1, face 0) is not on the boundary.
  vtkNew<Sides> faces;
  Build(*faces, two, F::SurfacesOfVolumes, S::Boundary, Parameters());
  for (vtkIdType keyId = 0; keyId < faces->GetNumberOfKeys(); ++keyId)
  {
    for (vtkIdType k = 0; k < faces->GetNumberOfSidesOfKey(keyId); ++k)
    {
      const vtkIdType cellId = faces->GetCellIdOfSide(keyId, k);
      const vtkIdType sideId = faces->GetSideIdOfSide(keyId, k);
      if (faces->GetDimensionOfSide(keyId, k) != 2 || (cellId == 0 && sideId == 1) ||
        (cellId == 1 && sideId == 0))
      {
        std::cerr << "Faces/Boundary: unexpected side (" << cellId << ", " << sideId << ")\n";
        return false;
      }
    }
  }

  // BuildScatteredSides() scatters all the faces, unmatched: key 1 has the shared face twice and
  // the 2 faces of cell 1 whose smallest point is 1. The sides are built again only to build them
  // in another way, or after a modification.
  faces->BuildScatteredSides();
  const vtkMTimeType scatteredTime = faces->GetBuildTime();
  faces->BuildScatteredSides();
  if (faces->GetNumberOfSides() != 12 || faces->GetNumberOfSidesOfKey(1) != 4 ||
    faces->GetBuildTime() != scatteredTime)
  {
    std::cerr << "Scattered: wrong keys of all the faces\n";
    ok = false;
  }
  faces->BuildReducedSides();
  const vtkMTimeType sidesTime = faces->GetBuildTime();
  two->Modified();
  faces->BuildReducedSides();
  if (faces->GetNumberOfSides() != 10 || faces->GetNumberOfSidesOfKey(1) != 2 ||
    sidesTime == scatteredTime || faces->GetBuildTime() == sidesTime)
  {
    std::cerr << "Scattered: the kept faces are not built again\n";
    ok = false;
  }

  // The boundary of a block of n^3 hexahedra has 6 n^2 faces.
  auto block = CreateHexGrid(7, 7, 7);
  ok &= Check("Block/Boundary", block, F::SurfacesOfVolumes, S::Boundary, Parameters(), 6 * 49);
  return ok;
}

// Two quads sharing an edge.
vtkSmartPointer<vtkUnstructuredGrid> CreateQuads()
{
  auto ug = vtkSmartPointer<vtkUnstructuredGrid>::New();
  vtkNew<vtkPoints> points;
  for (int j = 0; j < 2; ++j)
  {
    for (int i = 0; i < 3; ++i)
    {
      points->InsertNextPoint(i, j, 0);
    }
  }
  ug->SetPoints(points);
  const vtkIdType q0[4] = { 0, 1, 4, 3 };
  const vtkIdType q1[4] = { 1, 2, 5, 4 };
  ug->InsertNextCell(VTK_QUAD, 4, q0);
  ug->InsertNextCell(VTK_QUAD, 4, q1);
  return ug;
}

//------------------------------------------------------------------------------
// The edges of 2D cells, and the cells of dimension 2 or less preserved.
bool TestQuads()
{
  auto quads = CreateQuads();
  bool ok = Check("Quads/Boundary", quads, F::NextLowestDimension, S::Boundary, Parameters(), 6);
  ok &= Check("Quads/AnyOccurrence", quads, F::EdgesOfSurfaces, S::AnyOccurrence, Parameters(), 7);
  // The faces of the 3D cells are not the sides of the quads.
  ok &= Check("Quads/Faces", quads, F::SurfacesOfVolumes, S::Boundary, Parameters(), 0);

  Parameters preserve;
  preserve.PreserveRenderableInputs = true;
  ok &= Check("Quads/Preserve", quads, F::EdgesOfSurfaces, S::Boundary, preserve, 6 + 2);
  preserve.OmitSidesForRenderableInputs = true;
  ok &= Check("Quads/PreserveOmit", quads, F::EdgesOfSurfaces, S::Boundary, preserve, 2);

  vtkNew<Sides> cells;
  Build(*cells, quads, F::EdgesOfSurfaces, S::Boundary, preserve);
  for (vtkIdType keyId = 0; keyId < cells->GetNumberOfKeys(); ++keyId)
  {
    for (vtkIdType k = 0; k < cells->GetNumberOfSidesOfKey(keyId); ++k)
    {
      if (cells->GetSideIdOfSide(keyId, k) != -1 || cells->GetDimensionOfSide(keyId, k) != 2)
      {
        std::cerr << "Quads/PreserveOmit: the cells should be preserved\n";
        return false;
      }
    }
  }
  return ok;
}

//------------------------------------------------------------------------------
// A face shared by 3 cells (a non-manifold mesh): Winding keeps it, AnyOccurrence once.
bool TestNonManifold()
{
  auto ug = CreateHexGrid(2, 1, 1);
  vtkNew<vtkIdList> hex;
  ug->GetCellPoints(1, hex);
  ug->InsertNextCell(VTK_HEXAHEDRON, hex); // the same points as cell 1
  // The faces of cells 1 and 2 cancel, and face (0, 1) is shared by 3 cells.
  bool ok = Check("NonManifold/Winding", ug, F::SurfacesOfVolumes, S::Winding, Parameters(), 6);
  ok &= Check(
    "NonManifold/AnyOccurrence", ug, F::SurfacesOfVolumes, S::AnyOccurrence, Parameters(), 11);
  return ok;
}

// Set the ghost types of the cells of a grid, or remove them.
void SetCellGhosts(vtkUnstructuredGrid* ug, const unsigned char* ghosts)
{
  if (!ghosts)
  {
    ug->GetCellData()->RemoveArray(vtkDataSetAttributes::GhostArrayName());
    return;
  }
  vtkUnsignedCharArray* array = ug->AllocateCellGhostArray();
  for (vtkIdType i = 0; i < ug->GetNumberOfCells(); ++i)
  {
    array->SetValue(i, ghosts[i]);
  }
}

//------------------------------------------------------------------------------
// The masked cells: the clipped cells, and ghost cells with and without their ghost interfaces.
bool TestMasking()
{
  auto two = CreateHexGrid(2, 1, 1);
  Parameters parameters;

  // Cell 1 is clipped by its id, by its point 11, or by its points of x = 2.
  parameters.CellClipping = true;
  parameters.CellMaximum = 0;
  bool ok = Check("CellClipping", two, F::SurfacesOfVolumes, S::Boundary, parameters, 6);
  parameters = Parameters();
  parameters.PointClipping = true;
  parameters.PointMaximum = 10;
  ok &= Check("PointClipping", two, F::SurfacesOfVolumes, S::Boundary, parameters, 6);
  parameters = Parameters();
  parameters.ExtentClipping = true;
  parameters.Extent = { 0, 1, 0, 1, 0, 1 };
  ok &= Check("ExtentClipping", two, F::SurfacesOfVolumes, S::Boundary, parameters, 6);
  parameters.Extent = { 3, 4, 0, 1, 0, 1 };
  ok &= Check("ExtentClippingAll", two, F::SurfacesOfVolumes, S::Boundary, parameters, 0);
  parameters = Parameters();

  // A duplicate ghost cell cancels the shared face, but its own faces are not kept.
  const unsigned char ghosts[2] = { 0, vtkDataSetAttributes::DUPLICATECELL };
  SetCellGhosts(two, ghosts);
  ok &=
    Check("Ghosts/RemoveGhostInterfaces", two, F::SurfacesOfVolumes, S::Boundary, parameters, 5);
  // Otherwise, a duplicate ghost cell is an ordinary cell, as in vtkGeometryFilter.
  parameters.RemoveGhostInterfaces = false;
  ok &= Check("Ghosts/KeepGhostInterfaces", two, F::SurfacesOfVolumes, S::Boundary, parameters, 10);

  // A clipped duplicate ghost cell takes no part.
  parameters.RemoveGhostInterfaces = true;
  parameters.CellClipping = true;
  parameters.CellMaximum = 0;
  ok &= Check("Ghosts/CellClipping", two, F::SurfacesOfVolumes, S::Boundary, parameters, 6);
  parameters = Parameters();

  // A hidden or refined ghost cell takes no part.
  for (const unsigned char ghost :
    { vtkDataSetAttributes::HIDDENCELL, vtkDataSetAttributes::REFINEDCELL })
  {
    const unsigned char masked[2] = { 0, ghost };
    SetCellGhosts(two, masked);
    ok &= Check("Ghosts/Masked", two, F::SurfacesOfVolumes, S::Boundary, parameters, 6);
    // Unless its ghost type is not masked: it is then an ordinary cell.
    parameters.MaskedGhostTypes = static_cast<unsigned char>(
      (vtkDataSetAttributes::HIDDENCELL | vtkDataSetAttributes::REFINEDCELL) & ~ghost);
    ok &= Check("Ghosts/NotMasked", two, F::SurfacesOfVolumes, S::Boundary, parameters, 10);
    parameters.MaskedGhostTypes = 0;
    ok &= Check("Ghosts/NoneMasked", two, F::SurfacesOfVolumes, S::Boundary, parameters, 10);
    parameters = Parameters();
  }

  // AnyOccurrence keeps the shared face from the cell that is not a ghost.
  parameters = Parameters();
  SetCellGhosts(two, ghosts);
  ok &= Check("Ghosts/AnyOccurrence", two, F::SurfacesOfVolumes, S::AnyOccurrence, parameters, 6);
  SetCellGhosts(two, nullptr);
  ok &= Check("Ghosts/Removed", two, F::SurfacesOfVolumes, S::Boundary, parameters, 10);
  return ok;
}

//------------------------------------------------------------------------------
// The cell arrays with 32-bit ids, and a polyhedron, whose faces are not those of a linear cell.
bool TestStorageAndPolyhedron()
{
  auto block = CreateHexGrid(7, 7, 7);
  if (!block->GetCells()->ConvertTo32BitStorage())
  {
    std::cerr << "Storage32: the cells could not be converted\n";
    return false;
  }
  bool ok =
    Check("Storage32/Faces", block, F::SurfacesOfVolumes, S::Boundary, Parameters(), 6 * 49);
  ok &= Check(
    "Storage32/Edges", block, F::EdgesOfVolumes, S::AnyOccurrence, Parameters(), 3 * 7 * 8 * 8);

  // Cell 1 of 2 hexahedra sharing a face, as a polyhedron.
  auto two = CreateHexGrid(2, 1, 1);
  auto mixed = vtkSmartPointer<vtkUnstructuredGrid>::New();
  mixed->SetPoints(two->GetPoints());
  vtkNew<vtkIdList> hex;
  two->GetCellPoints(0, hex);
  mixed->InsertNextCell(VTK_HEXAHEDRON, hex);
  two->GetCellPoints(1, hex);
  vtkNew<vtkCellArray> faces;
  for (int f = 0; f < vtkHexahedron::NumberOfFaces; ++f)
  {
    const vtkIdType* face = vtkHexahedron::GetFaceArray(f);
    faces->InsertNextCell(4);
    for (int k = 0; k < 4; ++k)
    {
      faces->InsertCellPoint(hex->GetId(face[k]));
    }
  }
  mixed->InsertNextCell(VTK_POLYHEDRON, hex->GetNumberOfIds(), hex->GetPointer(0), faces);
  ok &= Check("Polyhedron/Faces", mixed, F::SurfacesOfVolumes, S::Boundary, Parameters(), 10);
  ok &= Check("Polyhedron/Edges", mixed, F::EdgesOfVolumes, S::AnyOccurrence, Parameters(), 20);
  ok &= Check("Polyhedron/EdgesBoundary", mixed, F::EdgesOfVolumes, S::Boundary, Parameters(), 20);
  ok &=
    Check("Polyhedron/AllSides", mixed, F::AllSides, S::AnyOccurrence, Parameters(), 11 + 20 + 12);

  // The cells of every dimension, as vtkGeometryFilter: the faces of the 3D cells, matched, and
  // the other cells preserved, in the order of their ids.
  const vtkIdType line[2] = { 0, 1 };
  const vtkIdType vertex[1] = { 2 };
  const vtkIdType quad[4] = { 0, 1, 7, 6 };
  mixed->InsertNextCell(VTK_LINE, 2, line);
  mixed->InsertNextCell(VTK_QUAD, 4, quad);
  mixed->InsertNextCell(VTK_VERTEX, 1, vertex);
  Parameters preserve;
  preserve.PreserveRenderableInputs = true;
  ok &= Check("AllDimensions/Preserve", mixed, F::SurfacesOfVolumes, S::Boundary, preserve, 10 + 3);
  ok &= Check("AllDimensions/Faces", mixed, F::SurfacesOfVolumes, S::Boundary, Parameters(), 10);
  ok &= Check(
    "AllDimensions/AnyOccurrence", mixed, F::SurfacesOfVolumes, S::AnyOccurrence, preserve, 11 + 3);
  return ok;
}

//------------------------------------------------------------------------------
// The quadratic cells, whose sides are read from the static tables of their class: a quadratic
// hexahedron twice, whose faces and edges match, and a quadratic tetrahedron, with the cells
// stored as vtkIdType, then as 32-bit values.
bool TestQuadraticCells()
{
  vtkNew<vtkPoints> points;
  for (int i = 0; i < 30; ++i)
  {
    points->InsertNextPoint(i, (i * i) % 7, (i * 3) % 5);
  }
  vtkNew<vtkUnstructuredGrid> ug;
  ug->SetPoints(points);
  std::array<vtkIdType, 20> hex;
  std::iota(hex.begin(), hex.end(), 0);
  std::array<vtkIdType, 10> tet;
  std::iota(tet.begin(), tet.end(), 20);
  ug->InsertNextCell(VTK_QUADRATIC_HEXAHEDRON, 20, hex.data());
  ug->InsertNextCell(VTK_QUADRATIC_HEXAHEDRON, 20, hex.data());
  ug->InsertNextCell(VTK_QUADRATIC_TETRA, 10, tet.data());
  bool ok = true;
  for (const char* storage : { "Quadratic64", "Quadratic32" })
  {
    const std::string name = storage;
    ok &= Check(name + "/Faces", ug, F::SurfacesOfVolumes, S::Boundary, Parameters(), 4);
    ok &=
      Check(name + "/AnyFaces", ug, F::SurfacesOfVolumes, S::AnyOccurrence, Parameters(), 6 + 4);
    ok &= Check(name + "/Edges", ug, F::EdgesOfVolumes, S::AnyOccurrence, Parameters(), 12 + 6);
    ok &= Check(
      name + "/AllSides", ug, F::AllSides, S::AnyOccurrence, Parameters(), 6 + 4 + 12 + 6 + 30);
    if (!ug->GetCells()->ConvertTo32BitStorage())
    {
      std::cerr << "Quadratic32: the cells could not be converted\n";
      return false;
    }
  }

  // Two quadratic tetrahedra sharing the face of corners 0, 1, 2, in opposite orders, whose
  // mid-edge points are listed after the corners: the sides are matched by their corners.
  vtkNew<vtkUnstructuredGrid> shared;
  shared->SetPoints(points);
  // The midpoint of the edge (a, b) is 5 + the index of the edge in 01 12 20 03 13 23 04 14 24.
  const vtkIdType tet0[10] = { 0, 1, 2, 3, 5, 6, 7, 8, 9, 10 };
  const vtkIdType tet1[10] = { 0, 2, 1, 4, 7, 6, 5, 11, 13, 12 };
  shared->InsertNextCell(VTK_QUADRATIC_TETRA, 10, tet0);
  shared->InsertNextCell(VTK_QUADRATIC_TETRA, 10, tet1);
  ok &= Check("QuadraticShared/Faces", shared, F::SurfacesOfVolumes, S::Boundary, Parameters(), 6);
  ok &= Check(
    "QuadraticShared/AnyFaces", shared, F::SurfacesOfVolumes, S::AnyOccurrence, Parameters(), 7);
  ok &=
    Check("QuadraticShared/Edges", shared, F::EdgesOfVolumes, S::AnyOccurrence, Parameters(), 9);
  return ok;
}

//------------------------------------------------------------------------------
// The types of the sides of every cell type of a fixed size, of the polygons and of the
// higher-order cells, as the sides of their built cells, see HasSideOfBuiltCell(): a cell of each
// type, with its own points, whose sides are all kept.
bool TestSideTypes()
{
  const std::array<std::pair<int, int>, 31> types = { {
    { VTK_LINE, 2 },
    { VTK_QUADRATIC_EDGE, 3 },
    { VTK_CUBIC_LINE, 4 },
    { VTK_POLY_LINE, 4 },
    { VTK_TRIANGLE, 3 },
    { VTK_POLYGON, 5 },
    { VTK_PIXEL, 4 },
    { VTK_QUAD, 4 },
    { VTK_TRIANGLE_STRIP, 5 },
    { VTK_QUADRATIC_TRIANGLE, 6 },
    { VTK_BIQUADRATIC_TRIANGLE, 7 },
    { VTK_QUADRATIC_QUAD, 8 },
    { VTK_QUADRATIC_LINEAR_QUAD, 6 },
    { VTK_BIQUADRATIC_QUAD, 9 },
    { VTK_LAGRANGE_QUADRILATERAL, 9 },
    { VTK_TETRA, 4 },
    { VTK_VOXEL, 8 },
    { VTK_HEXAHEDRON, 8 },
    { VTK_WEDGE, 6 },
    { VTK_PYRAMID, 5 },
    { VTK_PENTAGONAL_PRISM, 10 },
    { VTK_HEXAGONAL_PRISM, 12 },
    { VTK_QUADRATIC_TETRA, 10 },
    { VTK_QUADRATIC_HEXAHEDRON, 20 },
    { VTK_QUADRATIC_WEDGE, 15 },
    { VTK_QUADRATIC_PYRAMID, 13 },
    { VTK_TRIQUADRATIC_HEXAHEDRON, 27 },
    { VTK_TRIQUADRATIC_PYRAMID, 19 },
    { VTK_QUADRATIC_LINEAR_WEDGE, 12 },
    { VTK_BIQUADRATIC_QUADRATIC_WEDGE, 18 },
    { VTK_BIQUADRATIC_QUADRATIC_HEXAHEDRON, 24 },
  } };
  vtkNew<vtkUnstructuredGrid> ug;
  vtkNew<vtkPoints> points;
  ug->SetPoints(points);
  vtkNew<vtkGenericCell> cell;
  vtkIdType expected = 0;
  std::array<vtkIdType, 27> ids;
  for (const auto& type : types)
  {
    const vtkIdType first = points->GetNumberOfPoints();
    for (int i = 0; i < type.second; ++i)
    {
      points->InsertNextPoint(first + i, (i * i) % 7, (i * 3) % 5);
    }
    std::iota(ids.begin(), ids.begin() + type.second, first);
    const vtkIdType cellId = ug->InsertNextCell(type.first, type.second, ids.data());
    // All the sides of the cell, which has its own points: its points, edges and faces.
    ug->GetCell(cellId, cell);
    expected += type.second + (cell->GetCellDimension() >= 2 ? cell->GetNumberOfEdges() : 0) +
      (cell->GetCellDimension() == 3 ? cell->GetNumberOfFaces() : 0);
  }
  return Check("SideTypes", ug, F::AllSides, S::AnyOccurrence, Parameters(), expected);
}

//------------------------------------------------------------------------------
// Two hexahedra sharing a degenerate face, whose smallest point id is repeated, as in a mesh of
// collapsed hexahedra: the first has it as (0, 0, 1, 2), the second, reversed, from the other
// occurrence of its smallest point, as (0, 0, 2, 1). The shared face is matched.
bool TestDegenerateSides()
{
  vtkNew<vtkPoints> points;
  for (int i = 0; i < 11; ++i)
  {
    points->InsertNextPoint(i, (i * i) % 7, (i * 3) % 5);
  }
  vtkNew<vtkUnstructuredGrid> ug;
  ug->SetPoints(points);
  // The top face (4, 5, 6, 7) of the first, and the bottom face (0, 3, 2, 1) of the second.
  const vtkIdType hex0[8] = { 3, 4, 5, 6, 0, 0, 1, 2 };
  const vtkIdType hex1[8] = { 0, 1, 2, 0, 7, 8, 9, 10 };
  ug->InsertNextCell(VTK_HEXAHEDRON, 8, hex0);
  ug->InsertNextCell(VTK_HEXAHEDRON, 8, hex1);
  bool ok = Check("Degenerate/Boundary", ug, F::SurfacesOfVolumes, S::Boundary, Parameters(), 10);
  ok &=
    Check("Degenerate/AnyOccurrence", ug, F::SurfacesOfVolumes, S::AnyOccurrence, Parameters(), 11);
  return ok;
}

//------------------------------------------------------------------------------
// Two quadratic hexahedra sharing the corners of a face, the top face of the first one and the
// bottom face of the second one, with the same mid-edge points, then with others: the faces match
// by all their points only in the first case.
bool TestMatchMidEdgePoints()
{
  vtkNew<vtkPoints> points;
  for (int i = 0; i < 36; ++i)
  {
    points->InsertNextPoint(i, (i * i) % 7, (i * 3) % 5);
  }
  std::array<vtkIdType, 20> bottom;
  std::iota(bottom.begin(), bottom.end(), 0);
  // The corners of the second one, then the mid-edge points of its bottom edges 45 56 67 74, of its
  // top edges and of its vertical edges: its bottom ones are those of the top face of the first
  // one, 12 13 14 15, or others, 24 25 26 27.
  bool ok = true;
  for (const bool sharedMidEdges : { true, false })
  {
    const vtkIdType b = sharedMidEdges ? 12 : 24;
    const vtkIdType top[20] = { 4, 5, 6, 7, 20, 21, 22, 23, b, b + 1, b + 2, b + 3, 28, 29, 30, 31,
      32, 33, 34, 35 };
    vtkNew<vtkUnstructuredGrid> ug;
    ug->SetPoints(points);
    ug->InsertNextCell(VTK_QUADRATIC_HEXAHEDRON, 20, bottom.data());
    ug->InsertNextCell(VTK_QUADRATIC_HEXAHEDRON, 20, top);
    const std::string name = sharedMidEdges ? "SharedMidEdges" : "DistinctMidEdges";
    Parameters corners;
    corners.MatchSidePoints = false;
    const vtkIdType expected = sharedMidEdges ? 10 : 12;
    ok &= Check(name + "/Corners", ug, F::SurfacesOfVolumes, S::Boundary, corners, 10);
    ok &= Check(name + "/Points", ug, F::SurfacesOfVolumes, S::Boundary, Parameters(), expected);
  }
  return ok;
}

// Two Lagrange hexahedra of orders 2 and 3, whose orders are given by their numbers of points,
// sharing the corners of a face, the top face of the first one and the bottom face of the second
// one: the faces match by their corners only, since their other points differ.
bool TestMatchHigherOrders()
{
  vtkNew<vtkPoints> points;
  for (int i = 0; i < 27 + 4 + 56; ++i)
  {
    points->InsertNextPoint(i, (i * i) % 7, (i * 3) % 5);
  }
  std::array<vtkIdType, 27> order2;
  std::iota(order2.begin(), order2.end(), 0);
  // The corners of the top face of the first one, the other corners, then the other points.
  std::array<vtkIdType, 64> order3;
  const vtkIdType sharedCorners[4] = { 4, 5, 6, 7 };
  std::copy_n(sharedCorners, 4, order3.begin());
  std::iota(order3.begin() + 4, order3.end(), 27);
  vtkNew<vtkUnstructuredGrid> ug;
  ug->SetPoints(points);
  ug->InsertNextCell(VTK_LAGRANGE_HEXAHEDRON, 27, order2.data());
  ug->InsertNextCell(VTK_LAGRANGE_HEXAHEDRON, 64, order3.data());
  Parameters corners;
  corners.MatchSidePoints = false;
  bool ok = Check("HigherOrders/Corners", ug, F::SurfacesOfVolumes, S::Boundary, corners, 10);
  ok &= Check("HigherOrders/Points", ug, F::SurfacesOfVolumes, S::Boundary, Parameters(), 12);
  return ok;
}

//------------------------------------------------------------------------------
// A triquadratic hexahedron and a linear one sharing the corners of a face, a biquadratic quad and
// a quad, which match by their corners only when the other points are not matched, then a
// polyhedron on the linear one, whose linear faces always match by their corners.
bool TestMatchSidePoints()
{
  vtkNew<vtkPoints> points;
  for (int i = 0; i < 31; ++i)
  {
    points->InsertNextPoint(i, (i * i) % 7, (i * 3) % 5);
  }
  vtkNew<vtkUnstructuredGrid> ug;
  ug->SetPoints(points);
  std::array<vtkIdType, 27> triQuadraticHex;
  std::iota(triQuadraticHex.begin(), triQuadraticHex.end(), 0);
  // Its bottom face (0, 3, 2, 1) is the top face (4, 5, 6, 7) of the triquadratic one.
  const vtkIdType hex[8] = { 4, 5, 6, 7, 27, 28, 29, 30 };
  ug->InsertNextCell(VTK_TRIQUADRATIC_HEXAHEDRON, 27, triQuadraticHex.data());
  ug->InsertNextCell(VTK_HEXAHEDRON, 8, hex);
  Parameters corners;
  corners.MatchSidePoints = false;
  bool ok = Check("Corners/Boundary", ug, F::SurfacesOfVolumes, S::Boundary, corners, 10);
  ok &= Check("Points/Boundary", ug, F::SurfacesOfVolumes, S::Boundary, Parameters(), 12);
  ok &= Check("Corners/AnyOccurrence", ug, F::SurfacesOfVolumes, S::AnyOccurrence, corners, 11);
  ok &= Check("Points/AnyOccurrence", ug, F::SurfacesOfVolumes, S::AnyOccurrence, Parameters(), 12);

  // A polyhedron on the linear hexahedron, whose top face (27, 28, 29, 30), a quad, is its bottom
  // face, a polygon: the linear faces match by their corners, also with the types matched.
  const vtkIdType polyhedron[8] = { 27, 28, 29, 30, 31, 32, 33, 34 };
  for (int i = 31; i < 35; ++i)
  {
    points->InsertNextPoint(i, (i * i) % 7, (i * 3) % 5);
  }
  vtkNew<vtkCellArray> faces;
  for (int f = 0; f < vtkHexahedron::NumberOfFaces; ++f)
  {
    const vtkIdType* face = vtkHexahedron::GetFaceArray(f);
    faces->InsertNextCell(4);
    for (int k = 0; k < 4; ++k)
    {
      faces->InsertCellPoint(polyhedron[face[k]]);
    }
  }
  ug->InsertNextCell(VTK_POLYHEDRON, 8, polyhedron, faces);
  ok &= Check("Polyhedron/Corners/Boundary", ug, F::SurfacesOfVolumes, S::Boundary, corners, 14);
  ok &=
    Check("Polyhedron/Points/Boundary", ug, F::SurfacesOfVolumes, S::Boundary, Parameters(), 16);
  ok &= Check(
    "Polyhedron/Corners/AnyOccurrence", ug, F::SurfacesOfVolumes, S::AnyOccurrence, corners, 16);
  ok &= Check("Polyhedron/Points/AnyOccurrence", ug, F::SurfacesOfVolumes, S::AnyOccurrence,
    Parameters(), 17);
  return ok && TestMatchMidEdgePoints() && TestMatchHigherOrders();
}

//------------------------------------------------------------------------------
// The edges of a tetrahedron and of 2 triangles with Boundary, which keeps the edges of the
// surfaces used once and all the edges of the volumes: the edge 0 1 of the tetrahedron and of a
// triangle is kept, and the edge 1 4 of the triangles cancels, whether the tetrahedron is the
// first cell or the last one.
bool TestMixedRules()
{
  vtkNew<vtkPoints> points;
  for (int i = 0; i < 6; ++i)
  {
    points->InsertNextPoint(i, (i * i) % 7, (i * 3) % 5);
  }
  const vtkIdType tetra[4] = { 0, 1, 2, 3 };
  const vtkIdType triangle0[3] = { 0, 1, 4 };
  const vtkIdType triangle1[3] = { 1, 4, 5 };
  const auto edges = static_cast<F>(F::EdgesOfSurfaces | F::EdgesOfVolumes);
  bool ok = true;
  for (const bool tetraFirst : { true, false })
  {
    vtkNew<vtkUnstructuredGrid> ug;
    ug->SetPoints(points);
    if (tetraFirst)
    {
      ug->InsertNextCell(VTK_TETRA, 4, tetra);
    }
    ug->InsertNextCell(VTK_TRIANGLE, 3, triangle0);
    ug->InsertNextCell(VTK_TRIANGLE, 3, triangle1);
    if (!tetraFirst)
    {
      ug->InsertNextCell(VTK_TETRA, 4, tetra);
    }
    const std::string name = tetraFirst ? "MixedRules/TetraFirst" : "MixedRules/TetraLast";
    ok &= Check(name, ug, edges, S::Boundary, Parameters(), 6 + 3);
  }
  return ok;
}

// The cell, local index and dimension of the side k of a key.
template <typename SidesT>
std::tuple<vtkIdType, int, int> GetSide(SidesT* sides, vtkIdType keyId, vtkIdType k)
{
  return { sides->GetCellIdOfSide(keyId, k), static_cast<int>(sides->GetSideIdOfSide(keyId, k)),
    sides->GetDimensionOfSide(keyId, k) };
}

// The points of the side k of a key, sorted, which are the same for the equal sides of the meshes
// of TestGroupedSides().
template <typename SidesT>
std::vector<vtkIdType> GetSortedSidePoints(SidesT* sides, vtkIdType keyId, vtkIdType k)
{
  vtkNew<vtkIdList> sidePoints;
  vtkNew<vtkIdList> tempCellPoints;
  vtkNew<vtkGenericCell> tempCell;
  unsigned char cellType = VTK_EMPTY_CELL;
  sides->GetSidePoints(keyId, k, sidePoints, cellType, tempCellPoints, tempCell);
  std::vector<vtkIdType> ids(sidePoints->begin(), sidePoints->end());
  std::sort(ids.begin(), ids.end());
  return ids;
}

// BuildGroupedSides() keeps the sides of BuildScatteredSides(), with the equal sides of every key
// contiguous, in groups: one group per side kept by AnyOccurrence, in the order of their first
// sides, the sides of a group in the order of their cells. The preserved cells are in no group.
template <typename SidesT>
bool CheckGroupedSidesOf(
  const std::string& name, vtkUnstructuredGrid* input, F flags, const Parameters& parameters)
{
  vtkNew<SidesT> scattered;
  Build(*scattered, input, flags, S::AnyOccurrence, parameters);
  scattered->BuildScatteredSides();
  vtkNew<SidesT> kept;
  Build(*kept, input, flags, S::AnyOccurrence, parameters);
  vtkNew<SidesT> grouped;
  Build(*grouped, input, flags, S::AnyOccurrence, parameters);
  grouped->BuildGroupedSides();
  const auto numKeys = static_cast<vtkIdType>(grouped->GetNumberOfKeys());
  const vtkIdType numPreserved = grouped->GetNumberOfSidesOfKey(numKeys - 1);
  if (grouped->GetBuildMode() != SidesT::BuildMode::Grouped ||
    grouped->GetNumberOfSides() != scattered->GetNumberOfSides() ||
    grouped->GetNumberOfGroups() != kept->GetNumberOfSides() - numPreserved)
  {
    std::cerr << name << ": " << grouped->GetNumberOfSides() << " sides in "
              << grouped->GetNumberOfGroups() << " groups\n";
    return false;
  }
  for (vtkIdType keyId = 0; keyId < numKeys; ++keyId)
  {
    // The sides of a key are those of the scattered key, in another order.
    const vtkIdType numSides = grouped->GetNumberOfSidesOfKey(keyId);
    std::vector<std::tuple<vtkIdType, int, int>> scatteredSides;
    std::vector<std::tuple<vtkIdType, int, int>> groupedSides;
    for (vtkIdType k = 0; k < numSides && numSides == scattered->GetNumberOfSidesOfKey(keyId); ++k)
    {
      scatteredSides.push_back(GetSide(scattered.Get(), keyId, k));
      groupedSides.push_back(GetSide(grouped.Get(), keyId, k));
    }
    std::vector<std::tuple<vtkIdType, int, int>> sortedSides = groupedSides;
    std::sort(sortedSides.begin(), sortedSides.end());
    std::vector<std::tuple<vtkIdType, int, int>> sortedScatteredSides = scatteredSides;
    std::sort(sortedScatteredSides.begin(), sortedScatteredSides.end());
    if (static_cast<vtkIdType>(groupedSides.size()) != numSides ||
      sortedSides != sortedScatteredSides)
    {
      std::cerr << name << ": the sides of key " << keyId << " are not the scattered ones\n";
      return false;
    }
    // The groups of the key cover its sides, one after the other, with distinct sides, in the
    // order of their first sides among the scattered sides.
    const vtkIdType firstGroup = grouped->GetGroupOffsetOfKey(keyId);
    const vtkIdType endGroup = grouped->GetGroupOffsetOfKey(keyId + 1);
    if (keyId == numKeys - 1 &&
      (firstGroup != endGroup || endGroup != grouped->GetNumberOfGroups()))
    {
      std::cerr << name << ": the preserved cells are in groups\n";
      return false;
    }
    const vtkIdType keyOffset = grouped->GetSideOffsetOfKey(keyId);
    vtkIdType first = 0;
    std::ptrdiff_t lastFirstSide = -1;
    std::vector<std::vector<vtkIdType>> groupPoints;
    for (vtkIdType group = firstGroup; group < endGroup; ++group)
    {
      const vtkIdType size = grouped->GetNumberOfSidesOfGroup(group);
      if (grouped->GetSideOffsetOfGroup(group) != keyOffset + first || size < 1 ||
        first + size > numSides)
      {
        std::cerr << name << ": wrong sides of group " << group << " of key " << keyId << "\n";
        return false;
      }
      const std::vector<vtkIdType> points = GetSortedSidePoints(grouped.Get(), keyId, first);
      const std::ptrdiff_t firstSide =
        std::find(scatteredSides.begin(), scatteredSides.end(), groupedSides[first]) -
        scatteredSides.begin();
      if (firstSide <= lastFirstSide ||
        std::find(groupPoints.begin(), groupPoints.end(), points) != groupPoints.end())
      {
        std::cerr << name << ": group " << group << " of key " << keyId
                  << " is out of order or equal to another one\n";
        return false;
      }
      // The sides of the group, read without its key.
      for (vtkIdType i = 0; i < size; ++i)
      {
        if (grouped->GetCellIdsOfGroup(group)[i] != grouped->GetCellIdOfSide(keyId, first + i) ||
          grouped->GetSideIdsOfGroup(group)[i] != grouped->GetSideIdOfSide(keyId, first + i) ||
          grouped->GetDimensionOfGroup(group) != grouped->GetDimensionOfSide(keyId, first + i))
        {
          std::cerr << name << ": side " << i << " of group " << group << " is not its key's\n";
          return false;
        }
      }
      for (vtkIdType k = first + 1; k < first + size; ++k)
      {
        if (GetSortedSidePoints(grouped.Get(), keyId, k) != points ||
          grouped->GetCellIdOfSide(keyId, k) < grouped->GetCellIdOfSide(keyId, k - 1))
        {
          std::cerr << name << ": side " << k << " of key " << keyId << " is not in its group\n";
          return false;
        }
      }
      groupPoints.push_back(points);
      lastFirstSide = firstSide;
      first += size;
    }
    if (keyId < numKeys - 1 && first != numSides)
    {
      std::cerr << name << ": the groups of key " << keyId << " have " << first << " sides\n";
      return false;
    }
  }
  if (grouped->GetSideOffsetOfGroup(grouped->GetNumberOfGroups()) !=
    grouped->GetNumberOfSides() - numPreserved)
  {
    std::cerr << name << ": the groups do not end with the found sides\n";
    return false;
  }
  return true;
}

// CheckGroupedSidesOf() with the 4 instantiations of vtkUnstructuredGridSides.
bool CheckGroupedSides(const std::string& name, vtkUnstructuredGrid* input, F flags,
  const Parameters& parameters = Parameters())
{
  return CheckGroupedSidesOf<vtkUnstructuredGridSides<vtkTypeInt32, vtkTypeInt8>>(
           name + " <Int32, Int8>", input, flags, parameters) &&
    CheckGroupedSidesOf<vtkUnstructuredGridSides<vtkTypeInt32, vtkTypeInt32>>(
      name + " <Int32, Int32>", input, flags, parameters) &&
    CheckGroupedSidesOf<vtkUnstructuredGridSides<vtkTypeInt64, vtkTypeInt8>>(
      name + " <Int64, Int8>", input, flags, parameters) &&
    CheckGroupedSidesOf<vtkUnstructuredGridSides<vtkTypeInt64, vtkTypeInt32>>(
      name + " <Int64, Int32>", input, flags, parameters);
}

// n tetrahedra around the axis from point 0 to point 1, so that point 0 is the smallest point of
// 3 n faces, more than the sides of a key sorted by insertion, see SortSidesByCell.
vtkSmartPointer<vtkUnstructuredGrid> CreateFan(int n)
{
  auto ug = vtkSmartPointer<vtkUnstructuredGrid>::New();
  vtkNew<vtkPoints> points;
  points->InsertNextPoint(0, 0, 0);
  points->InsertNextPoint(0, 0, 1);
  for (int i = 0; i < n; ++i)
  {
    const double angle = 2 * vtkMath::Pi() * i / n;
    points->InsertNextPoint(std::cos(angle), std::sin(angle), 0.5);
  }
  ug->SetPoints(points);
  for (int i = 0; i < n; ++i)
  {
    const vtkIdType tetra[4] = { 0, 1, 2 + i, 2 + (i + 1) % n };
    ug->InsertNextCell(VTK_TETRA, 4, tetra);
  }
  return ug;
}

//------------------------------------------------------------------------------
// The grouped sides: of 2 hexahedra, whose shared face is the group (0, 1), (1, 0), of a block with
// sides of several dimensions, of a face shared by 3 cells, with preserved cells, and of a key of
// many sides. The sides are built again to build them in another way.
bool TestGroupedSides()
{
  auto two = CreateHexGrid(2, 1, 1);
  bool ok = CheckGroupedSides("Grouped/Two", two, F::SurfacesOfVolumes);
  vtkNew<Sides> grouped;
  grouped->SetDataSet(two);
  grouped->SetOutputDimensionControl(F::SurfacesOfVolumes);
  grouped->BuildGroupedSides();
  bool foundShared = false;
  for (vtkIdType keyId = 0; keyId < grouped->GetNumberOfKeys(); ++keyId)
  {
    for (vtkIdType group = grouped->GetGroupOffsetOfKey(keyId);
         group < grouped->GetGroupOffsetOfKey(keyId + 1); ++group)
    {
      const vtkIdType k = grouped->GetSideOffsetOfGroup(group) - grouped->GetSideOffsetOfKey(keyId);
      if (grouped->GetNumberOfSidesOfGroup(group) == 2)
      {
        foundShared = GetSide(grouped.Get(), keyId, k) == std::make_tuple(vtkIdType(0), 1, 2) &&
          GetSide(grouped.Get(), keyId, k + 1) == std::make_tuple(vtkIdType(1), 0, 2);
      }
    }
  }
  if (grouped->GetNumberOfGroups() != 11 || !foundShared)
  {
    std::cerr << "Grouped/Two: the shared face is not the group (0, 1), (1, 0)\n";
    ok = false;
  }
  const vtkMTimeType groupedTime = grouped->GetBuildTime();
  grouped->BuildGroupedSides();
  const bool kept = grouped->GetBuildTime() == groupedTime;
  grouped->BuildReducedSides();
  if (!kept || grouped->GetBuildMode() != Sides::BuildMode::Reduced ||
    grouped->GetNumberOfGroups() != 0 || grouped->GetGroupOffsetOfKey(1) != 0 ||
    grouped->GetNumberOfSides() != 10)
  {
    std::cerr << "Grouped/Rebuild: the sides are not built again in another way\n";
    ok = false;
  }

  auto block = CreateHexGrid(4, 3, 2);
  ok &= CheckGroupedSides("Grouped/Block", block, F::SurfacesOfVolumes);
  ok &= CheckGroupedSides("Grouped/Edges", block, F::EdgesOfVolumes);
  ok &= CheckGroupedSides("Grouped/AllSides", block, F::AllSides);

  auto nonManifold = CreateHexGrid(2, 1, 1);
  vtkNew<vtkIdList> hex;
  nonManifold->GetCellPoints(1, hex);
  nonManifold->InsertNextCell(VTK_HEXAHEDRON, hex);
  ok &= CheckGroupedSides("Grouped/NonManifold", nonManifold, F::SurfacesOfVolumes);

  Parameters preserve;
  preserve.PreserveRenderableInputs = true;
  ok &= CheckGroupedSides("Grouped/Preserve", CreateQuads(), F::EdgesOfSurfaces, preserve);

  auto fan = CreateFan(200);
  ok &= CheckGroupedSides("Grouped/Fan", fan, F::SurfacesOfVolumes);
  ok &= CheckGroupedSides("Grouped/FanAllSides", fan, F::AllSides);
  return ok;
}
} // namespace

int TestUnstructuredGridSides(int, char*[])
{
  vtkNew<vtkUnstructuredGrid> empty;
  vtkNew<Sides> sides;
  sides->SetDataSet(empty);
  sides->BuildReducedSides();
  if (sides->GetNumberOfSides() != 0)
  {
    std::cerr << "Empty: expected no side\n";
    return EXIT_FAILURE;
  }
  return TestHexahedra() && TestQuads() && TestNonManifold() && TestMasking() &&
      TestStorageAndPolyhedron() && TestQuadraticCells() && TestSideTypes() &&
      TestDegenerateSides() && TestMatchSidePoints() && TestMixedRules() && TestGroupedSides()
    ? EXIT_SUCCESS
    : EXIT_FAILURE;
}
