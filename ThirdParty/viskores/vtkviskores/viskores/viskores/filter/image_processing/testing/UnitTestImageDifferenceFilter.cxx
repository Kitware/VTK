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

#include <viskores/cont/DataSetBuilderUniform.h>
#include <viskores/cont/Logging.h>
#include <viskores/cont/testing/Testing.h>
#include <viskores/filter/image_processing/ImageDifference.h>

#include <random>
#include <vector>

namespace
{

template <typename T>
void CreateData(std::size_t numPts,
                const T& fudgeFactor,
                std::vector<viskores::Vec<T, 4>>& primary,
                std::vector<viskores::Vec<T, 4>>& secondary)
{
  primary.resize(numPts);
  secondary.resize(numPts);
  for (std::size_t i = 0; i < numPts; ++i)
  {
    primary[i] = viskores::Vec<T, 4>(1, 1, 1, 1);
    secondary[i] = viskores::Vec<T, 4>(fudgeFactor, 1, 1, 1);
  }
}


void CheckResult(const std::vector<viskores::Vec4f>& expectedDiff,
                 const std::vector<viskores::FloatDefault>& expectedThreshold,
                 const viskores::cont::DataSet& output,
                 bool inThreshold,
                 bool expectedInThreshold)
{
  VISKORES_TEST_ASSERT(output.HasPointField("image-diff"), "Output field is missing.");

  viskores::cont::ArrayHandle<viskores::Vec4f> outputArray;
  viskores::cont::ArrayHandle<viskores::FloatDefault> thresholdArray;
  output.GetPointField("image-diff").GetData().AsArrayHandle(outputArray);
  output.GetPointField("threshold-output").GetData().AsArrayHandle(thresholdArray);

  VISKORES_TEST_ASSERT(outputArray.GetNumberOfValues() ==
                         static_cast<viskores::Id>(expectedDiff.size()),
                       "Field sizes wrong");

  auto outPortal = outputArray.ReadPortal();

  for (viskores::Id j = 0; j < outputArray.GetNumberOfValues(); j++)
  {
    viskores::Vec4f val1 = outPortal.Get(j);
    viskores::Vec4f val2 = expectedDiff[j];
    VISKORES_TEST_ASSERT(test_equal(val1, val2), "Wrong result for image-diff");
  }

  auto thresholdPortal = thresholdArray.ReadPortal();
  for (viskores::Id j = 0; j < thresholdArray.GetNumberOfValues(); j++)
  {
    viskores::Vec4f val1 = thresholdPortal.Get(j);
    viskores::Vec4f val2 = expectedThreshold[j];
    VISKORES_TEST_ASSERT(test_equal(val1, val2), "Wrong result for threshold output");
  }

  if (expectedInThreshold)
  {
    VISKORES_TEST_ASSERT(inThreshold, "Diff image was not within the error threshold");
  }
  else
  {
    VISKORES_TEST_ASSERT(!inThreshold, "Diff image was found to be within the error threshold");
  }
}

viskores::cont::DataSet FillDataSet(const viskores::FloatDefault& fudgeFactor)
{
  viskores::cont::DataSetBuilderUniform builder;
  viskores::cont::DataSet dataSet = builder.Create(viskores::Id2(5, 5));

  std::vector<viskores::Vec4f> primary;
  std::vector<viskores::Vec4f> secondary;
  CreateData(static_cast<std::size_t>(25), fudgeFactor, primary, secondary);
  dataSet.AddPointField("primary", primary);
  dataSet.AddPointField("secondary", secondary);

  return dataSet;
}

void TestImageDifference()
{
  VISKORES_LOG_S(viskores::cont::LogLevel::Info, "Testing ImageDifference Filter");

  {
    VISKORES_LOG_S(viskores::cont::LogLevel::Info, "Matching Images");
    auto dataSet = FillDataSet(static_cast<viskores::FloatDefault>(1));
    viskores::filter::image_processing::ImageDifference filter;
    filter.SetPrimaryField("primary");
    filter.SetSecondaryField("secondary");
    filter.SetPixelDiffThreshold(0.05f);
    filter.SetPixelShiftRadius(0);
    viskores::cont::DataSet result = filter.Execute(dataSet);

    std::vector<viskores::Vec4f> expectedDiff = {
      { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 },
      { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 },
      { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 },
      { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 },
      { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }
    };
    std::vector<viskores::FloatDefault> expectedThreshold = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                                              0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    CheckResult(
      expectedDiff, expectedThreshold, result, filter.GetImageDiffWithinThreshold(), true);
  }

  {
    VISKORES_LOG_S(viskores::cont::LogLevel::Info, "Matching Images with Average");
    auto dataSet = FillDataSet(static_cast<viskores::FloatDefault>(1));
    viskores::filter::image_processing::ImageDifference filter;
    filter.SetPrimaryField("primary");
    filter.SetSecondaryField("secondary");
    filter.SetPixelDiffThreshold(0.05f);
    filter.SetPixelShiftRadius(1);
    filter.SetAverageRadius(1);
    viskores::cont::DataSet result = filter.Execute(dataSet);

    std::vector<viskores::Vec4f> expectedDiff = {
      { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 },
      { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 },
      { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 },
      { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 },
      { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }
    };
    std::vector<viskores::FloatDefault> expectedThreshold = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                                              0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    CheckResult(
      expectedDiff, expectedThreshold, result, filter.GetImageDiffWithinThreshold(), true);
  }

  {
    VISKORES_LOG_S(viskores::cont::LogLevel::Info, "Non Matching Images (Different R pixel)");
    auto dataSet = FillDataSet(static_cast<viskores::FloatDefault>(3));
    viskores::filter::image_processing::ImageDifference filter;
    filter.SetPrimaryField("primary");
    filter.SetSecondaryField("secondary");
    filter.SetPixelDiffThreshold(0.05f);
    filter.SetPixelShiftRadius(0);
    viskores::cont::DataSet result = filter.Execute(dataSet);

    std::vector<viskores::Vec4f> expectedDiff = {
      { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 },
      { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 },
      { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 },
      { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 },
      { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 }
    };
    std::vector<viskores::FloatDefault> expectedThreshold = { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
                                                              2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 };
    CheckResult(
      expectedDiff, expectedThreshold, result, filter.GetImageDiffWithinThreshold(), false);
  }

  {
    VISKORES_LOG_S(viskores::cont::LogLevel::Info, "Non Matching Images (Different R pixel)");
    auto dataSet = FillDataSet(static_cast<viskores::FloatDefault>(3));
    viskores::filter::image_processing::ImageDifference filter;
    filter.SetPrimaryField("primary");
    filter.SetSecondaryField("secondary");
    filter.SetPixelDiffThreshold(0.05f);
    filter.SetPixelShiftRadius(0);
    filter.SetAllowedPixelErrorRatio(1.00f);
    viskores::cont::DataSet result = filter.Execute(dataSet);

    std::vector<viskores::Vec4f> expectedDiff = {
      { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 },
      { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 },
      { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 },
      { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 },
      { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 }
    };
    std::vector<viskores::FloatDefault> expectedThreshold = { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
                                                              2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 };
    CheckResult(
      expectedDiff, expectedThreshold, result, filter.GetImageDiffWithinThreshold(), true);
  }

  {
    VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                   "Non Matching Images (Different R pixel), Large Threshold");
    auto dataSet = FillDataSet(static_cast<viskores::FloatDefault>(3));
    viskores::filter::image_processing::ImageDifference filter;
    filter.SetPrimaryField("primary");
    filter.SetSecondaryField("secondary");
    filter.SetPixelDiffThreshold(3.0f);
    filter.SetPixelShiftRadius(0);
    viskores::cont::DataSet result = filter.Execute(dataSet);

    std::vector<viskores::Vec4f> expectedDiff = {
      { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 },
      { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 },
      { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 },
      { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 },
      { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 }
    };
    std::vector<viskores::FloatDefault> expectedThreshold = { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
                                                              2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 };
    CheckResult(
      expectedDiff, expectedThreshold, result, filter.GetImageDiffWithinThreshold(), true);
  }
}
} // anonymous namespace

int UnitTestImageDifferenceFilter(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestImageDifference, argc, argv);
}
