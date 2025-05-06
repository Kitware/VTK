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
#include <viskores/filter/field_conversion/CellAverage.h>

namespace
{

void TestCellAverageRegular3D()
{
  std::cout << "Testing CellAverage Filter on 3D structured data" << std::endl;

  viskores::cont::testing::MakeTestDataSet testDataSet;
  viskores::cont::DataSet dataSet = testDataSet.Make3DUniformDataSet0();

  viskores::filter::field_conversion::CellAverage cellAverage;
  cellAverage.SetOutputFieldName("avgvals");
  cellAverage.SetActiveField("pointvar");
  viskores::cont::DataSet result = cellAverage.Execute(dataSet);

  VISKORES_TEST_ASSERT(result.HasCellField("avgvals") == true, "Result field not present.");

  viskores::cont::ArrayHandle<viskores::Float32> resultArrayHandle;
  result.GetCellField("avgvals").GetData().AsArrayHandle(resultArrayHandle);
  {
    viskores::Float32 expected[4] = { 60.1875f, 70.2125f, 120.3375f, 130.3625f };
    for (viskores::Id i = 0; i < 4; ++i)
    {
      VISKORES_TEST_ASSERT(test_equal(resultArrayHandle.ReadPortal().Get(i), expected[i]),
                           "Wrong result for CellAverage worklet on 3D regular data");
    }
  }

  std::cout << "Run again for point coordinates" << std::endl;
  cellAverage.SetOutputFieldName("avgpos");
  cellAverage.SetUseCoordinateSystemAsField(true);
  result = cellAverage.Execute(dataSet);

  VISKORES_TEST_ASSERT(result.HasCellField("avgpos"), "Result field not present.");

  viskores::cont::ArrayHandle<viskores::Vec3f> resultPointArray;
  viskores::cont::Field resultPointField = result.GetCellField("avgpos");
  resultPointField.GetData().AsArrayHandle(resultPointArray);
  {
    viskores::FloatDefault expected[4][3] = {
      { 0.5f, 0.5f, 0.5f }, { 1.5f, 0.5f, 0.5f }, { 0.5f, 0.5f, 1.5f }, { 1.5f, 0.5f, 1.5f }
    };
    for (viskores::Id i = 0; i < 4; ++i)
    {
      viskores::Vec3f expectedVec(expected[i][0], expected[i][1], expected[i][2]);
      viskores::Vec3f computedVec(resultPointArray.ReadPortal().Get(i));
      VISKORES_TEST_ASSERT(test_equal(computedVec, expectedVec),
                           "Wrong result for CellAverage worklet on 3D regular data");
    }
  }
}

void TestCellAverageRegular2D()
{
  std::cout << "Testing CellAverage Filter on 2D structured data" << std::endl;

  viskores::cont::testing::MakeTestDataSet testDataSet;
  viskores::cont::DataSet dataSet = testDataSet.Make2DUniformDataSet0();

  viskores::filter::field_conversion::CellAverage cellAverage;
  cellAverage.SetActiveField("pointvar");

  viskores::cont::DataSet result = cellAverage.Execute(dataSet);

  // If no name is given, should have the same name as the input.
  VISKORES_TEST_ASSERT(result.HasPointField("pointvar"), "Field missing.");

  viskores::cont::Field resultField = result.GetCellField("pointvar");
  viskores::cont::ArrayHandle<viskores::Float32> resultArrayHandle;
  resultField.GetData().AsArrayHandle(resultArrayHandle);
  viskores::Float32 expected[2] = { 30.1f, 40.1f };
  for (int i = 0; i < 2; ++i)
  {
    VISKORES_TEST_ASSERT(test_equal(resultArrayHandle.ReadPortal().Get(i), expected[i]),
                         "Wrong result for CellAverage worklet on 2D regular data");
  }
}

void TestCellAverageExplicit()
{
  std::cout << "Testing CellAverage Filter on Explicit data" << std::endl;

  viskores::cont::testing::MakeTestDataSet testDataSet;
  viskores::cont::DataSet dataSet = testDataSet.Make3DExplicitDataSet0();

  viskores::filter::field_conversion::CellAverage cellAverage;
  cellAverage.SetActiveField("pointvar");

  viskores::cont::DataSet result = cellAverage.Execute(dataSet);

  // If no name is given, should have the same name as the input.
  VISKORES_TEST_ASSERT(result.HasCellField("pointvar"), "Field missing.");

  viskores::cont::ArrayHandle<viskores::Float32> resultArrayHandle;
  viskores::cont::Field resultField = result.GetCellField("pointvar");
  resultField.GetData().AsArrayHandle(resultArrayHandle);
  viskores::Float32 expected[2] = { 20.1333f, 35.2f };
  for (int i = 0; i < 2; ++i)
  {
    VISKORES_TEST_ASSERT(test_equal(resultArrayHandle.ReadPortal().Get(i), expected[i]),
                         "Wrong result for CellAverage worklet on 3D regular data");
  }
}

void TestCellAverage()
{
  TestCellAverageRegular2D();
  TestCellAverageRegular3D();
  TestCellAverageExplicit();
}
}

int UnitTestCellAverageFilter(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestCellAverage, argc, argv);
}
