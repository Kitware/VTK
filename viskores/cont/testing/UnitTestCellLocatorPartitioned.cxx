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

#include <random>
#include <string>

#include <viskores/cont/CellLocatorPartitioned.h>
#include <viskores/cont/Invoker.h>

#include <viskores/cont/testing/MakeTestDataSet.h>
#include <viskores/cont/testing/Testing.h>

#include <viskores/source/Amr.h>

#include <viskores/ErrorCode.h>
#include <viskores/worklet/WorkletMapField.h>

namespace
{
struct QueryCellsWorklet : public viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, ExecObject, FieldOut, FieldOut);
  using ExecutionSignature = void(_1, _2, _3, _4);

  template <typename PointType, typename CellLocatorExecObjectType>
  VISKORES_EXEC void operator()(const PointType& point,
                                const CellLocatorExecObjectType& cellLocator,
                                viskores::Id& cellId,
                                viskores::Id& partitionId) const
  {
    viskores::Vec3f parametric;
    viskores::ErrorCode status = cellLocator.FindCell(point, partitionId, cellId, parametric);
    if (status != viskores::ErrorCode::Success)
    {
      this->RaiseError(viskores ::ErrorString(status));
      partitionId = -1;
      cellId = -1;
    }
  }
};

void Test()
{
  int dim = 3;
  int numberOfLevels = 3;
  int cellsPerDimension = 8;

  // Generate AMR
  viskores::source::Amr source;
  source.SetDimension(dim);
  source.SetNumberOfLevels(numberOfLevels);
  source.SetCellsPerDimension(cellsPerDimension);
  viskores::cont::PartitionedDataSet amrDataSet = source.Execute();

  // one point for each partition
  viskores::cont::ArrayHandle<viskores::Vec3f> queryPoints;
  queryPoints.Allocate(7);
  queryPoints.WritePortal().Set(0, viskores::Vec3f(0.1f, 0.9f, 0.1f));
  queryPoints.WritePortal().Set(1, viskores::Vec3f(0.1f, 0.4f, 0.4f));
  queryPoints.WritePortal().Set(2, viskores::Vec3f(0.8f, 0.5f, 0.5f));
  queryPoints.WritePortal().Set(3, viskores::Vec3f(0.0f));
  queryPoints.WritePortal().Set(4, viskores::Vec3f(0.4999999f));
  queryPoints.WritePortal().Set(5, viskores::Vec3f(0.5000001f));
  queryPoints.WritePortal().Set(6, viskores::Vec3f(1.0f));

  // generate cellLocator on cont side
  viskores::cont::CellLocatorPartitioned cellLocator;
  cellLocator.SetPartitions(amrDataSet);
  cellLocator.Update();
  viskores::cont::ArrayHandle<viskores::Id> cellIds;
  viskores::cont::ArrayHandle<viskores::Id> partitionIds;
  viskores::cont::Invoker invoke;
  invoke(QueryCellsWorklet{}, queryPoints, &cellLocator, cellIds, partitionIds);

  for (viskores::Id index = 0; index < queryPoints.GetNumberOfValues(); ++index)
  {
    VISKORES_TEST_ASSERT(partitionIds.ReadPortal().Get(index) == index, "Incorrect partitionId");
  }
}

} // anonymous namespace

int UnitTestCellLocatorPartitioned(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(Test, argc, argv);
}
