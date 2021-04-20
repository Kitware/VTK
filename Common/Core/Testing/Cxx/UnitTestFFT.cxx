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

static bool FuzzyCompare(const vtkFFT::ComplexNumber& result, const vtkFFT::ComplexNumber& test,
  vtkFFT::ScalarNumber epsilon)
{
  return ((vtkFFT::ComplexModule(result) - vtkFFT::ComplexModule(test)) < epsilon * epsilon);
}

static int Test_fft_direct();
static int Test_fft_inverse();
static int Test_complex_module();
static int Test_rfftfreq();
static int Test_fft_direct_inverse();

int UnitTestFFT(int, char*[])
{
  int status = 0;

  status += Test_fft_direct();
  status += Test_fft_inverse();
  status += Test_complex_module();
  status += Test_rfftfreq();
  status += Test_fft_direct_inverse();

  if (status != 0)
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
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

  auto resultZeroes = vtkFFT::FftDirect(zeroes);

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

  auto resultOnes = vtkFFT::FftDirect(ones);

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

  auto resultZeroes = vtkFFT::FftInverse(zeroes);

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

  auto resultOnes = vtkFFT::FftInverse(ones);

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

int Test_complex_module()
{
  int status = 0;
  std::cout << "Test_complex_module..";

  vtkFFT::ComplexNumber complexNumber1 = { 3, 4 };
  double module1 = vtkFFT::ComplexModule(complexNumber1);
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

int Test_rfftfreq()
{
  int status = 0;
  std::cout << "Test_rfftfreq..";

  constexpr auto samplingFrequency = 1000;
  constexpr auto windowLength = 1000;
  double sampleSpacing = 1.0 / samplingFrequency;
  std::vector<double> frequencies = vtkFFT::RFftFreq(windowLength, sampleSpacing);

  std::vector<double> test1;
  for (auto i = 0; i < windowLength / 2; i++)
    test1.push_back(i);

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

  auto spectrum = vtkFFT::FftDirect(input);

  auto result = vtkFFT::FftInverse(spectrum);

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
