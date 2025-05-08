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

#ifndef viskores_worklet_contourtree_augmented_contourtree_maker_inc_compute_regular_structure_set_arcs_h
#define viskores_worklet_contourtree_augmented_contourtree_maker_inc_compute_regular_structure_set_arcs_h

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

// Worklet for setting the arcs of the contour tree based on the sorted arcs
class ComputeRegularStructure_SetArcs : public viskores::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(WholeArrayIn arcSorter,               // (input) arcSorter array
                                WholeArrayIn contourTreeSuperparents, // (input)
                                WholeArrayIn contourTreeSuperarcs,    // (input)
                                WholeArrayIn contourTreeSupernodes,   // (input)
                                WholeArrayOut contourTreeArcs);       // (output)
  typedef void ExecutionSignature(_1, InputIndex, _2, _3, _4, _5);
  using InputDomain = _1;

  viskores::Id NumArcs; // contourTree.Arcs.GetNumberOfValues()

  // Default Constructor
  VISKORES_EXEC_CONT
  ComputeRegularStructure_SetArcs(viskores::Id numArcs)
    : NumArcs(numArcs)
  {
  }

  template <typename InFieldPortalType, typename OutFieldPortalType>
  VISKORES_EXEC void operator()(const InFieldPortalType& arcSorterPortal,
                                const viskores::Id sortedNode,
                                const InFieldPortalType& contourTreeSuperparentsPortal,
                                const InFieldPortalType& contourTreeSuperarcsPortal,
                                const InFieldPortalType& contourTreeSupernodesPortal,
                                const OutFieldPortalType& contourTreeArcsPortal) const
  {
    // per node
    // convert arcSorter to node ID
    viskores::Id nodeID = arcSorterPortal.Get(sortedNode);
    viskores::Id superparent = contourTreeSuperparentsPortal.Get(nodeID);

    // the end element is always the last
    bool isLastOnSuperarc = false;
    if (sortedNode == NumArcs - 1)
    {
      isLastOnSuperarc = true;
    }
    // otherwise look for a change in the superparent
    else
    {
      isLastOnSuperarc =
        (superparent != contourTreeSuperparentsPortal.Get(arcSorterPortal.Get(sortedNode + 1)));
    }

    // if it's the last on the superarc
    if (isLastOnSuperarc)
    { // last on superarc
      // retrieve the superarc's far end
      viskores::Id superarcEnd = contourTreeSuperarcsPortal.Get(superparent);
      // this only happens for the root of the tree, but is still needed
      if (NoSuchElement(superarcEnd))
      {
        contourTreeArcsPortal.Set(nodeID, (viskores::Id)NO_SUCH_ELEMENT);
      }
      else
      {
        contourTreeArcsPortal.Set(nodeID,
                                  contourTreeSupernodesPortal.Get(MaskedIndex(superarcEnd)) |
                                    (superarcEnd & IS_ASCENDING));
      }
    } // last on superarc
    else
    { // not last on superarc
      viskores::Id neighbour = arcSorterPortal.Get(sortedNode + 1);
      contourTreeArcsPortal.Set(nodeID, neighbour | ((neighbour > nodeID) ? IS_ASCENDING : 0));
    } // not last on superarc


    // In serial this worklet implements the following operation
    /*
      for (viskores::Id sortedNode = 0; sortedNode < contourTree.Arcs.size(); sortedNode++)
        { // per node
          // convert arcSorter to node ID
          viskores::Id nodeID = arcSorter[sortedNode];
          viskores::Id superparent = contourTree.Superparents[nodeID];

          // the end element is always the last
          bool isLastOnSuperarc = false;
          if (sortedNode == contourTree.Arcs.size()-1)
             isLastOnSuperarc = true;
          // otherwise look for a change in the superparent
          else
             isLastOnSuperarc = (superparent != contourTree.Superparents[arcSorter[sortedNode + 1]]);

          // if it's the last on the superarc
          if (isLastOnSuperarc)
            { // last on superarc
              // retrieve the superarc's far end
              viskores::Id superarcEnd = contourTree.superarcs[superparent];
              // this only happens for the root of the tree, but is still needed
              if (NoSuchElement(superarcEnd))
                contourTree.Arcs[nodeID] = NO_SUCH_ELEMENT;
              else
                contourTree.Arcs[nodeID] = contourTree.Supernodes[MaskedIndex(superarcEnd)] | (superarcEnd & IS_ASCENDING);
            } // last on superarc
          else
            { // not last on superarc
              viskores::Id neighbour = arcSorter[sortedNode+1];
              contourTree.Arcs[nodeID] = neighbour | ((neighbour > nodeID) ? IS_ASCENDING : 0);
            } // not last on superarc

        } // per node

      */
  }

}; // ComputeRegularStructure_SetArcs

