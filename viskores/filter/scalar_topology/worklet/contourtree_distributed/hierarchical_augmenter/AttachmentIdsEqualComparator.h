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


#ifndef viskores_worklet_contourtree_distributed_hierarchical_hyper_augmenter_attachment_ids_equal_comparator_h
#define viskores_worklet_contourtree_distributed_hierarchical_hyper_augmenter_attachment_ids_equal_comparator_h

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
class AttachmentIdsEqualComparatorImpl
{
public:
  using IdArrayPortalType =
    typename viskores::worklet::contourtree_augmented::IdArrayType::ReadPortalType;

  // constructor
  VISKORES_CONT
  AttachmentIdsEqualComparatorImpl(IdArrayPortalType globalRegularIdsPortal)
    : GlobalRegularIdsPortal(globalRegularIdsPortal)
  { // constructor
  } // constructor

  // () operator - gets called to do comparison
  VISKORES_EXEC
  bool operator()(const viskores::Id& x, const viskores::Id& y) const
  { // operator()
    return (this->GlobalRegularIdsPortal.Get(x) == this->GlobalRegularIdsPortal.Get(y));
  } // operator()

private:
  IdArrayPortalType GlobalRegularIdsPortal;
}; // AttachmentIdsEqualComparatorImpl


/// Execution object for a comparator that sorts supernode pairs by:
///  1.  the superparent round
///  2.  global regular Id
///  3.  supernode Id (if any)
class AttachmentIdsEqualComparator : public viskores::cont::ExecutionObjectBase
{
public:
  // constructor - takes vectors as parameters
  VISKORES_CONT
  AttachmentIdsEqualComparator(
    const viskores::worklet::contourtree_augmented::IdArrayType globalRegularIds)
    : GlobalRegularIds(globalRegularIds)
  { // constructor
  } // constructor

  /// Create a AttachmentIdsEqualComparatorImpl object for use in the sort or worklet
  VISKORES_CONT AttachmentIdsEqualComparatorImpl
  PrepareForExecution(viskores::cont::DeviceAdapterId device, viskores::cont::Token& token) const
  {
    return AttachmentIdsEqualComparatorImpl(this->GlobalRegularIds.PrepareForInput(device, token));
  }

private:
  viskores::worklet::contourtree_augmented::IdArrayType GlobalRegularIds;
}; // AttachmentIdsEqualComparator

} // namespace hierarchical_augmenter
} // namespace contourtree_distributed
} // namespace worklet
} // namespace viskores

#endif
