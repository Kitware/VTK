// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellGrid.h"
#include "vtkCellGridComputeSurface.h"
#include "vtkCellGridReader.h"
#include "vtkDGHex.h"
#include "vtkDGTet.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkIntArray.h"
#include "vtkNew.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"

namespace
{

bool LoadAndExtractSurface(const char* filename, const std::string& sideAttributes,
  const std::set<std::array<vtkIdType, 2>>& expected)
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
  auto* sidesOut =
    vtkIdTypeArray::SafeDownCast(dataWithSides->GetAttributes(sideAttributes)->GetScalars());
  bool ok = true;
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

} // anonymous namespace

int TestCellGridExtractSurface(int argc, char* argv[])
{
  if (!LoadAndExtractSurface(
        // vtkTestUtilities::ExpandDataFileName(argc, argv,
        // "Common/DataModel/Testing/Data/Input/dgHexahedra.dg", 0),
        vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/dgHexahedra.dg", 0),
        // VTK_DATA_ROOT "/Data/dgHexahedra.dg",
        "quadrilateral sides of DGHex",
        { { 0, 0 }, { 0, 2 }, { 0, 3 }, { 0, 4 }, { 0, 5 }, { 1, 1 }, { 1, 2 }, { 1, 3 }, { 1, 4 },
          { 1, 5 } }))
  {
    return EXIT_FAILURE;
  }

  if (!LoadAndExtractSurface(
        // vtkTestUtilities::ExpandDataFileName(argc, argv,
        // "Common/DataModel/Testing/Data/Input/dgTetrahedra.dg", 0),
        vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/dgTetrahedra.dg", 0),
        // VTK_DATA_ROOT "/Data/dgTetrahedra.dg",
        "triangle sides of DGTet", { { 0, 0 }, { 0, 2 }, { 0, 3 }, { 1, 1 }, { 1, 2 }, { 1, 3 } }))
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
