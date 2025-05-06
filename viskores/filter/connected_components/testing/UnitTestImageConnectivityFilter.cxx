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

#include <viskores/filter/connected_components/ImageConnectivity.h>

#include <viskores/cont/DataSet.h>

#include <viskores/cont/DataSetBuilderUniform.h>
#include <viskores/cont/testing/Testing.h>

namespace
{

viskores::cont::DataSet MakeTestDataSet()
{
  // example from Figure 35.7 of Connected Component Labeling in CUDA by OndˇrejˇŚtava,
  // Bedˇrich Beneˇ
  std::vector<viskores::UInt8> pixels{
    0, 1, 1, 1, 0, 1, 1, 0, 0, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 1, 1, 0,
    0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0,
  };

  viskores::cont::DataSet dataSet =
    viskores::cont::DataSetBuilderUniform::Create(viskores::Id3(8, 8, 1));

  dataSet.AddPointField("color", pixels);

  return dataSet;
}

void TestImageConnectivity()
{
  viskores::cont::DataSet dataSet = MakeTestDataSet();

  viskores::filter::connected_components::ImageConnectivity connectivity;
  connectivity.SetActiveField("color");

  const viskores::cont::DataSet outputData = connectivity.Execute(dataSet);

  auto temp = outputData.GetField("component").GetData();
  viskores::cont::ArrayHandle<viskores::Id> resultArrayHandle;
  temp.AsArrayHandle(resultArrayHandle);

  std::vector<viskores::Id> componentExpected = { 0, 1, 1, 1, 0, 1, 1, 2, 0, 0, 0, 1, 0, 1, 1, 2,
                                                  0, 1, 1, 0, 0, 1, 1, 2, 0, 1, 0, 0, 0, 1, 1, 2,
                                                  0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1,
                                                  0, 1, 0, 1, 1, 1, 3, 3, 0, 1, 1, 1, 1, 1, 3, 3 };

  for (viskores::Id i = 0; i < resultArrayHandle.GetNumberOfValues(); ++i)
  {
    VISKORES_TEST_ASSERT(
      test_equal(resultArrayHandle.ReadPortal().Get(i), componentExpected[size_t(i)]),
      "Wrong result for ImageConnectivity");
  }
}
}

int UnitTestImageConnectivityFilter(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestImageConnectivity, argc, argv);
}
