// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellGrid.h"
#include "vtkCellGridComputeSides.h"
#include "vtkCellGridReader.h"
#include "vtkDGHex.h"
#include "vtkDGTet.h"
#include "vtkDGTri.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkIntArray.h"
#include "vtkNew.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"

using ExpectSides = std::set<std::array<vtkIdType, 2>>;

namespace
{

bool CheckSides(
  vtkCellGrid* dataWithSides, const std::string& sideAttributes, const ExpectSides& expected)
{
  bool ok = true;
  if (sideAttributes.empty())
  {
    return ok;
  }

  auto* sidesOut =
    vtkIdTypeArray::SafeDownCast(dataWithSides->GetAttributes(sideAttributes)->GetScalars());
  std::cout << sideAttributes << ":\n";
  for (vtkIdType ii = 0; ii < sidesOut->GetNumberOfTuples(); ++ii)
  {
    std::array<vtkIdType, 2> xx;
    sidesOut->GetTypedTuple(ii, xx.data());
    std::cout << "  " << ii << ": " << xx[0] << " " << xx[1] << "\n";
    if (expected.find(xx) == expected.end())
    {
      std::cerr << "    ERROR: Unexpected side.\n";
      ok = false;
    }
  }
  if (expected.size() != static_cast<std::size_t>(sidesOut->GetNumberOfTuples()))
  {
    std::cerr << "  ERROR: Expected " << expected.size() << " sides.\n";
    ok = false;
  }
  return ok;
}

// clang-format off
bool LoadAndExtractSurface(
  const char* filename, int outputDimensionControl,
  const std::string& sideGroup1,      const ExpectSides& expected1,
  const std::string& sideGroup2 = "", const ExpectSides& expected2 = {},
  const std::string& sideGroup3 = "", const ExpectSides& expected3 = {})
// clang-format on
{
  vtkIndent indent;
  if (!filename)
  {
    return false;
  }
  std::cout << "Sides of " << filename << " with flags " << outputDimensionControl << "\n";

  vtkNew<vtkCellGridReader> reader;
  reader->SetFileName(filename);
  reader->Update();
  delete[] filename;

  vtkNew<vtkCellGridComputeSides> extractSurface;
  extractSurface->SetInputConnection(reader->GetOutputPort());
  extractSurface->OmitSidesForRenderableInputsOff();
  extractSurface->SetOutputDimensionControl(outputDimensionControl);
  extractSurface->PrintSelf(std::cout, indent);
  extractSurface->Update();

  auto* dataWithSides = vtkCellGrid::SafeDownCast(extractSurface->GetOutputDataObject(0));

  auto totalExpected =
    static_cast<vtkIdType>(expected1.size() + expected2.size() + expected3.size());
  // First, check that the number of cells is equal to the number of sides.
  // NB: This ensures that the source cells themselves are blanked as the sides
  //     are added so that the output dataset only reports the sides.
  // clang-format off
  bool ok = (dataWithSides->GetNumberOfCells() == totalExpected);
  // clang-format on
  if (!ok)
  {
    std::cerr << "ERROR: Have " << dataWithSides->GetNumberOfCells() << " cells, expected "
              << totalExpected << ".\n";
  }

  // Now test that the sides are the ones we expect by extracting the arrays
  // holding sides by name. This requires special knowledge of the way DG cells
  // are represented and is not intended to work in general (i.e. for other cell
  // types).
  ok &= CheckSides(dataWithSides, sideGroup1, expected1);
  ok &= CheckSides(dataWithSides, sideGroup2, expected2);
  ok &= CheckSides(dataWithSides, sideGroup3, expected3);
  return ok;
}

// clang-format off
bool LoadAndExtractSidesOfSurface(
  const char* filename, int outputDimensionControl,
  const std::string& sideGroup1,      const ExpectSides& expected1,
  const std::string& sideGroup2 = "", const ExpectSides& expected2 = {},
  const std::string& sideGroup3 = "", const ExpectSides& expected3 = {})
// clang-format on
{
  vtkIndent indent;
  if (!filename)
  {
    return false;
  }
  std::cout << "Sides of " << filename << " with flags " << outputDimensionControl << "\n";

  vtkNew<vtkCellGridReader> reader;
  reader->SetFileName(filename);
  reader->Update();
  delete[] filename;

  vtkNew<vtkCellGridComputeSides> extractSurface;
  extractSurface->SetInputConnection(reader->GetOutputPort());
  extractSurface->OmitSidesForRenderableInputsOff();
  extractSurface->SetOutputDimensionControl(outputDimensionControl);
  extractSurface->PrintSelf(std::cout, indent);
  extractSurface->Update();

  vtkNew<vtkCellGridComputeSides> extractSurfaceSides;
  extractSurfaceSides->SetInputConnection(extractSurface->GetOutputPort());
  extractSurfaceSides->PreserveRenderableInputsOff();
  extractSurfaceSides->OmitSidesForRenderableInputsOff();
  extractSurfaceSides->SetOutputDimensionControl(outputDimensionControl);
  extractSurfaceSides->PrintSelf(std::cout, indent);
  extractSurfaceSides->Update();

  auto* dataWithSides = vtkCellGrid::SafeDownCast(extractSurfaceSides->GetOutputDataObject(0));

  auto totalExpected =
    static_cast<vtkIdType>(expected1.size() + expected2.size() + expected3.size());
  // First, check that the number of cells is equal to the number of sides.
  // NB: This ensures that the source cells themselves are blanked as the sides
  //     are added so that the output dataset only reports the sides.
  // clang-format off
  bool ok = (dataWithSides->GetNumberOfCells() == totalExpected);
  // clang-format on
  if (!ok)
  {
    std::cerr << "ERROR: Have " << dataWithSides->GetNumberOfCells() << " cells, expected "
              << totalExpected << ".\n";
  }

  // Now test that the sides are the ones we expect by extracting the arrays
  // holding sides by name. This requires special knowledge of the way DG cells
  // are represented and is not intended to work in general (i.e. for other cell
  // types).
  ok &= CheckSides(dataWithSides, sideGroup1, expected1);
  ok &= CheckSides(dataWithSides, sideGroup2, expected2);
  ok &= CheckSides(dataWithSides, sideGroup3, expected3);
  return ok;
}

} // anonymous namespace

