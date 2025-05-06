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

#include <viskores/CellShape.h>
#include <viskores/VectorAnalysis.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/CellSetStructured.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/DataSetBuilderUniform.h>
#include <viskores/cont/PartitionedDataSet.h>
#include <viskores/cont/serial/DeviceAdapterSerial.h>
#include <viskores/exec/ConnectivityStructured.h>

#include <viskores/cont/testing/Testing.h>
#include <viskores/filter/field_conversion/CellAverage.h>


template <typename T>
viskores::cont::PartitionedDataSet PartitionedDataSetBuilder(std::size_t partitionNum,
                                                             std::string fieldName)
{
  viskores::cont::DataSetBuilderUniform dataSetBuilder;
  viskores::cont::DataSet dataSet;

  viskores::Vec<T, 2> origin(0);
  viskores::Vec<T, 2> spacing(1);
  viskores::cont::PartitionedDataSet partitions;
  for (viskores::Id partId = 0; partId < static_cast<viskores::Id>(partitionNum); partId++)
  {
    viskores::Id2 dimensions((partId + 2) * (partId + 2), (partId + 2) * (partId + 2));

    if (fieldName == "cellvar")
    {
      viskores::Id numCells = (dimensions[0] - 1) * (dimensions[1] - 1);

      std::vector<T> varC2D(static_cast<std::size_t>(numCells));
      for (viskores::Id i = 0; i < numCells; i++)
      {
        varC2D[static_cast<std::size_t>(i)] = static_cast<T>(partId * i);
      }
      dataSet = dataSetBuilder.Create(viskores::Id2(dimensions[0], dimensions[1]),
                                      viskores::Vec<T, 2>(origin[0], origin[1]),
                                      viskores::Vec<T, 2>(spacing[0], spacing[1]));
      dataSet.AddCellField("cellvar", varC2D);
    }

    if (fieldName == "pointvar")
    {
      viskores::Id numPoints = dimensions[0] * dimensions[1];
      std::vector<T> varP2D(static_cast<std::size_t>(numPoints));
      for (viskores::Id i = 0; i < numPoints; i++)
      {
        varP2D[static_cast<std::size_t>(i)] = static_cast<T>(partId);
      }
      dataSet = dataSetBuilder.Create(viskores::Id2(dimensions[0], dimensions[1]),
                                      viskores::Vec<T, 2>(origin[0], origin[1]),
                                      viskores::Vec<T, 2>(spacing[0], spacing[1]));
      dataSet.AddPointField("pointvar", varP2D);
    }

    partitions.AppendPartition(dataSet);
  }
  return partitions;
}
template <typename T, typename D>
void Result_Verify(const viskores::cont::PartitionedDataSet& result,
                   D& filter,
                   const viskores::cont::PartitionedDataSet& partitions,
                   std::string fieldName)
{
  VISKORES_TEST_ASSERT(result.GetNumberOfPartitions() == partitions.GetNumberOfPartitions(),
                       "result partition number incorrect");
  const std::string outputFieldName = filter.GetOutputFieldName();
  for (viskores::Id j = 0; j < result.GetNumberOfPartitions(); j++)
  {
    filter.SetActiveField(fieldName);
    viskores::cont::DataSet partitionResult = filter.Execute(partitions.GetPartition(j));

    VISKORES_TEST_ASSERT(result.GetPartition(j).GetField(outputFieldName).GetNumberOfValues() ==
                           partitionResult.GetField(outputFieldName).GetNumberOfValues(),
                         "result vectors' size incorrect");

    viskores::cont::ArrayHandle<T> partitionArray;
    result.GetPartition(j).GetField(outputFieldName).GetData().AsArrayHandle(partitionArray);
    viskores::cont::ArrayHandle<T> sDataSetArray;
    partitionResult.GetField(outputFieldName).GetData().AsArrayHandle(sDataSetArray);

    const viskores::Id numValues =
      result.GetPartition(j).GetField(outputFieldName).GetNumberOfValues();
    for (viskores::Id i = 0; i < numValues; i++)
    {
      VISKORES_TEST_ASSERT(partitionArray.ReadPortal().Get(i) == sDataSetArray.ReadPortal().Get(i),
                           "result values incorrect");
    }
  }
  return;
}

