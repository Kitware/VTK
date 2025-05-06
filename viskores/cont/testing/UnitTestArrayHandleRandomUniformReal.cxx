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

#include <viskores/cont/ArrayHandleRandomUniformReal.h>
#include <viskores/cont/testing/Testing.h>
#include <viskores/worklet/DescriptiveStatistics.h>

void TestRangeBounds()
{
  // the random numbers should fall into the range of [0, 1).
  auto array = viskores::cont::ArrayHandleRandomUniformReal<viskores::Float32>(100, { 0xceed });
  auto portal = array.ReadPortal();
  for (viskores::Id i = 0; i < array.GetNumberOfValues(); ++i)
  {
    auto value = portal.Get(i);
    VISKORES_TEST_ASSERT(0.0 <= value && value < 1.0);
  }
}

void TestStatisticsProperty()
{
  auto array = viskores::cont::ArrayHandleRandomUniformReal<viskores::Float32>(10000, { 0xceed });
  auto result = viskores::worklet::DescriptiveStatistics::Run(array);

  VISKORES_TEST_ASSERT(test_equal(result.Mean(), 0.5, 0.001));
  VISKORES_TEST_ASSERT(test_equal(result.SampleVariance(), 1.0 / 12.0, 0.001));
}

void TestArrayHandleUniformReal()
{
  TestRangeBounds();
  TestStatisticsProperty();
}

int UnitTestArrayHandleRandomUniformReal(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestArrayHandleUniformReal, argc, argv);
}
