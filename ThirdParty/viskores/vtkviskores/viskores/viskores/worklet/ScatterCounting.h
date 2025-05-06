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
#ifndef viskores_worklet_ScatterCounting_h
#define viskores_worklet_ScatterCounting_h

#include <viskores/worklet/internal/ScatterBase.h>
#include <viskores/worklet/viskores_worklet_export.h>

#include <viskores/cont/UnknownArrayHandle.h>

#include <sstream>

namespace viskores
{
namespace worklet
{

namespace detail
{

struct ScatterCountingBuilder;

} // namespace detail

/// \brief A scatter that maps input to some numbers of output.
///
/// The \c Scatter classes are responsible for defining how much output is
/// generated based on some sized input. \c ScatterCounting establishes a 1 to
/// N mapping from input to output. That is, every input element generates 0 or
/// more output elements associated with it. The output elements are grouped by
/// the input associated.
///
/// A counting scatter takes an array of counts for each input. The data is
/// taken in the constructor and the index arrays are derived from that. So
/// changing the counts after the scatter is created will have no effect.
///
struct VISKORES_WORKLET_EXPORT ScatterCounting : internal::ScatterBase
{
  using CountTypes = viskores::List<viskores::Int64,
                                    viskores::Int32,
                                    viskores::Int16,
                                    viskores::Int8,
                                    viskores::UInt64,
                                    viskores::UInt32,
                                    viskores::UInt16,
                                    viskores::UInt8>;

  /// Construct a \c ScatterCounting object using an array of counts for the
  /// number of outputs for each input. Part of the construction requires
  /// generating an input to output map, but this map is not needed for the
  /// operations of \c ScatterCounting, so by default it is deleted. However,
  /// other users might make use of it, so you can instruct the constructor
  /// to save the input to output map.
  ///
  VISKORES_CONT ScatterCounting(
    const viskores::cont::UnknownArrayHandle& countArray,
    viskores::cont::DeviceAdapterId device = viskores::cont::DeviceAdapterTagAny(),
    bool saveInputToOutputMap = false)
  {
    this->BuildArrays(countArray, device, saveInputToOutputMap);
  }
  VISKORES_CONT ScatterCounting(const viskores::cont::UnknownArrayHandle& countArray,
                                bool saveInputToOutputMap)
  {
    this->BuildArrays(countArray, viskores::cont::DeviceAdapterTagAny(), saveInputToOutputMap);
  }

  using OutputToInputMapType = viskores::cont::ArrayHandle<viskores::Id>;

  template <typename RangeType>
  VISKORES_CONT OutputToInputMapType GetOutputToInputMap(RangeType) const
  {
    return this->OutputToInputMap;
  }

  using VisitArrayType = viskores::cont::ArrayHandle<viskores::IdComponent>;
  template <typename RangeType>
  VISKORES_CONT VisitArrayType GetVisitArray(RangeType) const
  {
    return this->VisitArray;
  }

  VISKORES_CONT
  viskores::Id GetOutputRange(viskores::Id inputRange) const
  {
    if (inputRange != this->InputRange)
    {
      std::stringstream msg;
      msg << "ScatterCounting initialized with input domain of size " << this->InputRange
          << " but used with a worklet invoke of size " << inputRange << std::endl;
      throw viskores::cont::ErrorBadValue(msg.str());
    }
    return this->VisitArray.GetNumberOfValues();
  }
  VISKORES_CONT
  viskores::Id GetOutputRange(viskores::Id3 inputRange) const
  {
    return this->GetOutputRange(inputRange[0] * inputRange[1] * inputRange[2]);
  }

  VISKORES_CONT
  OutputToInputMapType GetOutputToInputMap() const { return this->OutputToInputMap; }

  /// This array will not be valid unless explicitly instructed to be saved.
  /// (See documentation for the constructor.)
  ///
  VISKORES_CONT
  viskores::cont::ArrayHandle<viskores::Id> GetInputToOutputMap() const
  {
    return this->InputToOutputMap;
  }

private:
  viskores::Id InputRange;
  viskores::cont::ArrayHandle<viskores::Id> InputToOutputMap;
  OutputToInputMapType OutputToInputMap;
  VisitArrayType VisitArray;

  friend struct detail::ScatterCountingBuilder;

  VISKORES_CONT void BuildArrays(const viskores::cont::UnknownArrayHandle& countArray,
                                 viskores::cont::DeviceAdapterId device,
                                 bool saveInputToOutputMap);
};
}
} // namespace viskores::worklet

#endif //viskores_worklet_ScatterCounting_h
