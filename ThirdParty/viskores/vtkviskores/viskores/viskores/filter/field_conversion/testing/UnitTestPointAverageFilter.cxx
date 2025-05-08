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

#include <viskores/cont/testing/MakeTestDataSet.h>
#include <viskores/cont/testing/Testing.h>
#include <viskores/filter/field_conversion/PointAverage.h>

namespace
{

void TestPointAverageUniform3D()
{
  std::cout << "Testing PointAverage Filter on 3D structured data" << std::endl;

  viskores::cont::testing::MakeTestDataSet testDataSet;
  viskores::cont::DataSet dataSet = testDataSet.Make3DUniformDataSet0();

  viskores::filter::field_conversion::PointAverage pointAverage;
  pointAverage.SetOutputFieldName("avgvals");
  pointAverage.SetActiveField("cellvar");
  auto result = pointAverage.Execute(dataSet);

  VISKORES_TEST_ASSERT(result.HasPointField("avgvals"), "Field missing.");
  viskores::cont::ArrayHandle<viskores::Float32> resultArrayHandle;
  result.GetPointField("avgvals").GetData().AsArrayHandle(resultArrayHandle);
  viskores::Float32 expected[18] = { 100.1f, 100.15f, 100.2f, 100.1f, 100.15f, 100.2f,
                                     100.2f, 100.25f, 100.3f, 100.2f, 100.25f, 100.3f,
                                     100.3f, 100.35f, 100.4f, 100.3f, 100.35f, 100.4f };
  for (viskores::Id i = 0; i < 18; ++i)
  {
    VISKORES_TEST_ASSERT(test_equal(resultArrayHandle.ReadPortal().Get(i), expected[i]),
                         "Wrong result for PointAverage worklet on 3D regular data");
  }
}

void TestPointAverageRegular3D()
{
  std::cout << "Testing PointAverage Filter on 2D structured data" << std::endl;

  viskores::cont::testing::MakeTestDataSet testDataSet;
  viskores::cont::DataSet dataSet = testDataSet.Make3DRectilinearDataSet0();

  viskores::filter::field_conversion::PointAverage pointAverage;
  pointAverage.SetActiveField("cellvar");
  auto result = pointAverage.Execute(dataSet);

  // If no name is given, should have the same name as the input.
  VISKORES_TEST_ASSERT(result.HasPointField("cellvar"), "Field missing.");
  viskores::cont::Field Result = result.GetPointField("cellvar");
  viskores::cont::ArrayHandle<viskores::Float32> resultArrayHandle;
  Result.GetData().AsArrayHandle(resultArrayHandle);

  viskores::Float32 expected[18] = { 0.f, 0.5f, 1.f, 0.f, 0.5f, 1.f, 1.f, 1.5f, 2.f,
                                     1.f, 1.5f, 2.f, 2.f, 2.5f, 3.f, 2.f, 2.5f, 3.f };
  for (viskores::Id i = 0; i < 18; ++i)
  {
    VISKORES_TEST_ASSERT(test_equal(resultArrayHandle.ReadPortal().Get(i), expected[i]),
                         "Wrong result for PointAverage worklet on 3D regular data");
  }
}

void TestPointAverageExplicit1()
{
  std::cout << "Testing PointAverage Filter on Explicit data" << std::endl;

  viskores::cont::testing::MakeTestDataSet testDataSet;
  viskores::cont::DataSet dataSet = testDataSet.Make3DExplicitDataSet1();

  viskores::filter::field_conversion::PointAverage pointAverage;
  pointAverage.SetActiveField("cellvar");
  auto result = pointAverage.Execute(dataSet);

  // If no name is given, should have the same name as the input.
  VISKORES_TEST_ASSERT(result.HasPointField("cellvar"), "Field missing.");
  viskores::cont::ArrayHandle<viskores::Float32> resultArrayHandle;
  result.GetPointField("cellvar").GetData().AsArrayHandle(resultArrayHandle);
  viskores::Float32 expected[5] = { 100.1f, 100.15f, 100.15f, 100.2f, 100.2f };
  for (int i = 0; i < 5; ++i)
  {
    VISKORES_TEST_ASSERT(test_equal(resultArrayHandle.ReadPortal().Get(i), expected[i]),
                         "Wrong result for PointAverage worklet on 3D regular data");
  }
}

void TestPointAverageExplicit2()
{
  std::cout << "Testing PointAverage Filter on Explicit data" << std::endl;

  viskores::cont::testing::MakeTestDataSet testDataSet;
  viskores::cont::DataSet dataSet = testDataSet.Make3DExplicitDataSet5();

  viskores::filter::field_conversion::PointAverage pointAverage;
  pointAverage.SetActiveField("cellvar");
  auto result = pointAverage.Execute(dataSet);

  // If no name is given, should have the same name as the input.
  VISKORES_TEST_ASSERT(result.HasPointField("cellvar"), "Field missing.");
  viskores::cont::ArrayHandle<viskores::Float32> resultArrayHandle;
  result.GetPointField("cellvar").GetData().AsArrayHandle(resultArrayHandle);
  viskores::Float32 expected[11] = { 100.1f, 105.05f, 105.05f, 100.1f, 115.3f, 115.2f,
                                     115.2f, 115.3f,  115.1f,  130.5f, 125.35f };
  for (int i = 0; i < 11; ++i)
  {
    VISKORES_TEST_ASSERT(test_equal(resultArrayHandle.ReadPortal().Get(i), expected[i]),
                         "Wrong result for PointAverage worklet on 3D regular data");
  }
}

void TestPointAverage()
{
  TestPointAverageUniform3D();
  TestPointAverageRegular3D();
  TestPointAverageExplicit1();
  TestPointAverageExplicit2();
}
}

int UnitTestPointAverageFilter(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestPointAverage, argc, argv);
}
