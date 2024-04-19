// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellGrid.h"
#include "vtkCellGridComputeSurface.h"
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

namespace
{

bool CheckSides(vtkCellGrid* dataWithSides, const std::string& sideAttributes,
  const std::set<std::array<vtkIdType, 2>>& expected)
{
  bool ok = true;
  if (sideAttributes.empty() || expected.empty())
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

bool LoadAndExtractSurface(const char* filename, const std::string& sideAttributes,
  const std::set<std::array<vtkIdType, 2>>& expected, const std::string& sideAttributes2 = "",
  const std::set<std::array<vtkIdType, 2>>& expected2 = {})
{
  if (!filename)
  {
    return false;
  }

  vtkNew<vtkCellGridReader> reader;
  reader->SetFileName(filename);
  reader->Update();
  delete[] filename;

  vtkNew<vtkCellGridComputeSurface> extractSurface;
  extractSurface->SetInputConnection(reader->GetOutputPort());
  extractSurface->Update();

  auto* dataWithSides = vtkCellGrid::SafeDownCast(extractSurface->GetOutputDataObject(0));

  // First, check that the number of cells is equal to the number of sides.
  // NB: This ensures that the source cells themselves are blanked as the sides
  //     are added so that the output dataset only reports the sides.
  bool ok = (dataWithSides->GetNumberOfCells() ==
    static_cast<vtkIdType>((expected.size() + expected2.size())));
  if (!ok)
  {
    std::cerr << "ERROR: Have " << dataWithSides->GetNumberOfCells() << " cells, expected "
              << (expected.size() + expected2.size()) << ".\n";
  }

  // Now test that the sides are the ones we expect by extracting the arrays
  // holding sides by name. This requires special knowledge of the way DG cells
  // are represented and is not intended to work in general (i.e. for other cell
  // types).
  ok &= CheckSides(dataWithSides, sideAttributes, expected);
  ok &= CheckSides(dataWithSides, sideAttributes2, expected2);
  return ok;
}

} // anonymous namespace

int TestCellGridExtractSurface(int argc, char* argv[])
{
  if (!LoadAndExtractSurface(
        vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/dgPyramids.dg", 0),
        "quadrilateral sides of vtkDGPyr", { { 0, 4 }, { 1, 4 } }, "triangle sides of vtkDGPyr",
        { { 0, 0 }, { 0, 2 }, { 0, 3 }, { 1, 0 }, { 1, 1 }, { 1, 2 } }))
  {
    return EXIT_FAILURE;
  }

  if (!LoadAndExtractSurface(
        vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/dgWedges.dg", 0),
        "quadrilateral sides of vtkDGWdg", { { 0, 0 }, { 0, 2 }, { 1, 0 }, { 1, 1 } },
        "triangle sides of vtkDGWdg", { { 0, 3 }, { 0, 4 }, { 1, 3 }, { 1, 4 } }))
  {
    return EXIT_FAILURE;
  }

  if (!LoadAndExtractSurface(
        vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/dgHexahedra.dg", 0),
        "quadrilateral sides of vtkDGHex",
        { { 0, 0 }, { 0, 2 }, { 0, 3 }, { 0, 4 }, { 0, 5 }, { 1, 0 }, { 1, 1 }, { 1, 2 }, { 1, 4 },
          { 1, 5 } }))
  {
    return EXIT_FAILURE;
  }

  if (!LoadAndExtractSurface(
        vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/dgTetrahedra.dg", 0),
        "triangle sides of vtkDGTet",
        { { 0, 0 }, { 0, 2 }, { 0, 3 }, { 1, 0 }, { 1, 1 }, { 1, 3 } }))
  {
    return EXIT_FAILURE;
  }

  if (!LoadAndExtractSurface(
        vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/dgQuadrilateral.dg", 0),
        "edge sides of vtkDGQuad", { { 0, 0 }, { 0, 2 }, { 0, 3 }, { 1, 0 }, { 1, 1 }, { 1, 2 } }))
  {
    return EXIT_FAILURE;
  }

  if (!LoadAndExtractSurface(
        vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/dgTriangle.dg", 0),
        "edge sides of vtkDGTri", { { 0, 0 }, { 0, 2 }, { 1, 1 }, { 1, 2 } }))
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
