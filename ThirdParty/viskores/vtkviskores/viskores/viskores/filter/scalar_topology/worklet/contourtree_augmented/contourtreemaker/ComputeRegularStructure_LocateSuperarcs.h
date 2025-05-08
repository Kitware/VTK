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

#ifndef viskores_worklet_contourtree_augmented_contourtree_maker_inc_compute_regular_structure_locate_superarcs_h
#define viskores_worklet_contourtree_augmented_contourtree_maker_inc_compute_regular_structure_locate_superarcs_h

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

// Worklet for the second step of ContourTreeMaker::ComputeRegularStructure ---
// for all remaining (regular) nodes, locate the superarc to which they belong
class ComputeRegularStructure_LocateSuperarcs : public viskores::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(WholeArrayInOut contourTreeSuperparents, // (input/output)
                                WholeArrayIn contourTreeWhenTransferred, // (input)
                                WholeArrayIn contourTreeHyperparents,    // (input)
                                WholeArrayIn contourTreeHyperarcs,       // (input)
                                WholeArrayIn contourTreeHypernodes,      // (input)
                                WholeArrayIn contourTreeSupernodes,      // (input)
                                FieldIn meshExtremaPeaks,                // (input)
                                FieldIn meshExtremaPits);                // (input)

  typedef void ExecutionSignature(_1, InputIndex, _2, _3, _4, _5, _6, _7, _8);
  using InputDomain = _1;

  viskores::Id NumHypernodes; // contourTree.Hypernodes.GetNumberOfValues()
  viskores::Id NumSupernodes; // contourTree.Supernodes.GetNumberOfValues()

  // Default Constructor
  VISKORES_EXEC_CONT
  ComputeRegularStructure_LocateSuperarcs(viskores::Id numHypernodes, viskores::Id numSupernodes)
    : NumHypernodes(numHypernodes)
    , NumSupernodes(numSupernodes)
  {
  }

  template <typename InOutFieldPortalType, typename InFieldPortalType>
  VISKORES_EXEC void operator()(const InOutFieldPortalType& contourTreeSuperparentsPortal,
                                const viskores::Id node,
                                const InFieldPortalType& contourTreeWhenTransferredPortal,
                                const InFieldPortalType& contourTreeHyperparentsPortal,
                                const InFieldPortalType& contourTreeHyperarcsPortal,
                                const InFieldPortalType& contourTreeHypernodesPortal,
                                const InFieldPortalType& contourTreeSupernodesPortal,
                                viskores::Id top,
                                viskores::Id bottom) const
  {
    // per node
    // if the superparent is already set, it's a supernode, so skip it.
    if (NoSuchElement(contourTreeSuperparentsPortal.Get(node)))
    { // regular nodes only
      // we will need to prune top and bottom until one of them prunes past the node
      // these are the regular IDs of supernodes, so their superparents are already set
      viskores::Id topSuperparent = contourTreeSuperparentsPortal.Get(MaskedIndex(top));
      viskores::Id bottomSuperparent = contourTreeSuperparentsPortal.Get(MaskedIndex(bottom));
      // and we can also find out when they transferred
      viskores::Id topWhen = contourTreeWhenTransferredPortal.Get(topSuperparent);
      viskores::Id bottomWhen = contourTreeWhenTransferredPortal.Get(bottomSuperparent);
      // and their hyperparent
      viskores::Id topHyperparent = contourTreeHyperparentsPortal.Get(topSuperparent);
      viskores::Id bottomHyperparent = contourTreeHyperparentsPortal.Get(bottomSuperparent);
      // our goal is to work out the true hyperparent of the node
      viskores::Id hyperparent = (viskores::Id)NO_SUCH_ELEMENT;

      // now we loop until one of them goes past the vertex
      // the invariant here is that the first direction to prune past the vertex prunes it
      while (NoSuchElement(hyperparent))
      { // loop to find pruner
        // we test the one that prunes first
        if (MaskedIndex(topWhen) < MaskedIndex(bottomWhen))
        { // top pruned first
          // we prune down to the bottom of the hyperarc in either case, by updating the top superparent
          topSuperparent = contourTreeHyperarcsPortal.Get(MaskedIndex(topHyperparent));
          top = contourTreeSupernodesPortal.Get(MaskedIndex(topSuperparent));

          topWhen = contourTreeWhenTransferredPortal.Get(MaskedIndex(topSuperparent));
          // test to see if we've passed the node
          if (top < node)
          { // just pruned past
            hyperparent = topHyperparent;
          } // just pruned past
          // == is not possible, since node is regular
          else // top < node
          {    // not pruned past
            topHyperparent = contourTreeHyperparentsPortal.Get(MaskedIndex(topSuperparent));
          } // not pruned past
        }   // top pruned first
        else if (MaskedIndex(topWhen) > MaskedIndex(bottomWhen))
        { // bottom pruned first
          // we prune up to the top of the hyperarc in either case, by updating the bottom superparent
          bottomSuperparent = contourTreeHyperarcsPortal.Get(MaskedIndex(bottomHyperparent));
          bottom = contourTreeSupernodesPortal.Get(MaskedIndex(bottomSuperparent));
          bottomWhen = contourTreeWhenTransferredPortal.Get(MaskedIndex(bottomSuperparent));
          // test to see if we've passed the node
          if (bottom > node)
          { // just pruned past
            hyperparent = bottomHyperparent;
          } // just pruned past
          // == is not possible, since node is regular
          else // bottom > node
          {    // not pruned past
            bottomHyperparent = contourTreeHyperparentsPortal.Get(MaskedIndex(bottomSuperparent));
          } // not pruned past
        }   // bottom pruned first
        else
        { // both prune simultaneously
          // this can happen when both top & bottom prune in the same pass because they belong to the same hyperarc
          // but this means that they must have the same hyperparent, so we know the correct hyperparent & can check whether it ascends
          hyperparent = bottomHyperparent;
        } // both prune simultaneously
      }   // loop to find pruner
      // we have now set the hyperparent correctly, so we retrieve it's hyperarc to find whether it ascends or descends
      if (IsAscending(contourTreeHyperarcsPortal.Get(hyperparent)))
      { // ascending hyperarc
        // the supernodes on the hyperarc are in sorted low-high order
        viskores::Id lowSupernode = contourTreeHypernodesPortal.Get(hyperparent);
        viskores::Id highSupernode;
        // if it's at the right hand end, take the last supernode in the array
        if (MaskedIndex(hyperparent) == NumHypernodes - 1)
          highSupernode = NumSupernodes - 1;
        // otherwise, take the supernode just before the next hypernode
        else
          highSupernode = contourTreeHypernodesPortal.Get(MaskedIndex(hyperparent) + 1) - 1;
        // now, the high supernode may be lower than the element, because the node belongs
        // between it and the high end of the hyperarc
        if (contourTreeSupernodesPortal.Get(highSupernode) < node)
          contourTreeSuperparentsPortal.Set(node, highSupernode);
        // otherwise, we do a binary search of the superarcs
        else
        { // node between high & low
          // keep going until we span exactly
          while (highSupernode - lowSupernode > 1)
          { // binary search
            // find the midway supernode
            viskores::Id midSupernode = (lowSupernode + highSupernode) / 2;
            // test against the node
            if (contourTreeSupernodesPortal.Get(midSupernode) > node)
              highSupernode = midSupernode;
            // == can't happen since node is regular
            else
              lowSupernode = midSupernode;
          } // binary search

          // now we can use the low node as the superparent
          contourTreeSuperparentsPortal.Set(node, lowSupernode);
        } // node between high & low
      }   // ascending hyperarc
      else
      { // descending hyperarc
        // the supernodes on the hyperarc are in sorted high-low order
        viskores::Id highSupernode = contourTreeHypernodesPortal.Get(hyperparent);
        viskores::Id lowSupernode;
        // if it's at the right hand end, take the last supernode in the array
        if (MaskedIndex(hyperparent) == NumHypernodes - 1)
        { // last hyperarc
          lowSupernode = NumSupernodes - 1;
        } // last hyperarc
        // otherwise, take the supernode just before the next hypernode
        else
        { // other hyperarc
          lowSupernode = contourTreeHypernodesPortal.Get(MaskedIndex(hyperparent) + 1) - 1;
        } // other hyperarc
        // now, the low supernode may be higher than the element, because the node belongs
        // between it and the low end of the hyperarc
        if (contourTreeSupernodesPortal.Get(lowSupernode) > node)
          contourTreeSuperparentsPortal.Set(node, lowSupernode);
        // otherwise, we do a binary search of the superarcs
        else
        { // node between low & high
          // keep going until we span exactly
          while (lowSupernode - highSupernode > 1)
          { // binary search
            // find the midway supernode
            viskores::Id midSupernode = (highSupernode + lowSupernode) / 2;
            // test against the node
            if (contourTreeSupernodesPortal.Get(midSupernode) > node)
              highSupernode = midSupernode;
            // == can't happen since node is regular
            else
              lowSupernode = midSupernode;
          } // binary search
          // now we can use the high node as the superparent
          contourTreeSuperparentsPortal.Set(node, highSupernode);
        } // node between low & high
      }   // descending hyperarc
    }     // regular nodes only
    /*
    // In serial this worklet implements the following operation
    for (indexType node = 0; node < contourTree.Arcs.size(); node++)
    { // per node
        // if the superparent is already set, it's a supernode, so skip it.
        if (NoSuchElement(contourTree.superparents[node]))
        { // regular nodes only
            // we will need to prune top and bottom until one of them prunes past the node
            indexType top = meshExtrema.Peaks[node];
            indexType bottom = meshExtrema.Pits[node];
            // these are the regular IDs of supernodes, so their superparents are already set
            indexType topSuperparent = contourTree.Superparents[top];
            indexType bottomSuperparent = contourTree.Superparents[bottom];
            // and we can also find out when they transferred
            indexType topWhen = contourTree.WhenTransferred[topSuperparent];
            indexType bottomWhen = contourTree.WhenTransferred[bottomSuperparent];
            // and their hyperparent
            indexType topHyperparent = contourTree.hyperparents[topSuperparent];
            indexType bottomHyperparent = contourTree.hyperparents[bottomSuperparent];

            // our goal is to work out the true hyperparent of the node
            indexType hyperparent = NO_SUCH_ELEMENT;

            // now we loop until one of them goes past the vertex
            // the invariant here is that the first direction to prune past the vertex prunes it
            while (NoSuchElement(hyperparent))
            { // loop to find pruner

                // we test the one that prunes first
                if (MaskedIndex(topWhen) < MaskedIndex(bottomWhen))
                { // top pruned first
                    // we prune down to the bottom of the hyperarc in either case, by updating the top superparent
                    topSuperparent = contourTree.hyperarcs[MaskedIndex(topHyperparent)];
                    top = contourTree.Supernodes[MaskedIndex(topSuperparent)];

                    topWhen = contourTree.WhenTransferred[MaskedIndex(topSuperparent)];
                    // test to see if we've passed the node
                    if (top < node)
                    { // just pruned past
                        hyperparent = topHyperparent;
                    } // just pruned past
                    // == is not possible, since node is regular
                    else // top < node
                    { // not pruned past
                        topHyperparent = contourTree.hyperparents[MaskedIndex(topSuperparent)];
                    } // not pruned past
                } // top pruned first
                else if (MaskedIndex(topWhen) > MaskedIndex(bottomWhen))
                { // bottom pruned first
                    // we prune up to the top of the hyperarc in either case, by updating the bottom superparent
                    bottomSuperparent = contourTree.hyperarcs[MaskedIndex(bottomHyperparent)];
                    bottom = contourTree.Supernodes[MaskedIndex(bottomSuperparent)];
                    bottomWhen = contourTree.WhenTransferred[MaskedIndex(bottomSuperparent)];
                    // test to see if we've passed the node
                    if (bottom > node)
                    { // just pruned past
                        hyperparent = bottomHyperparent;
                    } // just pruned past
                    // == is not possible, since node is regular
                    else // bottom > node
                    { // not pruned past
                        bottomHyperparent = contourTree.hyperparents[MaskedIndex(bottomSuperparent)];
                    } // not pruned past
                } // bottom pruned first
                else
                { // both prune simultaneously
                    // this can happen when both top & bottom prune in the same pass because they belong to the same hyperarc
                    // but this means that they must have the same hyperparent, so we know the correct hyperparent & can check whether it ascends
                    hyperparent = bottomHyperparent;
                } // both prune simultaneously
            } // loop to find pruner


            // we have now set the hyperparent correctly, so we retrieve it's hyperarc to find whether it ascends or descends
            if (IsAscending(contourTree.hyperarcs[hyperparent]))
            { // ascending hyperarc
                // the supernodes on the hyperarc are in sorted low-high order
                indexType lowSupernode = contourTree.Hypernodes[hyperparent];
                indexType highSupernode;
                // if it's at the right hand end, take the last supernode in the array
                if (MaskedIndex(hyperparent) == contourTree.Hypernodes.size() - 1)
                    highSupernode = contourTree.Supernodes.size() - 1;
                // otherwise, take the supernode just before the next hypernode
                else
                    highSupernode = contourTree.Hypernodes[MaskedIndex(hyperparent) + 1] - 1;
                // now, the high supernode may be lower than the element, because the node belongs
                // between it and the high end of the hyperarc
                if (contourTree.Supernodes[highSupernode] < node)
                    contourTree.Superparents[node] = highSupernode;
                // otherwise, we do a binary search of the superarcs
                else
                { // node between high & low
                    // keep going until we span exactly
                    while (highSupernode - lowSupernode > 1)
                    { // binary search
                        // find the midway supernode
                        indexType midSupernode = (lowSupernode + highSupernode) / 2;
                        // test against the node
                        if (contourTree.Supernodes[midSupernode] > node)
                            highSupernode = midSupernode;
                        // == can't happen since node is regular
                        else
                            lowSupernode = midSupernode;
                    } // binary search
                    // now we can use the low node as the superparent
                    contourTree.Superparents[node] = lowSupernode;
                } // node between high & low
            } // ascending hyperarc
            else
            { // descending hyperarc
                // the supernodes on the hyperarc are in sorted high-low order
                indexType highSupernode = contourTree.Hypernodes[hyperparent];
                indexType lowSupernode;
                // if it's at the right hand end, take the last supernode in the array
                if (MaskedIndex(hyperparent) == contourTree.Hypernodes.size() - 1)
                { // last hyperarc
                    lowSupernode = contourTree.Supernodes.size() - 1;
                } // last hyperarc
                // otherwise, take the supernode just before the next hypernode
                else
                { // other hyperarc
                    lowSupernode = contourTree.Hypernodes[MaskedIndex(hyperparent) + 1] - 1;
                } // other hyperarc
                // now, the low supernode may be higher than the element, because the node belongs
                // between it and the low end of the hyperarc
                if (contourTree.Supernodes[lowSupernode] > node)
                    contourTree.Superparents[node] = lowSupernode;
                // otherwise, we do a binary search of the superarcs
                else
                { // node between low & high
                    // keep going until we span exactly
                    while (lowSupernode - highSupernode > 1)
                    { // binary search
                        // find the midway supernode
                        indexType midSupernode = (highSupernode + lowSupernode) / 2;
                        // test against the node
                        if (contourTree.Supernodes[midSupernode] > node)
                            highSupernode = midSupernode;
                        // == can't happen since node is regular
                        else
                            lowSupernode = midSupernode;
                    } // binary search
                    // now we can use the high node as the superparent
                    contourTree.Superparents[node] = highSupernode;
                } // node between low & high
            } // descending hyperarc
        } // regular nodes only
    } // per node
    */
  }

}; // ComputeRegularStructure_LocateSuperarcs.h



