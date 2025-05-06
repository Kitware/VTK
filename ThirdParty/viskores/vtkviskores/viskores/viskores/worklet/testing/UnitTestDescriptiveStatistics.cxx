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

#include <viskores/cont/testing/Testing.h>
#include <viskores/worklet/DescriptiveStatistics.h>

#include <random>

void TestSingle()
{
  auto single_array = viskores::cont::make_ArrayHandle<viskores::Float32>({ 42 });
  auto result = viskores::worklet::DescriptiveStatistics::Run(single_array);

  VISKORES_TEST_ASSERT(test_equal(result.N(), 1));
  VISKORES_TEST_ASSERT(test_equal(result.Mean(), 42));
  VISKORES_TEST_ASSERT(test_equal(result.PopulationVariance(), 0));

  // A single number does not have skewness nor kurtosis
  VISKORES_TEST_ASSERT(test_equal(result.Skewness(), 0));
  VISKORES_TEST_ASSERT(test_equal(result.Kurtosis(), 0));
}

void TestConstant()
{
  auto constants = viskores::cont::make_ArrayHandleConstant(1234.f, 10000);
  auto result = viskores::worklet::DescriptiveStatistics::Run(constants);

  VISKORES_TEST_ASSERT(test_equal(result.N(), 10000));
  VISKORES_TEST_ASSERT(test_equal(result.Sum(), 12340000));
  VISKORES_TEST_ASSERT(test_equal(result.PopulationVariance(), 0));
  VISKORES_TEST_ASSERT(test_equal(result.Skewness(), 0));
  VISKORES_TEST_ASSERT(test_equal(result.Kurtosis(), 0));
}

void TestIntegerSequence()
{
  // We only have 23 bits for FloatInt in Float32. This limits N to 11 bits.
  constexpr viskores::Float32 N = 1000;

  auto integers = viskores::cont::ArrayHandleCounting<viskores::Float32>(
    0.0f, 1.0f, static_cast<viskores::Id>(N));
  auto result = viskores::worklet::DescriptiveStatistics::Run(integers);

  VISKORES_TEST_ASSERT(test_equal(result.N(), N));
  VISKORES_TEST_ASSERT(test_equal(result.Sum(), N * (N - 1) / 2));
  VISKORES_TEST_ASSERT(test_equal(result.Mean(), (N - 1) / 2));

  // Expected values are from Numpy/SciPy
  VISKORES_TEST_ASSERT(test_equal(result.PopulationVariance(), 83333.25));
  VISKORES_TEST_ASSERT(test_equal(result.Skewness(), 0));
  // We are using the Pearson's definition, with fisher = False when calling
  // numpy.
  VISKORES_TEST_ASSERT(test_equal(result.Kurtosis(), 1.8));
}

void TestStandardNormal()
{
  // Draw random numbers from the Standard Normal distribution, with mean = 0, stddev = 1
  std::mt19937 gen(0xceed);
  std::normal_distribution<viskores::Float32> dis(0.0f, 1.0f);

  std::vector<viskores::Float32> x(1000000);
  std::generate(x.begin(), x.end(), [&gen, &dis]() { return dis(gen); });

  auto array = viskores::cont::make_ArrayHandle(x, viskores::CopyFlag::Off);
  auto result = viskores::worklet::DescriptiveStatistics::Run(array);

  // Variance should be positive
  VISKORES_TEST_ASSERT(result.SampleVariance() >= 0);
  // SampleStddev should be very close to 1.0, Skewness ~= 0 and Kurtosis ~= 3.0
  VISKORES_TEST_ASSERT(test_equal(result.SampleStddev(), 1.0f, 1.0f / 100));
  VISKORES_TEST_ASSERT(test_equal(result.Skewness(), 0.0f, 1.0f / 100));
  VISKORES_TEST_ASSERT(test_equal(result.Kurtosis(), 3.0f, 1.0f / 100));
}

