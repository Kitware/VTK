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

#ifndef viskores_worklet_contourtree_distributed_tree_grafter_init_active_superarcs_worklet_h
#define viskores_worklet_contourtree_distributed_tree_grafter_init_active_superarcs_worklet_h

#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/Types.h>
#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace worklet
{
namespace contourtree_distributed
{
namespace tree_grafter
{

// In TreeGrafter.InitializeActiveSuperarcs set TreeGrafter.ActiveSuperarcs
class InitActiceSuperarcsWorklet : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(
    FieldIn
      superarcIndex, // input iteration index. loop to one less than ContourTree->Supernodes.GetNumberOfValues()
    WholeArrayIn contourTreeSuperarcs,      // input
    WholeArrayIn interiorForestIsNecessary, // input
    FieldIn activeSuperarcId,               // input
    WholeArrayOut activeSuperarcs           // output
  );

  using ExecutionSignature = void(_1, _2, _3, _4, _5);
  using InputDomain = _1;

  // Default Constructor
  VISKORES_EXEC_CONT
  InitActiceSuperarcsWorklet() {}

  template <typename InFieldPortalType, typename SuperarcsPortalType>
  VISKORES_EXEC void operator()(
    const viskores::Id& from,                            // same as superarc
    const InFieldPortalType& contourTreeSuperarcsPortal, // ContourTree->Superarcs[from]
    const InFieldPortalType& isNecessaryPortal,
    const viskores::Id& activeSuperarcId,
    const SuperarcsPortalType& activeSuperarcsPortal) const
  { // operator ()
    // per active supernode
    // retrieve the supernode ID in the CT
    // retrieve the superarc target
    viskores::Id unmaskedTo = contourTreeSuperarcsPortal.Get(from);
    viskores::Id to = viskores::worklet::contourtree_augmented::MaskedIndex(unmaskedTo);

    // test to see whether the superarc needs to be transferred
    if (isNecessaryPortal.Get(from) && isNecessaryPortal.Get(to))
    {
      return;
    }

    // retrieve the position (note that -1 converts partial sum to prefix sum, since all values were 1's originally)
    viskores::Id activeSuperarc = activeSuperarcId - 1;

    // if to is higher
    if (viskores::worklet::contourtree_augmented::IsAscending(unmaskedTo))
    { // to is higher
      activeSuperarcsPortal.Set(activeSuperarc,
                                viskores::worklet::contourtree_augmented::EdgePair(from, to));
    } // to is higher
    else
    { // to is lower
      activeSuperarcsPortal.Set(activeSuperarc,
                                viskores::worklet::contourtree_augmented::EdgePair(to, from));
    } // to is lower

    // In serial this worklet implements the following operation
    /*
      // loop does not include the terminal superarc: hence the -1
       for (indexType superarc = 0; superarc < contourTree->supernodes.size() - 1; superarc++)
         { // per active supernode
         // retrieve the supernode ID in the CT
         indexType from = superarc;

         // retrieve the superarc target
         indexType unmaskedTo = contourTree->superarcs[from];
         indexType to = maskedIndex(unmaskedTo);

         // test to see whether the superarc needs to be transferred
         if (residue->isNecessary[from] && residue->isNecessary[to])
           continue;

         // retrieve the position (note that -1 converts partial sum to prefix sum, since all values were 1's originally)
         indexType activeSuperarc = activeSuperarcID[superarc] - 1;

         // if to is higher
         if (isAscending(unmaskedTo))
           { // to is higher
           activeSuperarcs[activeSuperarc].low = from;
           activeSuperarcs[activeSuperarc].high = to;
           } // to is higher
         else
           { // to is lower
           activeSuperarcs[activeSuperarc].high = from;
           activeSuperarcs[activeSuperarc].low = to;
           } // to is lower
         } // per active supernode
    */
  } // operator ()

}; // BoundaryVerticiesPerSuperArcStepOneWorklet

} // namespace tree_grafter
} // namespace contourtree_distributed
} // namespace worklet
} // namespace viskores

#endif
