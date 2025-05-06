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
#include <viskores/cont/CellSetPermutation.h>

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleCounting.h>
#include <viskores/cont/testing/MakeTestDataSet.h>
#include <viskores/cont/testing/Testing.h>
#include <viskores/worklet/DispatcherMapTopology.h>
#include <viskores/worklet/WorkletMapTopology.h>

namespace
{

struct WorkletPointToCell : public viskores::worklet::WorkletVisitCellsWithPoints
{
  using ControlSignature = void(CellSetIn cellset, FieldOutCell numPoints);
  using ExecutionSignature = void(PointIndices, _2);
  using InputDomain = _1;

  template <typename PointIndicesType>
  VISKORES_EXEC void operator()(const PointIndicesType& pointIndices, viskores::Id& numPoints) const
  {
    numPoints = pointIndices.GetNumberOfComponents();
  }
};

struct WorkletCellToPoint : public viskores::worklet::WorkletVisitPointsWithCells
{
  using ControlSignature = void(CellSetIn cellset, FieldOutPoint numCells);
  using ExecutionSignature = void(CellIndices, _2);
  using InputDomain = _1;

  template <typename CellIndicesType>
  VISKORES_EXEC void operator()(const CellIndicesType& cellIndices, viskores::Id& numCells) const
  {
    numCells = cellIndices.GetNumberOfComponents();
  }
};

struct CellsOfPoint : public viskores::worklet::WorkletVisitPointsWithCells
{
  using ControlSignature = void(CellSetIn cellset, FieldInPoint offset, WholeArrayOut cellIds);
  using ExecutionSignature = void(CellIndices, _2, _3);
  using InputDomain = _1;

