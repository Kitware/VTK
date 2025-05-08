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

#include <viskores/Bounds.h>
#include <viskores/VectorAnalysis.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/BoundsCompute.h>
#include <viskores/cont/CellSetStructured.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/FieldRangeCompute.h>
#include <viskores/cont/PartitionedDataSet.h>
#include <viskores/cont/serial/DeviceAdapterSerial.h>
#include <viskores/cont/testing/MakeTestDataSet.h>
#include <viskores/cont/testing/Testing.h>
#include <viskores/exec/ConnectivityStructured.h>
#include <viskores/thirdparty/diy/Configure.h>

#include <viskores/thirdparty/diy/diy.h>

void DataSet_Compare(viskores::cont::DataSet& LeftDateSet, viskores::cont::DataSet& RightDateSet);
static void PartitionedDataSetTest()
{
  viskores::cont::testing::MakeTestDataSet testDataSet;
  viskores::cont::PartitionedDataSet pds;

  viskores::cont::DataSet TDset1 = testDataSet.Make2DUniformDataSet0();
  viskores::cont::DataSet TDset2 = testDataSet.Make3DUniformDataSet0();

  pds.AppendPartition(TDset1);
  pds.AppendPartition(TDset2);

  std::vector<viskores::Id> ids = { 0, 1 };
  std::vector<viskores::FloatDefault> var = { 1, 2 };
  auto idsField = viskores::cont::make_Field(
    "ids", viskores::cont::Field::Association::Partitions, ids, viskores::CopyFlag::On);
  auto pdsVar = viskores::cont::make_Field(
    "pds_var", viskores::cont::Field::Association::Partitions, ids, viskores::CopyFlag::On);
  pds.AddField(idsField);
  pds.AddField(pdsVar);

  VISKORES_TEST_ASSERT(pds.GetNumberOfPartitions() == 2, "Incorrect number of partitions");
  VISKORES_TEST_ASSERT(pds.GetNumberOfFields() == 2, "Incorrect number of fields");

  viskores::cont::DataSet TestDSet = pds.GetPartition(0);
  VISKORES_TEST_ASSERT(TDset1.GetNumberOfFields() == TestDSet.GetNumberOfFields(),
                       "Incorrect number of fields");
  VISKORES_TEST_ASSERT(TDset1.GetNumberOfCoordinateSystems() ==
                         TestDSet.GetNumberOfCoordinateSystems(),
                       "Incorrect number of coordinate systems");

  TestDSet = pds.GetPartition(1);
  VISKORES_TEST_ASSERT(TDset2.GetNumberOfFields() == TestDSet.GetNumberOfFields(),
                       "Incorrect number of fields");
  VISKORES_TEST_ASSERT(TDset2.GetNumberOfCoordinateSystems() ==
                         TestDSet.GetNumberOfCoordinateSystems(),
                       "Incorrect number of coordinate systems");

  viskores::Bounds Set1Bounds = TDset1.GetCoordinateSystem(0).GetBounds();
  viskores::Bounds Set2Bounds = TDset2.GetCoordinateSystem(0).GetBounds();
  viskores::Bounds GlobalBound;
  GlobalBound.Include(Set1Bounds);
  GlobalBound.Include(Set2Bounds);

  VISKORES_TEST_ASSERT(viskores::cont::BoundsCompute(pds) == GlobalBound,
                       "Global bounds info incorrect");
  VISKORES_TEST_ASSERT(viskores::cont::BoundsCompute(pds.GetPartition(0)) == Set1Bounds,
                       "Local bounds info incorrect");
  VISKORES_TEST_ASSERT(viskores::cont::BoundsCompute(pds.GetPartition(1)) == Set2Bounds,
                       "Local bounds info incorrect");

  viskores::Range Set1Field1Range;
  viskores::Range Set1Field2Range;
  viskores::Range Set2Field1Range;
  viskores::Range Set2Field2Range;
  viskores::Range Field1GlobeRange;
  viskores::Range Field2GlobeRange;

  TDset1.GetField("pointvar").GetRange(&Set1Field1Range);
  TDset1.GetField("cellvar").GetRange(&Set1Field2Range);
  TDset2.GetField("pointvar").GetRange(&Set2Field1Range);
  TDset2.GetField("cellvar").GetRange(&Set2Field2Range);

  Field1GlobeRange.Include(Set1Field1Range);
  Field1GlobeRange.Include(Set2Field1Range);
  Field2GlobeRange.Include(Set1Field2Range);
  Field2GlobeRange.Include(Set2Field2Range);

  using viskores::cont::FieldRangeCompute;
  VISKORES_TEST_ASSERT(FieldRangeCompute(pds, "pointvar").ReadPortal().Get(0) == Field1GlobeRange,
                       "Local field value range info incorrect");
  VISKORES_TEST_ASSERT(FieldRangeCompute(pds, "cellvar").ReadPortal().Get(0) == Field2GlobeRange,
                       "Local field value range info incorrect");

  viskores::Range SourceRange; //test the validity of member function GetField(FieldName, BlockId)
  pds.GetFieldFromPartition("cellvar", 0).GetRange(&SourceRange);
  viskores::Range TestRange;
  pds.GetPartition(0).GetField("cellvar").GetRange(&TestRange);
  VISKORES_TEST_ASSERT(TestRange == SourceRange, "Local field value info incorrect");

  //test partition fields.
  idsField.GetRange(&SourceRange);
  pds.GetField("ids").GetRange(&TestRange);
  VISKORES_TEST_ASSERT(TestRange == SourceRange, "Partitions field values incorrect");

  pdsVar.GetRange(&SourceRange);
  pds.GetField("pds_var").GetRange(&TestRange);
  VISKORES_TEST_ASSERT(TestRange == SourceRange, "Global field values incorrect");

  viskores::cont::PartitionedDataSet testblocks1;
  std::vector<viskores::cont::DataSet> partitions = pds.GetPartitions();
  testblocks1.AppendPartitions(partitions);
  VISKORES_TEST_ASSERT(pds.GetNumberOfPartitions() == testblocks1.GetNumberOfPartitions(),
                       "inconsistent number of partitions");

  viskores::cont::PartitionedDataSet testblocks2(2);
  testblocks2.InsertPartition(0, TDset1);
  testblocks2.InsertPartition(1, TDset2);

  TestDSet = testblocks2.GetPartition(0);
  DataSet_Compare(TDset1, TestDSet);

  TestDSet = testblocks2.GetPartition(1);
  DataSet_Compare(TDset2, TestDSet);

  testblocks2.ReplacePartition(0, TDset2);
  testblocks2.ReplacePartition(1, TDset1);

  TestDSet = testblocks2.GetPartition(0);
  DataSet_Compare(TDset2, TestDSet);

  TestDSet = testblocks2.GetPartition(1);
  DataSet_Compare(TDset1, TestDSet);
}

