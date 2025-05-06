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

#ifndef viskores_filter_scalar_topology_worklet_branch_decomposition_hierarchical_volumetric_branch_decomposer_LocalBestUpDownByVolumeWorklet_h
#define viskores_filter_scalar_topology_worklet_branch_decomposition_hierarchical_volumetric_branch_decomposer_LocalBestUpDownByVolumeWorklet_h

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

/// Worklet used in HierarchicalAugmenter::CopyBaseRegularStructure for
/// finding the superparent for each node needed

/// Template parameter is bool indicating whether we are processing up- or
/// down best volumes and corresponding whether we need to use the low or
/// high end of the edge. Note: We make this a template paramter so that
/// the corresponding if statement can already be optimozed away during
/// compile time.
template <bool IsDown>
class LocalBestUpDownByVolumeWorklet : public viskores::worklet::WorkletMapField
{
public:
  /// Control signature for the worklet
  using ControlSignature = void(
    WholeArrayIn actualSuperacrs,
    WholeArrayIn superarcList, // superarc list
    FieldIn
      permutedUpDownVolume, // upVolumne if IsDown==True, or downVolume if IsDown==False. These are swapped as IsDown refers to the output arrays
    WholeArrayIn hierarchicalTreeRegularNodeGlobalIds,
    WholeArrayIn hierarchicalTreeSupernodes,
    WholeArrayInOut
      bestUpDownSupernode, // bestUpSupernode if IsDown==False, or bestDownSupernode if IsDown==True
    WholeArrayInOut
      bestUpDownVolume // bestUpVolume if IsDown==False, or bestDownVolume if IsDown==True
  );
  // TODO: Check if we need WholeArrayInOut here for the output arrays or if WholeArrayOut is sufficient
  using ExecutionSignature = void(InputIndex, _1, _2, _3, _4, _5, _6, _7);
  using InputDomain = _1;

  /// Default Constructor
  VISKORES_EXEC_CONT
  LocalBestUpDownByVolumeWorklet(const viskores::Id numActualSuperarcs)
    : NumberActualSuperarcs(numActualSuperarcs)
  {
  }

