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
#ifndef viskores_cont_arg_TransportTagAtomicArray_h
#define viskores_cont_arg_TransportTagAtomicArray_h

#include <viskores/Types.h>

#include <viskores/cont/ArrayHandle.h>

#include <viskores/cont/arg/Transport.h>

#include <viskores/cont/AtomicArray.h>

namespace viskores
{
namespace cont
{
namespace arg
{

/// \brief \c Transport tag for in-place arrays with atomic operations.
///
/// \c TransportTagAtomicArray is a tag used with the \c Transport class to
/// transport \c ArrayHandle objects for data that is both input and output
/// (that is, in place modification of array data). The array will be wrapped
/// in a viskores::exec::AtomicArray class that provides atomic operations (like
/// add and compare/swap).
///
struct TransportTagAtomicArray
{
};

template <typename T, typename Device>
struct Transport<viskores::cont::arg::TransportTagAtomicArray,
                 viskores::cont::ArrayHandle<T, viskores::cont::StorageTagBasic>,
                 Device>
{
  using ExecObjectType = viskores::exec::AtomicArrayExecutionObject<T>;
  using ExecType = viskores::cont::AtomicArray<T>;

  template <typename InputDomainType>
  VISKORES_CONT ExecObjectType
  operator()(viskores::cont::ArrayHandle<T, viskores::cont::StorageTagBasic>& array,
             const InputDomainType&,
             viskores::Id,
             viskores::Id,
             viskores::cont::Token& token) const
  {
    // Note: we ignore the size of the domain because the randomly accessed
    // array might not have the same size depending on how the user is using
    // the array.
    ExecType obj(array);
    return obj.PrepareForExecution(Device(), token);
  }
};

}
}
} // namespace viskores::cont::arg

#endif //viskores_cont_arg_TransportTagAtomicArray_h
