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
#ifndef viskores_exec_arg_testing_ThreadIndicesTesting_h
#define viskores_exec_arg_testing_ThreadIndicesTesting_h

#include <viskores/Types.h>

namespace viskores
{
namespace exec
{
namespace arg
{

/// \brief Simplified version of ThreadIndices for unit testing purposes
///
class ThreadIndicesTesting
{
public:
  VISKORES_EXEC_CONT
  ThreadIndicesTesting(viskores::Id index)
    : InputIndex(index)
    , OutputIndex(index)
    , VisitIndex(0)
  {
  }

  VISKORES_EXEC_CONT
  ThreadIndicesTesting(viskores::Id inputIndex,
                       viskores::Id outputIndex,
                       viskores::IdComponent visitIndex)
    : InputIndex(inputIndex)
    , OutputIndex(outputIndex)
    , VisitIndex(visitIndex)
  {
  }

  VISKORES_EXEC_CONT
  viskores::Id GetInputIndex() const { return this->InputIndex; }

  VISKORES_EXEC_CONT
  viskores::Id3 GetInputIndex3D() const { return viskores::Id3(this->GetInputIndex(), 0, 0); }

  VISKORES_EXEC_CONT
  viskores::Id GetOutputIndex() const { return this->OutputIndex; }

  VISKORES_EXEC_CONT
  viskores::IdComponent GetVisitIndex() const { return this->VisitIndex; }

  VISKORES_EXEC_CONT
  viskores::Id GetThreadIndex() const { return this->OutputIndex; }

private:
  viskores::Id InputIndex;
  viskores::Id OutputIndex;
  viskores::IdComponent VisitIndex;
};
}
}
} // namespace viskores::exec::arg

#endif //viskores_exec_arg_testing_ThreadIndicesTesting_h
