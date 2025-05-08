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

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleConstant.h>
#include <viskores/cont/ArrayHandleIndex.h>
#include <viskores/cont/CellSetPermutation.h>
#include <viskores/cont/DataSet.h>

#include <viskores/worklet/DispatcherMapField.h>
#include <viskores/worklet/WorkletMapField.h>

#include <viskores/cont/testing/MakeTestDataSet.h>
#include <viskores/cont/testing/Testing.h>

struct TestWholeCellSetIn
{
  template <typename VisitTopology, typename IncidentTopology>
  struct WholeCellSetWorklet : public viskores::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn indices,
                                  WholeCellSetIn<VisitTopology, IncidentTopology>,
                                  FieldOut numberOfElements,
                                  FieldOut shapes,
                                  FieldOut numberOfindices,
                                  FieldOut connectionSum);
    using ExecutionSignature = void(_1, _2, _3, _4, _5, _6);
    using InputDomain = _1;

    template <typename ConnectivityType>
    VISKORES_EXEC void operator()(viskores::Id index,
                                  const ConnectivityType& connectivity,
                                  viskores::Id& numberOfElements,
                                  viskores::UInt8& shape,
                                  viskores::IdComponent& numberOfIndices,
                                  viskores::Id& connectionSum) const
    {
      numberOfElements = connectivity.GetNumberOfElements();
      shape = connectivity.GetCellShape(index).Id;
      numberOfIndices = connectivity.GetNumberOfIndices(index);

      typename ConnectivityType::IndicesType indices = connectivity.GetIndices(index);
      if (numberOfIndices != indices.GetNumberOfComponents())
      {
        this->RaiseError("Got wrong number of connections.");
      }

      connectionSum = 0;
      for (viskores::IdComponent componentIndex = 0;
           componentIndex < indices.GetNumberOfComponents();
           componentIndex++)
      {
        connectionSum += indices[componentIndex];
      }
    }
  };

  template <typename CellSetType>
  VISKORES_CONT static void RunCells(
    const CellSetType& cellSet,
    viskores::cont::ArrayHandle<viskores::Id> numberOfElements,
    viskores::cont::ArrayHandle<viskores::UInt8> shapeIds,
    viskores::cont::ArrayHandle<viskores::IdComponent> numberOfIndices,
    viskores::cont::ArrayHandle<viskores::Id> connectionSum)
  {
    using WorkletType =
      WholeCellSetWorklet<viskores::TopologyElementTagCell, viskores::TopologyElementTagPoint>;
    viskores::worklet::DispatcherMapField<WorkletType> dispatcher;
    dispatcher.Invoke(viskores::cont::ArrayHandleIndex(cellSet.GetNumberOfCells()),
                      cellSet,
                      numberOfElements,
                      shapeIds,
                      numberOfIndices,
                      &connectionSum);
  }

  template <typename CellSetType>
  VISKORES_CONT static void RunPoints(
    const CellSetType* cellSet,
    viskores::cont::ArrayHandle<viskores::Id> numberOfElements,
    viskores::cont::ArrayHandle<viskores::UInt8> shapeIds,
    viskores::cont::ArrayHandle<viskores::IdComponent> numberOfIndices,
    viskores::cont::ArrayHandle<viskores::Id> connectionSum)
  {
    using WorkletType =
      WholeCellSetWorklet<viskores::TopologyElementTagPoint, viskores::TopologyElementTagCell>;
    viskores::worklet::DispatcherMapField<WorkletType> dispatcher;
    dispatcher.Invoke(viskores::cont::ArrayHandleIndex(cellSet->GetNumberOfPoints()),
                      cellSet,
                      numberOfElements,
                      &shapeIds,
                      numberOfIndices,
                      connectionSum);
  }
};

template <typename CellSetType,
          typename ShapeArrayType,
          typename NumIndicesArrayType,
          typename ConnectionSumArrayType>