static void PartitionedDataSetFieldTest()
{
  viskores::cont::testing::MakeTestDataSet testDataSet;

  viskores::cont::DataSet TDset1 = testDataSet.Make2DUniformDataSet0();
  viskores::cont::DataSet TDset2 = testDataSet.Make3DUniformDataSet0();

  constexpr viskores::Id id0 = 0, id1 = 1;
  constexpr viskores::FloatDefault globalScalar = 1.0f;

  for (int i = 0; i < 4; i++)
  {
    viskores::cont::PartitionedDataSet pds({ TDset1, TDset2 });
    std::vector<viskores::Id> ids = { id0, id1 };
    std::vector<viskores::FloatDefault> gs = { globalScalar };

    auto idsArr = viskores::cont::make_ArrayHandle(ids, viskores::CopyFlag::Off);
    auto gsArr = viskores::cont::make_ArrayHandle(gs, viskores::CopyFlag::Off);

    if (i == 0) //field
    {
      auto idField = viskores::cont::make_Field(
        "id", viskores::cont::Field::Association::Partitions, ids, viskores::CopyFlag::Off);
      auto gScalar = viskores::cont::make_Field(
        "global_scalar", viskores::cont::Field::Association::Global, gs, viskores::CopyFlag::Off);

      pds.AddField(idField);
      pds.AddField(gScalar);
    }
    else if (i == 1) //array handle
    {
      pds.AddPartitionsField("id", idsArr);
      pds.AddGlobalField("global_scalar", gsArr);
    }
    else if (i == 2) //std::vector
    {
      pds.AddPartitionsField("id", ids);
      pds.AddGlobalField("global_scalar", gs);
    }
    else if (i == 3) //pointer
    {
      pds.AddPartitionsField("id", ids.data(), 2);
      pds.AddGlobalField("global_scalar", gs.data(), 1);
    }

    //Validate each method.
    VISKORES_TEST_ASSERT(pds.GetNumberOfFields() == 2, "Wrong number of fields");

    //Make sure fields are there and of the right type.
    VISKORES_TEST_ASSERT(pds.HasPartitionsField("id"), "id field misssing.");
    VISKORES_TEST_ASSERT(pds.HasGlobalField("global_scalar"), "global_scalar field misssing.");


    for (int j = 0; j < 2; j++)
    {
      viskores::cont::Field f0, f1;

      if (j == 0)
      {
        f0 = pds.GetField("id");
        f1 = pds.GetField("global_scalar");
      }
      else
      {
        f0 = pds.GetPartitionsField("id");
        f1 = pds.GetGlobalField("global_scalar");
      }

      //Check the values.
      auto portal0 =
        f0.GetData().AsArrayHandle<viskores::cont::ArrayHandle<viskores::Id>>().ReadPortal();
      auto portal1 = f1.GetData()
                       .AsArrayHandle<viskores::cont::ArrayHandle<viskores::FloatDefault>>()
                       .ReadPortal();

      VISKORES_TEST_ASSERT(portal0.GetNumberOfValues() == 2, "Wrong number of values in field");
      VISKORES_TEST_ASSERT(portal1.GetNumberOfValues() == 1, "Wrong number of values in field");

      VISKORES_TEST_ASSERT(portal0.Get(0) == id0 && portal0.Get(1) == id1, "Wrong field value");
      VISKORES_TEST_ASSERT(portal1.Get(0) == globalScalar, "Wrong field value");
    }
  }
}

void DataSet_Compare(viskores::cont::DataSet& leftDataSet, viskores::cont::DataSet& rightDataSet)
{
  for (viskores::Id j = 0; j < leftDataSet.GetNumberOfFields(); j++)
  {
    if (leftDataSet.HasCoordinateSystem(leftDataSet.GetField(j).GetName()))
    {
      // Skip coordinate systems, which have a different array type.
      continue;
    }
    viskores::cont::ArrayHandle<viskores::Float32> lDataArray;
    leftDataSet.GetField(j).GetData().AsArrayHandle(lDataArray);
    viskores::cont::ArrayHandle<viskores::Float32> rDataArray;
    rightDataSet.GetField(j).GetData().AsArrayHandle(rDataArray);
    VISKORES_TEST_ASSERT(lDataArray == rDataArray, "field value info incorrect");
  }
  return;
}

static void PartitionedDataSetTests()
{
  PartitionedDataSetTest();
  PartitionedDataSetFieldTest();
}

int UnitTestPartitionedDataSet(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(PartitionedDataSetTests, argc, argv);
}