// TODO This algorithm looks to be a 3d/2d volume algorithm that is iterating 'points' and concerned about being on the 'boundary'. This would be better suited as WorkletMapPointNeighborhood as it can provide the boundary condition logic for you.

// Worklet for the second step of template <class Mesh> void ContourTreeMaker::ComputeRegularStructure ---
// for all remaining (regular) nodes on the boundary, locate the superarc to which they belong
class ComputeRegularStructure_LocateSuperarcsOnBoundary : public viskores::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(WholeArrayInOut contourTreeSuperparents, // (input/output)
                                WholeArrayIn contourTreeWhenTransferred, // (input)
                                WholeArrayIn contourTreeHyperparents,    // (input)
                                WholeArrayIn contourTreeHyperarcs,       // (input)
                                WholeArrayIn contourTreeHypernodes,      // (input)
                                WholeArrayIn contourTreeSupernodes,      // (input)
                                FieldIn meshExtremaPeaks,                // (input)
                                FieldIn meshExtremaPits,                 // (input)
                                FieldIn sortOrder,                       // (input)
                                ExecObject meshBoundary);                // (input)

  typedef void ExecutionSignature(_1, InputIndex, _2, _3, _4, _5, _6, _7, _8, _9, _10);
  using InputDomain = _1;

  viskores::Id NumHypernodes; // contourTree.Hypernodes.GetNumberOfValues()
  viskores::Id NumSupernodes; // contourTree.Supernodes.GetNumberOfValues()

  // Default Constructor
  VISKORES_EXEC_CONT
  ComputeRegularStructure_LocateSuperarcsOnBoundary(viskores::Id numHypernodes,
                                                    viskores::Id numSupernodes)
    : NumHypernodes(numHypernodes)
    , NumSupernodes(numSupernodes)
  {
  }

  template <typename InOutFieldPortalType, typename InFieldPortalType, typename MeshBoundaryType>
  VISKORES_EXEC void operator()(const InOutFieldPortalType& contourTreeSuperparentsPortal,
                                const viskores::Id node,
                                const InFieldPortalType& contourTreeWhenTransferredPortal,
                                const InFieldPortalType& contourTreeHyperparentsPortal,
                                const InFieldPortalType& contourTreeHyperarcsPortal,
                                const InFieldPortalType& contourTreeHypernodesPortal,
                                const InFieldPortalType& contourTreeSupernodesPortal,
                                viskores::Id top,
                                viskores::Id bottom,
                                viskores::Id sortOrder,
                                const MeshBoundaryType& meshBoundary) const
  {
    // per node
    // if the superparent is already set, it's a supernode, so skip it.
    if (NoSuchElement(contourTreeSuperparentsPortal.Get(node)) &&
        meshBoundary.LiesOnBoundary(sortOrder))
    { // regular nodes only
      // we will need to prune top and bottom until one of them prunes past the node
      // these are the regular IDs of supernodes, so their superparents are already set
      viskores::Id topSuperparent = contourTreeSuperparentsPortal.Get(MaskedIndex(top));
      viskores::Id bottomSuperparent = contourTreeSuperparentsPortal.Get(MaskedIndex(bottom));
      // and we can also find out when they transferred
      viskores::Id topWhen = contourTreeWhenTransferredPortal.Get(topSuperparent);
      viskores::Id bottomWhen = contourTreeWhenTransferredPortal.Get(bottomSuperparent);
      // and their hyperparent
      viskores::Id topHyperparent = contourTreeHyperparentsPortal.Get(topSuperparent);
      viskores::Id bottomHyperparent = contourTreeHyperparentsPortal.Get(bottomSuperparent);
      // our goal is to work out the true hyperparent of the node
      viskores::Id hyperparent = (viskores::Id)NO_SUCH_ELEMENT;

      // now we loop until one of them goes past the vertex
      // the invariant here is that the first direction to prune past the vertex prunes it
      while (NoSuchElement(hyperparent))
      { // loop to find pruner
        // we test the one that prunes first
        if (MaskedIndex(topWhen) < MaskedIndex(bottomWhen))
        { // top pruned first
          // we prune down to the bottom of the hyperarc in either case, by updating the top superparent
          topSuperparent = contourTreeHyperarcsPortal.Get(MaskedIndex(topHyperparent));
          top = contourTreeSupernodesPortal.Get(MaskedIndex(topSuperparent));

          topWhen = contourTreeWhenTransferredPortal.Get(MaskedIndex(topSuperparent));
          // test to see if we've passed the node
          if (top < node)
          { // just pruned past
            hyperparent = topHyperparent;
          } // just pruned past
          // == is not possible, since node is regular
          else // top < node
          {    // not pruned past
            topHyperparent = contourTreeHyperparentsPortal.Get(MaskedIndex(topSuperparent));
          } // not pruned past
        }   // top pruned first
        else if (MaskedIndex(topWhen) > MaskedIndex(bottomWhen))
        { // bottom pruned first
          // we prune up to the top of the hyperarc in either case, by updating the bottom superparent
          bottomSuperparent = contourTreeHyperarcsPortal.Get(MaskedIndex(bottomHyperparent));
          bottom = contourTreeSupernodesPortal.Get(MaskedIndex(bottomSuperparent));
          bottomWhen = contourTreeWhenTransferredPortal.Get(MaskedIndex(bottomSuperparent));
          // test to see if we've passed the node
          if (bottom > node)
          { // just pruned past
            hyperparent = bottomHyperparent;
          } // just pruned past
          // == is not possible, since node is regular
          else // bottom > node
          {    // not pruned past
            bottomHyperparent = contourTreeHyperparentsPortal.Get(MaskedIndex(bottomSuperparent));
          } // not pruned past
        }   // bottom pruned first
        else
        { // both prune simultaneously
          // this can happen when both top & bottom prune in the same pass because they belong to the same hyperarc
          // but this means that they must have the same hyperparent, so we know the correct hyperparent & can check whether it ascends
          hyperparent = bottomHyperparent;
        } // both prune simultaneously
      }   // loop to find pruner
      // we have now set the hyperparent correctly, so we retrieve it's hyperarc to find whether it ascends or descends
      if (IsAscending(contourTreeHyperarcsPortal.Get(hyperparent)))
      { // ascending hyperarc
        // the supernodes on the hyperarc are in sorted low-high order
        viskores::Id lowSupernode = contourTreeHypernodesPortal.Get(hyperparent);
        viskores::Id highSupernode;
        // if it's at the right hand end, take the last supernode in the array
        if (MaskedIndex(hyperparent) == NumHypernodes - 1)
          highSupernode = NumSupernodes - 1;
        // otherwise, take the supernode just before the next hypernode
        else
          highSupernode = contourTreeHypernodesPortal.Get(MaskedIndex(hyperparent) + 1) - 1;
        // now, the high supernode may be lower than the element, because the node belongs
        // between it and the high end of the hyperarc
        if (contourTreeSupernodesPortal.Get(highSupernode) < node)
          contourTreeSuperparentsPortal.Set(node, highSupernode);
        // otherwise, we do a binary search of the superarcs
        else
        { // node between high & low
          // keep going until we span exactly
          while (highSupernode - lowSupernode > 1)
          { // binary search
            // find the midway supernode
            viskores::Id midSupernode = (lowSupernode + highSupernode) / 2;
            // test against the node
            if (contourTreeSupernodesPortal.Get(midSupernode) > node)
              highSupernode = midSupernode;
            // == can't happen since node is regular
            else
              lowSupernode = midSupernode;
          } // binary search

          // now we can use the low node as the superparent
          contourTreeSuperparentsPortal.Set(node, lowSupernode);
        } // node between high & low
      }   // ascending hyperarc
      else
      { // descending hyperarc
        // the supernodes on the hyperarc are in sorted high-low order
        viskores::Id highSupernode = contourTreeHypernodesPortal.Get(hyperparent);
        viskores::Id lowSupernode;
        // if it's at the right hand end, take the last supernode in the array
        if (MaskedIndex(hyperparent) == NumHypernodes - 1)
        { // last hyperarc
          lowSupernode = NumSupernodes - 1;
        } // last hyperarc
        // otherwise, take the supernode just before the next hypernode
        else
        { // other hyperarc
          lowSupernode = contourTreeHypernodesPortal.Get(MaskedIndex(hyperparent) + 1) - 1;
        } // other hyperarc
        // now, the low supernode may be higher than the element, because the node belongs
        // between it and the low end of the hyperarc
        if (contourTreeSupernodesPortal.Get(lowSupernode) > node)
          contourTreeSuperparentsPortal.Set(node, lowSupernode);
        // otherwise, we do a binary search of the superarcs
        else
        { // node between low & high
          // keep going until we span exactly
          while (lowSupernode - highSupernode > 1)
          { // binary search
            // find the midway supernode
            viskores::Id midSupernode = (highSupernode + lowSupernode) / 2;
            // test against the node
            if (contourTreeSupernodesPortal.Get(midSupernode) > node)
              highSupernode = midSupernode;
            // == can't happen since node is regular
            else
              lowSupernode = midSupernode;
          } // binary search
          // now we can use the high node as the superparent
          contourTreeSuperparentsPortal.Set(node, highSupernode);
        } // node between low & high
      }   // descending hyperarc
    }     // regular nodes only
    /*
    // In serial this worklet implements the following operation
    for (indexType node = 0; node < contourTree.Arcs.size(); node++)
    { // per node
        // if the superparent is already set, it's a supernode, so skip it.
        if (NoSuchElement(contourTree.Superparents[node]) && mesh.liesOnBoundary(node))
        { // regular nodes only
            // we will need to prune top and bottom until one of them prunes past the node
            indexType top = meshExtrema.Peaks[node];
            indexType bottom = meshExtrema.Pits[node];
            // these are the regular IDs of supernodes, so their superparents are already set
            indexType topSuperparent = contourTree.Superparents[top];
            indexType bottomSuperparent = contourTree.Superparents[bottom];
            // and we can also find out when they transferred
            indexType topWhen = contourTree.WhenTransferred[topSuperparent];
            indexType bottomWhen = contourTree.WhenTransferred[bottomSuperparent];
            // and their hyperparent
            indexType topHyperparent = contourTree.hyperparents[topSuperparent];
            indexType bottomHyperparent = contourTree.hyperparents[bottomSuperparent];

            // our goal is to work out the true hyperparent of the node
            indexType hyperparent = NO_SUCH_ELEMENT;

            // now we loop until one of them goes past the vertex
            // the invariant here is that the first direction to prune past the vertex prunes it
            while (NoSuchElement(hyperparent))
            { // loop to find pruner

                // we test the one that prunes first
                if (MaskedIndex(topWhen) < MaskedIndex(bottomWhen))
                { // top pruned first
                    // we prune down to the bottom of the hyperarc in either case, by updating the top superparent
                    topSuperparent = contourTree.hyperarcs[MaskedIndex(topHyperparent)];
                    top = contourTree.Supernodes[MaskedIndex(topSuperparent)];

                    topWhen = contourTree.WhenTransferred[MaskedIndex(topSuperparent)];
                    // test to see if we've passed the node
                    if (top < node)
                    { // just pruned past
                        hyperparent = topHyperparent;
                    } // just pruned past
                    // == is not possible, since node is regular
                    else // top < node
                    { // not pruned past
                        topHyperparent = contourTree.hyperparents[MaskedIndex(topSuperparent)];
                    } // not pruned past
                } // top pruned first
                else if (MaskedIndex(topWhen) > MaskedIndex(bottomWhen))
                { // bottom pruned first
                    // we prune up to the top of the hyperarc in either case, by updating the bottom superparent
                    bottomSuperparent = contourTree.hyperarcs[MaskedIndex(bottomHyperparent)];
                    bottom = contourTree.Supernodes[MaskedIndex(bottomSuperparent)];
                    bottomWhen = contourTree.WhenTransferred[MaskedIndex(bottomSuperparent)];
                    // test to see if we've passed the node
                    if (bottom > node)
                    { // just pruned past
                        hyperparent = bottomHyperparent;
                    } // just pruned past
                    // == is not possible, since node is regular
                    else // bottom > node
                    { // not pruned past
                        bottomHyperparent = contourTree.hyperparents[MaskedIndex(bottomSuperparent)];
                    } // not pruned past
                } // bottom pruned first
                else
                { // both prune simultaneously
                    // this can happen when both top & bottom prune in the same pass because they belong to the same hyperarc
                    // but this means that they must have the same hyperparent, so we know the correct hyperparent & can check whether it ascends
                    hyperparent = bottomHyperparent;
                } // both prune simultaneously
            } // loop to find pruner


            // we have now set the hyperparent correctly, so we retrieve it's hyperarc to find whether it ascends or descends
            if (IsAscending(contourTree.hyperarcs[hyperparent]))
            { // ascending hyperarc
                // the supernodes on the hyperarc are in sorted low-high order
                indexType lowSupernode = contourTree.Hypernodes[hyperparent];
                indexType highSupernode;
                // if it's at the right hand end, take the last supernode in the array
                if (MaskedIndex(hyperparent) == contourTree.Hypernodes.size() - 1)
                    highSupernode = contourTree.Supernodes.size() - 1;
                // otherwise, take the supernode just before the next hypernode
                else
                    highSupernode = contourTree.Hypernodes[MaskedIndex(hyperparent) + 1] - 1;
                // now, the high supernode may be lower than the element, because the node belongs
                // between it and the high end of the hyperarc
                if (contourTree.Supernodes[highSupernode] < node)
                    contourTree.Superparents[node] = highSupernode;
                // otherwise, we do a binary search of the superarcs
                else
                { // node between high & low
                    // keep going until we span exactly
                    while (highSupernode - lowSupernode > 1)
                    { // binary search
                        // find the midway supernode
                        indexType midSupernode = (lowSupernode + highSupernode) / 2;
                        // test against the node
                        if (contourTree.Supernodes[midSupernode] > node)
                            highSupernode = midSupernode;
                        // == can't happen since node is regular
                        else
                            lowSupernode = midSupernode;
                    } // binary search
                    // now we can use the low node as the superparent
                    contourTree.Superparents[node] = lowSupernode;
                } // node between high & low
            } // ascending hyperarc
            else
            { // descending hyperarc
                // the Supernodes on the hyperarc are in sorted high-low order
                indexType highSupernode = contourTree.Hypernodes[hyperparent];
                indexType lowSupernode;
                // if it's at the right hand end, take the last supernode in the array
                if (MaskedIndex(hyperparent) == contourTree.Hypernodes.size() - 1)
                { // last hyperarc
                    lowSupernode = contourTree.Supernodes.size() - 1;
                } // last hyperarc
                // otherwise, take the supernode just before the next hypernode
                else
                { // other hyperarc
                    lowSupernode = contourTree.Hypernodes[MaskedIndex(hyperparent) + 1] - 1;
                } // other hyperarc
                // now, the low supernode may be higher than the element, because the node belongs
                // between it and the low end of the hyperarc
                if (contourTree.Supernodes[lowSupernode] > node)
                    contourTree.Superparents[node] = lowSupernode;
                // otherwise, we do a binary search of the superarcs
                else
                { // node between low & high
                    // keep going until we span exactly
                    while (lowSupernode - highSupernode > 1)
                    { // binary search
                        // find the midway supernode
                        indexType midSupernode = (highSupernode + lowSupernode) / 2;
                        // test against the node
                        if (contourTree.Supernodes[midSupernode] > node)
                            highSupernode = midSupernode;
                        // == can't happen since node is regular
                        else
                            lowSupernode = midSupernode;
                    } // binary search
                    // now we can use the high node as the superparent
                    contourTree.Superparents[node] = highSupernode;
                } // node between low & high
            } // descending hyperarc
        } // regular nodes only
    } // per node
    */
  }

}; // ComputeRegularStructure_LocateSuperarcsOnBoundary.h

} // namespace contourtree_maker_inc
} // namespace contourtree_augmented
} // namespace worklet
} // namespace viskores

#endif
