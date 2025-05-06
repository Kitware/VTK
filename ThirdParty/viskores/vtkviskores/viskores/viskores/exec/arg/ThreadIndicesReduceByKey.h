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
#ifndef viskores_exec_arg_ThreadIndicesReduceByKey_h
#define viskores_exec_arg_ThreadIndicesReduceByKey_h

#include <viskores/exec/arg/ThreadIndicesBasic.h>

#include <viskores/exec/internal/ReduceByKeyLookup.h>

namespace viskores
{
namespace exec
{
namespace arg
{

/// \brief Container for thread indices in a reduce by key invocation.
///
/// This specialization of \c ThreadIndices adds extra indices that deal with a
/// reduce by key. In particular, it save the indices used to map from a unique
/// key index to the group of input values that has that key associated with
/// it.
///
class ThreadIndicesReduceByKey : public viskores::exec::arg::ThreadIndicesBasic
{
  using Superclass = viskores::exec::arg::ThreadIndicesBasic;

public:
  template <typename P1, typename P2>
  VISKORES_EXEC ThreadIndicesReduceByKey(
    viskores::Id threadIndex,
    viskores::Id inIndex,
    viskores::IdComponent visitIndex,
    viskores::Id outIndex,
    const viskores::exec::internal::ReduceByKeyLookupBase<P1, P2>& keyLookup)
    : Superclass(threadIndex, inIndex, visitIndex, outIndex)
    , ValueOffset(keyLookup.Offsets.Get(inIndex))
    , NumberOfValues(static_cast<viskores::IdComponent>(keyLookup.Offsets.Get(inIndex + 1) -
                                                        keyLookup.Offsets.Get(inIndex)))
  {
  }

  VISKORES_EXEC
  viskores::Id GetValueOffset() const { return this->ValueOffset; }

  VISKORES_EXEC
  viskores::IdComponent GetNumberOfValues() const { return this->NumberOfValues; }

private:
  viskores::Id ValueOffset;
  viskores::IdComponent NumberOfValues;
};
}
}
} // namespace viskores::exec::arg

#endif //viskores_exec_arg_ThreadIndicesReduceByKey_h
