// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkReservoirSampler<IntegerType>
 * @brief Generate a monotonic sequence of integers that randomly kk-sample
 *        a range without substitution.
 *
 * Given a sequence of size nn, we wish to choose kk random values from the array.
 * This class returns kk (or fewer, if nn < kk) indices in the range [0, nn-1[ that
 * are ordered from smallest to largest.
 *
 * The algorithm is an implementation of Kim-Hung Li's approach, known as
 * "Algorithm L" and documented in the article "Reservoir-Sampling Algorithms of
 * Time Complexity O(n(1+log(N/n)))". ACM Transactions on Mathematical Software.
 * 20 (4): 481–493. doi:10.1145/198429.198435.
 */

#ifndef vtkReservoirSampler_h
#define vtkReservoirSampler_h

#include "vtkAbstractArray.h"
#include "vtkCommonMathModule.h"
#include "vtkTypeTraits.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <random>
#include <stdexcept>

VTK_ABI_NAMESPACE_BEGIN

class VTKCOMMONMATH_EXPORT vtkReservoirSamplerBase
{
protected:
  using SeedType = typename std::random_device::result_type;

  static SeedType RandomSeed();
};

template <typename Integer, bool Monotonic = true>
class vtkReservoirSampler : public vtkReservoirSamplerBase
{
public:
  /// Choose kk items from a sequence of (0, nn - 1).
  ///
  /// This will throw an exception if kk <= 0.
  const std::vector<Integer>& operator()(Integer kk, Integer nn) const
  {
    VTK_THREAD_LOCAL static std::vector<Integer> data;
    this->GenerateSample(kk, nn, data);
    return data;
  }

  /// Choose kk items from a sequence of (0, \a array->GetNumberOfTuples() - 1).
  ///
  /// This will throw an exception if kk <= 0.
  const std::vector<Integer>& operator()(Integer kk, vtkAbstractArray* array) const
  {
    VTK_THREAD_LOCAL static std::vector<Integer> data;
    if (!array)
    {
      throw std::invalid_argument("Null arrays are disallowed.");
    }
    if (array->GetNumberOfTuples() > std::numeric_limits<Integer>::max())
    {
      throw std::invalid_argument("Array size would overflow integer type.");
    }
    this->GenerateSample(kk, array->GetNumberOfTuples(), data);
    return data;
  }

protected:
  void GenerateSample(Integer kk, Integer nn, std::vector<Integer>& data) const
  {
    if (nn < kk)
    {
      kk = nn;
    }
    if (kk < 0)
    {
      throw std::invalid_argument(
        "You must choose a non-negative number of values from a proper sequence.");
    }
    data.resize(kk);
    if (kk == 0)
    {
      return;
    }
    // I. Fill the output with the first kk values.
    Integer ii;
    for (ii = 0; ii < kk; ++ii)
    {
      data[ii] = ii;
    }
    if (kk == nn)
    {
      return;
    }

    std::mt19937 generator(vtkReservoirSampler::RandomSeed());
    std::uniform_real_distribution<> unitUniform(0., 1.);
    std::uniform_int_distribution<Integer> randomIndex(0, kk - 1);
    double w = exp(log(unitUniform(generator)) / kk);

    while (true)
    {
      double delta = floor(log(unitUniform(generator)) / log(1.0 - w)) + 1.0;
      if (delta < 0.0 || delta > static_cast<double>(vtkTypeTraits<Integer>::Max()))
      {
        // If delta overflows the size of the integer, we are done.
        break;
      }
      Integer intDelta = static_cast<Integer>(delta);
      // Be careful here since delta may be large and nn may be
      // at or near numeric_limits<Integer>::max().
      if (nn - ii > intDelta)
      {
        Integer jj = randomIndex(generator);
#if 0
        std::cout << "      i " << ii << " δ " << intDelta << " w " << w << " → j " << jj << "\n";
#endif
        ii += intDelta;
        data[jj] = ii;
        w *= exp(log(unitUniform(generator)) / kk);
      }
      else
      {
        // Adding delta to ii would step beyond our sequence size,
        // so we are done.
        break;
      }
    }
    if (Monotonic)
    {
      std::sort(data.begin(), data.end());
    }
  }
};

VTK_ABI_NAMESPACE_END
#endif // vtkReservoirSampler_h
// VTK-HeaderTest-Exclude: vtkReservoirSampler.h
