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
//  The PPP2 algorithm and software were jointly developed by
//  Hamish Carr (University of Leeds), Gunther H. Weber (LBNL), and
//  Oliver Ruebel (LBNL)
//==============================================================================

#ifndef viskores_worklet_contourtree_distributed_hierarchical_augmenter_set_first_attachment_point_in_round_worklet_h
#define viskores_worklet_contourtree_distributed_hierarchical_augmenter_set_first_attachment_point_in_round_worklet_h

#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/Types.h>
#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace worklet
{
namespace contourtree_distributed
{
namespace hierarchical_augmenter
{

/// Worklet used in HierarchicalHyperSweeper.TransferWeights(...) to implement
/// step 7b. Now find the LHE of each group and subtract out the prior weight
class SetFirstAttachmentPointInRoundWorklet : public viskores::worklet::WorkletMapField
{
public:
  /// Control signature for the worklet
  /// NOTE: we need this to be in/out because any valyes we don't set here need
  /// to remain NO_SUCH_ELEMENT for further processing
  using ControlSignature = void(WholeArrayIn attachmentIds,                 // input
                                WholeArrayIn superparentRounds,             // input
                                WholeArrayInOut firstAttachmentPointInRound // input/output

  );
  using ExecutionSignature = void(InputIndex, _1, _2, _3);
  using InputDomain = _1;

  // Default Constructor
  VISKORES_EXEC_CONT
  SetFirstAttachmentPointInRoundWorklet() {}

  template <typename InFieldPortalType, typename InOutFieldPortalType>
  VISKORES_EXEC void operator()(const viskores::Id& attachmentPoint,
                                const InFieldPortalType& attachmentIdsPortal,
                                const InFieldPortalType& superparentRoundsPortal,
                                const InOutFieldPortalType& firstAttachmentPointInRoundPortal) const
  {
    // per attachment point
    // retrieve the ID of the attachment point
    viskores::Id attachmentPointId = attachmentIdsPortal.Get(attachmentPoint);
    // the 0th element always starts a segment, so set the round's beginning
    if (attachmentPoint == 0)
    {
      firstAttachmentPointInRoundPortal.Set(superparentRoundsPortal.Get(attachmentPointId),
                                            static_cast<viskores::Id>(0));
    }
    // otherwise, a segment begins when it's SP round is different from the next one down
    else
    { // not the beginning of the array
      // retrieve the two attachment point IDs
      viskores::Id previousAttachmentPointId = attachmentIdsPortal.Get(attachmentPoint - 1);
      // and the corresponding superparent rounds
      viskores::Id superparentRound = superparentRoundsPortal.Get(attachmentPointId);
      viskores::Id previousSuperparentRound =
        superparentRoundsPortal.Get(previousAttachmentPointId);
      // detect where the segment ID changes & use that to set the value
      if (superparentRound != previousSuperparentRound)
      {
        firstAttachmentPointInRoundPortal.Set(superparentRound, attachmentPoint);
      }
    } // not the beginning of the array


    // In serial this worklet implements the following operation
    /*
    for (viskores::Id attachmentPoint = 0; attachmentPoint < attachmentIDs.size(); attachmentPoint++)
    { // per attachment point
      // retrieve the ID of the attachment point
      viskores::Id attachmentPointID = attachmentIDs[attachmentPoint];
      // the 0th element always starts a segment, so set the round's beginning
      if (attachmentPoint == 0)
        firstAttachmentPointInRound[superparentRounds[attachmentPointID]] = 0;
      // otherwise, a segment begins when it's SP round is different from the next one down
      else
      { // not the beginning of the array
        // retrieve the two attachment point IDs
        viskores::Id previousAttachmentPointID = attachmentIDs[attachmentPoint-1];
        // and the corresponding superparent rounds
        viskores::Id superparentRound = superparentRounds[attachmentPointID];
        viskores::Id previousSuperparentRound = superparentRounds[previousAttachmentPointID];

        // detect where the segment ID changes & use that to set the value
        if (superparentRound != previousSuperparentRound)
          firstAttachmentPointInRound[superparentRound] = attachmentPoint;
      } // not the beginning of the array

    } // per attachment point

    */
  } // operator()()
};  // SetFirstAttachmentPointInRoundWorklet

} // namespace hierarchical_augmenter
} // namespace contourtree_distributed
} // namespace worklet
} // namespace viskores

#endif
