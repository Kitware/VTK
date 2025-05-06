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
#ifndef viskores_cont_AtomicArray_h
#define viskores_cont_AtomicArray_h

#include <viskores/List.h>
#include <viskores/StaticAssert.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/DeviceAdapter.h>
#include <viskores/cont/ExecutionObjectBase.h>
#include <viskores/exec/AtomicArrayExecutionObject.h>

namespace viskores
{
namespace cont
{

/// \brief A type list containing types that can be used with an AtomicArray.
///
/// @cond NONE
using AtomicArrayTypeList = viskores::List<viskores::UInt32,
                                           viskores::Int32,
                                           viskores::UInt64,
                                           viskores::Int64,
                                           viskores::Float32,
                                           viskores::Float64>;
/// @endcond


/// A class that can be used to atomically operate on an array of values safely
/// across multiple instances of the same worklet. This is useful when you have
/// an algorithm that needs to accumulate values in parallel, but writing out a
/// value per worklet might be memory prohibitive.
///
/// To construct an AtomicArray you will need to pass in an
/// viskores::cont::ArrayHandle that is used as the underlying storage for the
/// AtomicArray
///
/// Supported Operations: get / add / compare and swap (CAS). See
/// AtomicArrayExecutionObject for details.
///
/// Supported Types: 32 / 64 bit signed/unsigned integers.
///
///
template <typename T>
class AtomicArray : public viskores::cont::ExecutionObjectBase
{
  static constexpr bool ValueTypeIsValid = viskores::ListHas<AtomicArrayTypeList, T>::value;
  VISKORES_STATIC_ASSERT_MSG(ValueTypeIsValid, "AtomicArray used with unsupported ValueType.");


public:
  using ValueType = T;

  VISKORES_CONT
  AtomicArray()
    : Handle(viskores::cont::ArrayHandle<T>())
  {
  }

  VISKORES_CONT AtomicArray(viskores::cont::ArrayHandle<T> handle)
    : Handle(handle)
  {
  }

  VISKORES_CONT viskores::exec::AtomicArrayExecutionObject<T> PrepareForExecution(
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token) const
  {
    return viskores::exec::AtomicArrayExecutionObject<T>(this->Handle, device, token);
  }

private:
  /// @cond NONE
  viskores::cont::ArrayHandle<T> Handle;
  /// @endcond
};
}
} // namespace viskores::exec

#endif //viskores_cont_AtomicArray_h
