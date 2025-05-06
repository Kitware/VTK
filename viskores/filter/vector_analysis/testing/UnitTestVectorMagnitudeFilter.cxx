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
#include <viskores/filter/vector_analysis/VectorMagnitude.h>

#include <vector>

namespace
{

void TestVectorMagnitude()
{
  std::cout << "Testing VectorMagnitude Filter" << std::endl;

  viskores::cont::testing::MakeTestDataSet testDataSet;
  viskores::cont::DataSet dataSet = testDataSet.Make3DUniformDataSet0();

  const int nVerts = 18;
  viskores::Float64 fvars[nVerts] = {
    10.1,  20.1,  30.1,  40.1,  50.2,  60.2,  70.2,  80.2,  90.3,
    100.3, 110.3, 120.3, 130.4, 140.4, 150.4, 160.4, 170.5, 180.5
  };

  std::vector<viskores::Vec3f_64> fvec(nVerts);
  for (std::size_t i = 0; i < fvec.size(); ++i)
  {
    fvec[i] = viskores::make_Vec(fvars[i], fvars[i], fvars[i]);
  }
  viskores::cont::ArrayHandle<viskores::Vec3f_64> finput =
    viskores::cont::make_ArrayHandle(fvec, viskores::CopyFlag::On);

  dataSet.AddPointField("double_vec_pointvar", finput);

  viskores::filter::vector_analysis::VectorMagnitude vm;
  vm.SetActiveField("double_vec_pointvar");
  auto result = vm.Execute(dataSet);

  VISKORES_TEST_ASSERT(result.HasPointField("magnitude"), "Output field missing.");

  viskores::cont::ArrayHandle<viskores::Float64> resultArrayHandle;
  result.GetPointField("magnitude").GetData().AsArrayHandle(resultArrayHandle);
  for (viskores::Id i = 0; i < resultArrayHandle.GetNumberOfValues(); ++i)
  {
    VISKORES_TEST_ASSERT(
      test_equal(std::sqrt(3 * fvars[i] * fvars[i]), resultArrayHandle.ReadPortal().Get(i)),
      "Wrong result for Magnitude worklet");
  }
}
}

int UnitTestVectorMagnitudeFilter(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestVectorMagnitude, argc, argv);
}