void TestPartitionedDataSetFilters()
{
  std::size_t partitionNum = 7;
  viskores::cont::PartitionedDataSet result;
  viskores::cont::PartitionedDataSet partitions;

  partitions = PartitionedDataSetBuilder<viskores::FloatDefault>(partitionNum, "pointvar");
  viskores::filter::field_conversion::CellAverage cellAverage;
  cellAverage.SetOutputFieldName("average");
  cellAverage.SetActiveField("pointvar");
  result = cellAverage.Execute(partitions);
  Result_Verify<viskores::FloatDefault>(result, cellAverage, partitions, std::string("pointvar"));

  //Make sure that any Fields are propagated to the output.
  //Test it with and without using SetFieldsToPass
  std::vector<std::vector<std::string>> fieldsToPass;
  fieldsToPass.push_back({});
  fieldsToPass.push_back({ "ids" });
  fieldsToPass.push_back({ "scalar" });
  fieldsToPass.push_back({ "ids", "scalar" });

  for (auto& fields : fieldsToPass)
  {
    partitionNum = 3;
    partitions = PartitionedDataSetBuilder<viskores::FloatDefault>(partitionNum, "pointvar");
    std::vector<viskores::Id> ids = { 0, 1, 2 };
    std::vector<viskores::FloatDefault> scalar = { 10.0f };
    partitions.AddPartitionsField("ids", ids);
    partitions.AddGlobalField("scalar", scalar);

    //On second iteration, only allow "ids" to pass through.
    cellAverage.GetFieldsToPass().ClearFields();
    if (!fields.empty())
    {
      cellAverage.GetFieldsToPass().SetMode(viskores::filter::FieldSelection::Mode::Select);
      for (auto& f : fields)
        cellAverage.GetFieldsToPass().AddField(f);
    }

    result = cellAverage.Execute(partitions);

    if (fields.empty() || std::find(fields.begin(), fields.end(), "ids") != fields.end())
    {
      VISKORES_TEST_ASSERT(result.HasPartitionsField("ids"), "Missing field on result");
      auto field0 = result.GetField("ids");
      auto portal0 =
        field0.GetData().AsArrayHandle<viskores::cont::ArrayHandle<viskores::Id>>().ReadPortal();
      VISKORES_TEST_ASSERT(portal0.GetNumberOfValues() == static_cast<viskores::Id>(ids.size()),
                           "Wrong number of field values.");
      for (std::size_t i = 0; i < ids.size(); i++)
        VISKORES_TEST_ASSERT(portal0.Get(static_cast<viskores::Id>(i)) == ids[i],
                             "Wrong field value.");
    }
    else
    {
      VISKORES_TEST_ASSERT(!result.HasPartitionsField("ids"), "Field should not be on result");
    }

    if (fields.empty() || std::find(fields.begin(), fields.end(), "scalar") != fields.end())
    {
      VISKORES_TEST_ASSERT(result.HasGlobalField("scalar"), "Missing field on result");
      auto field1 = result.GetField("scalar");
      auto portal1 = field1.GetData()
                       .AsArrayHandle<viskores::cont::ArrayHandle<viskores::FloatDefault>>()
                       .ReadPortal();
      VISKORES_TEST_ASSERT(portal1.GetNumberOfValues() == static_cast<viskores::Id>(scalar.size()),
                           "Wrong number of field values.");
      VISKORES_TEST_ASSERT(portal1.Get(0) == scalar[0], "Wrong field value.");
    }
    else
    {
      VISKORES_TEST_ASSERT(!result.HasGlobalField("scalar"), "Field should not be on result");
    }
  }
}

int UnitTestPartitionedDataSetFilters(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestPartitionedDataSetFilters, argc, argv);
}
