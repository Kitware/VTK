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
#ifndef viskores_worklet_AverageByKey_h
#define viskores_worklet_AverageByKey_h

#include <viskores/VecTraits.h>
#include <viskores/cont/ArrayCopyDevice.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/worklet/DescriptiveStatistics.h>
#include <viskores/worklet/Keys.h>
#include <viskores/worklet/WorkletReduceByKey.h>

namespace viskores
{
namespace worklet
{

struct AverageByKey
{
  struct AverageWorklet : public viskores::worklet::WorkletReduceByKey
  {
    using ControlSignature = void(KeysIn keys, ValuesIn valuesIn, ReducedValuesOut averages);
    using ExecutionSignature = void(_2, _3);
    using InputDomain = _1;

    template <typename ValuesVecType, typename OutType>
    VISKORES_EXEC void operator()(const ValuesVecType& valuesIn, OutType& sum) const
    {
      sum = valuesIn[0];
      for (viskores::IdComponent index = 1; index < valuesIn.GetNumberOfComponents(); ++index)
      {
        sum += valuesIn[index];
      }

      // To get the average, we (of course) divide the sum by the amount of values, which is
      // returned from valuesIn.GetNumberOfComponents(). To do this, we need to cast the number of
      // components (returned as a viskores::IdComponent) to a FieldType. This is a little more complex
      // than it first seems because FieldType might be a Vec type or a Vec-like type that cannot
      // be constructed. To do this safely, we will do a component-wise divide.
      using VTraits = viskores::VecTraits<OutType>;
      using ComponentType = typename VTraits::ComponentType;
      ComponentType divisor = static_cast<ComponentType>(valuesIn.GetNumberOfComponents());
      for (viskores::IdComponent cIndex = 0; cIndex < VTraits::GetNumberOfComponents(sum); ++cIndex)
      {
        VTraits::SetComponent(sum, cIndex, VTraits::GetComponent(sum, cIndex) / divisor);
      }
    }
  };

  /// \brief Compute average values based on a set of Keys.
  ///
  /// This method uses an existing \c Keys object to collected values by those keys and find
  /// the average of those groups.
  ///
  template <typename InArrayType, typename OutArrayType>
  VISKORES_CONT static void Run(const viskores::worklet::internal::KeysBase& keys,
                                const InArrayType& inValues,
                                const OutArrayType& outAverages)
  {
    VISKORES_LOG_SCOPE(viskores::cont::LogLevel::Perf, "AverageByKey::Run");

    viskores::worklet::DispatcherReduceByKey<AverageWorklet> dispatcher;
    dispatcher.Invoke(keys, inValues, outAverages);
  }

  /// \brief Compute average values based on a set of Keys.
  ///
  /// This method uses an existing \c Keys object to collected values by those keys and find
  /// the average of those groups.
  ///
  template <typename ValueType, typename InValuesStorage>
  VISKORES_CONT static viskores::cont::ArrayHandle<ValueType> Run(
    const viskores::worklet::internal::KeysBase& keys,
    const viskores::cont::ArrayHandle<ValueType, InValuesStorage>& inValues)
  {

    viskores::cont::ArrayHandle<ValueType> outAverages;
    Run(keys, inValues, outAverages);
    return outAverages;
  }

  struct ExtractMean
  {
    template <typename ValueType>
    VISKORES_EXEC ValueType
    operator()(const viskores::worklet::DescriptiveStatistics::StatState<ValueType>& state) const
    {
      return state.Mean();
    }
  };

  /// \brief Compute average values based on an array of keys.
  ///
  /// This method uses an array of keys and an equally sized array of values. The keys in that
  /// array are collected into groups of equal keys, and the values corresponding to those groups
  /// are averaged.
  ///
  /// This method is less sensitive to constructing large groups with the keys than doing the
  /// similar reduction with a \c Keys object. For example, if you have only one key, the reduction
  /// will still be parallel. However, if you need to run the average of different values with the
  /// same keys, you will have many duplicated operations.
  ///
  template <class KeyType,
            class ValueType,
            class KeyInStorage,
            class KeyOutStorage,
            class ValueInStorage,
            class ValueOutStorage>
  VISKORES_CONT static void Run(
    const viskores::cont::ArrayHandle<KeyType, KeyInStorage>& keyArray,
    const viskores::cont::ArrayHandle<ValueType, ValueInStorage>& valueArray,
    viskores::cont::ArrayHandle<KeyType, KeyOutStorage>& outputKeyArray,
    viskores::cont::ArrayHandle<ValueType, ValueOutStorage>& outputValueArray)
  {
    VISKORES_LOG_SCOPE(viskores::cont::LogLevel::Perf, "AverageByKey::Run");

    auto results = viskores::worklet::DescriptiveStatistics::Run(keyArray, valueArray);
    // Extract results to outputKeyArray and outputValueArray
    outputKeyArray = results.GetFirstArray();
    // TODO: DescriptiveStatistics should write its output to a SOA instead of an AOS.
    // An ArrayHandle of a weird struct by itself is not useful in any general algorithm.
    // In fact, using DescriptiveStatistics at all seems like way overkill. It computes
    // all sorts of statistics, and we then throw them all away except for mean.
    auto resultsMean =
      viskores::cont::make_ArrayHandleTransform(results.GetSecondArray(), ExtractMean{});
    viskores::cont::ArrayCopyDevice(resultsMean, outputValueArray);
  }
};
}
} // viskores::worklet

#endif //viskores_worklet_AverageByKey_h
