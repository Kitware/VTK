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
#include <viskores/cont/ArrayCopyDevice.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleConstant.h>
#include <viskores/cont/ArrayHandleCounting.h>
#include <viskores/cont/ArrayHandleImplicit.h>
#include <viskores/cont/CellSet.h>
#include <viskores/cont/CellSetExplicit.h>
#include <viskores/cont/CellSetPermutation.h>
#include <viskores/cont/CellSetSingleType.h>
#include <viskores/cont/CellSetStructured.h>
#include <viskores/cont/ConvertNumComponentsToOffsets.h>
#include <viskores/cont/testing/Testing.h>

namespace
{

constexpr viskores::Id xdim = 3, ydim = 5, zdim = 7;
constexpr viskores::Id3 BaseLinePointDimensions{ xdim, ydim, zdim };
constexpr viskores::Id BaseLineNumberOfPoints = xdim * ydim * zdim;
constexpr viskores::Id BaseLineNumberOfCells = (xdim - 1) * (ydim - 1) * (zdim - 1);

viskores::cont::CellSetStructured<3> BaseLine;

void InitializeBaseLine()
{
  BaseLine.SetPointDimensions(BaseLinePointDimensions);
}

class BaseLineConnectivityFunctor
{
public:
  explicit BaseLineConnectivityFunctor()
  {
    this->Structure.SetPointDimensions(BaseLinePointDimensions);
  }

  VISKORES_EXEC_CONT
  viskores::Id operator()(viskores::Id idx) const
  {
    auto i = idx / this->Structure.NUM_POINTS_IN_CELL;
    auto c = static_cast<viskores::IdComponent>(idx % this->Structure.NUM_POINTS_IN_CELL);
    return this->Structure.GetPointsOfCell(i)[c];
  }

private:
  viskores::internal::ConnectivityStructuredInternals<3> Structure;
};

using BaseLineConnectivityType = viskores::cont::ArrayHandleImplicit<BaseLineConnectivityFunctor>;
BaseLineConnectivityType BaseLineConnectivity(BaseLineConnectivityFunctor{},
                                              BaseLineNumberOfCells * 8);

auto PermutationArray =
  viskores::cont::ArrayHandleCounting<viskores::Id>(0, 2, BaseLineNumberOfCells / 2);

//-----------------------------------------------------------------------------
viskores::cont::CellSetExplicit<> MakeCellSetExplicit()
{
  viskores::cont::ArrayHandle<viskores::UInt8> shapes;
  shapes.AllocateAndFill(BaseLineNumberOfCells, viskores::CELL_SHAPE_HEXAHEDRON);

  viskores::cont::ArrayHandle<viskores::IdComponent> numIndices;
  numIndices.AllocateAndFill(BaseLineNumberOfCells, 8);

  viskores::cont::ArrayHandle<viskores::Id> connectivity;
  viskores::cont::ArrayCopyDevice(BaseLineConnectivity, connectivity);

  auto offsets = viskores::cont::ConvertNumComponentsToOffsets(numIndices);

  viskores::cont::CellSetExplicit<> cellset;
  cellset.Fill(BaseLineNumberOfPoints, shapes, connectivity, offsets);
  return cellset;
}

viskores::cont::CellSetSingleType<typename BaseLineConnectivityType::StorageTag>
MakeCellSetSingleType()
{
  viskores::cont::CellSetSingleType<typename BaseLineConnectivityType::StorageTag> cellset;
  cellset.Fill(BaseLineNumberOfPoints, viskores::CELL_SHAPE_HEXAHEDRON, 8, BaseLineConnectivity);
  return cellset;
}

viskores::cont::CellSetStructured<3> MakeCellSetStructured()
{
  viskores::cont::CellSetStructured<3> cellset;
  cellset.SetPointDimensions(BaseLinePointDimensions);
  return cellset;
}

//-----------------------------------------------------------------------------
enum class IsPermutationCellSet
{
  NO = 0,
  YES = 1
};

void TestAgainstBaseLine(const viskores::cont::CellSet& cellset,
                         IsPermutationCellSet flag = IsPermutationCellSet::NO)
{
  viskores::internal::ConnectivityStructuredInternals<3> baseLineStructure;
  baseLineStructure.SetPointDimensions(BaseLinePointDimensions);

  VISKORES_TEST_ASSERT(cellset.GetNumberOfPoints() == BaseLineNumberOfPoints,
                       "Wrong number of points");

  viskores::Id numCells = cellset.GetNumberOfCells();
  viskores::Id expectedNumCell = (flag == IsPermutationCellSet::NO)
    ? BaseLineNumberOfCells
    : PermutationArray.GetNumberOfValues();
  VISKORES_TEST_ASSERT(numCells == expectedNumCell, "Wrong number of cells");

  auto permutationPortal = PermutationArray.ReadPortal();
  for (viskores::Id i = 0; i < numCells; ++i)
  {
    VISKORES_TEST_ASSERT(cellset.GetCellShape(i) == viskores::CELL_SHAPE_HEXAHEDRON, "Wrong shape");
    VISKORES_TEST_ASSERT(cellset.GetNumberOfPointsInCell(i) == 8, "Wrong number of points-of-cell");

    viskores::Id baseLineCellId =
      (flag == IsPermutationCellSet::YES) ? permutationPortal.Get(i) : i;
    auto baseLinePointIds = baseLineStructure.GetPointsOfCell(baseLineCellId);

    viskores::Id pointIds[8];
    cellset.GetCellPointIds(i, pointIds);
    for (int j = 0; j < 8; ++j)
    {
      VISKORES_TEST_ASSERT(pointIds[j] == baseLinePointIds[j], "Wrong points-of-cell point id");
    }
  }
}

void RunTests(const viskores::cont::CellSet& cellset,
              IsPermutationCellSet flag = IsPermutationCellSet::NO)
{
  TestAgainstBaseLine(cellset, flag);
  auto deepcopy = cellset.NewInstance();
  deepcopy->DeepCopy(&cellset);
  TestAgainstBaseLine(*deepcopy, flag);
}

void TestCellSet()
{
  InitializeBaseLine();

  std::cout << "Testing CellSetExplicit\n";
  auto csExplicit = MakeCellSetExplicit();
  RunTests(csExplicit);
  std::cout << "Testing CellSetPermutation of CellSetExplicit\n";
  RunTests(viskores::cont::make_CellSetPermutation(PermutationArray, csExplicit),
           IsPermutationCellSet::YES);

  std::cout << "Testing CellSetSingleType\n";
  auto csSingle = MakeCellSetSingleType();
  RunTests(csSingle);
  std::cout << "Testing CellSetPermutation of CellSetSingleType\n";
  RunTests(viskores::cont::make_CellSetPermutation(PermutationArray, csSingle),
           IsPermutationCellSet::YES);

  std::cout << "Testing CellSetStructured\n";
  auto csStructured = MakeCellSetStructured();
  RunTests(csStructured);
  std::cout << "Testing CellSetPermutation of CellSetStructured\n";
  RunTests(viskores::cont::make_CellSetPermutation(PermutationArray, csStructured),
           IsPermutationCellSet::YES);
}

} // anonymous namespace

//-----------------------------------------------------------------------------
int UnitTestCellSet(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestCellSet, argc, argv);
}
