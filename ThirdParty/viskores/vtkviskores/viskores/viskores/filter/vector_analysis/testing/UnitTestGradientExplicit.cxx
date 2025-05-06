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

#include <viskores/filter/vector_analysis/Gradient.h>

#include <viskores/cont/testing/MakeTestDataSet.h>
#include <viskores/cont/testing/Testing.h>

namespace
{

void TestCellGradientExplicit()
{
  std::cout << "Testing Gradient Filter with cell output on Explicit data" << std::endl;

  viskores::cont::testing::MakeTestDataSet testDataSet;
  viskores::cont::DataSet dataSet = testDataSet.Make3DExplicitDataSet0();

  viskores::filter::vector_analysis::Gradient gradient;
  gradient.SetOutputFieldName("gradient");
  gradient.SetActiveField("pointvar");

  viskores::cont::DataSet result = gradient.Execute(dataSet);

  VISKORES_TEST_ASSERT(result.HasCellField("gradient"), "Result field missing.");

  viskores::cont::ArrayHandle<viskores::Vec3f_32> resultArrayHandle;
  result.GetCellField("gradient").GetData().AsArrayHandle(resultArrayHandle);
  viskores::Vec3f_32 expected[2] = { { 10.f, 10.1f, 0.0f }, { 10.f, 10.1f, -0.0f } };
  for (int i = 0; i < 2; ++i)
  {
    VISKORES_TEST_ASSERT(test_equal(resultArrayHandle.ReadPortal().Get(i), expected[i]),
                         "Wrong result for CellGradient filter on 3D explicit data");
  }
}

void TestPointGradientExplicit()
{
  std::cout << "Testing Gradient Filter with point output on Explicit data" << std::endl;

  viskores::cont::testing::MakeTestDataSet testDataSet;
  viskores::cont::DataSet dataSet = testDataSet.Make3DExplicitDataSet0();

  viskores::filter::vector_analysis::Gradient gradient;
  gradient.SetComputePointGradient(true);
  gradient.SetOutputFieldName("gradient");
  gradient.SetActiveField("pointvar");

  viskores::cont::DataSet result = gradient.Execute(dataSet);

  VISKORES_TEST_ASSERT(result.HasPointField("gradient"), "Result field missing.");

  viskores::cont::ArrayHandle<viskores::Vec3f_32> resultArrayHandle;
  result.GetPointField("gradient").GetData().AsArrayHandle(resultArrayHandle);

  viskores::Vec3f_32 expected[2] = { { 10.f, 10.1f, 0.0f }, { 10.f, 10.1f, 0.0f } };
  for (int i = 0; i < 2; ++i)
  {
    VISKORES_TEST_ASSERT(test_equal(resultArrayHandle.ReadPortal().Get(i), expected[i]),
                         "Wrong result for CellGradient filter on 3D explicit data");
  }
}


void TestGradient()
{
  TestCellGradientExplicit();
  TestPointGradientExplicit();
}
}

int UnitTestGradientExplicit(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestGradient, argc, argv);
}
