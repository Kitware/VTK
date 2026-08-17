// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellAttribute.h"
#include "vtkCellGrid.h"
#include "vtkCellGridComputeSides.h"
#include "vtkDGHex.h"
#include "vtkDGPyr.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkFiltersCellGrid.h"
#include "vtkIdTypeArray.h"
#include "vtkNew.h"
#include "vtkSmartPointer.h"
#include "vtkStringToken.h"

#include <array>
#include <iostream>
#include <vector>

namespace
{
using namespace vtk::literals;

vtkSmartPointer<vtkDoubleArray> MakeCoords(
  const char* name, const std::vector<std::array<double, 3>>& pts)
{
  auto coords = vtkSmartPointer<vtkDoubleArray>::New();
  coords->SetName(name);
  coords->SetNumberOfComponents(3);
  coords->SetNumberOfTuples(static_cast<vtkIdType>(pts.size()));
  for (std::size_t ii = 0; ii < pts.size(); ++ii)
  {
    coords->SetTuple(static_cast<vtkIdType>(ii), pts[ii].data());
  }
  return coords;
}

// Convenience method to add a cell to the grid. coordsGroup is used as the name
// of the grid attributes and the shapeInfo's "DOFSharing". So two cells created
// using the same coordsGroup will share coordinates.
template <typename CellType>
void AddCell(vtkCellGrid* grid, vtkCellAttribute* shape, const std::string& coordsGroup,
  vtkDoubleArray* coords, const std::vector<vtkIdType>& connIds)
{
  auto* cell = grid->AddCellMetadata<CellType>();
  vtkStringToken cellTypeToken = cell->GetClassName();
  vtkStringToken coordsGroupToken(coordsGroup);
  const int npts = static_cast<int>(connIds.size());

  vtkNew<vtkIdTypeArray> conn;
  conn->SetName("connectivity");
  conn->SetNumberOfComponents(npts);
  conn->SetNumberOfTuples(1);
  conn->SetTypedTuple(0, connIds.data());

  // Connectivity lives in a per-type group (named for the cell type).
  grid->GetAttributes(cellTypeToken)->SetScalars(conn);
  cell->GetCellSpec().Connectivity = conn;

  // Coordinates live in `coordsGroup`; storing the same array twice is harmless.
  grid->GetAttributes(coordsGroupToken)->SetScalars(coords);

  vtkCellAttribute::CellTypeInfo shapeInfo;
  shapeInfo.DOFSharing = coordsGroupToken;
  shapeInfo.FunctionSpace = "HGRAD"_token;
  shapeInfo.Basis = "C"_token;
  shapeInfo.Order = 1;
  shapeInfo.ArraysByRole["connectivity"_token] = conn;
  shapeInfo.ArraysByRole["values"_token] = coords;
  shape->SetCellTypeInfo(cellTypeToken, shapeInfo);
}

// Get the number of sides associated with the named group
vtkIdType SideCount(vtkCellGrid* grid, const std::string& sideGroup)
{
  auto* atts = grid->GetAttributes(vtkStringToken(sideGroup));
  if (!atts)
  {
    return 0;
  }
  auto* arr = atts->GetScalars();
  return arr ? arr->GetNumberOfTuples() : 0;
}

// Standard unit-cube hexahedron corners
const std::vector<std::array<double, 3>> kHexCube{ { { 0, 0, 0 }, { 1, 0, 0 }, { 1, 1, 0 },
  { 0, 1, 0 }, { 0, 0, 1 }, { 1, 0, 1 }, { 1, 1, 1 }, { 0, 1, 1 } } };

// Report whether the extracted side counts match
bool Check(vtkCellGrid* out, const char* caseName,
  const std::vector<std::pair<std::string, vtkIdType>>& expected)
{
  bool ok = true;
  std::cout << caseName << ":\n";
  for (const auto& e : expected)
  {
    vtkIdType got = SideCount(out, e.first);
    std::cout << "  " << e.first << ": got " << got << ", expected " << e.second << "\n";
    if (got != e.second)
    {
      std::cerr << "    ERROR: expected " << e.second << ".\n";
      ok = false;
    }
  }
  return ok;
}

// Test a hex and a pyramid with separate coordinate arrays and no shared face. Make
// sure that no faces are dropped from either cell.
bool TestSeparateArraysNotDropped()
{
  vtkNew<vtkCellGrid> grid;
  vtkNew<vtkCellAttribute> shape;
  shape->Initialize("shape"_token, "ℝ³", 3);

  auto hexCoords = MakeCoords("pointsHex", kHexCube);

  // Translated pyramid has none of the same physical points as the hex
  auto pyrCoords = MakeCoords(
    "pointsPyr", { { 10, 0, 0 }, { 11, 0, 0 }, { 11, 1, 0 }, { 10, 1, 0 }, { 10.5, 0.5, 1 } });

  AddCell<vtkDGHex>(grid, shape, "pointsHex", hexCoords, { 0, 1, 2, 3, 4, 5, 6, 7 });
  AddCell<vtkDGPyr>(grid, shape, "pointsPyr", pyrCoords, { 0, 1, 2, 3, 4 });
  grid->SetShapeAttribute(shape);

  vtkNew<vtkCellGridComputeSides> sides;
  sides->SetInputDataObject(grid);
  sides->OmitSidesForRenderableInputsOff();
  sides->SetOutputDimensionControl(vtkCellGridComputeSides::SideFlags::NextLowestDimension);
  sides->Update();
  auto* out = vtkCellGrid::SafeDownCast(sides->GetOutputDataObject(0));

  // Check we get the expected face counts for each side group
  return Check(out, "Separate arrays (no face should be dropped)",
    { { "quadrilateral sides of vtkDGHex", 6 }, { "quadrilateral sides of vtkDGPyr", 1 },
      { "triangle sides of vtkDGPyr", 4 } });
}

// Test a hex and a pyramid sharing a coordinate array, with the pyramid's base
// coincident with the hex's top face. That shared quad is an interior face and
// should be dropped.
bool TestSharedFaceStillCulled()
{
  vtkNew<vtkCellGrid> grid;
  vtkNew<vtkCellAttribute> shape;
  shape->Initialize("shape"_token, "ℝ³", 3);

  // Shared point array holds 8 hex corners and a the pyramid's apex
  auto pts = kHexCube;
  pts.push_back({ 0.5, 0.5, 2 }); // apex, point id 8
  auto shared = MakeCoords("points", pts);

  // The pyramid base and the top face of the hex share {4,5,6,7}, the apex of
  // the pyramid is 8.
  AddCell<vtkDGHex>(grid, shape, "points", shared, { 0, 1, 2, 3, 4, 5, 6, 7 });
  AddCell<vtkDGPyr>(grid, shape, "points", shared, { 4, 5, 6, 7, 8 });
  grid->SetShapeAttribute(shape);

  vtkNew<vtkCellGridComputeSides> sides;
  sides->SetInputDataObject(grid);
  sides->OmitSidesForRenderableInputsOff();
  sides->SetOutputDimensionControl(vtkCellGridComputeSides::SideFlags::NextLowestDimension);
  sides->Update();
  auto* out = vtkCellGrid::SafeDownCast(sides->GetOutputDataObject(0));

  // Validate expected face counts
  return Check(out, "Shared array (genuine shared face must be culled)",
    { { "quadrilateral sides of vtkDGHex", 5 }, { "quadrilateral sides of vtkDGPyr", 0 },
      { "triangle sides of vtkDGPyr", 4 } });
}

} // anonymous namespace

int TestCellGridHeterogeneousSides(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  vtkFiltersCellGrid::RegisterCellsAndResponders();

  bool ok = true;
  ok &= TestSeparateArraysNotDropped();
  ok &= TestSharedFaceStillCulled();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
