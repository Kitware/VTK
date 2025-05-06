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

#ifndef viskores_worklet_contourtree_augmented_contourtree_maker_inc_find_degrees_find_rhe_h
#define viskores_worklet_contourtree_augmented_contourtree_maker_inc_find_degrees_find_rhe_h

#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/Types.h>
#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace worklet
{
namespace contourtree_augmented
{
namespace contourtree_maker_inc
{

// Worklet to find the RHE of each segment. This worklet is used for both the join and split tree.
// In the case of the join tre we should use the updegree as input and for the split tree the downdegree
class FindDegrees_FindRHE : public viskores::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(
    WholeArrayIn inNeighbour,      // (input)
    WholeArrayOut upOrDowndegree); // (output) updegree in the case of the
                                   //join tree and downdegree for the split tree
  typedef void ExecutionSignature(_1, InputIndex, _2);
  using InputDomain = _1;

  viskores::Id nActiveSupernodes;

  // Default Constructor
  VISKORES_EXEC_CONT
  FindDegrees_FindRHE(viskores::Id numActiveSupernodes)
    : nActiveSupernodes(numActiveSupernodes)
  {
  }

  template <typename InFieldPortalType, typename OutFieldPortalType>
  VISKORES_EXEC void operator()(const InFieldPortalType& inNeighbourPortal,
                                const viskores::Id joinOrSplitIndex,
                                const OutFieldPortalType& upOrDownDegreePortal) const
  {
    viskores::Id whichJoinOrSplit = inNeighbourPortal.Get(joinOrSplitIndex);
    if (!NoSuchElement(whichJoinOrSplit))
    { // an actual neighbour
      // RHE always computes - this may be redundant, since NO_SUCH_ELEMENT should sort high
      if (joinOrSplitIndex == (nActiveSupernodes - 1))
        upOrDownDegreePortal.Set(whichJoinOrSplit, joinOrSplitIndex);
      else if (whichJoinOrSplit != inNeighbourPortal.Get(joinOrSplitIndex + 1))
        upOrDownDegreePortal.Set(whichJoinOrSplit, joinOrSplitIndex);
    } // an actual neighbour

    // In serial this worklet implements the following operation
    /*
      for (viskores::Id joinIndex = 0; joinIndex < nActiveSupernodes; joinIndex++)
        { // per in edge index
          viskores::Id whichJoin = inNeighbour[joinIndex];
          if (!NoSuchElement(whichJoin))
                  { // an actual neighbour
                  // RHE always computes - this may be redundant, since NO_SUCH_ELEMENT should sort high
                  if (joinIndex == (nActiveSupernodes-1))
                          updegree[whichJoin] = joinIndex;
                  else if (whichJoin != inNeighbour[joinIndex+1])
                          updegree[whichJoin] = joinIndex;
                  } // an actual neighbour
        } // per in edge index
      */
  }

}; // FindDegrees_FindRHE

} // namespace contourtree_maker_inc
} // namespace contourtree_augmented
} // namespace worklet
} // namespace viskores

#endif