VISKORES_CONT void TryCellConnectivity(const CellSetType& cellSet,
                                       const ShapeArrayType& expectedShapeIds,
                                       const NumIndicesArrayType& expectedNumberOfIndices,
                                       const ConnectionSumArrayType& expectedSum)
{
  std::cout << "  trying point to cell connectivity" << std::endl;
  viskores::cont::ArrayHandle<viskores::Id> numberOfElements;
  viskores::cont::ArrayHandle<viskores::UInt8> shapeIds;
  viskores::cont::ArrayHandle<viskores::IdComponent> numberOfIndices;
  viskores::cont::ArrayHandle<viskores::Id> connectionSum;

  TestWholeCellSetIn::RunCells(cellSet, numberOfElements, shapeIds, numberOfIndices, connectionSum);

  std::cout << "    Number of elements: " << numberOfElements.ReadPortal().Get(0) << std::endl;
  VISKORES_TEST_ASSERT(test_equal_portals(numberOfElements.ReadPortal(),
                                          viskores::cont::make_ArrayHandleConstant(
                                            cellSet.GetNumberOfCells(), cellSet.GetNumberOfCells())
                                            .ReadPortal()),
                       "Incorrect number of elements.");

  std::cout << "    Shape Ids: ";
  viskores::cont::printSummary_ArrayHandle(shapeIds, std::cout, true);
  VISKORES_TEST_ASSERT(test_equal_portals(shapeIds.ReadPortal(), expectedShapeIds.ReadPortal()),
                       "Incorrect shape Ids.");

  std::cout << "    Number of indices: ";
  viskores::cont::printSummary_ArrayHandle(numberOfIndices, std::cout, true);
  VISKORES_TEST_ASSERT(
    test_equal_portals(numberOfIndices.ReadPortal(), expectedNumberOfIndices.ReadPortal()),
    "Incorrect number of indices.");

  std::cout << "    Sum of indices: ";
  viskores::cont::printSummary_ArrayHandle(connectionSum, std::cout, true);
  VISKORES_TEST_ASSERT(test_equal_portals(connectionSum.ReadPortal(), expectedSum.ReadPortal()),
                       "Incorrect sum of indices.");
}

template <typename CellSetType,
          typename ShapeArrayType,
          typename NumIndicesArrayType,
          typename ConnectionSumArrayType>
VISKORES_CONT void TryPointConnectivity(const CellSetType& cellSet,
                                        const ShapeArrayType& expectedShapeIds,
                                        const NumIndicesArrayType& expectedNumberOfIndices,
                                        const ConnectionSumArrayType& expectedSum)
{
  std::cout << "  trying cell to point connectivity" << std::endl;
  viskores::cont::ArrayHandle<viskores::Id> numberOfElements;
  viskores::cont::ArrayHandle<viskores::UInt8> shapeIds;
  viskores::cont::ArrayHandle<viskores::IdComponent> numberOfIndices;
  viskores::cont::ArrayHandle<viskores::Id> connectionSum;

  TestWholeCellSetIn::RunPoints(
    &cellSet, numberOfElements, shapeIds, numberOfIndices, connectionSum);

  std::cout << "    Number of elements: " << numberOfElements.ReadPortal().Get(0) << std::endl;
  VISKORES_TEST_ASSERT(
    test_equal_portals(numberOfElements.ReadPortal(),
                       viskores::cont::make_ArrayHandleConstant(cellSet.GetNumberOfPoints(),
                                                                cellSet.GetNumberOfPoints())
                         .ReadPortal()),
    "Incorrect number of elements.");

  std::cout << "    Shape Ids: ";
  viskores::cont::printSummary_ArrayHandle(shapeIds, std::cout, true);
  VISKORES_TEST_ASSERT(test_equal_portals(shapeIds.ReadPortal(), expectedShapeIds.ReadPortal()),
                       "Incorrect shape Ids.");

  std::cout << "    Number of indices: ";
  viskores::cont::printSummary_ArrayHandle(numberOfIndices, std::cout, true);
  VISKORES_TEST_ASSERT(
    test_equal_portals(numberOfIndices.ReadPortal(), expectedNumberOfIndices.ReadPortal()),
    "Incorrect number of indices.");

  std::cout << "    Sum of indices: ";
  viskores::cont::printSummary_ArrayHandle(connectionSum, std::cout, true);
  VISKORES_TEST_ASSERT(test_equal_portals(connectionSum.ReadPortal(), expectedSum.ReadPortal()),
                       "Incorrect sum of indices.");
}

