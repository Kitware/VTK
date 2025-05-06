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
#include <viskores/cont/CellSetSingleType.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/DataSetBuilderExplicit.h>
#include <viskores/cont/DeviceAdapterAlgorithm.h>

#include <viskores/cont/testing/MakeTestDataSet.h>
#include <viskores/cont/testing/Testing.h>

#include <viskores/filter/field_conversion/worklet/CellAverage.h>
#include <viskores/worklet/DispatcherMapTopology.h>

namespace
{

template <typename T, typename Storage>
bool TestArrayHandle(const viskores::cont::ArrayHandle<T, Storage>& ah,
                     const T* expected,
                     viskores::Id size)
{
  if (size != ah.GetNumberOfValues())
  {
    return false;
  }

  auto ahPortal = ah.ReadPortal();
  for (viskores::Id i = 0; i < size; ++i)
  {
    if (ahPortal.Get(i) != expected[i])
    {
      return false;
    }
  }

  return true;
}

inline viskores::cont::DataSet make_SingleTypeDataSet()
{
  using CoordType = viskores::Vec3f_32;
  std::vector<CoordType> coordinates;
  coordinates.push_back(CoordType(0, 0, 0));
  coordinates.push_back(CoordType(1, 0, 0));
  coordinates.push_back(CoordType(1, 1, 0));
  coordinates.push_back(CoordType(2, 1, 0));
  coordinates.push_back(CoordType(2, 2, 0));

  std::vector<viskores::Id> conn;
  // First Cell
  conn.push_back(0);
  conn.push_back(1);
  conn.push_back(2);
  // Second Cell
  conn.push_back(1);
  conn.push_back(2);
  conn.push_back(3);
  // Third Cell
  conn.push_back(2);
  conn.push_back(3);
  conn.push_back(4);

  viskores::cont::DataSet ds;
  viskores::cont::DataSetBuilderExplicit builder;
  ds = builder.Create(coordinates, viskores::CellShapeTagTriangle(), 3, conn);

  //Set point scalar
  const int nVerts = 5;
  viskores::Float32 vars[nVerts] = { 10.1f, 20.1f, 30.2f, 40.2f, 50.3f };

  ds.AddPointField("pointvar", vars, nVerts);

  return ds;
}

void TestDataSet_Explicit()
{

  viskores::cont::DataSet dataSet = make_SingleTypeDataSet();

  //iterate the 2nd cell 4 times
  viskores::cont::ArrayHandle<viskores::Id> validCellIds =
    viskores::cont::make_ArrayHandle<viskores::Id>({ 1, 1, 1, 1 });

  //get the cellset single type from the dataset
  viskores::cont::CellSetSingleType<> cellSet;
  dataSet.GetCellSet().AsCellSet(cellSet);

  //verify that we can create a subset of a singlset
  using SubsetType = viskores::cont::CellSetPermutation<viskores::cont::CellSetSingleType<>>;
  SubsetType subset;
  subset.Fill(validCellIds, cellSet);

  subset.PrintSummary(std::cout);

  //run a basic for-each topology algorithm on this
  viskores::cont::ArrayHandle<viskores::Float32> result;
  viskores::worklet::DispatcherMapTopology<viskores::worklet::CellAverage> dispatcher;
  dispatcher.Invoke(subset,
                    dataSet.GetField("pointvar")
                      .GetData()
                      .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Float32>>(),
                    result);

  //iterate same cell 4 times
  viskores::Float32 expected[4] = { 30.1667f, 30.1667f, 30.1667f, 30.1667f };
  auto resultPortal = result.ReadPortal();
  for (int i = 0; i < 4; ++i)
  {
    VISKORES_TEST_ASSERT(test_equal(resultPortal.Get(i), expected[i]),
                         "Wrong result for CellAverage worklet on explicit subset data");
  }
}

void TestDataSet_Structured2D()
{

  viskores::cont::testing::MakeTestDataSet testDataSet;
  viskores::cont::DataSet dataSet = testDataSet.Make2DUniformDataSet0();

  //iterate the 2nd cell 4 times
  viskores::cont::ArrayHandle<viskores::Id> validCellIds =
    viskores::cont::make_ArrayHandle<viskores::Id>({ 1, 1, 1, 1 });

  viskores::cont::CellSetStructured<2> cellSet;
  dataSet.GetCellSet().AsCellSet(cellSet);

  //verify that we can create a subset of a 2d UniformDataSet
  viskores::cont::CellSetPermutation<viskores::cont::CellSetStructured<2>> subset;
  subset.Fill(validCellIds, cellSet);

  subset.PrintSummary(std::cout);

  //run a basic for-each topology algorithm on this
  viskores::cont::ArrayHandle<viskores::Float32> result;
  viskores::worklet::DispatcherMapTopology<viskores::worklet::CellAverage> dispatcher;
  dispatcher.Invoke(subset,
                    dataSet.GetField("pointvar")
                      .GetData()
                      .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Float32>>(),
                    result);

  viskores::Float32 expected[4] = { 40.1f, 40.1f, 40.1f, 40.1f };
  auto resultPortal = result.ReadPortal();
  for (int i = 0; i < 4; ++i)
  {
    VISKORES_TEST_ASSERT(test_equal(resultPortal.Get(i), expected[i]),
                         "Wrong result for CellAverage worklet on 2d structured subset data");
  }
}

void TestDataSet_Structured3D()
{

  viskores::cont::testing::MakeTestDataSet testDataSet;
  viskores::cont::DataSet dataSet = testDataSet.Make3DUniformDataSet0();

  //iterate the 2nd cell 4 times
  viskores::cont::ArrayHandle<viskores::Id> validCellIds =
    viskores::cont::make_ArrayHandle<viskores::Id>({ 1, 1, 1, 1 });

  viskores::cont::CellSetStructured<3> cellSet;
  dataSet.GetCellSet().AsCellSet(cellSet);

  //verify that we can create a subset of a 2d UniformDataSet
  viskores::cont::CellSetPermutation<viskores::cont::CellSetStructured<3>> subset;
  subset.Fill(validCellIds, cellSet);

  subset.PrintSummary(std::cout);

  //run a basic for-each topology algorithm on this
  viskores::cont::ArrayHandle<viskores::Float32> result;
  viskores::worklet::DispatcherMapTopology<viskores::worklet::CellAverage> dispatcher;
  dispatcher.Invoke(subset,
                    dataSet.GetField("pointvar")
                      .GetData()
                      .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Float32>>(),
                    result);

  viskores::Float32 expected[4] = { 70.2125f, 70.2125f, 70.2125f, 70.2125f };
  auto resultPortal = result.ReadPortal();
  for (int i = 0; i < 4; ++i)
  {
    VISKORES_TEST_ASSERT(test_equal(resultPortal.Get(i), expected[i]),
                         "Wrong result for CellAverage worklet on 2d structured subset data");
  }
}

void TestDataSet_Permutation()
{
  std::cout << std::endl;
  std::cout << "--TestDataSet_Permutation--" << std::endl << std::endl;

  TestDataSet_Explicit();
  TestDataSet_Structured2D();
  TestDataSet_Structured3D();
}
}

int UnitTestDataSetPermutation(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestDataSet_Permutation, argc, argv);
}
