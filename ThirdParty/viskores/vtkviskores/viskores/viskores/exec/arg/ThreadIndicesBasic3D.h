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
#ifndef viskores_exec_arg_ThreadIndicesBasic3D_h
#define viskores_exec_arg_ThreadIndicesBasic3D_h

#include <viskores/exec/arg/ThreadIndicesBasic.h>

namespace viskores
{
namespace exec
{
namespace arg
{

/// \brief Container for 3D thread indices in a worklet invocation
///
/// During the execution of a worklet function in an execution environment
/// thread, Viskores has to manage several indices. To simplify this management
/// and to provide a single place to store them (so that they do not have to be
/// recomputed), \c WorkletInvokeFunctor creates a \c ThreadIndices object.
/// This object gets passed to \c Fetch operations to help them load data.
///
///
class ThreadIndicesBasic3D : public viskores::exec::arg::ThreadIndicesBasic
{
public:
  VISKORES_EXEC
  ThreadIndicesBasic3D(const viskores::Id3& threadIndex3D,
                       viskores::Id threadIndex1D,
                       viskores::Id inIndex,
                       viskores::IdComponent visitIndex,
                       viskores::Id outIndex)
    : ThreadIndicesBasic(threadIndex1D, inIndex, visitIndex, outIndex)
    , ThreadIndex3D(threadIndex3D)
  {
  }

  /// \brief The 3D index into the input domain.
  ///
  /// This index refers to the input element (array value, cell, etc.) that
  /// this thread is being invoked for. If the input domain has 2 or 3
  /// dimensional indexing, this result will preserve that. If the domain
  /// indexing is just one dimensional, the result will have the index in the
  /// first component with the remaining components set to 0.
  ///
  VISKORES_EXEC
  viskores::Id3 GetInputIndex3D() const { return this->ThreadIndex3D; }

private:
  viskores::Id3 ThreadIndex3D;
};
}
}
} // namespace viskores::exec::arg

#endif //viskores_exec_arg_ThreadIndicesBasic3D_h
