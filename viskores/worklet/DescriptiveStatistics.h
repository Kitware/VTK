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
#ifndef viskores_worklet_DescriptiveStatistics_h
#define viskores_worklet_DescriptiveStatistics_h

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandleTransform.h>
#include <viskores/cont/ArrayHandleZip.h>

namespace viskores
{
namespace worklet
{
class DescriptiveStatistics
{
public:
  template <typename T>
  struct StatState
  {
    VISKORES_EXEC_CONT
    StatState()
      : n_(0)
      , min_(std::numeric_limits<T>::max())
      , max_(std::numeric_limits<T>::lowest())
      , sum_(0)
      , mean_(0)
      , M2_(0)
      , M3_(0)
      , M4_(0)
    {
    }

    VISKORES_EXEC_CONT
    StatState(T value)
      : n_(1)
      , min_(value)
      , max_(value)
      , sum_(value)
      , mean_(value)
      , M2_(0)
      , M3_(0)
      , M4_(0)
    {
    }

    VISKORES_EXEC_CONT
    StatState(T n, T min, T max, T sum, T mean, T M2, T M3, T M4)
      : n_(n)
      , min_(min)
      , max_(max)
      , sum_(sum)
      , mean_(mean)
      , M2_(M2)
      , M3_(M3)
      , M4_(M4)
    {
    }

    VISKORES_EXEC_CONT
    StatState operator+(const StatState<T>& y) const
    {
      const StatState<T>& x = *this;
      if (y.n_ == 0)
      {
        return x;
      }
      if (x.n_ == 0)
      {
        return y;
      }

      StatState result;
      result.n_ = x.n_ + y.n_;

      result.min_ = viskores::Min(x.min_, y.min_);
      result.max_ = viskores::Max(x.max_, y.max_);

      // TODO: consider implementing compensated sum
      // https://en.wikipedia.org/wiki/Kahan_summation_algorithm
      result.sum_ = x.sum_ + y.sum_;

      // It is tempting to try to deviate from the literature and calculate
      // mean in each "reduction" from sum and n. This saves one multiplication.
      // However, RESIST THE TEMPTATION!!! This takes us back to the naive
      // algorithm (mean = sum of a bunch of numbers / N) that actually
      // accumulates more error and causes problem when calculating M2
      // (and thus variance).
      // TODO: Verify that FieldStatistics exhibits the same problem since
      // it is using a "parallel" version of the naive algorithm as well.
      // TODO: or better, just deprecate FieldStatistics.
      T delta = y.mean_ - x.mean_;
      result.mean_ = x.mean_ + delta * y.n_ / result.n_;

      T delta2 = delta * delta;
      result.M2_ = x.M2_ + y.M2_ + delta2 * x.n_ * y.n_ / result.n_;

      T delta3 = delta * delta2;
      T n2 = result.n_ * result.n_;
      result.M3_ = x.M3_ + y.M3_;
      result.M3_ += delta3 * x.n_ * y.n_ * (x.n_ - y.n_) / n2;
      result.M3_ += T(3.0) * delta * (x.n_ * y.M2_ - y.n_ * x.M2_) / result.n_;

      T delta4 = delta2 * delta2;
      T n3 = result.n_ * n2;
      result.M4_ = x.M4_ + y.M4_;
      result.M4_ += delta4 * x.n_ * y.n_ * (x.n_ * x.n_ - x.n_ * y.n_ + y.n_ * y.n_) / n3;
      result.M4_ += T(6.0) * delta2 * (x.n_ * x.n_ * y.M2_ + y.n_ * y.n_ * x.M2_) / n2;
      result.M4_ += T(4.0) * delta * (x.n_ * y.M3_ - y.n_ * x.M3_) / result.n_;

      return result;
    }

    VISKORES_EXEC_CONT T N() const { return this->n_; }

    VISKORES_EXEC_CONT
    T Min() const { return this->min_; }

    VISKORES_EXEC_CONT
    T Max() const { return this->max_; }

    VISKORES_EXEC_CONT
    T Sum() const { return this->sum_; }

    VISKORES_EXEC_CONT
    T Mean() const { return this->mean_; }

    VISKORES_EXEC_CONT
    T M2() const { return this->M2_; }

    VISKORES_EXEC_CONT
    T M3() const { return this->M3_; }

    VISKORES_EXEC_CONT
    T M4() const { return this->M4_; }

    VISKORES_EXEC_CONT
    T SampleStddev() const { return viskores::Sqrt(this->SampleVariance()); }

