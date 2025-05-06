//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <viskores/worklet/CellDeepCopy.h>

#include <viskores/cont/ArrayHandleCounting.h>
#include <viskores/cont/CellSetExplicit.h>
#include <viskores/cont/CellSetPermutation.h>

#include <viskores/cont/testing/MakeTestDataSet.h>
#include <viskores/cont/testing/Testing.h>

namespace
{

viskores::cont::CellSetExplicit<> CreateCellSet()
{
  viskores::cont::testing::MakeTestDataSet makeData;
  viskores::cont::DataSet data = makeData.Make3DExplicitDataSet0();
  viskores::cont::CellSetExplicit<> cellSet;
  data.GetCellSet().AsCellSet(cellSet);
  return cellSet;
}

viskores::cont::CellSetPermutation<viskores::cont::CellSetExplicit<>,
                                   viskores::cont::ArrayHandleCounting<viskores::Id>>
CreatePermutedCellSet()
{
  std::cout << "Creating input cell set" << std::endl;

  viskores::cont::CellSetExplicit<> cellSet = CreateCellSet();
  return viskores::cont::make_CellSetPermutation(
    viskores::cont::ArrayHandleCounting<viskores::Id>(
      cellSet.GetNumberOfCells() - 1, -1, cellSet.GetNumberOfCells()),
    cellSet);
}

template <typename CellSetType>
viskores::cont::CellSetExplicit<> DoCellDeepCopy(const CellSetType& inCells)
{
  std::cout << "Doing cell copy" << std::endl;

  return viskores::worklet::CellDeepCopy::Run(inCells);
}

void CheckOutput(const viskores::cont::CellSetExplicit<>& copiedCells)
{
  std::cout << "Checking copied cells" << std::endl;

  viskores::cont::CellSetExplicit<> originalCells = CreateCellSet();

  viskores::Id numberOfCells = copiedCells.GetNumberOfCells();
  VISKORES_TEST_ASSERT(numberOfCells == originalCells.GetNumberOfCells(),
                       "Result has wrong number of cells");

  // Cells should be copied backward. Check that.
  for (viskores::Id cellIndex = 0; cellIndex < numberOfCells; cellIndex++)
  {
    viskores::Id oCellIndex = numberOfCells - cellIndex - 1;
    VISKORES_TEST_ASSERT(copiedCells.GetCellShape(cellIndex) ==
                           originalCells.GetCellShape(oCellIndex),
                         "Bad cell shape");

    viskores::IdComponent numPoints = copiedCells.GetNumberOfPointsInCell(cellIndex);
    VISKORES_TEST_ASSERT(numPoints == originalCells.GetNumberOfPointsInCell(oCellIndex),
                         "Bad number of points in cell");

    // Only checking 3 points. All cells should have at least 3
    viskores::Id3 cellPoints{ 0 };
    copiedCells.GetIndices(cellIndex, cellPoints);
    viskores::Id3 oCellPoints{ 0 };
    originalCells.GetIndices(oCellIndex, oCellPoints);
    VISKORES_TEST_ASSERT(cellPoints == oCellPoints, "Point indices not copied correctly");
  }
}

void RunTest()
{
  viskores::cont::CellSetExplicit<> cellSet = DoCellDeepCopy(CreatePermutedCellSet());
  CheckOutput(cellSet);
}

} // anonymous namespace

int UnitTestCellDeepCopy(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(RunTest, argc, argv);
}
