// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkSmartPointer.h"

#include "vtkNumberToString.h"
#include "vtkTypeTraits.h"

#include <cmath>
#include <limits>
#include <random>
#include <sstream>

namespace
{
template <typename T>
int TestConvertPrecision(unsigned int samples);
template <typename T>
int TestConvertLowHigh(unsigned int samples);
template <typename T>
int TestConvertNotations(unsigned int samples);
template <typename T>
int ConvertNumericLimitsValue();
}
int TestNumberToString(int, char*[])
{
  int status = EXIT_SUCCESS;

  std::cout << "Testing <numeric_limits>..." << std::endl;
  if (ConvertNumericLimitsValue<unsigned short>() == EXIT_FAILURE)
  {
    status = EXIT_FAILURE;
  }
  if (ConvertNumericLimitsValue<short>() == EXIT_FAILURE)
  {
    status = EXIT_FAILURE;
  }
  if (ConvertNumericLimitsValue<unsigned int>() == EXIT_FAILURE)
  {
    status = EXIT_FAILURE;
  }
  if (ConvertNumericLimitsValue<int>() == EXIT_FAILURE)
  {
    status = EXIT_FAILURE;
  }
  if (ConvertNumericLimitsValue<unsigned long>() == EXIT_FAILURE)
  {
    status = EXIT_FAILURE;
  }
  if (ConvertNumericLimitsValue<long>() == EXIT_FAILURE)
  {
    status = EXIT_FAILURE;
  }
  if (ConvertNumericLimitsValue<float>() == EXIT_FAILURE)
  {
    status = EXIT_FAILURE;
  }
  if (ConvertNumericLimitsValue<double>() == EXIT_FAILURE)
  {
    status = EXIT_FAILURE;
  }
  if (status == EXIT_FAILURE)
  {
    return status;
  }

  unsigned int samples = 10000;
  std::cout << "Testing convertion precision..." << std::endl;
  if (TestConvertPrecision<float>(samples) == EXIT_FAILURE)
  {
    return EXIT_FAILURE;
  }
  if (TestConvertPrecision<double>(samples) == EXIT_FAILURE)
  {
    return EXIT_FAILURE;
  }

  std::cout << "Testing convertion low high..." << std::endl;
  if (TestConvertLowHigh<float>(samples) == EXIT_FAILURE)
  {
    return EXIT_FAILURE;
  }
  if (TestConvertLowHigh<float>(samples) == EXIT_FAILURE)
  {
    return EXIT_FAILURE;
  }

  std::cout << "Testing convertion notations..." << std::endl;
  if (TestConvertNotations<float>(samples) == EXIT_FAILURE)
  {
    return EXIT_FAILURE;
  }
  if (TestConvertNotations<double>(samples) == EXIT_FAILURE)
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

namespace
{
template <typename T>
int TestConvertPrecision(unsigned int samples)
{
  const char* t = typeid(T).name();
  std::cout << "Testing type: " << t << std::endl;
  vtkNumberToString converter;
  for (int p = 5; p < 20; ++p)
  {
    unsigned int matches = 0;
    unsigned int mismatches = 0;

    // Now convert numbers to strings. Read the strings as floats and doubles
    // and compare the results with the original values.
    {
      std::mt19937 gen(1); // Mersenne Twister engine
      std::uniform_real_distribution<T> dist(-1.0, 1.0);
      for (unsigned int i = 0; i < samples; ++i)
      {
        T value = dist(gen);
        std::stringstream convertedStr;
        convertedStr << converter.Convert(value);
        std::stringstream rawStr;
        rawStr << std::setprecision(p) << value;

        T convertedValue;
        std::istringstream convertedStream(convertedStr.str());
        convertedStream >> convertedValue;
        if (convertedValue != value)
        {
          std::cout << "ERROR: " << value << " != " << convertedValue << std::endl;
          ++mismatches;
        }
        T rawValue;
        std::istringstream rawStream(rawStr.str());
        rawStream >> rawValue;
        if (rawValue == value)
        {
          ++matches;
        }
      }
    }
    std::cout << "For precision " << p << " Matches without conversion: " << matches << std::endl;
    std::cout << "                 MisMatches with conversion: " << mismatches << std::endl;
    if (mismatches)
    {
      return EXIT_FAILURE;
    }
    if (matches == samples)
    {
      std::cout << "The minimum precision for type " << t << " is " << p << std::endl;
      break;
    }
  }
  return EXIT_SUCCESS;
}

template <typename T>
int TestConvertLowHigh(unsigned int samples)
{
  // Now convert numbers to strings. Read the strings as floats and doubles
  // and compare the results with the original values.
  for (int iLow = -20; iLow <= 0; iLow++)
  {
    for (int iHigh = 0; iHigh <= 20; iHigh++)
    {
      std::cout << "Testing low exponent: " << iLow << ", high exponent: " << iHigh << "."
                << std::endl;
      vtkNumberToString converter;
      converter.SetLowExponent(iLow);
      converter.SetHighExponent(iHigh);
      std::mt19937 gen(1); // Mersenne Twister engine
      std::uniform_real_distribution<T> dist(
        std::numeric_limits<T>::min(), std::numeric_limits<T>::max());
      for (unsigned int i = 0; i < samples; ++i)
      {
        T value = dist(gen);

        std::istringstream convertedStream(converter.Convert(value));
        T convertedValue;
        convertedStream >> convertedValue;
        if (convertedValue != value)
        {
          std::cout << "ERROR: " << value << " != " << convertedValue << std::endl;
          return EXIT_FAILURE;
        }
      }
    }
  }

  return EXIT_SUCCESS;
}

template <typename T>
int TestConvertNotations(unsigned int samples)
{
  for (int precision = 1; precision <= 10; precision++)
  {
    for (int notation = vtkNumberToString::Scientific; notation <= vtkNumberToString::Fixed;
         notation++)
    {
      std::cout << "Testing notation: " << notation << ", precision: " << precision << "."
                << std::endl;
      vtkNumberToString converter;
      converter.SetNotation(notation);
      converter.SetPrecision(precision);
      std::mt19937 gen(1); // Mersenne Twister engine
      std::uniform_real_distribution<T> dist(
        std::numeric_limits<T>::min() * 2, std::numeric_limits<T>::max() / 2);
      for (unsigned int i = 0; i < samples; ++i)
      {
        T value = dist(gen);

        if (notation == vtkNumberToString::Fixed && (value > 9e59 || value < 9e-59))
        {
          continue; // Fixed-point can't have more than 60 characters before point.
        }
        std::istringstream convertedStream(converter.Convert(value));
        T convertedValue;
        convertedStream >> convertedValue;
        T acceptablePrecision =
          2 * std::pow(10, std::floor(std::log10(convertedValue)) - precision);
        if (std::abs(convertedValue - value) >= acceptablePrecision)
        {
          std::cout << "ERROR: " << value << " - " << convertedValue << "<" << acceptablePrecision
                    << std::endl;
          return EXIT_FAILURE;
        }
      }
    }
  }

  return EXIT_SUCCESS;
}

template <typename T>
int ConvertNumericLimitsValue()
{
  vtkNumberToString converter;
  const char* t = typeid(T).name();
  int status = EXIT_SUCCESS;
  {
    T value = std::numeric_limits<T>::max();
    std::stringstream convertedStr;
    convertedStr << converter.Convert(value);
    std::istringstream convertedStream(convertedStr.str());
    std::cout << t << "(max) "
              << "raw: " << value << " converted: " << convertedStream.str() << std::endl;

    T convertedValue;
    convertedStream >> convertedValue;
    if (value != convertedValue)
    {
      std::cout << "ERROR: Bad conversion of std::max" << std::endl;
      status = EXIT_FAILURE;
    }
  }
  {
    T value = std::numeric_limits<T>::min();
    std::stringstream convertedStr;
    convertedStr << converter.Convert(value);
    std::istringstream convertedStream(convertedStr.str());
    std::cout << t << "(min) "
              << "raw: " << value << " converted: " << convertedStream.str() << std::endl;

    T convertedValue;
    convertedStream >> convertedValue;
    if (value != convertedValue)
    {
      std::cout << "ERROR: Bad conversion of std::min" << std::endl;
      status = EXIT_FAILURE;
    }
  }
  {
    T value = std::numeric_limits<T>::lowest();
    std::stringstream convertedStr;
    convertedStr << converter.Convert(value);
    std::istringstream convertedStream(convertedStr.str());
    std::cout << t << "(lowest) "
              << "raw: " << value << " converted: " << convertedStream.str() << std::endl;

    T convertedValue;
    convertedStream >> convertedValue;
    if (value != convertedValue)
    {
      std::cout << "ERROR: Bad conversion of std::lowest" << std::endl;
      status = EXIT_FAILURE;
    }
  }
  {
    T value = std::numeric_limits<T>::epsilon();
    std::stringstream convertedStr;
    convertedStr << converter.Convert(value);
    std::istringstream convertedStream(convertedStr.str());
    std::cout << t << "(epsilon) "
              << "raw: " << value << " converted: " << convertedStream.str() << std::endl;

    T convertedValue;
    convertedStream >> convertedValue;
    if (value != convertedValue)
    {
      std::cout << "ERROR: Bad conversion of std::epsilon" << std::endl;
      status = EXIT_FAILURE;
    }
  }
  return status;
}
}
