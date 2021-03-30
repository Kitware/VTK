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

static bool FuzzyCompare(
  const vtkFFT::ComplexNumber& result, const vtkFFT::ComplexNumber& test, double epsilon)
{
  return (vtkFFT::ComplexModule(result - test) < epsilon * epsilon);
}

static int Test_fft_direct();
static int Test_fft_inverse();
static int Test_complexes_to_doubles();
static int Test_complex_module();
static int Test_rfftfreq();
static int Test_fft_direct_inverse();

int UnitTestFFT(int, char*[])
{
  int status = 0;

  status += Test_fft_direct();
  status += Test_fft_inverse();
  status += Test_complexes_to_doubles();
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
  return Test_fft_direct_inverse();
}

int Test_fft_inverse()
{
  return Test_fft_direct_inverse();
}

int Test_complexes_to_doubles()
{
  int status = 0;
  std::cout << "Test_complexes_to_doubles..";

  static constexpr int countOut = 6;
  static constexpr int countIn = 10;

  std::array<vtkFFT::ComplexNumber, countIn> complexNumbers;
  auto sign = -1;
  for (auto i = 0; i < countIn; i++)
  {
    sign *= -1;
    complexNumbers[i] = vtkFFT::ComplexNumber(i, 2 * i * sign);
  }

  std::array<double, countOut> test1 = { 0.0, 1.0, 2.0, 3.0, 4.0, 5.0 };
  std::array<double, countOut> result1;
  vtkFFT::ComplexesToDoubles(&result1[0], complexNumbers.data(), countOut);

  for (auto i = 0; i < countOut; i++)
  {
    if (!vtkMathUtilities::FuzzyCompare(
          result1.at(i), test1.at(i), std::numeric_limits<double>::epsilon()))
    {
      std::cout << "Expected " << test1.at(i) << " but got " << result1.at(i) << " difference is "
                << test1.at(i) - result1.at(i) << std::endl;
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

int Test_complex_module()
{
  int status = 0;
  std::cout << "Test_complex_module..";

  vtkFFT::ComplexNumber complexNumber1(3, 4);
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

  for (auto i = 0; i < frequencies.size(); i++)
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
  std::array<double, countIn> input;
  auto val = 0;
  std::generate(input.begin(), input.end(), [&val]() { return std::sin(val++); });

  int countOut = -1;
  vtkFFT::ComplexNumber* spectrum = nullptr;
  vtkFFT::FftDirect(input.data(), countIn, &countOut, spectrum);

  int countRes = -1;
  vtkFFT::ComplexNumber* resultComplex = nullptr;
  vtkFFT::FftInverse(spectrum, countOut, &countRes, resultComplex);

  double result[countRes];
  vtkFFT::ComplexesToDoubles(&result[0], resultComplex, countIn);

  delete[] spectrum;
  delete[] resultComplex;

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