void TestCatastrophicCancellation()
{
  // Good examples of the effect of catastrophic cancellation from Wikipedia.
  auto arrayOK =
    viskores::cont::make_ArrayHandle<viskores::Float64>({ 1e8 + 4, 1e8 + 7, 1e8 + 13, 1.0e8 + 16 });
  auto resultOK = viskores::worklet::DescriptiveStatistics::Run(arrayOK);

  VISKORES_TEST_ASSERT(test_equal(resultOK.N(), 4));
  VISKORES_TEST_ASSERT(test_equal(resultOK.Sum(), 4.0e8 + 40));
  VISKORES_TEST_ASSERT(test_equal(resultOK.Min(), 1.0e8 + 4));
  VISKORES_TEST_ASSERT(test_equal(resultOK.Max(), 1.0e8 + 16));
  VISKORES_TEST_ASSERT(test_equal(resultOK.SampleVariance(), 30));
  VISKORES_TEST_ASSERT(test_equal(resultOK.PopulationVariance(), 22.5));

  // Bad examples of the effect of catastrophic cancellation from Wikipedia.
  // A naive algorithm will fail in calculating the correct variance
  auto arrayEvil =
    viskores::cont::make_ArrayHandle<viskores::Float64>({ 1e9 + 4, 1e9 + 7, 1e9 + 13, 1.0e9 + 16 });
  auto resultEvil = viskores::worklet::DescriptiveStatistics::Run(arrayEvil);

  VISKORES_TEST_ASSERT(test_equal(resultEvil.N(), 4));
  VISKORES_TEST_ASSERT(test_equal(resultEvil.Sum(), 4.0e9 + 40));
  VISKORES_TEST_ASSERT(test_equal(resultEvil.Min(), 1.0e9 + 4));
  VISKORES_TEST_ASSERT(test_equal(resultEvil.Max(), 1.0e9 + 16));
  VISKORES_TEST_ASSERT(test_equal(resultEvil.SampleVariance(), 30));
  VISKORES_TEST_ASSERT(test_equal(resultEvil.PopulationVariance(), 22.5));
}

void TestGeneGolub()
{
  // Bad case example proposed by Gene Golub, the variance may come out
  // as negative due to numerical precision. Thanks to Nick Thompson for
  // providing this unit test.

  // Draw random numbers from the Normal distribution, with mean = 500, stddev = 0.01
  std::mt19937 gen(0xceed);
  std::normal_distribution<viskores::Float32> dis(500.0f, 0.01f);

  std::vector<viskores::Float32> v(50000);
  for (float& i : v)
  {
    i = dis(gen);
  }

  auto array = viskores::cont::make_ArrayHandle(v, viskores::CopyFlag::Off);
  auto result = viskores::worklet::DescriptiveStatistics::Run(array);

  // Variance should be positive
  VISKORES_TEST_ASSERT(result.SampleVariance() >= 0);
}

void TestMeanProperties()
{
  // Draw random numbers from the Normal distribution, with mean = 500, stddev = 0.01
  std::mt19937 gen(0xceed);
  std::normal_distribution<viskores::Float32> dis(500.0f, 0.01f);

  std::vector<viskores::Float32> x(50000);
  std::generate(x.begin(), x.end(), [&gen, &dis]() { return dis(gen); });

  // 1. Linearity, Mean(a * x + b) = a * Mean(x) + b
  std::vector<viskores::Float32> axpb(x.size());
  std::transform(x.begin(),
                 x.end(),
                 axpb.begin(),
                 [](viskores::Float32 value) { return 4.0f * value + 1000.f; });

  auto x_array = viskores::cont::make_ArrayHandle(x, viskores::CopyFlag::Off);
  auto axpb_array = viskores::cont::make_ArrayHandle(axpb, viskores::CopyFlag::Off);

  auto mean_x = viskores::worklet::DescriptiveStatistics::Run(x_array).Mean();
  auto mean_axpb = viskores::worklet::DescriptiveStatistics::Run(axpb_array).Mean();

  VISKORES_TEST_ASSERT(test_equal(4.0f * mean_x + 1000.f, mean_axpb, 0.01f));

  // 2. Random shuffle
  std::vector<viskores::Float32> px = x;
  std::shuffle(px.begin(), px.end(), gen);

  auto px_array = viskores::cont::make_ArrayHandle(px, viskores::CopyFlag::Off);
  auto mean_px = viskores::worklet::DescriptiveStatistics::Run(px_array).Mean();

  VISKORES_TEST_ASSERT(test_equal(mean_x, mean_px, 0.01f));
}