VISKORES_CONT
void TryExplicitGrid()
{
  std::cout << "Testing explicit grid." << std::endl;
  viskores::cont::DataSet dataSet =
    viskores::cont::testing::MakeTestDataSet().Make3DExplicitDataSet5();
  viskores::cont::CellSetExplicit<> cellSet;
  dataSet.GetCellSet().AsCellSet(cellSet);

  viskores::UInt8 expectedCellShapes[] = { viskores::CELL_SHAPE_HEXAHEDRON,
                                           viskores::CELL_SHAPE_PYRAMID,
                                           viskores::CELL_SHAPE_TETRA,
                                           viskores::CELL_SHAPE_WEDGE };

  viskores::IdComponent expectedCellNumIndices[] = { 8, 5, 4, 6 };

  viskores::Id expectedCellIndexSum[] = { 28, 22, 29, 41 };

  viskores::Id numCells = cellSet.GetNumberOfCells();
  TryCellConnectivity(
    cellSet,
    viskores::cont::make_ArrayHandle(expectedCellShapes, numCells, viskores::CopyFlag::Off),
    viskores::cont::make_ArrayHandle(expectedCellNumIndices, numCells, viskores::CopyFlag::Off),
    viskores::cont::make_ArrayHandle(expectedCellIndexSum, numCells, viskores::CopyFlag::Off));

  viskores::IdComponent expectedPointNumIndices[] = { 1, 2, 2, 1, 2, 4, 4, 2, 2, 1, 2 };

  viskores::Id expectedPointIndexSum[] = { 0, 1, 1, 0, 3, 6, 6, 3, 3, 3, 5 };

  viskores::Id numPoints = cellSet.GetNumberOfPoints();
  TryPointConnectivity(
    cellSet,
    viskores::cont::make_ArrayHandleConstant(viskores::CellShapeTagVertex::Id, numPoints),
    viskores::cont::make_ArrayHandle(expectedPointNumIndices, numPoints, viskores::CopyFlag::Off),
    viskores::cont::make_ArrayHandle(expectedPointIndexSum, numPoints, viskores::CopyFlag::Off));
}

VISKORES_CONT
void TryCellSetPermutation()
{
  std::cout << "Testing permutation grid." << std::endl;
  viskores::cont::DataSet dataSet =
    viskores::cont::testing::MakeTestDataSet().Make3DExplicitDataSet5();
  viskores::cont::CellSetExplicit<> originalCellSet;
  dataSet.GetCellSet().AsCellSet(originalCellSet);

  viskores::Id permutationArray[] = { 2, 0, 1 };

  viskores::cont::CellSetPermutation<viskores::cont::CellSetExplicit<>,
                                     viskores::cont::ArrayHandle<viskores::Id>>
    cellSet(viskores::cont::make_ArrayHandle(permutationArray, 3, viskores::CopyFlag::Off),
            originalCellSet);

  viskores::UInt8 expectedCellShapes[] = { viskores::CELL_SHAPE_TETRA,
                                           viskores::CELL_SHAPE_HEXAHEDRON,
                                           viskores::CELL_SHAPE_PYRAMID };

  viskores::IdComponent expectedCellNumIndices[] = { 4, 8, 5 };

  viskores::Id expectedCellIndexSum[] = { 29, 28, 22 };

  viskores::Id numCells = cellSet.GetNumberOfCells();
  TryCellConnectivity(
    cellSet,
    viskores::cont::make_ArrayHandle(expectedCellShapes, numCells, viskores::CopyFlag::Off),
    viskores::cont::make_ArrayHandle(expectedCellNumIndices, numCells, viskores::CopyFlag::Off),
    viskores::cont::make_ArrayHandle(expectedCellIndexSum, numCells, viskores::CopyFlag::Off));

  // Permutation cell set does not support cell to point connectivity.
}

VISKORES_CONT
void TryStructuredGrid3D()
{
  std::cout << "Testing 3D structured grid." << std::endl;
  viskores::cont::DataSet dataSet =
    viskores::cont::testing::MakeTestDataSet().Make3DUniformDataSet0();
  viskores::cont::CellSetStructured<3> cellSet;
  dataSet.GetCellSet().AsCellSet(cellSet);

  viskores::Id expectedCellIndexSum[4] = { 40, 48, 88, 96 };

  viskores::Id numCells = cellSet.GetNumberOfCells();
  TryCellConnectivity(
    cellSet,
    viskores::cont::ArrayHandleConstant<viskores::UInt8>(viskores::CELL_SHAPE_HEXAHEDRON, numCells),
    viskores::cont::ArrayHandleConstant<viskores::IdComponent>(8, numCells),
    viskores::cont::make_ArrayHandle(expectedCellIndexSum, numCells, viskores::CopyFlag::Off));

  viskores::IdComponent expectedPointNumIndices[18] = { 1, 2, 1, 1, 2, 1, 2, 4, 2,
                                                        2, 4, 2, 1, 2, 1, 1, 2, 1 };

  viskores::Id expectedPointIndexSum[18] = { 0, 1, 1, 0, 1, 1, 2, 6, 4, 2, 6, 4, 2, 5, 3, 2, 5, 3 };

  viskores::Id numPoints = cellSet.GetNumberOfPoints();
  TryPointConnectivity(
    cellSet,
    viskores::cont::ArrayHandleConstant<viskores::UInt8>(viskores::CELL_SHAPE_VERTEX, numPoints),
    viskores::cont::make_ArrayHandle(expectedPointNumIndices, numPoints, viskores::CopyFlag::Off),
    viskores::cont::make_ArrayHandle(expectedPointIndexSum, numPoints, viskores::CopyFlag::Off));
}