  template <typename CellIndicesType, typename CellIdsPortal>
  VISKORES_EXEC void operator()(const CellIndicesType& cellIndices,
                                viskores::Id offset,
                                const CellIdsPortal& out) const
  {
    viskores::IdComponent count = cellIndices.GetNumberOfComponents();
    for (viskores::IdComponent i = 0; i < count; ++i)
    {
      out.Set(offset++, cellIndices[i]);
    }
  }
};

template <typename CellSetType, typename PermutationArrayHandleType>
std::vector<viskores::Id> ComputeCellToPointExpected(const CellSetType& cellset,
                                                     const PermutationArrayHandleType& permutation)
{
  viskores::cont::ArrayHandle<viskores::Id> numIndices;
  viskores::worklet::DispatcherMapTopology<WorkletCellToPoint>().Invoke(cellset, numIndices);
  std::cout << "\n";

  viskores::cont::ArrayHandle<viskores::Id> indexOffsets;
  viskores::Id connectivityLength =
    viskores::cont::Algorithm::ScanExclusive(numIndices, indexOffsets);

  viskores::cont::ArrayHandle<viskores::Id> connectivity;
  connectivity.Allocate(connectivityLength);
  viskores::worklet::DispatcherMapTopology<CellsOfPoint>().Invoke(
    cellset, indexOffsets, connectivity);

  std::vector<bool> permutationMask(static_cast<std::size_t>(cellset.GetNumberOfCells()), false);
  auto permPortal = permutation.ReadPortal();
  for (viskores::Id i = 0; i < permutation.GetNumberOfValues(); ++i)
  {
    permutationMask[static_cast<std::size_t>(permPortal.Get(i))] = true;
  }

  viskores::Id numberOfPoints = cellset.GetNumberOfPoints();
  std::vector<viskores::Id> expected(static_cast<std::size_t>(numberOfPoints), 0);
  auto indexPortal = indexOffsets.ReadPortal();
  auto numPortal = numIndices.ReadPortal();
  auto connPortal = connectivity.ReadPortal();
  for (viskores::Id i = 0; i < numberOfPoints; ++i)
  {
    viskores::Id offset = indexPortal.Get(i);
    viskores::Id count = numPortal.Get(i);
    for (viskores::Id j = 0; j < count; ++j)
    {
      viskores::Id cellId = connPortal.Get(offset++);
      if (permutationMask[static_cast<std::size_t>(cellId)])
      {
        ++expected[static_cast<std::size_t>(i)];
      }
    }
  }

  return expected;
}

template <typename CellSetType>
viskores::cont::CellSetPermutation<CellSetType, viskores::cont::ArrayHandleCounting<viskores::Id>>
TestCellSet(const CellSetType& cellset)
{
  viskores::Id numberOfCells = cellset.GetNumberOfCells() / 2;
  viskores::cont::ArrayHandleCounting<viskores::Id> permutation(0, 2, numberOfCells);
  auto cs = viskores::cont::make_CellSetPermutation(permutation, cellset);
  viskores::cont::ArrayHandle<viskores::Id> result;

  std::cout << "\t\tTesting PointToCell\n";
  viskores::worklet::DispatcherMapTopology<WorkletPointToCell>().Invoke(cs, result);

  VISKORES_TEST_ASSERT(result.GetNumberOfValues() == numberOfCells,
                       "result length not equal to number of cells");
  auto resultPortal = result.ReadPortal();
  auto permPortal = permutation.ReadPortal();
  for (viskores::Id i = 0; i < result.GetNumberOfValues(); ++i)
  {
    VISKORES_TEST_ASSERT(resultPortal.Get(i) == cellset.GetNumberOfPointsInCell(permPortal.Get(i)),
                         "incorrect result");
  }

  std::cout << "\t\tTesting CellToPoint\n";
  viskores::worklet::DispatcherMapTopology<WorkletCellToPoint>().Invoke(cs, result);

  VISKORES_TEST_ASSERT(result.GetNumberOfValues() == cellset.GetNumberOfPoints(),
                       "result length not equal to number of points");
  auto expected = ComputeCellToPointExpected(cellset, permutation);
  resultPortal = result.ReadPortal();
  for (viskores::Id i = 0; i < result.GetNumberOfValues(); ++i)
  {
    VISKORES_TEST_ASSERT(resultPortal.Get(i) == expected[static_cast<std::size_t>(i)],
                         "incorrect result");
  }
  std::cout << "Testing resource releasing in CellSetPermutation:\n";
  cs.ReleaseResourcesExecution();
  VISKORES_TEST_ASSERT(cs.GetNumberOfCells() == cellset.GetNumberOfCells() / 2,
                       "release execution resources should not change the number of cells");
  VISKORES_TEST_ASSERT(cs.GetNumberOfPoints() == cellset.GetNumberOfPoints(),
                       "release execution resources should not change the number of points");

  return cs;
}

template <typename CellSetType>
void RunTests(const CellSetType& cellset)
{
  std::cout << "\tTesting CellSetPermutation:\n";
  auto p1 = TestCellSet(cellset);
  std::cout << "\tTesting CellSetPermutation of CellSetPermutation:\n";
  TestCellSet(p1);
  std::cout << "----------------------------------------------------------\n";
}

void TestCellSetPermutation()
{
  viskores::cont::DataSet dataset;
  viskores::cont::testing::MakeTestDataSet maker;

  std::cout << "Testing CellSetStructured<2>\n";
  dataset = maker.Make2DUniformDataSet1();
  RunTests(dataset.GetCellSet().AsCellSet<viskores::cont::CellSetStructured<2>>());

  std::cout << "Testing CellSetStructured<3>\n";
  dataset = maker.Make3DUniformDataSet1();
  RunTests(dataset.GetCellSet().AsCellSet<viskores::cont::CellSetStructured<3>>());

  std::cout << "Testing CellSetExplicit\n";
  dataset = maker.Make3DExplicitDataSetPolygonal();
  RunTests(dataset.GetCellSet().AsCellSet<viskores::cont::CellSetExplicit<>>());

  std::cout << "Testing CellSetSingleType\n";
  dataset = maker.Make3DExplicitDataSetCowNose();
  RunTests(dataset.GetCellSet().AsCellSet<viskores::cont::CellSetSingleType<>>());
}

} // anonymous namespace

int UnitTestCellSetPermutation(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestCellSetPermutation, argc, argv);
}