void TestVarianceProperty()
{
  // Draw random numbers from the Normal distribution, with mean = 500, stddev = 0.01
  std::mt19937 gen(0xceed);
  std::normal_distribution<viskores::Float32> dis(500.0f, 0.01f);

  std::vector<viskores::Float32> v(50000);
  std::generate(v.begin(), v.end(), [&gen, &dis]() { return dis(gen); });

  // 1. Linearity, Var(a * x + b) = a^2 * Var(x)
  std::vector<viskores::Float32> kv(v.size());
  std::transform(
    v.begin(), v.end(), kv.begin(), [](viskores::Float32 value) { return 4.0f * value + 5.0f; });

  auto array_v = viskores::cont::make_ArrayHandle(v, viskores::CopyFlag::Off);
  auto array_kv = viskores::cont::make_ArrayHandle(kv, viskores::CopyFlag::Off);
  auto result_v = viskores::worklet::DescriptiveStatistics::Run(array_v);
  auto result_kv = viskores::worklet::DescriptiveStatistics::Run(array_kv);
  auto mean_v = result_v.Mean();
  auto mean_kv = result_kv.Mean();
  auto var_v = result_v.SampleVariance();
  auto var_kv = result_kv.SampleVariance();

  viskores::Float32 condition_number_kv = 0;
  auto rp = array_kv.ReadPortal();
  for (viskores::Id i = 0; i < rp.GetNumberOfValues(); ++i)
  {
    condition_number_kv += viskores::Abs(rp.Get(i) - mean_kv) * viskores::Abs(rp.Get(i));
  }
  condition_number_kv *= (2.0f / (static_cast<float>(rp.GetNumberOfValues() - 1) * var_kv));
  VISKORES_TEST_ASSERT(
    test_equal(var_kv,
               4.0 * 4.0 * var_v,
               condition_number_kv * std::numeric_limits<viskores::Float32>::epsilon()));

  // Random shuffle
  std::vector<viskores::Float32> px = v;
  std::shuffle(px.begin(), px.end(), gen);

  auto px_array = viskores::cont::make_ArrayHandle(px, viskores::CopyFlag::Off);
  auto var_px = viskores::worklet::DescriptiveStatistics::Run(px_array).SampleVariance();

  viskores::Float32 condition_number_v = 0;
  rp = px_array.ReadPortal();
  for (viskores::Id i = 0; i < rp.GetNumberOfValues(); ++i)
  {
    condition_number_v += viskores::Abs(rp.Get(i) - mean_v) * viskores::Abs(rp.Get(i));
  }
  condition_number_v *= (2.0f / (static_cast<float>(rp.GetNumberOfValues() - 1) * var_v));

  VISKORES_TEST_ASSERT(test_equal(
    var_v, var_px, condition_number_v * std::numeric_limits<viskores::Float32>::epsilon()));
}

void TestMomentsByKey()
{
  auto keys_array =
    viskores::cont::make_ArrayHandle<viskores::UInt32>({ 0, 1, 2, 2, 3, 3, 3, 4, 4, 4, 4 });
  auto values_array =
    viskores::cont::make_ArrayHandleConstant(1.0f, keys_array.GetNumberOfValues());

  auto results = viskores::worklet::DescriptiveStatistics::Run(keys_array, values_array);
  VISKORES_TEST_ASSERT(results.GetNumberOfValues() == 5);

  std::vector<viskores::UInt32> expected_ns{ 1, 1, 2, 3, 4 };
  std::vector<viskores::Float32> expected_sums{ 1, 1, 2, 3, 4 };
  std::vector<viskores::Float32> expected_means{ 1, 1, 1, 1, 1 };

  auto resultsPortal = results.ReadPortal();
  for (viskores::Id i = 0; i < results.GetNumberOfValues(); ++i)
  {
    auto result = resultsPortal.Get(i);
    VISKORES_TEST_ASSERT(test_equal(result.first, i));
    VISKORES_TEST_ASSERT(test_equal(result.second.N(), expected_ns[static_cast<std::size_t>(i)]));
    VISKORES_TEST_ASSERT(test_equal(result.second.PopulationVariance(), 0));
  }
}

