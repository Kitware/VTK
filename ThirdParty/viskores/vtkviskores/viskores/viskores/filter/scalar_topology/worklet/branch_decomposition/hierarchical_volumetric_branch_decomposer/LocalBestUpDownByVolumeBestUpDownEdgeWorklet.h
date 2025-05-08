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

#ifndef viskores_filter_scalar_topology_worklet_branch_decomposition_hierarchical_volumetric_branch_decomposer_LocalBestUpDownByVolumeBestUpDownEdgeWorklet_h
#define viskores_filter_scalar_topology_worklet_branch_decomposition_hierarchical_volumetric_branch_decomposer_LocalBestUpDownByVolumeBestUpDownEdgeWorklet_h

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
class LocalBestUpDownByVolumeBestUpDownEdgeWorklet : public viskores::worklet::WorkletMapField
{
public:
  /// Control signature for the worklet
  /// NOTE: We require the input arrays (aside form the input domain) to be permutted by the
  ///       regularNodesNeeded input domain so that we can use FieldIn instead of WholeArrayIn
  /// NOTE: We require ArrayHandleView for the output arrays of the range [numExistingRegular:end] so
  ///       that we can use FieldOut instead of requiring WholeArrayInOut
  using ControlSignature = void(
    FieldIn
      permutedHierarchicalTreeSuperarcs, // hierarchicalTree.Superarcs permuted by actualSuperarcs
    FieldIn permutedDependetValues,      // dependentValues permuted by actualSuperarcs
    FieldIn permutedIntrinsicValues,     // intrinsicValues permuted by actualSuperarcs
    FieldOut permutedUpVolume,           // upVolume permuted by actualSuperarcs
    FieldOut permitedDownVolume          // downVolume permited by actualSuperarcs
  );
  using ExecutionSignature = void(_1, _2, _3, _4, _5);
  using InputDomain = _1;

  /// Default Constructor
  VISKORES_EXEC_CONT
  LocalBestUpDownByVolumeBestUpDownEdgeWorklet(const viskores::Id totalVolume)
    : TotalVolume(totalVolume)
  {
  }

  /// operator() of the workelt
  template <typename FieldType>
  VISKORES_EXEC void operator()(
    const viskores::Id&
      hierarchicalTreeSuperarc,      // hierarchicalTree.superarcs[actualSuperarcs[InputIndex]]
    const FieldType& dependentValue, // dependentValues[actualSuperarcs[InputIndex]]
    const FieldType& intrinsicValue, // intrinsicValues[actualSuperarcs[InputIndex]]
    viskores::Id& upVolume,          // upVolume[actualSuperarcs[InputIndex]]
    viskores::Id& downVolume         // downVolume[actualSuperarcs[InputIndex]]
  ) const
  {
    // per actual superarc
    // retrieve the superarc Id (down via the superarcId FieldIn parameter)
    if (viskores::worklet::contourtree_augmented::IsAscending(hierarchicalTreeSuperarc))
    { // ascending superarc
      upVolume = dependentValue;
      // at the inner end, dependent volume is the total in the subtree.  Then there are vertices along the edge
      // itself (intrinsic volume), including the supernode at the outer end
      // So, to get the "dependent" volume in the other direction, we start
      // with totalVolume - dependent, then subtract (intrinsic - 1)
      downVolume = (this->TotalVolume - dependentValue) + (intrinsicValue - 1);
    } // ascending superarc
    else
    { // descending superarc
      downVolume = dependentValue;
      // at the inner end, dependent volume is the total in the subtree.  Then there are vertices along the edge
      // itself (intrinsic volume), including the supernode at the outer end
      // So, to get the "dependent" volume in the other direction, we start
      // with totalVolume - dependent, then subtract (intrinsic - 1)
      upVolume = (this->TotalVolume - dependentValue) + (intrinsicValue - 1);
    } // descending superarc

    /* // This worklet implements the follwing loop
         for (viskores::Id actualSuperarc = 0; actualSuperarc < nActualSuperarcs; actualSuperarc++)
      { // per actual superarc
        // retrieve the superarc ID
        viskores::Id superarcID = actualSuperarcs[actualSuperarc];
        if (isAscending(hierarchicalTree.superarcs[superarcID]))
          { // ascending superarc
          upVolume[superarcID] = dependentValues[superarcID];
          // at the inner end, dependent volume is the total in the subtree.  Then there are vertices along the edge itself (intrinsic volume), including the supernode at the outer end
          // So, to get the "dependent" volume in the other direction, we start with totalVolume - dependent, then subtract (intrinsic - 1)
          downVolume[superarcID] = (totalVolume - dependentValues[superarcID]) + (intrinsicValues[superarcID] - 1);
          } // ascending superarc
        else
          { // descending superarc
          downVolume[superarcID] = dependentValues[superarcID];
          // at the inner end, dependent volume is the total in the subtree.  Then there are vertices along the edge itself (intrinsic volume), including the supernode at the outer end
          // So, to get the "dependent" volume in the other direction, we start with totalVolume - dependent, then subtract (intrinsic - 1)
          upVolume[superarcID] = (totalVolume - dependentValues[superarcID]) + (intrinsicValues[superarcID] - 1);
          } // descending superarc
      } // per superarc
     */
  } // operator()()

private:
  viskores::Id TotalVolume;

}; // LocalBestUpDownByVolumeBestUpDownEdgeWorklet

} // namespace hierarchical_volumetric_branch_decomposer
} // namespace scalar_topology
} // namespace worklet
} // namespace viskores

#endif
