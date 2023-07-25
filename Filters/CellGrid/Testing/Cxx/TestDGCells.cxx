// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellGrid.h"
#include "vtkDGHex.h"
#include "vtkDGTet.h"
#include "vtkNew.h"
#include "vtkSmartPointer.h"
#include "vtkTypeFloat32Array.h"
#include "vtkTypeInt32Array.h"

namespace
{

template <typename CellType>
bool TestDGCellType()
{
  vtkNew<vtkCellGrid> grid;
  auto cell = vtkCellMetadata::NewInstance<CellType>(grid);
  if (!cell)
  {
    return false;
  }

  std::cout << "Created " << cell->GetClassName() << " metadata:\n";

  if (cell->GetNumberOfCells() != 0)
  {
    std::cerr << "ERROR: Expected 0 cells present, found " << cell->GetNumberOfCells() << ".\n";
    return false;
  }

  auto refPts = cell->GetReferencePoints();
  if (!refPts || refPts->GetNumberOfTuples() != cell->GetNumberOfCorners())
  {
    std::cerr << "ERROR: Expected " << cell->GetNumberOfCorners() << ", got "
              << (refPts ? refPts->GetNumberOfTuples() : -1) << "\n";
    return false;
  }
  if (cell->GetNumberOfCorners() != vtkDGCell::GetShapeCornerCount(cell->GetShape()))
  {
    std::cerr << "ERROR: Mismatched corner counts " << cell->GetNumberOfCorners() << " vs. "
              << vtkDGCell::GetShapeCornerCount(cell->GetShape()) << "\n";
    return false;
  }
  std::cout << "  " << refPts->GetNumberOfTuples() << " reference points:\n";
  for (vtkIdType ii = 0; ii < refPts->GetNumberOfTuples(); ++ii)
  {
    std::array<double, 3> coords;
    refPts->GetTuple(ii, coords.data());
    std::cout << "    " << ii << ": " << coords[0] << " " << coords[1] << " " << coords[2] << "\n";
    if (coords != cell->GetCornerParameter(ii))
    {
      std::cerr << "ERROR: Bad reference point " << ii << "\n";
      return false;
    }
  }

  auto sideConn = cell->GetSideConnectivity();
  if (!sideConn)
  {
    std::cerr << "ERROR: Expected non-null side connectivity.\n";
    return false;
  }

  auto sideOffs = cell->GetSideOffsetsAndShapes();
  if (!sideOffs)
  {
    std::cerr << "ERROR: Expected non-null side offsets and shapes.\n";
    return false;
  }
  std::cout << "  " << (sideOffs->GetNumberOfTuples() - 1) << " side types:\n";
  int ss = 0;
  for (vtkIdType ii = 0; ii < sideOffs->GetNumberOfTuples() - 1; ++ii)
  {
    int offset = sideOffs->GetTuple(ii)[0];
    int shapeValue = sideOffs->GetTuple(ii)[1];
    auto shape = static_cast<vtkDGCell::Shape>(shapeValue);
    std::cout << "    " << (ii + 1) << ". " << vtkDGCell::GetShapeName(shape).Data() << " sides (@ "
              << offset << ")\n";
    int nn = vtkDGCell::GetShapeCornerCount(shape);
    int nextOffset = sideOffs->GetTuple(ii + 1)[0];
    int numSidesOfType = cell->GetNumberOfSidesOfDimension(cell->GetDimension() - ii - 1);
    if (nextOffset - offset != nn * numSidesOfType)
    {
      std::cerr << "ERROR: Bad offset " << offset << " to " << nextOffset << " vs "
                << (nn * numSidesOfType) << "\n";
      return false;
    }
    for (int jj = 0; jj < numSidesOfType; ++jj, ++ss)
    {
      std::cout << "      " << ss << ":";
      for (int kk = 0; kk < nn; ++kk)
      {
        std::cout << " " << sideConn->GetTuple1(offset + jj * nn + kk);
        if (sideConn->GetTuple1(offset + jj * nn + kk) != cell->GetSideConnectivity(ss)[kk])
        {
          std::cerr << "\nERROR: Bad point ID @ kk = " << kk << "\n";
          return false;
        }
      }
      std::cout << "\n";
    }
  }

  std::cout << "  … passed\n\n";
  return true;
}

}

int TestDGCells(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  vtkCellMetadata::RegisterType<vtkDGHex>();
  vtkCellMetadata::RegisterType<vtkDGTet>();
  if (!TestDGCellType<vtkDGHex>() || !TestDGCellType<vtkDGTet>())
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
