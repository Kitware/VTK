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
#include <viskores/cont/ArrayHandleRandomStandardNormal.h>
#include <viskores/cont/testing/Testing.h>
#include <viskores/worklet/DescriptiveStatistics.h>

void TestArrayHandleStandardNormal()
{
  auto array =
    viskores::cont::ArrayHandleRandomStandardNormal<viskores::Float32>(50000, { 0xceed });
  auto stats = viskores::worklet::DescriptiveStatistics::Run(array);

  VISKORES_TEST_ASSERT(test_equal(stats.Mean(), 0, 0.01));
  VISKORES_TEST_ASSERT(test_equal(stats.PopulationStddev(), 1, 0.01));
  VISKORES_TEST_ASSERT(test_equal(stats.Skewness(), 0.0f, 1.0f / 100));
  VISKORES_TEST_ASSERT(test_equal(stats.Kurtosis(), 3.0f, 1.0f / 100));
}

int UnitTestArrayHandleRandomStandardNormal(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestArrayHandleStandardNormal, argc, argv);
}