int TestCellGridExtractSurface(int argc, char* argv[])
{
  // clang-format off

  // Test computing **all** sides of input cells, not just the (d-1)-dimensional boundaries.
  if (
    !LoadAndExtractSurface(
      vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/dgHexahedra.dg", 0),
      vtkCellGridComputeSides::SideFlags::AllSides,
      "quadrilateral sides of vtkDGHex", {
      { 0, 0 }, { 0, 2 }, { 0, 3 }, { 0, 4 }, { 0, 5 }, { 1, 0 }, { 1, 1 }, { 1, 2 }, { 1, 4 }, { 1, 5 } },
      "edge sides of vtkDGHex", {
      { 0,  6 }, { 0,  7 }, { 0,  8 }, { 0,  9 }, { 0, 10 },
      { 0, 11 }, { 0, 12 }, { 0, 13 }, { 0, 14 }, { 0, 15 },
      { 0, 16 }, { 0, 17 }, { 1,  6 }, { 1,  7 }, { 1,  8 },
      { 1, 11 }, { 1, 13 }, { 1, 14 }, { 1, 15 }, { 1, 16 } },
      "vertex sides of vtkDGHex", {
      { 0, 18 }, { 0, 19 }, { 0, 20 }, { 0, 21 },
      { 0, 22 }, { 0, 23 }, { 0, 24 }, { 0, 25 },
      { 1, 19 }, { 1, 20 }, { 1, 23 }, { 1, 24 } }
  ))
  {
    return EXIT_FAILURE;
  }

  // Test computing (d-1)-dimensional boundaries of input cells.
  // Each shape has a separate test.
  if (!LoadAndExtractSurface(
        vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/dgPyramids.dg", 0),
        vtkCellGridComputeSides::SideFlags::NextLowestDimension,
        "quadrilateral sides of vtkDGPyr", { { 0, 4 }, { 1, 4 } }, "triangle sides of vtkDGPyr",
        { { 0, 0 }, { 0, 2 }, { 0, 3 }, { 1, 0 }, { 1, 1 }, { 1, 2 } }))
  {
    return EXIT_FAILURE;
  }

  if (!LoadAndExtractSurface(
        vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/dgWedges.dg", 0),
        vtkCellGridComputeSides::SideFlags::NextLowestDimension,
        "quadrilateral sides of vtkDGWdg", { { 0, 0 }, { 0, 2 }, { 1, 0 }, { 1, 1 } },
        "triangle sides of vtkDGWdg", { { 0, 3 }, { 0, 4 }, { 1, 3 }, { 1, 4 } }))
  {
    return EXIT_FAILURE;
  }

  if (!LoadAndExtractSurface(
        vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/dgHexahedra.dg", 0),
        vtkCellGridComputeSides::SideFlags::NextLowestDimension,
        "quadrilateral sides of vtkDGHex",
        { { 0, 0 }, { 0, 2 }, { 0, 3 }, { 0, 4 }, { 0, 5 }, { 1, 0 }, { 1, 1 }, { 1, 2 }, { 1, 4 },
          { 1, 5 } }))
  {
    return EXIT_FAILURE;
  }

  if (!LoadAndExtractSurface(
        vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/dgTetrahedra.dg", 0),
        vtkCellGridComputeSides::SideFlags::NextLowestDimension,
        "triangle sides of vtkDGTet",
        { { 0, 0 }, { 0, 2 }, { 0, 3 }, { 1, 0 }, { 1, 1 }, { 1, 3 } }))
  {
    return EXIT_FAILURE;
  }

  if (!LoadAndExtractSurface(
        vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/dgQuadrilateral.dg", 0),
        vtkCellGridComputeSides::SideFlags::NextLowestDimension,
        "edge sides of vtkDGQuad", { { 0, 0 }, { 0, 2 }, { 0, 3 }, { 1, 0 }, { 1, 1 }, { 1, 2 } }))
  {
    return EXIT_FAILURE;
  }

  if (!LoadAndExtractSurface(
        vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/dgTriangle.dg", 0),
        vtkCellGridComputeSides::SideFlags::NextLowestDimension,
        "edge sides of vtkDGTri", { { 0, 0 }, { 0, 2 }, { 1, 1 }, { 1, 2 } }))
  {
    return EXIT_FAILURE;
  }

  // dgEdges is a polyline that wraps back onto itself.
  // Vertices 0 (cell 0, side 0) and 2 (cell 1, side 1) are repeated 3 times in the connectivity
  // while all other vertices are repeated exactly twice and are thus rejected as "external sides."
  if (!LoadAndExtractSurface(
        vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/dgEdges.dg", 0),
        vtkCellGridComputeSides::SideFlags::NextLowestDimension,
        "vertex sides of vtkDGEdge", { { 0, 0 }, { 1, 1 } }))
  {
    return EXIT_FAILURE;
  }

  // Test computing sides of sides (i.e., extracting the boundary surface and then
  // extracting all sides of surface sides).
  if (!LoadAndExtractSidesOfSurface(
        vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/dgWedges.dg", 0),
        vtkCellGridComputeSides::SideFlags::NextLowestDimension,
        "edge sides of vtkDGWdg", {
        { 0,  5 }, { 0,  6 }, { 0,  7 }, { 0,  8 },
        { 0,  9 }, { 0, 10 }, { 0, 11 }, { 0, 12 },
        { 0, 13 },
        { 1,  5 }, { 1,  6 }, { 1,  9 }, { 1, 11 },
        { 1, 12 }
        },
        "vertex sides of vtkDGWdg", {
        { 0, 14 }, { 0, 15 }, { 0, 16 },
        { 0, 17 }, { 0, 18 }, { 0, 19 },
        { 1, 15 }, { 1, 18 }
        }))
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
  // clang-format on
}
