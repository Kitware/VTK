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

#ifndef viskores_filter_scalar_topology_worklet_branch_decomposition_hierarchical_volumetric_branch_decomposer_CollapseBranchesWorklet_h
#define viskores_filter_scalar_topology_worklet_branch_decomposition_hierarchical_volumetric_branch_decomposer_CollapseBranchesWorklet_h

#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/Types.h>
#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace worklet
{
namespace scalar_topology
{
namespace hierarchical_volumetric_branch_decomposer
{

class CollapseBranchesWorklet : public viskores::worklet::WorkletMapField
{
public:
  /// Control signature for the worklet
  using ControlSignature = void(
    FieldIn bestUpSupernode,
    FieldIn bestDownSupernode,
    FieldIn superarcs,
    // Execution objects from the hierarchical tree to use the FindRegularByGlobal function
    ExecObject findRegularByGlobal,
    // Execution objects from the hierarchical tree to use the FindSuperArcBetweenNodes, function
    ExecObject findSuperArcBetweenNodes,
    WholeArrayIn hierarchicalTreeRegular2supernode,
    WholeArrayIn hierarchicalTreeWhichRound,
    WholeArrayInOut branchRoot);
  using ExecutionSignature = void(InputIndex, _1, _2, _3, _4, _5, _6, _7, _8);
  using InputDomain = _1;

  /// Default Constructor
  VISKORES_EXEC_CONT
  CollapseBranchesWorklet(viskores::Id numRounds)
    : NumRounds(numRounds)
  {
  }

  /// operator() of the workelt
  template <typename ExecObjectType1,
            typename ExecObjectType2,
            typename InFieldPortalType,
            typename InOutFieldPortalType>
  VISKORES_EXEC void operator()(
    const viskores::Id& supernode,              // iteration index
    const viskores::Id& bestUpSupernodeId,      // bestUpSupernode[supernode]
    const viskores::Id& bestDownSupernodeId,    // bestDownSupernode[supernode]
    const viskores::Id& superarcsId,            // hierarchicalTree.superarcs[supernode]
    const ExecObjectType1& findRegularByGlobal, // Execution object to call FindRegularByGlobal
    const ExecObjectType2&
      findSuperArcBetweenNodes, // Execution object to call FindSuperArcBetweenNodes
    const InFieldPortalType& hierarchicalTreeRegular2supernodePortal,
    const InFieldPortalType& hierarchicalTreeWhichRoundPortal,
    const InOutFieldPortalType& branchRootPortal) const
  {
    // per supernode
    //  For each supernode, convert the best up into a superarc ID
    //  Note that the superarc may not belong to this rank, and that the superarc might be oriented either direction
    //  So we search for the best up global ID in the hierarchical tree
    //          If it does not exist, then this superarc does not belong to the rank, and can be ignored
    //          If it does exist and is a downwards superarc, we now have the correct ID
    //          If it does exist and is an upwards superarc, then the current supernode must have an ascending arc to it, and we're done
    //  Also do the same for the best down, then for each supernode, point the higher numbered at the lower

    // ADDED 19/07/2023
    // If there are any attachment points left in the hierarchical tree, there is an extra edge case we need to deal with.
    // It occurs when a supernode is simultaneously the target of an ascending superarc and a descending one
    // What we do is to test for this here: if we are an attachment point, we omit connecting the best up and down
    // ADDED 19/07/2023
    // test for attachment points
    if ((hierarchicalTreeWhichRoundPortal.Get(supernode) != this->NumRounds) &&
        (viskores::worklet::contourtree_augmented::NoSuchElement(superarcsId)))
    {
      return;
    }

    // if there is no best up, we're at an upper leaf and will not connect up two superarcs anyway, so we can skip the supernode
    if (viskores::worklet::contourtree_augmented::NoSuchElement(bestUpSupernodeId))
    {
      return;
    }

    // Search for the regular ID of the best up supernode
    viskores::Id bestUpLocalRegularId = findRegularByGlobal.FindRegularByGlobal(bestUpSupernodeId);

    // test to see whether it exists in this rank's hierarchical tree.
    if (viskores::worklet::contourtree_augmented::NoSuchElement(bestUpLocalRegularId))
    {
      return;
    }

    // do the same for the best down
    // Search for the regular ID of the best down supernode
    viskores::Id bestDownLocalRegularId =
      findRegularByGlobal.FindRegularByGlobal(bestDownSupernodeId);

    // test to see whether it exists in this rank's hierarchical tree.
    if (viskores::worklet::contourtree_augmented::NoSuchElement(bestDownLocalRegularId))
    {
      return;
    }

    // Convert regular to super ID
    viskores::Id bestUpLocalSupernodeId =
      hierarchicalTreeRegular2supernodePortal.Get(bestUpLocalRegularId);
    viskores::Id bestDownLocalSupernodeId =
      hierarchicalTreeRegular2supernodePortal.Get(bestDownLocalRegularId);

    // local variable for the superarc IDs
    viskores::Id bestUpSuperarc =
      findSuperArcBetweenNodes.FindSuperArcBetweenNodes(bestUpLocalSupernodeId, supernode);
    viskores::Id bestDownSuperarc =
      findSuperArcBetweenNodes.FindSuperArcBetweenNodes(bestDownLocalSupernodeId, supernode);

    // right: we now know the local IDs of both.  Take the more junior and point it at the more senior - i.e. always orient inbound
    // at the root supernode, the virtual root superarc will not be used, so we compare round/iteration/ID of the two superarcs anyway
    // WARNING: it might appear that there is potential for loops in the algorithm &/or write collisions
    // but there isn't because our superarcs are *ALWAYS* oriented inwards, as long as the test is correct ;->

    // so to find seniority, &c., we retrieve round number.
    // we don't need the iteration number, because a higher iteration (more senior) always has a higher ID for the same round
    viskores::Id bestUpRound = hierarchicalTreeWhichRoundPortal.Get(bestUpSuperarc);
    viskores::Id bestDownRound = hierarchicalTreeWhichRoundPortal.Get(bestDownSuperarc);

    // more senior rounds are higher numbered
    if ((bestUpRound > bestDownRound) ||
        // within each round, more senior iterations are higher numbered
        ((bestUpRound == bestDownRound) && (bestUpSuperarc > bestDownSuperarc)))
    { // up is more senior
      branchRootPortal.Set(bestDownSuperarc, bestUpSuperarc);
    } // up is more senior
    else // all other cases, go the opposite way - NB: assumes we will never have the same superarc twice
    { // down is more senior
      branchRootPortal.Set(bestUpSuperarc, bestDownSuperarc);
    } // down is more senior

    /*
      // This worklet implements the following loop.
      for (viskores::Id supernode = 0; supernode < hierarchicalTree.supernodes.size(); supernode++)
                        { // per supernode
                        //      For each supernode, convert the best up into a superarc ID
                        //      Note that the superarc may not belong to this rank, and that the superarc might be oriented either direction
                        //      So we search for the best up global ID in the hierarchical tree
                        //              If it does not exist, then this superarc does not belong to the rank, and can be ignored
                        //              If it does exist and is a downwards superarc, we now have the correct ID
                        //              If it does exist and is an upwards superarc, then the current supernode must have an ascending arc to it, and we're done
                        //      Also do the same for the best down, then for each supernode, point the higher numbered at the lower

                        viskores::Id bestUpSupernodeID = bestUpSupernode[supernode];

                        // if there is no best up, we're at an upper leaf and will not connect up two superarcs anyway, so we can skip the supernode
                        if (noSuchElement(bestUpSupernodeID))
                                continue;

                        // Search for the regular ID of the best up supernode
                        viskores::Id bestUpLocalRegularID = hierarchicalTree.FindRegularByGlobal(bestUpSupernodeID);

                        // test to see whether it exists in this rank's hierarchical tree.
                        if (noSuchElement(bestUpLocalRegularID))
                                continue;

                        // do the same for the best down
                        viskores::Id bestDownSupernodeID = bestDownSupernode[supernode];

                        // Search for the regular ID of the best down supernode
                        viskores::Id bestDownLocalRegularID = hierarchicalTree.FindRegularByGlobal(bestDownSupernodeID);

                        // test to see whether it exists in this rank's hierarchical tree.
                        if (noSuchElement(bestDownLocalRegularID))
                                continue;

                        // Convert regular to super ID
                        viskores::Id bestUpLocalSupernodeID = hierarchicalTree.regular2supernode[bestUpLocalRegularID];

                        viskores::Id bestDownLocalSupernodeID = hierarchicalTree.regular2supernode[bestDownLocalRegularID];

                        // local variable for the superarc IDs
                        viskores::Id bestUpSuperarc = hierarchicalTree.FindSuperArcBetweenNodes(bestUpLocalSupernodeID, supernode);

                        viskores::Id bestDownSuperarc = hierarchicalTree.FindSuperArcBetweenNodes(bestDownLocalSupernodeID, supernode);

                        // right: we now know the local IDs of both.  Take the more junior and point it at the more senior - i.e. always orient inbound
                        // at the root supernode, the virtual root superarc will not be used, so we compare round/iteration/ID of the two superarcs anyway
                        // WARNING: it might appear that there is potential for loops in the algorithm &/or write collisions
                        // but there isn't because our superarcs are *ALWAYS* oriented inwards, as long as the test is correct ;->

                        // so to find seniority, &c., we retrieve round number.
                        // we don't need the iteration number, because a higher iteration (more senior) always has a higher ID for the same round
                        viskores::Id bestUpRound = hierarchicalTree.whichRound[bestUpSuperarc];
                        viskores::Id bestDownRound = hierarchicalTree.whichRound[bestDownSuperarc];

                        // more senior rounds are higher numbered
                        if      (       (bestUpRound > bestDownRound)   ||
                                        // within each round, more senior iterations are higher numbered
                                        (       (bestUpRound == bestDownRound) && (bestUpSuperarc > bestDownSuperarc)   ))
                                                { // up is more senior
                                                branchRoot[bestDownSuperarc] = bestUpSuperarc;
                                                } // up is more senior
                        else // all other cases, go the opposite way - NB: assumes we will never have the same superarc twice
                                                { // down is more senior
                                                branchRoot[bestUpSuperarc] = bestDownSuperarc;
                                                } // down is more senior
                        } // per supernode
    */
  } // operator()()

private:
  viskores::Id NumRounds;

}; // CollapseBranchesWorklet

} // namespace hierarchical_volumetric_branch_decomposer
} // namespace scalar_topology
} // namespace worklet
} // namespace viskores

#endif