VISKORES_CONT
void TryStructuredGrid2D()
{
  std::cout << "Testing 2D structured grid." << std::endl;
  viskores::cont::DataSet dataSet =
    viskores::cont::testing::MakeTestDataSet().Make2DUniformDataSet0();
  viskores::cont::CellSetStructured<2> cellSet;
  dataSet.GetCellSet().AsCellSet(cellSet);

  viskores::Id expectedCellIndexSum[2] = { 8, 12 };

  viskores::Id numCells = cellSet.GetNumberOfCells();
  TryCellConnectivity(
    cellSet,
    viskores::cont::ArrayHandleConstant<viskores::UInt8>(viskores::CELL_SHAPE_QUAD, numCells),
    viskores::cont::ArrayHandleConstant<viskores::IdComponent>(4, numCells),
    viskores::cont::make_ArrayHandle(expectedCellIndexSum, numCells, viskores::CopyFlag::Off));

  viskores::IdComponent expectedPointNumIndices[6] = { 1, 2, 1, 1, 2, 1 };

  viskores::Id expectedPointIndexSum[6] = { 0, 1, 1, 0, 1, 1 };

  viskores::Id numPoints = cellSet.GetNumberOfPoints();
  TryPointConnectivity(
    cellSet,
    viskores::cont::ArrayHandleConstant<viskores::UInt8>(viskores::CELL_SHAPE_VERTEX, numPoints),
    viskores::cont::make_ArrayHandle(expectedPointNumIndices, numPoints, viskores::CopyFlag::Off),
    viskores::cont::make_ArrayHandle(expectedPointIndexSum, numPoints, viskores::CopyFlag::Off));
}

VISKORES_CONT
void TryStructuredGrid1D()
{
  std::cout << "Testing 1D structured grid." << std::endl;
  viskores::cont::DataSet dataSet =
    viskores::cont::testing::MakeTestDataSet().Make1DUniformDataSet0();
  viskores::cont::CellSetStructured<1> cellSet;
  dataSet.GetCellSet().AsCellSet(cellSet);

  viskores::Id expectedCellIndexSum[5] = { 1, 3, 5, 7, 9 };

  viskores::Id numCells = cellSet.GetNumberOfCells();
  TryCellConnectivity(
    cellSet,
    viskores::cont::ArrayHandleConstant<viskores::UInt8>(viskores::CELL_SHAPE_LINE, numCells),
    viskores::cont::ArrayHandleConstant<viskores::IdComponent>(2, numCells),
    viskores::cont::make_ArrayHandle(expectedCellIndexSum, numCells, viskores::CopyFlag::Off));

  viskores::IdComponent expectedPointNumIndices[6] = { 1, 2, 2, 2, 2, 1 };

  viskores::Id expectedPointIndexSum[6] = { 0, 1, 3, 5, 7, 4 };

  viskores::Id numPoints = cellSet.GetNumberOfPoints();
  TryPointConnectivity(
    cellSet,
    viskores::cont::ArrayHandleConstant<viskores::UInt8>(viskores::CELL_SHAPE_VERTEX, numPoints),
    viskores::cont::make_ArrayHandle(expectedPointNumIndices, numPoints, viskores::CopyFlag::Off),
    viskores::cont::make_ArrayHandle(expectedPointIndexSum, numPoints, viskores::CopyFlag::Off));
}

VISKORES_CONT
void RunWholeCellSetInTests()
{
  TryExplicitGrid();
  TryCellSetPermutation();
  TryStructuredGrid3D();
  TryStructuredGrid2D();
  TryStructuredGrid1D();
}

int UnitTestWholeCellSetIn(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(RunWholeCellSetInTests, argc, argv);
}
