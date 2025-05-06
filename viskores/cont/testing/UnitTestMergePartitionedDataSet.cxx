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

#include <viskores/Bounds.h>
#include <viskores/cont/BoundsCompute.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/FieldRangeCompute.h>
#include <viskores/cont/MergePartitionedDataSet.h>
#include <viskores/cont/PartitionedDataSet.h>
#include <viskores/cont/testing/MakeTestDataSet.h>
#include <viskores/cont/testing/Testing.h>

static void MergePartitionedDataSetTest()
{
  viskores::cont::testing::MakeTestDataSet testDataSet;
  viskores::cont::PartitionedDataSet pds;

  viskores::cont::DataSet TDset1 = testDataSet.Make2DUniformDataSet0();
  viskores::cont::DataSet TDset2 = testDataSet.Make3DUniformDataSet0();

  pds.AppendPartition(TDset1);
  pds.AppendPartition(TDset2);

  viskores::cont::DataSet mergedDataset = viskores::cont::MergePartitionedDataSet(pds);

  VISKORES_TEST_ASSERT(TDset1.GetNumberOfFields() == mergedDataset.GetNumberOfFields(),
                       "Incorrect number of fields");
  VISKORES_TEST_ASSERT(TDset2.GetNumberOfFields() == mergedDataset.GetNumberOfFields(),
                       "Incorrect number of fields");

  VISKORES_TEST_ASSERT(TDset1.GetNumberOfCoordinateSystems() ==
                         mergedDataset.GetNumberOfCoordinateSystems(),
                       "Incorrect number of coordinate systems");

  viskores::Bounds Set1Bounds = TDset1.GetCoordinateSystem(0).GetBounds();
  viskores::Bounds Set2Bounds = TDset2.GetCoordinateSystem(0).GetBounds();
  viskores::Bounds GlobalBound;
  GlobalBound.Include(Set1Bounds);
  GlobalBound.Include(Set2Bounds);

  VISKORES_TEST_ASSERT(viskores::cont::BoundsCompute(mergedDataset) == GlobalBound,
                       "Global bounds info incorrect");

  viskores::Range MergedField1Range;
  viskores::Range MergedField2Range;
  viskores::Range Set1Field1Range;
  viskores::Range Set1Field2Range;
  viskores::Range Set2Field1Range;
  viskores::Range Set2Field2Range;
  viskores::Range Field1GlobeRange;
  viskores::Range Field2GlobeRange;

  mergedDataset.GetField("pointvar").GetRange(&MergedField1Range);
  mergedDataset.GetField("cellvar").GetRange(&MergedField2Range);
  TDset1.GetField("pointvar").GetRange(&Set1Field1Range);
  TDset1.GetField("cellvar").GetRange(&Set1Field2Range);
  TDset2.GetField("pointvar").GetRange(&Set2Field1Range);
  TDset2.GetField("cellvar").GetRange(&Set2Field2Range);

  Field1GlobeRange.Include(Set1Field1Range);
  Field1GlobeRange.Include(Set2Field1Range);
  Field2GlobeRange.Include(Set1Field2Range);
  Field2GlobeRange.Include(Set2Field2Range);

  using viskores::cont::FieldRangeCompute;
  VISKORES_TEST_ASSERT(MergedField1Range == Field1GlobeRange,
                       "Local field value range info incorrect");
  VISKORES_TEST_ASSERT(MergedField2Range == Field2GlobeRange,
                       "Local field value range info incorrect");

  VISKORES_TEST_ASSERT(mergedDataset.GetNumberOfPoints() ==
                         TDset1.GetNumberOfPoints() + TDset2.GetNumberOfPoints(),
                       "Incorrect number of points");
  VISKORES_TEST_ASSERT(mergedDataset.GetNumberOfCells() ==
                         TDset1.GetNumberOfCells() + TDset2.GetNumberOfCells(),
                       "Incorrect number of cells");
}

int UnitTestMergePartitionedDataSet(int argc, char* argv[])
{
  //More test cases can be found in the viskores/filter/multi_block/testing/UnitTestMergeDataSetsFilter.cxx
  //which is a filter that wraps MergePartitionedDataSet algorithm.
  return viskores::cont::testing::Testing::Run(MergePartitionedDataSetTest, argc, argv);
}