void TestEdgeCases()
{
  using StatValueType = viskores::worklet::DescriptiveStatistics::StatState<viskores::FloatDefault>;
  StatValueType state1(42);
  StatValueType state2;

  StatValueType result = state1 + state2;
  VISKORES_TEST_ASSERT(test_equal(result.N(), 1));
  VISKORES_TEST_ASSERT(test_equal(result.Min(), 42));
  VISKORES_TEST_ASSERT(test_equal(result.Max(), 42));
  VISKORES_TEST_ASSERT(test_equal(result.Mean(), 42));
  VISKORES_TEST_ASSERT(test_equal(result.SampleVariance(), 0));
  VISKORES_TEST_ASSERT(test_equal(result.PopulationVariance(), 0));
  VISKORES_TEST_ASSERT(test_equal(result.Skewness(), 0));
  VISKORES_TEST_ASSERT(test_equal(result.Kurtosis(), 0));

  result = state2 + state1;
  VISKORES_TEST_ASSERT(test_equal(result.N(), 1));
  VISKORES_TEST_ASSERT(test_equal(result.Min(), 42));
  VISKORES_TEST_ASSERT(test_equal(result.Max(), 42));
  VISKORES_TEST_ASSERT(test_equal(result.Mean(), 42));
  VISKORES_TEST_ASSERT(test_equal(result.SampleVariance(), 0));
  VISKORES_TEST_ASSERT(test_equal(result.PopulationVariance(), 0));
  VISKORES_TEST_ASSERT(test_equal(result.Skewness(), 0));
  VISKORES_TEST_ASSERT(test_equal(result.Kurtosis(), 0));

  StatValueType empty;
  VISKORES_TEST_ASSERT(test_equal(empty.N(), 0));
  VISKORES_TEST_ASSERT(test_equal(empty.Min(), std::numeric_limits<viskores::FloatDefault>::max()));
  VISKORES_TEST_ASSERT(
    test_equal(empty.Max(), std::numeric_limits<viskores::FloatDefault>::lowest()));
  VISKORES_TEST_ASSERT(test_equal(empty.Mean(), 0));
  VISKORES_TEST_ASSERT(test_equal(empty.SampleVariance(), 0));
  VISKORES_TEST_ASSERT(test_equal(empty.PopulationVariance(), 0));
  VISKORES_TEST_ASSERT(test_equal(empty.Skewness(), 0));
  VISKORES_TEST_ASSERT(test_equal(empty.Kurtosis(), 0));

  result = empty + empty;
  VISKORES_TEST_ASSERT(test_equal(empty.N(), 0));
  VISKORES_TEST_ASSERT(test_equal(empty.Min(), std::numeric_limits<viskores::FloatDefault>::max()));
  VISKORES_TEST_ASSERT(
    test_equal(empty.Max(), std::numeric_limits<viskores::FloatDefault>::lowest()));
  VISKORES_TEST_ASSERT(test_equal(empty.Mean(), 0));
  VISKORES_TEST_ASSERT(test_equal(empty.SampleVariance(), 0));
  VISKORES_TEST_ASSERT(test_equal(empty.PopulationVariance(), 0));
  VISKORES_TEST_ASSERT(test_equal(empty.Skewness(), 0));
  VISKORES_TEST_ASSERT(test_equal(empty.Kurtosis(), 0));
}

void TestDescriptiveStatistics()
{
  TestSingle();
  TestConstant();
  TestIntegerSequence();
  TestStandardNormal();
  TestCatastrophicCancellation();
  TestGeneGolub();
  TestMeanProperties();
  TestVarianceProperty();
  TestMomentsByKey();
  TestEdgeCases();
}

int UnitTestDescriptiveStatistics(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestDescriptiveStatistics, argc, argv);
}
