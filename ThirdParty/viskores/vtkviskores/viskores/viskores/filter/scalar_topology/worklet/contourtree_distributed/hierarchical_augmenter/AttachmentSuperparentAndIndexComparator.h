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
// COMMENTS:
//
// A comparator that sorts supernode pairs by:
//  1.  the superparent round
//  2.  global regular Id
//  3.  supernode Id (if any)
//
// We don't care about the orientation of the superarc for this comparator
//
// For duplicates, we assume that at MOST one (in fact, it should always be EXACTLY one)
// copy has a supernode Id set.  This is because when we exchange between blocks, we set
// the supernode Id to NO_SUCH_ELEMENT. That way, only the copy that belongs on the block
// has the supernode Id set. We want to ensure that it appears at the beginning of the segment,
// and don't care about the ordering of any others.
//
//=======================================================================================


#ifndef viskores_worklet_contourtree_distributed_hierarchical_hyper_augmenter_attachment_superparent_and_index_comparator_h
#define viskores_worklet_contourtree_distributed_hierarchical_hyper_augmenter_attachment_superparent_and_index_comparator_h

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ExecutionObjectBase.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/Types.h>

namespace viskores
{
namespace worklet
{
namespace contourtree_distributed
{
namespace hierarchical_augmenter
{


/// Implementation  for a comparator that sorts supernode pairs by:
///  1.  the superparent round
///  2.  global regular Id
///  3.  supernode Id (if any)
class AttachmentSuperparentAndIndexComparatorImpl
{
public:
  using IdArrayPortalType =
    typename viskores::worklet::contourtree_augmented::IdArrayType::ReadPortalType;

  // constructor
  VISKORES_CONT
  AttachmentSuperparentAndIndexComparatorImpl(IdArrayPortalType superparentsPortal,
                                              IdArrayPortalType globalRegularIdsPortal,
                                              IdArrayPortalType supernodeIdsPortal)
    : SuperparentsPortal(superparentsPortal)
    , GlobalRegularIdsPortal(globalRegularIdsPortal)
    , SupernodeIdsPortal(supernodeIdsPortal)
  { // constructor
  } // constructor

  // () operator - gets called to do comparison
  VISKORES_EXEC
  bool operator()(const viskores::Id& left, const viskores::Id& right) const
  { // operator()
    // optimisation for sorts which compare an element with itself
    // if the element compares with itself, always return false (it's not less than itself)
    if (left == right)
    {
      return false;
    }
    // first comparison is on superparent WITHOUT ascending descending flag
    if (viskores::worklet::contourtree_augmented::MaskedIndex(this->SuperparentsPortal.Get(left)) <
        viskores::worklet::contourtree_augmented::MaskedIndex(this->SuperparentsPortal.Get(right)))
    {
      return true;
    }
    if (viskores::worklet::contourtree_augmented::MaskedIndex(this->SuperparentsPortal.Get(left)) >
        viskores::worklet::contourtree_augmented::MaskedIndex(this->SuperparentsPortal.Get(right)))
    {
      return false;
    }

    // second comparison is on global regular Id
    if (this->GlobalRegularIdsPortal.Get(left) < this->GlobalRegularIdsPortal.Get(right))
    {
      return viskores::worklet::contourtree_augmented::IsAscending(
        this->SuperparentsPortal.Get(left));
    }
    if (this->GlobalRegularIdsPortal.Get(left) > this->GlobalRegularIdsPortal.Get(right))
    {
      return !viskores::worklet::contourtree_augmented::IsAscending(
        this->SuperparentsPortal.Get(left));
    }

    // it now depends on whether they have actual IDs (ie they are on this block anyway)
    if (viskores::worklet::contourtree_augmented::NoSuchElement(this->SupernodeIdsPortal.Get(left)))
    { // left does not exist
      if (viskores::worklet::contourtree_augmented::NoSuchElement(
            this->SupernodeIdsPortal.Get(right)))
      { // right does not exist
        // neither exists: sort on input indices instead
        return (left < right);
      } // right does not exist
      else
      { // right does exist
        // right exists but left doesn't - sort right lower
        return false;
      } // right does exist
    }   // left does not exist
    else
    { // left does exist
      if (viskores::worklet::contourtree_augmented::NoSuchElement(
            this->SupernodeIdsPortal.Get(right)))
      { // right does not exist
        // left exists but right doesn't - sort left lower
        return true;
      } // right does not exist
      else
      { // right does exist
        // both exist
        return (this->SupernodeIdsPortal.Get(left) < this->SupernodeIdsPortal.Get(right));
      } // right does exist
    }   // left does exist
  }     // operator()

private:
  IdArrayPortalType SuperparentsPortal;
  IdArrayPortalType GlobalRegularIdsPortal;
  IdArrayPortalType SupernodeIdsPortal;
}; // AttachmentSuperparentAndIndexComparatorImpl


/// Execution object for a comparator that sorts supernode pairs by:
///  1.  the superparent round
///  2.  global regular Id
///  3.  supernode Id (if any)
class AttachmentSuperparentAndIndexComparator : public viskores::cont::ExecutionObjectBase
{
public:
  // constructor - takes vectors as parameters
  VISKORES_CONT
  AttachmentSuperparentAndIndexComparator(
    const viskores::worklet::contourtree_augmented::IdArrayType superparents,
    const viskores::worklet::contourtree_augmented::IdArrayType globalRegularIds,
    const viskores::worklet::contourtree_augmented::IdArrayType supernodeIds)
    : Superparents(superparents)
    , GlobalRegularIds(globalRegularIds)
    , SupernodeIds(supernodeIds)
  { // constructor
  } // constructor

  /// Create a AttachmentSuperparentAndIndexComparatorImpl object for use in the sort or worklet
  VISKORES_CONT AttachmentSuperparentAndIndexComparatorImpl
  PrepareForExecution(viskores::cont::DeviceAdapterId device, viskores::cont::Token& token) const
  {
    return AttachmentSuperparentAndIndexComparatorImpl(
      this->Superparents.PrepareForInput(device, token),
      this->GlobalRegularIds.PrepareForInput(device, token),
      this->SupernodeIds.PrepareForInput(device, token));
  }

private:
  /// the superparent Id
  viskores::worklet::contourtree_augmented::IdArrayType Superparents;
  /// the global rergular Id for tiebreak
  viskores::worklet::contourtree_augmented::IdArrayType GlobalRegularIds;
  /// the supernode Id for tiebreak
  viskores::worklet::contourtree_augmented::IdArrayType SupernodeIds;
}; // AttachmentSuperparentAndIndexComparator

} // namespace hierarchical_augmenter
} // namespace contourtree_distributed
} // namespace worklet
} // namespace viskores

#endif
