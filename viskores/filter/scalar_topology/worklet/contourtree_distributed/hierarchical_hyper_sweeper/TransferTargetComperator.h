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
// Copyright (c) 2018, The Regents of the University of California, through
// Lawrence Berkeley National Laboratory (subject to receipt of any required approvals
// from the U.S. Dept. of Energy).  All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
// (1) Redistributions of source code must retain the above copyright notice, this
//     list of conditions and the following disclaimer.
//
// (2) Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
// (3) Neither the name of the University of California, Lawrence Berkeley National
//     Laboratory, U.S. Dept. of Energy nor the names of its contributors may be
//     used to endorse or promote products derived from this software without
//     specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//
//=============================================================================
//
//  This code is an extension of the algorithm presented in the paper:
//  Parallel Peak Pruning for Scalable SMP Contour Tree Computation.
//  Hamish Carr, Gunther Weber, Christopher Sewell, and James Ahrens.
//  Proceedings of the IEEE Symposium on Large Data Analysis and Visualization
//  (LDAV), October 2016, Baltimore, Maryland.
//
//  The PPP2 algorithm and software were jointly developed by
//  Hamish Carr (University of Leeds), Gunther H. Weber (LBNL), and
//  Oliver Ruebel (LBNL)
//==============================================================================

#ifndef viskores_worklet_contourtree_distributed_hierarchical_hyper_sweeper_transfer_target_comperator_h
#define viskores_worklet_contourtree_distributed_hierarchical_hyper_sweeper_transfer_target_comperator_h

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ExecutionObjectBase.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/Types.h>

namespace viskores
{
namespace worklet
{
namespace contourtree_distributed
{
namespace hierarchical_hyper_sweeper
{


// comperator function for an indirect sort on the superarc target
class TransferTargetComperatorImpl
{
public:
  using IdArrayPortalType =
    typename viskores::worklet::contourtree_augmented::IdArrayType::ReadPortalType;

  // constructor - takes vectors as parameters
  VISKORES_CONT
  TransferTargetComperatorImpl(IdArrayPortalType superarcPortal)
    : SuperarcPortal(superarcPortal)
  { // constructor
  } // constructor

  // () operator - gets called to do comparison
  VISKORES_EXEC
  bool operator()(const viskores::Id& left, const viskores::Id& right) const
  { // operator()
    // NOTE: We need to explicitly check for NO_SUCH_ELEMENT here since viskores::Id is signed
    // while the index time in PPP is unsigned. Thus, for PPP "regular" indices are always
    // smaller that NO_SUCH_ELEMENT, while with the signed viskores::Id, NO_SUCH_ELEMENT is
    // negative and the order is not as intented.
    auto leftVal = this->SuperarcPortal.Get(left);
    auto rightVal = this->SuperarcPortal.Get(right);

    // Elements with TRANSFER_TO_SUPERARC as a heading flag should have higher "value order" than other elements without it.
    // Although TRANSFER_TO_SUPERARC is positive, a safer way to do this is to extract the heading indicator explicitly.
    bool isLeftTransferTo = viskores::worklet::contourtree_augmented::TransferToSuperarc(leftVal);
    bool isRightTransferTo = viskores::worklet::contourtree_augmented::TransferToSuperarc(rightVal);

    // If both elements are NO_SUCH_ELEMENT, we compare the index order to guarantee a determined order
    // It is not necessary to enforce the order, but a fixed order is beneficial for debug purposes
    if (viskores::worklet::contourtree_augmented::NoSuchElement(leftVal))
    {
      if (viskores::worklet::contourtree_augmented::NoSuchElement(rightVal))
        return left < right;
      else
        return false;
    }
    else if (viskores::worklet::contourtree_augmented::NoSuchElement(rightVal))
    {
      return true;
    }
    else
    {
      if (isLeftTransferTo && !isRightTransferTo)
        return false;
      else if (!isLeftTransferTo && isRightTransferTo)
        return true;
      else
      {
        viskores::Id leftMaskedVal = viskores::worklet::contourtree_augmented::MaskedIndex(leftVal);
        viskores::Id rightMaskedVal =
          viskores::worklet::contourtree_augmented::MaskedIndex(rightVal);
        if (leftMaskedVal < rightMaskedVal)
          return true;
        if (leftMaskedVal > rightMaskedVal)
          return false;
        // tiebreaker using the ID for debug purpose
        return left < right;
      }
    }
  } // operator()

private:
  IdArrayPortalType SuperarcPortal;
}; // TransferTargetComperatorImpl


class TransferTargetComperator : public viskores::cont::ExecutionObjectBase
{
public:
  // constructor - takes vectors as parameters
  VISKORES_CONT
  TransferTargetComperator(const viskores::worklet::contourtree_augmented::IdArrayType superarcs)
    : Superarcs(superarcs)
  { // constructor
  } // constructor

  VISKORES_CONT TransferTargetComperatorImpl
  PrepareForExecution(viskores::cont::DeviceAdapterId device, viskores::cont::Token& token) const
  {
    return TransferTargetComperatorImpl(this->Superarcs.PrepareForInput(device, token));
  }

private:
  viskores::worklet::contourtree_augmented::IdArrayType Superarcs;
}; // TransferTargetComperator

} // namespace hierarchical_hyper_sweeper
} // namespace contourtree_distributed
} // namespace worklet
} // namespace viskores

#endif