  /// operator() of the workelt
  template <typename InFieldPortalType1,
            typename InFieldPortalType2,
            typename InFieldPortalType3,
            typename InFieldPortalType4,
            typename OutFieldPortalType1,
            typename OutFieldPortalType2>
  VISKORES_EXEC void operator()(
    const viskores::Id& actualSuperarcIndex,
    const InFieldPortalType1& actualSuperarcsPortal,
    const InFieldPortalType2& superarcListPortal,
    const viskores::Id& upDownVolumeValue, // upDownVolume[superarcID]
    const InFieldPortalType3& hierarchicalTreeRegularNodeGlobalIdsPortal,
    const InFieldPortalType4& hierarchicalTreeSupernodesPortal,
    const OutFieldPortalType1& bestUpDownSupernodePortal,
    const OutFieldPortalType2& bestUpDownVolumePortal) const
  {
    // per actual superarc
    viskores::Id superarcId = actualSuperarcsPortal.Get(actualSuperarcIndex);
    const viskores::worklet::contourtree_augmented::EdgePair& edge =
      superarcListPortal.Get(superarcId);

    if (IsDown)
    {
      // if it's the last one
      if (actualSuperarcIndex == NumberActualSuperarcs - 1)
      { // last in array
        bestUpDownSupernodePortal.Set(edge.second,
                                      hierarchicalTreeRegularNodeGlobalIdsPortal.Get(
                                        hierarchicalTreeSupernodesPortal.Get(edge.first)));
        bestUpDownVolumePortal.Set(edge.second, upDownVolumeValue);
      } // last in array
      else
      { // not the last one
        const viskores::worklet::contourtree_augmented::EdgePair& nextEdge =
          superarcListPortal.Get(actualSuperarcsPortal.Get(actualSuperarcIndex + 1));
        // if the next edge belongs to another, we're the highest
        if (nextEdge.second != edge.second)
        { // last in group
          bestUpDownSupernodePortal.Set(edge.second,
                                        hierarchicalTreeRegularNodeGlobalIdsPortal.Get(
                                          hierarchicalTreeSupernodesPortal.Get(edge.first)));
          bestUpDownVolumePortal.Set(edge.second, upDownVolumeValue);
        } // last in group
      }   // not the last one
    }     // if(this->IsDown)
    else // Processing the Up arrays. This is essentiall the same, but we need to use the lower end of the edge instead
    {
      // if it's the last one
      if (actualSuperarcIndex == NumberActualSuperarcs - 1)
      { // last in array
        bestUpDownSupernodePortal.Set(edge.first,
                                      hierarchicalTreeRegularNodeGlobalIdsPortal.Get(
                                        hierarchicalTreeSupernodesPortal.Get(edge.second)));
        bestUpDownVolumePortal.Set(edge.first, upDownVolumeValue);
      } // last in array
      else
      { // not the last one
        const viskores::worklet::contourtree_augmented::EdgePair& nextEdge =
          superarcListPortal.Get(actualSuperarcsPortal.Get(actualSuperarcIndex + 1));
        // if the next edge belongs to another, we're the highest
        if (nextEdge.first != edge.first)
        { // best in group
          bestUpDownSupernodePortal.Set(edge.first,
                                        hierarchicalTreeRegularNodeGlobalIdsPortal.Get(
                                          hierarchicalTreeSupernodesPortal.Get(edge.second)));
          bestUpDownVolumePortal.Set(edge.first, upDownVolumeValue);
        } // best in group
      }   // not the last one
    }     // else (if(this->isDown))
    /*
      // This worklet implements the following loop. Depending on whether we are working with the Up- or DownVolumes
      // This function implements one of the following logic
      // II B 2.  Per vertex, best superarc writes to the best downward array
            for (viskores::Id actualSuperarc = 0; actualSuperarc < nActualSuperarcs; actualSuperarc++)
                        { // per actual superarc
          viskores::Id superarcID = actualSuperarcs[actualSuperarc];
          Edge &edge = superarcList[superarcID];
          // if it's the last one
          if (actualSuperarc == nActualSuperarcs-1)
            { // last in array
            bestDownSupernode[edge.high] = hierarchicalTree.regularNodeGlobalIDs[hierarchicalTree.supernodes[edge.low]];
            bestDownVolume[edge.high] = upVolume[superarcID];
            } // last in array
          else
            { // not the last one
            Edge &nextEdge = superarcList[actualSuperarcs[actualSuperarc+1]];
            // if the next edge belongs to another, we're the highest
            if (nextEdge.high != edge.high)
              { // last in group
              bestDownSupernode[edge.high] = hierarchicalTree.regularNodeGlobalIDs[hierarchicalTree.supernodes[edge.low]];
              bestDownVolume[edge.high] = upVolume[superarcID];
              } // last in group
            } // not the last one
        } // per actual superarc
     */
    /* // Or in the Up case we have. THe main difference is the we either use the Up or Down Volume and
        // and we following here the low end of the edge and top end of the edge in the other case

    for (viskores::Id actualSuperarc = 0; actualSuperarc < nActualSuperarcs; actualSuperarc++)
                        { // per actual superarc
                        viskores::Id superarcID = actualSuperarcs[actualSuperarc];
                        Edge &edge = superarcList[superarcID];
                        // if it's the last one
                        if (actualSuperarc == nActualSuperarcs-1)
                                { // last in array
                                bestUpSupernode[edge.low] = hierarchicalTree.regularNodeGlobalIDs[hierarchicalTree.supernodes[edge.high]];
                                bestUpVolume[edge.low] = downVolume[superarcID];
                                } // last in array
                        else
                                { // not the last one
                                Edge &nextEdge = superarcList[actualSuperarcs[actualSuperarc+1]];
                                // if the next edge belongs to another, we're the highest
                                if (nextEdge.low != edge.low)
                                        { // best in group
                                        bestUpSupernode[edge.low] = hierarchicalTree.regularNodeGlobalIDs[hierarchicalTree.supernodes[edge.high]];
                                        bestUpVolume[edge.low] = downVolume[superarcID];
                                        } // best in group
                                } // not the last one

                        } // per actual superarc
    */
  } // operator()()

private:
  viskores::Id NumberActualSuperarcs;

}; // LocalBestUpDownByVolumeWorklet

} // namespace hierarchical_volumetric_branch_decomposer
} // namespace scalar_topology
} // namespace worklet
} // namespace viskores

#endif
