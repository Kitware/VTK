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
#include <viskores/filter/image_processing/ImageMedian.h>

namespace
{

void TestImageMedian()
{
  std::cout << "Testing Image Median Filter on 3D structured data" << std::endl;

  viskores::cont::testing::MakeTestDataSet testDataSet;
  viskores::cont::DataSet dataSet = testDataSet.Make3DUniformDataSet2();

  viskores::filter::image_processing::ImageMedian median;
  median.Perform3x3();
  median.SetActiveField("pointvar");
  auto result = median.Execute(dataSet);

  VISKORES_TEST_ASSERT(result.HasPointField("median"), "Field missing.");
  viskores::cont::ArrayHandle<viskores::Float32> resultArrayHandle;
  result.GetPointField("median").GetData().AsArrayHandle(resultArrayHandle);

  auto cells = result.GetCellSet().AsCellSet<viskores::cont::CellSetStructured<3>>();
  auto pdims = cells.GetPointDimensions();

  //verified by hand
  {
    auto portal = resultArrayHandle.ReadPortal();
    viskores::Float32 expected_median = portal.Get(1 + pdims[0]);
    VISKORES_TEST_ASSERT(test_equal(expected_median, 2), "incorrect median value");

    expected_median = portal.Get(1 + pdims[0] + (pdims[1] * pdims[0] * 2));
    VISKORES_TEST_ASSERT(test_equal(expected_median, 2.82843), "incorrect median value");
  }
}
}

int UnitTestImageMedianFilter(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestImageMedian, argc, argv);
}