    VISKORES_EXEC_CONT
    T PopulationStddev() const { return viskores::Sqrt(this->PopulationVariance()); }

    VISKORES_EXEC_CONT
    T SampleVariance() const
    {
      if (this->n_ <= 1)
      {
        return 0;
      }
      return this->M2_ / (this->n_ - 1);
    }

    VISKORES_EXEC_CONT
    T PopulationVariance() const
    {
      if (this->M2_ == 0 || this->n_ == 0)
      {
        return T(0);
      }
      return this->M2_ / this->n_;
    }

    VISKORES_EXEC_CONT
    T Skewness() const
    {
      if (this->M2_ == 0 || this->n_ == 0)
        // Shamelessly swiped from Boost Math
        // The limit is technically undefined, but the interpretation here is clear:
        // A constant dataset has no skewness.
        return T(0);
      else
        return viskores::Sqrt(this->n_) * this->M3_ / viskores::Pow(this->M2_, T{ 1.5 });
    }

    VISKORES_EXEC_CONT
    T Kurtosis() const
    {
      if (this->M2_ == 0 || this->n_ == 0)
        // Shamelessly swiped from Boost Math
        // The limit is technically undefined, but the interpretation here is clear:
        // A constant dataset has no kurtosis.
        return T(0);
      else
        return this->n_ * this->M4_ / (this->M2_ * this->M2_);
    }

  private:
    // GCC4.8 is not happy about initializing data members here.
    T n_;
    T min_;
    T max_;
    T sum_;
    T mean_;
    T M2_;
    T M3_;
    T M4_;
  }; // StatState

  struct MakeStatState
  {
    template <typename T>
    VISKORES_EXEC_CONT viskores::worklet::DescriptiveStatistics::StatState<T> operator()(
      T value) const
    {
      return viskores::worklet::DescriptiveStatistics::StatState<T>{ value };
    }
  };

  /// \brief Calculate various summary statistics for the input ArrayHandle
  ///
  /// Reference:
  ///    [1] Wikipeida, parallel algorithm for calculating variance
  ///        http://en.wikipedia.org/wiki/Algorithms_for_calculating_variance#Parallel_algorithm
  ///    [2] Implementation of [1] in the Trust library
  ///        https://github.com/thrust/thrust/blob/master/examples/summary_statistics.cu
  ///    [3] Bennett, Janine, et al. "Numerically stable, single-pass, parallel statistics algorithms."
  ///        2009 IEEE International Conference on Cluster Computing and Workshops. IEEE, 2009.
  template <typename FieldType, typename Storage>
  VISKORES_CONT static StatState<FieldType> Run(
    const viskores::cont::ArrayHandle<FieldType, Storage>& field)
  {
    using Algorithm = viskores::cont::Algorithm;

    // Essentially a TransformReduce. Do we have that convenience in Viskores?
    auto states = viskores::cont::make_ArrayHandleTransform(field, MakeStatState{});
    return Algorithm::Reduce(states, StatState<FieldType>{});
  }

  template <typename KeyType, typename ValueType, typename KeyInStorage, typename ValueInStorage>
  VISKORES_CONT static auto Run(
    const viskores::cont::ArrayHandle<KeyType, KeyInStorage>& keys,
    const viskores::cont::ArrayHandle<ValueType, ValueInStorage>& values)
    -> viskores::cont::ArrayHandleZip<viskores::cont::ArrayHandle<KeyType>,
                                      viskores::cont::ArrayHandle<StatState<ValueType>>>
  {
    using Algorithm = viskores::cont::Algorithm;

    // Make a copy of the input arrays so we don't modify them
    viskores::cont::ArrayHandle<KeyType> keys_copy;
    viskores::cont::ArrayCopy(keys, keys_copy);

    viskores::cont::ArrayHandle<ValueType> values_copy;
    viskores::cont::ArrayCopy(values, values_copy);

    // Gather values of the same key by sorting them according to keys
    Algorithm::SortByKey(keys_copy, values_copy);

    auto states = viskores::cont::make_ArrayHandleTransform(values_copy, MakeStatState{});
    viskores::cont::ArrayHandle<KeyType> keys_out;

    viskores::cont::ArrayHandle<StatState<ValueType>> results;
    Algorithm::ReduceByKey(keys_copy, states, keys_out, results, viskores::Add{});

    return viskores::cont::make_ArrayHandleZip(keys_out, results);
  }
}; // DescriptiveStatistics

} // worklet
} // viskores
#endif // viskores_worklet_DescriptiveStatistics_h
