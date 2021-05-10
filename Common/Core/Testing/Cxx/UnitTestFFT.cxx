/*=========================================================================

  Program:   Visualization Toolkit
  Module:    UnitTestMath.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkFFT.h>

#include <vtkMathUtilities.h>

#include <algorithm>
#include <array>
#include <memory>
#include <numeric>

static bool FuzzyCompare(const vtkFFT::ComplexNumber& result, const vtkFFT::ComplexNumber& test,
  vtkFFT::ScalarNumber epsilon)
{
  return ((vtkFFT::Abs(result) - vtkFFT::Abs(test)) < epsilon * epsilon);
}

static int Test_fft_cplx();
static int Test_fft_direct();
static int Test_fft_inverse();
static int Test_fft_inverse_cplx();
static int Test_complex_module();
static int Test_fftfreq();
static int Test_rfftfreq();
static int Test_fft_direct_inverse();

int UnitTestFFT(int, char*[])
{
  int status = 0;

  status += Test_fft_cplx();
  status += Test_fft_direct();
  status += Test_fft_inverse();
  status += Test_fft_inverse_cplx();
  status += Test_complex_module();
  status += Test_fftfreq();
  status += Test_rfftfreq();
  status += Test_fft_direct_inverse();

  if (status != 0)
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

int Test_fft_cplx()
{
  std::cout << "Test_fft_cplx..";

  static constexpr auto countIn = 16;
  static constexpr auto countOut = 16;
  auto comparator = [](vtkFFT::ComplexNumber l, vtkFFT::ComplexNumber r) {
    return FuzzyCompare(l, r, std::numeric_limits<vtkFFT::ScalarNumber>::epsilon());
  };
  auto status = 0;
  // Test with zeroes
  {
    std::vector<vtkFFT::ComplexNumber> zeroes(countIn);
    std::fill(zeroes.begin(), zeroes.end(), vtkFFT::ComplexNumber{ 0.0, 0.0 });

    auto resultZeroes = vtkFFT::Fft(zeroes);

    std::vector<vtkFFT::ComplexNumber> expected(countOut);
    std::fill(expected.begin(), expected.end(), vtkFFT::ComplexNumber{ 0.0, 0.0 });
    auto is_equal = std::equal(expected.begin(), expected.end(), resultZeroes.begin(), comparator);
    if (!is_equal)
    {
      status++;
    }
  }

  // Test with 1 freq
  {
    std::vector<vtkFFT::ComplexNumber> f1(countIn);
    for (std::size_t i = 0; i < countIn; ++i)
    {
      f1[i] = vtkFFT::ComplexNumber{ static_cast<vtkFFT::ScalarNumber>(i % 2), 0.0 };
    }

    auto res = vtkFFT::Fft(f1);

    std::vector<vtkFFT::ComplexNumber> expected(countOut);
    std::fill(expected.begin(), expected.end(), vtkFFT::ComplexNumber{ 0.0, 0.0 });
    expected[0] = vtkFFT::ComplexNumber{ 8.0, 0.0 };
    expected[8] = vtkFFT::ComplexNumber{ -8.0, 0.0 };
    auto is_equal = std::equal(expected.begin(), expected.end(), res.begin(), comparator);
    if (!is_equal)
    {
      status++;
    }
  }

  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}

int Test_fft_direct()
{
  std::cout << "Test_fft_direct..";

  static constexpr auto countIn = 16;
  static constexpr auto countOut = (countIn / 2) + 1;
  auto comparator = [](vtkFFT::ComplexNumber l, vtkFFT::ComplexNumber r) {
    return FuzzyCompare(l, r, std::numeric_limits<vtkFFT::ScalarNumber>::epsilon());
  };
  auto status = 0;
  // zeroes
  std::vector<vtkFFT::ScalarNumber> zeroes(countIn);
  std::generate(zeroes.begin(), zeroes.end(), []() { return 0; });

  auto resultZeroes = vtkFFT::RFft(zeroes);

  std::vector<vtkFFT::ComplexNumber> expectedZeroes(countOut);
  std::generate(expectedZeroes.begin(), expectedZeroes.end(), []() {
    return vtkFFT::ComplexNumber{ 0.0, 0.0 };
  });
  auto is_equal =
    std::equal(expectedZeroes.begin(), expectedZeroes.end(), resultZeroes.begin(), comparator);
  if (!is_equal)
  {
    status++;
  }

  // ones
  std::vector<vtkFFT::ScalarNumber> ones(countIn);
  std::generate(ones.begin(), ones.end(), []() { return 1.0; });

  auto resultOnes = vtkFFT::RFft(ones);

  std::vector<vtkFFT::ComplexNumber> expectedOnes(countOut);
  std::generate(expectedOnes.begin(), expectedOnes.end(), []() {
    return vtkFFT::ComplexNumber{ 0.0, 0.0 };
  });
  expectedOnes[0] = { 16.0, 0.0 };
  is_equal = std::equal(expectedOnes.begin(), expectedOnes.end(), resultOnes.begin(), comparator);
  if (!is_equal)
  {
    status++;
  }

  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}

int Test_fft_inverse()
{
  std::cout << "Test_fft_inverse..";

  static constexpr auto countIn = 9;
  static constexpr auto countOut = (countIn - 1) * 2;
  auto comparator = [](vtkFFT::ScalarNumber l, vtkFFT::ScalarNumber r) {
    return vtkMathUtilities::FuzzyCompare(
      l, r, std::numeric_limits<vtkFFT::ScalarNumber>::epsilon());
  };
  auto status = 0;
  // zeroes
  std::vector<vtkFFT::ComplexNumber> zeroes(countIn);
  std::generate(zeroes.begin(), zeroes.end(), []() { return vtkFFT::ComplexNumber{ 0.0, 0.0 }; });

  auto resultZeroes = vtkFFT::IRFft(zeroes);

  std::vector<vtkFFT::ScalarNumber> expectedZeroes(countOut);
  std::generate(expectedZeroes.begin(), expectedZeroes.end(), []() { return 0.0; });
  auto is_equal =
    std::equal(expectedZeroes.begin(), expectedZeroes.end(), resultZeroes.begin(), comparator);
  if (!is_equal)
  {
    status++;
  }

  // ones
  std::vector<vtkFFT::ComplexNumber> ones(countIn);
  std::generate(ones.begin(), ones.end(), []() { return vtkFFT::ComplexNumber{ 0.0, 0.0 }; });
  ones[0] = { 16.0, 0.0 };

  auto resultOnes = vtkFFT::IRFft(ones);

  std::vector<vtkFFT::ScalarNumber> expectedOnes(countOut);
  std::generate(expectedOnes.begin(), expectedOnes.end(), []() { return 1.0; });
  is_equal = std::equal(expectedOnes.begin(), expectedOnes.end(), resultOnes.begin(), comparator);
  if (!is_equal)
  {
    status++;
  }

  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}

int Test_fft_inverse_cplx()
{
  std::cout << "Test_fft_inverse_cplx..";

  auto comparator = [](vtkFFT::ComplexNumber l, vtkFFT::ComplexNumber r) {
    return vtkMathUtilities::FuzzyCompare(
             l.r, r.r, std::numeric_limits<vtkFFT::ScalarNumber>::epsilon()) +
      vtkMathUtilities::FuzzyCompare(
        l.i, r.i, std::numeric_limits<vtkFFT::ScalarNumber>::epsilon());
  };
  int status = 0;

  // zeroes
  std::vector<vtkFFT::ComplexNumber> zeroes(9);
  std::fill(zeroes.begin(), zeroes.end(), vtkFFT::ComplexNumber{ 0.0, 0.0 });
  auto resultZeroes = vtkFFT::IFft(zeroes);
  bool equal = std::equal(zeroes.begin(), zeroes.end(), resultZeroes.begin(), comparator);
  status += static_cast<int>(!equal);

  // ones
  std::vector<vtkFFT::ComplexNumber> signal(9);
  std::fill(zeroes.begin(), zeroes.end(), vtkFFT::ComplexNumber{ 0.0, 0.0 });
  signal[0] = vtkFFT::ComplexNumber{ 9.0, 0.0 };
  std::vector<vtkFFT::ComplexNumber> expectedSignal(9);
  std::fill(zeroes.begin(), zeroes.end(), vtkFFT::ComplexNumber{ 1.0, 0.0 });
  auto resultSignal = vtkFFT::IFft(signal);
  equal =
    std::equal(expectedSignal.begin(), expectedSignal.end(), resultSignal.begin(), comparator);
  status += static_cast<int>(!equal);

  std::cout << (status ? "..FAILED" : ".PASSED") << std::endl;
  return status;
}

int Test_complex_module()
{
  int status = 0;
  std::cout << "Test_complex_module..";

  vtkFFT::ComplexNumber complexNumber1 = { 3, 4 };
  double module1 = vtkFFT::Abs(complexNumber1);
  double test1 = 5;
  if (!vtkMathUtilities::FuzzyCompare(module1, test1, std::numeric_limits<double>::epsilon()))
  {
    std::cout << "Expected " << test1 << " but got " << module1 << " difference is "
              << module1 - test1 << std::endl;
    status++;
  }

  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}

int Test_fftfreq()
{
  int status = 0;
  std::cout << "Test_fftfreq..";

  constexpr double sampleSpacing = 1.0;
  std::vector<double> frequencies = vtkFFT::FftFreq(8, sampleSpacing);
  std::vector<double> expected1 = { 0., 0.125, 0.25, 0.375, -0.5, -0.375, -0.25, -0.125 };

  if (!(frequencies.size() == expected1.size()))
  {
    std::cout << "Difference size: expected " << expected1.size() << " but got "
              << frequencies.size() << std::endl;
    status++;
  }

  for (size_t i = 0; i < frequencies.size(); i++)
  {
    const double& expected = expected1[i];
    const double& real = frequencies[i];

    if (!vtkMathUtilities::FuzzyCompare(real, expected, std::numeric_limits<double>::epsilon()))
    {
      std::cout << "Expected " << expected << " but got " << real << " difference is "
                << expected - real << std::endl;
      status++;
    }
  }

  frequencies = vtkFFT::FftFreq(9, sampleSpacing);
  std::vector<double> expected2 = { 0.0, 0.111111111, 0.222222222, 0.333333333, 0.444444444,
    -0.444444444, -0.333333333, -0.222222222, -0.111111111 };
  if (!(frequencies.size() == expected2.size()))
  {
    std::cout << "Difference size: expected " << expected2.size() << " but got "
              << frequencies.size() << std::endl;
    status++;
  }

  for (size_t i = 0; i < frequencies.size(); i++)
  {
    const double& expected = expected2[i];
    const double& real = frequencies[i];

    if (!vtkMathUtilities::FuzzyCompare(real, expected, 1.0e-6))
    {
      std::cout << "Expected " << expected << " but got " << real << " difference is "
                << expected - real << std::endl;
      status++;
    }
  }

  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}

int Test_rfftfreq()
{
  int status = 0;
  std::cout << "Test_rfftfreq..";

  constexpr auto samplingFrequency = 1000;
  constexpr auto windowLength = 1000;
  double sampleSpacing = 1.0 / samplingFrequency;
  std::vector<double> frequencies = vtkFFT::RFftFreq(windowLength, sampleSpacing);

  std::vector<double> test1((windowLength / 2) + 1);
  std::iota(test1.begin(), test1.end(), 0);

  if (!(frequencies.size() == test1.size()))
  {
    std::cout << "Difference size: expected " << test1.size() << " but got " << frequencies.size()
              << std::endl;
    status++;
  }

  for (size_t i = 0; i < frequencies.size(); i++)
  {
    const auto& expected = test1[i];
    const auto& real = frequencies[i];

    if (!vtkMathUtilities::FuzzyCompare(real, expected, std::numeric_limits<double>::epsilon()))
    {
      std::cout << "Expected " << expected << " but got " << real << " difference is "
                << expected - real << std::endl;
      status++;
    }
  }

  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}

int Test_fft_direct_inverse()
{
  int status = 0;
  std::cout << "Test_fft_direct_inverse..";

  static constexpr auto countIn = 1000;
  std::vector<double> input(countIn);
  auto val = 0;
  std::generate(input.begin(), input.end(), [&val]() { return std::sin(val++); });

  auto spectrum = vtkFFT::RFft(input);

  auto result = vtkFFT::IRFft(spectrum);

  for (auto i = 0; i < countIn; i++)
  {
    if (!vtkMathUtilities::FuzzyCompare(input[i], result[i], 1e-06))
    {
      std::cout << "Expected " << input[i] << " but got " << result[i] << " difference is "
                << input[i] - result[i] << std::endl;
      status++;
    }
  }

  if (status)
  {
    std::cout << "..FAILED" << std::endl;
  }
  else
  {
    std::cout << ".PASSED" << std::endl;
  }
  return status;
}
