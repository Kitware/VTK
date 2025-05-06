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
#ifndef viskores_exec_arg_ThreadIndicesNeighborhood_h
#define viskores_exec_arg_ThreadIndicesNeighborhood_h


#include <viskores/exec/BoundaryState.h>
#include <viskores/exec/ConnectivityStructured.h>
#include <viskores/exec/arg/ThreadIndicesBasic.h>
#include <viskores/exec/arg/ThreadIndicesTopologyMap.h> //for Deflate and Inflate

#include <viskores/Math.h>

namespace viskores
{
namespace exec
{
namespace arg
{

namespace detail
{
/// Given a \c Vec of (semi) arbitrary size, inflate it to a viskores::Id3 by padding with zeros.
///
inline VISKORES_EXEC viskores::Id3 To3D(viskores::Id3 index)
{
  return index;
}

/// Given a \c Vec of (semi) arbitrary size, inflate it to a viskores::Id3 by padding with zeros.
/// \overload
inline VISKORES_EXEC viskores::Id3 To3D(viskores::Id2 index)
{
  return viskores::Id3(index[0], index[1], 1);
}

/// Given a \c Vec of (semi) arbitrary size, inflate it to a viskores::Id3 by padding with zeros.
/// \overload
inline VISKORES_EXEC viskores::Id3 To3D(viskores::Vec<viskores::Id, 1> index)
{
  return viskores::Id3(index[0], 1, 1);
}

/// Given a \c Vec of (semi) arbitrary size, inflate it to a viskores::Id3 by padding with zeros.
/// \overload
inline VISKORES_EXEC viskores::Id3 To3D(viskores::Id index)
{
  return viskores::Id3(index, 1, 1);
}
}

class ThreadIndicesNeighborhood
{

public:
  VISKORES_EXEC ThreadIndicesNeighborhood(viskores::Id threadIndex1D,
                                          const viskores::exec::BoundaryState& state)
    : State(state)
    , ThreadIndex(threadIndex1D)
    , InputIndex(threadIndex1D)
    , OutputIndex(threadIndex1D)
    , VisitIndex(0)
  {
  }

  VISKORES_EXEC ThreadIndicesNeighborhood(viskores::Id threadIndex1D,
                                          viskores::Id inputIndex,
                                          viskores::IdComponent visitIndex,
                                          viskores::Id outputIndex,
                                          const viskores::exec::BoundaryState& state)
    : State(state)
    , ThreadIndex(threadIndex1D)
    , InputIndex(inputIndex)
    , OutputIndex(outputIndex)
    , VisitIndex(visitIndex)
  {
  }

  VISKORES_EXEC
  const viskores::exec::BoundaryState& GetBoundaryState() const { return this->State; }

  VISKORES_EXEC
  viskores::Id GetThreadIndex() const { return this->ThreadIndex; }

  VISKORES_EXEC
  viskores::Id GetInputIndex() const { return this->InputIndex; }

  VISKORES_EXEC
  viskores::Id3 GetInputIndex3D() const { return this->State.IJK; }

  VISKORES_EXEC
  viskores::Id GetOutputIndex() const { return this->OutputIndex; }

  VISKORES_EXEC
  viskores::IdComponent GetVisitIndex() const { return this->VisitIndex; }

private:
  viskores::exec::BoundaryState State;
  viskores::Id ThreadIndex;
  viskores::Id InputIndex;
  viskores::Id OutputIndex;
  viskores::IdComponent VisitIndex;
};
}
}
} // namespace viskores::exec::arg

#endif //viskores_exec_arg_ThreadIndicesNeighborhood_h
