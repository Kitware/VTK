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
#ifndef viskores_exec_arg_FetchTagArrayDirectInOut_h
#define viskores_exec_arg_FetchTagArrayDirectInOut_h

#include <viskores/exec/arg/AspectTagDefault.h>
#include <viskores/exec/arg/Fetch.h>

namespace viskores
{
namespace exec
{
namespace arg
{

/// \brief \c Fetch tag for in-place modifying array values with direct indexing.
///
/// \c FetchTagArrayDirectInOut is a tag used with the \c Fetch class to do
/// in-place modification of values in an array portal. The fetch uses direct
/// indexing, so the thread index given to \c Store is used as the index into
/// the array.
///
/// When using \c FetchTagArrayDirectInOut with a worklet invocation with a
/// scatter, it is a bit undefined how the in/out array should be indexed.
/// Should it be the size of the input arrays and written back there, or
/// should it be the size of the output arrays and pre-filled with the output.
/// The implementation indexes based on the output because it is safer. The
/// output will have a unique index for each worklet instance, so you don't
/// have to worry about writes stomping on each other (which they would
/// inevitably do if index as input).
///
struct FetchTagArrayDirectInOut
{
};

template <typename ExecObjectType>
struct Fetch<viskores::exec::arg::FetchTagArrayDirectInOut,
             viskores::exec::arg::AspectTagDefault,
             ExecObjectType>
{
  using ValueType = typename ExecObjectType::ValueType;

  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename ThreadIndicesType>
  VISKORES_EXEC ValueType Load(const ThreadIndicesType& indices,
                               const ExecObjectType& arrayPortal) const
  {
    return arrayPortal.Get(indices.GetOutputIndex());
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename ThreadIndicesType>
  VISKORES_EXEC void Store(const ThreadIndicesType& indices,
                           const ExecObjectType& arrayPortal,
                           const ValueType& value) const
  {
    arrayPortal.Set(indices.GetOutputIndex(), value);
  }
};
}
}
} // namespace viskores::exec::arg

#endif //viskores_exec_arg_FetchTagArrayDirectInOut_h
