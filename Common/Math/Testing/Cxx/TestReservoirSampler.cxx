// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDoubleArray.h"
#include "vtkMathUtilities.h"
#include "vtkNew.h"
#include "vtkReservoirSampler.h"

#include <chrono>

// Test that exceptions are thrown for improper parameters and not thrown for proper parameters.
int TestReservoirSamplerExceptions()
{
  std::cout << "Testing exceptional behavior\n";
  int result = 0;
  bool didCatch = false;
  // Cause intentional exceptions.
  try
  {
    vtkReservoirSampler<int> bad;
    bad(-1, 10);
  }
  catch (std::invalid_argument& e)
  {
    std::cout << "  Caught expected exception: \"" << e.what() << "\"\n";
    didCatch = true;
  }
  if (!didCatch)
  {
    std::cerr << "Failed to throw exception for invalid sample size.\n";
    ++result;
  }
  didCatch = false;

  try
  {
    vtkReservoirSampler<int> bad;
    bad(10, nullptr);
  }
  catch (std::invalid_argument& e)
  {
    std::cout << "  Caught expected exception: \"" << e.what() << "\"\n";
    didCatch = true;
  }
  if (!didCatch)
  {
    std::cerr << "Failed to throw exception for null array pointer.\n";
    ++result;
  }
  didCatch = false;

  try
  {
    vtkNew<vtkDoubleArray> array;
    array->SetNumberOfTuples(2 * std::numeric_limits<short>::max());
    vtkReservoirSampler<short> bad;
    bad(10, array.GetPointer());
  }
  catch (std::invalid_argument& e)
  {
    std::cout << "  Caught expected exception: \"" << e.what() << "\"\n";
    didCatch = true;
  }
  if (!didCatch)
  {
    std::cerr << "Failed to throw exception for an oversized array (relative to integer type).\n";
    ++result;
  }
  didCatch = false;

  // Test that unintentional exceptions do not occur.
  try
  {
    vtkReservoirSampler<int> good;
    good(10, 20);
  }
  catch (std::invalid_argument& e)
  {
    std::cerr << "  Caught unexpected exception: \"" << e.what() << "\"\n";
    didCatch = true;
  }
  if (didCatch)
  {
    std::cerr << "Threw exception for usual valid values.\n";
    ++result;
    didCatch = false;
  }

  try
  {
    vtkReservoirSampler<int> good;
    good(50, 20);
  }
  catch (std::invalid_argument& e)
  {
    std::cerr << "  Caught unexpected exception: \"" << e.what() << "\"\n";
    didCatch = true;
  }
  if (didCatch)
  {
    std::cerr << "Threw exception for unusual but valid values.\n";
    ++result;
    didCatch = false;
  }

  try
  {
    vtkReservoirSampler<int> good;
    good(0, 10);
  }
  catch (std::invalid_argument& e)
  {
    std::cerr << "  Caught unexpected exception: \"" << e.what() << "\"\n";
    didCatch = true;
  }
  if (didCatch)
  {
    std::cerr << "Threw exception for empty sample of non-empty sequence.\n";
    ++result;
    didCatch = false;
  }

  try
  {
    vtkReservoirSampler<int> good;
    good(0, static_cast<int>(0));
  }
  catch (std::invalid_argument& e)
  {
    std::cerr << "  Caught unexpected exception: \"" << e.what() << "\"\n";
    didCatch = true;
  }
  if (didCatch)
  {
    std::cerr << "Threw exception for empty sample of empty sequence.\n";
    ++result;
    didCatch = false;
  }

  return result;
}

int TestReservoirSamplerPlainSequence()
{
  constexpr vtkIdType kk = 5;
  constexpr vtkIdType nn = 40;
  int result = 0;
  int ii = 0;
  std::cout << "non-monotonic plain subsequence\n";
  for (const auto& seq : vtkReservoirSampler<vtkIdType, /* monotonic? */ false>()(kk, nn))
  {
    std::cout << "  " << ii << " " << seq << "\n";
    if (seq < 0 || seq >= nn)
    {
      std::cerr << "    Bad index " << seq << " not in [0," << nn << "[\n";
      ++result;
    }
    ++ii;
  }
  if (ii != kk)
  {
    std::cerr << "Incorrect plain subsequence has " << ii << " not " << kk << " entries.\n";
    ++result;
  }
  return result;
}

int TestReservoirSamplerArraySizeSequence()
{
  constexpr vtkIdType kk = 5;
  constexpr vtkIdType nn = 40;
  vtkNew<vtkDoubleArray> array;
  array->SetNumberOfTuples(nn);
  int result = 0;
  int ii = 0;
  std::cout << "monotonic array index sequence\n";
  for (const auto& seq : vtkReservoirSampler<vtkIdType>()(kk, array.GetPointer()))
  {
    std::cout << "  " << ii << " " << seq << "\n";
    if (seq < 0 || seq >= nn)
    {
      std::cerr << "    Bad index " << seq << " not in [0," << nn << "[\n";
      ++result;
    }
    ++ii;
  }
  if (ii != kk)
  {
    std::cerr << "Incorrect array subsequence has " << ii << " not " << kk << " entries.\n";
    ++result;
  }
  return result;
}

int TestReservoirSamplerBenchmark()
{
  constexpr vtkIdType kk = 128;
  constexpr vtkIdType nn = std::numeric_limits<vtkIdType>::max();
  int result = 0;
  std::cout << "non-monotonic benchmark subsequences\n";
  vtkReservoirSampler<vtkIdType, /* monotonic? */ false> sampler;
  auto tStart = std::chrono::steady_clock::now();
  for (int ii = 0; ii < 128; ++ii)
  {
    if (sampler(kk, nn).size() != 128)
    {
      ++result;
    }
  }
  auto tStop = std::chrono::steady_clock::now();
  auto dt = std::chrono::duration_cast<std::chrono::microseconds>(tStop - tStart).count();
  std::cout << "  " << dt << "µs for 128 samples of 128 values from a large sequence.\n";
  if (dt > 5e6)
  {
    std::cerr << "Expected sampling to be much faster. Failing test for bad benchmark.\n";
    ++result;
  }
  return result;
}

int TestReservoirSampler(int, char*[])
{
  // Store up any errors, return non-zero if something fails.
  int retVal = 0;

  retVal += TestReservoirSamplerExceptions();
  retVal += TestReservoirSamplerPlainSequence();
  retVal += TestReservoirSamplerArraySizeSequence();
  retVal += TestReservoirSamplerBenchmark();

  return retVal;
}