// Worklet for setting the arcs of the contour tree based on the sorted augmented nodes
class ComputeRegularStructure_SetAugmentArcs : public viskores::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(WholeArrayIn arcSorter,               // (input) arcSorter array
                                WholeArrayIn contourTreeSuperparents, // (input)
                                WholeArrayIn contourTreeSuperarcs,    // (input)
                                WholeArrayIn contourTreeSupernodes,   // (input)
                                WholeArrayIn toCompressed,            // (input)
                                WholeArrayOut contourTreeArcs);       // (output)
  typedef void ExecutionSignature(_1, InputIndex, _2, _3, _4, _5, _6);
  using InputDomain = _1;

  viskores::Id NumArcs; // contourTree.Arcs.GetNumberOfValues()

  // Default Constructor
  VISKORES_EXEC_CONT
  ComputeRegularStructure_SetAugmentArcs(viskores::Id numArcs)
    : NumArcs(numArcs)
  {
  }

  template <typename InFieldPortalType, typename OutFieldPortalType>
  VISKORES_EXEC void operator()(const InFieldPortalType& arcSorterPortal,
                                const viskores::Id sortedNode,
                                const InFieldPortalType& contourTreeSuperparentsPortal,
                                const InFieldPortalType& contourTreeSuperarcsPortal,
                                const InFieldPortalType& contourTreeSupernodesPortal,
                                const InFieldPortalType& toCompressedPortal,
                                const OutFieldPortalType& contourTreeArcsPortal) const
  {
    // per node
    // convert arcSorter to node ID
    viskores::Id nodeID = arcSorterPortal.Get(sortedNode);
    viskores::Id superparent = contourTreeSuperparentsPortal.Get(nodeID);

    // the end element is always the last
    bool isLastOnSuperarc = false;
    if (sortedNode == NumArcs - 1)
    {
      isLastOnSuperarc = true;
    }
    // otherwise look for a change in the superparent
    else
    {
      isLastOnSuperarc =
        (superparent != contourTreeSuperparentsPortal.Get(arcSorterPortal.Get(sortedNode + 1)));
    }

    // if it's the last on the superarc
    if (isLastOnSuperarc)
    { // last on superarc
      // retrieve the superarc's far end
      viskores::Id superarcEnd = contourTreeSuperarcsPortal.Get(superparent);
      // this only happens for the root of the tree, but is still needed
      if (NoSuchElement(superarcEnd))
      {
        contourTreeArcsPortal.Set(nodeID, (viskores::Id)NO_SUCH_ELEMENT);
      }
      else
      {
        contourTreeArcsPortal.Set(
          nodeID,
          toCompressedPortal.Get(contourTreeSupernodesPortal.Get(MaskedIndex(superarcEnd))) |
            (superarcEnd & IS_ASCENDING));
      }
    } // last on superarc
    else
    { // not last on superarc
      viskores::Id neighbour = arcSorterPortal.Get(sortedNode + 1);
      contourTreeArcsPortal.Set(nodeID, neighbour | ((neighbour > nodeID) ? IS_ASCENDING : 0));
    } // not last on superarc
  }

}; // ComputeRegularStructure_SetAugmentArcs




} // namespace contourtree_maker_inc
} // namespace contourtree_augmented
} // namespace worklet
} // namespace viskores

#endif
