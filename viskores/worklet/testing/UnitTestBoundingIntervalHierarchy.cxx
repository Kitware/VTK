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

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayHandleConcatenate.h>
#include <viskores/cont/CellLocatorBoundingIntervalHierarchy.h>
#include <viskores/cont/DataSetBuilderUniform.h>
#include <viskores/cont/Invoker.h>
#include <viskores/cont/RuntimeDeviceInformation.h>
#include <viskores/cont/Timer.h>
#include <viskores/cont/openmp/internal/DeviceAdapterTagOpenMP.h>
#include <viskores/cont/testing/Testing.h>
#include <viskores/exec/CellInterpolate.h>
#include <viskores/exec/ParametricCoordinates.h>
#include <viskores/io/VTKDataSetReader.h>
#include <viskores/worklet/WorkletMapField.h>
#include <viskores/worklet/WorkletMapTopology.h>

namespace
{
struct CellCentroidCalculator : public viskores::worklet::WorkletVisitCellsWithPoints
{
  using ControlSignature = void(CellSetIn, FieldInPoint, FieldOut);
  using ExecutionSignature = void(_1, PointCount, _2, _3);

  template <typename CellShape, typename InputPointField>
  VISKORES_EXEC void operator()(CellShape shape,
                                viskores::IdComponent numPoints,
                                const InputPointField& inputPointField,
                                typename InputPointField::ComponentType& outputField) const
  {
    viskores::Vec3f parametricCenter;
    viskores::exec::ParametricCoordinatesCenter(numPoints, shape, parametricCenter);
    viskores::exec::CellInterpolate(inputPointField, parametricCenter, shape, outputField);
  }
}; // struct CellCentroidCalculator

struct BoundingIntervalHierarchyTester : public viskores::worklet::WorkletMapField
{
  typedef void ControlSignature(FieldIn, ExecObject, FieldIn, FieldOut);
  typedef _4 ExecutionSignature(_1, _2, _3);

  template <typename Point, typename BoundingIntervalHierarchyExecObject>
  VISKORES_EXEC viskores::IdComponent operator()(const Point& point,
                                                 const BoundingIntervalHierarchyExecObject& bih,
                                                 const viskores::Id expectedId) const
  {
    viskores::Vec3f parametric;
    viskores::Id cellId = -1;
    bih.FindCell(point, cellId, parametric);
    return (1 - static_cast<viskores::IdComponent>(expectedId == cellId));
  }
}; // struct BoundingIntervalHierarchyTester

viskores::cont::DataSet ConstructDataSet(viskores::Id size)
{
  return viskores::cont::DataSetBuilderUniform().Create(viskores::Id3(size, size, size));
}

void TestBoundingIntervalHierarchy(viskores::cont::DataSet dataSet, viskores::IdComponent numPlanes)
{

  viskores::cont::UnknownCellSet cellSet = dataSet.GetCellSet();
  auto vertices = dataSet.GetCoordinateSystem().GetDataAsMultiplexer();

  viskores::cont::CellLocatorBoundingIntervalHierarchy bih =
    viskores::cont::CellLocatorBoundingIntervalHierarchy(numPlanes, 5);
  bih.SetCellSet(cellSet);
  bih.SetCoordinates(dataSet.GetCoordinateSystem());
  bih.Update();

  viskores::cont::ArrayHandle<viskores::Vec3f> centroids;
  viskores::worklet::DispatcherMapTopology<CellCentroidCalculator>().Invoke(
    cellSet, vertices, centroids);


  viskores::cont::ArrayHandleCounting<viskores::Id> expectedCellIds(
    0, 1, cellSet.GetNumberOfCells());
  viskores::cont::ArrayHandle<viskores::IdComponent> results;

  viskores::worklet::DispatcherMapField<BoundingIntervalHierarchyTester>().Invoke(
    centroids, bih, expectedCellIds, results);

  viskores::Id numDiffs = viskores::cont::Algorithm::Reduce(results, 0, viskores::Add());
  VISKORES_TEST_ASSERT(numDiffs == 0, "Calculated cell Ids not the same as expected cell Ids");
}

void RunTest()
{
//If this test is run on a machine that already has heavy
//cpu usage it will fail, so we limit the number of threads
//to avoid the test timing out
#ifdef VISKORES_ENABLE_OPENMP
  auto& runtimeConfig = viskores::cont::RuntimeDeviceInformation{}.GetRuntimeConfiguration(
    viskores::cont::DeviceAdapterTagOpenMP());
  viskores::Id maxThreads = 0;
  runtimeConfig.GetMaxThreads(maxThreads);
  runtimeConfig.SetThreads(std::min(static_cast<viskores::Id>(4), maxThreads));
#endif

  TestBoundingIntervalHierarchy(ConstructDataSet(8), 3);
  TestBoundingIntervalHierarchy(ConstructDataSet(8), 4);
  TestBoundingIntervalHierarchy(ConstructDataSet(8), 6);
  TestBoundingIntervalHierarchy(ConstructDataSet(8), 9);
}

} // anonymous namespace

int UnitTestBoundingIntervalHierarchy(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(RunTest, argc, argv);
}
