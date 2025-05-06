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

#ifndef viskores_worklet_contourtree_augmented_process_contourtree_inc_set_triangle_superarc_id_h
#define viskores_worklet_contourtree_augmented_process_contourtree_inc_set_triangle_superarc_id_h

#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/Types.h>
#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace worklet
{
namespace contourtree_augmented
{
namespace process_contourtree_inc
{

/*
 * This code is taken from viskores::worklet::contourtree_augmented::contourtree_maker_inc::ComputeRegularStructure_LocateSuperarcs
 * The only changes that were made are annotated with the @peter comment.
 * The difference with the original workelet is that this one only identifies the superacs using two endpoints and an isovalue,
 * that is it can work with regular values (not necessarily vertices).
 */
class SetTriangleSuperarcId : public viskores::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(
    // The endpoints are given in terms of their meshIndex, not CtNodeIndex
    WholeArrayIn endpoints,                  // (input)
    WholeArrayIn dataField,                  // (input)
    WholeArrayIn isovalue,                   // (input)
    WholeArrayIn sortOrder,                  // (input)
    WholeArrayIn sortIndices,                // (input)
    WholeArrayIn contourTreeSuperparents,    // (input)
    WholeArrayIn contourTreeWhenTransferred, // (input)
    WholeArrayIn contourTreeHyperparents,    // (input)
    WholeArrayIn contourTreeHyperarcs,       // (input)
    WholeArrayIn contourTreeHypernodes,      // (input)
    WholeArrayIn contourTreeSupernodes,      // (input)
    WholeArrayIn meshExtremaPeaks,           // (input)
    WholeArrayIn meshExtremaPits,            // (input)
    WholeArrayOut superarcIds                // (output)
  );                                         // (input)

  typedef void
    ExecutionSignature(InputIndex, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14);
  using InputDomain = _1;

  viskores::Id numHypernodes; // contourTree.Hypernodes.GetNumberOfValues()
  viskores::Id numSupernodes; // contourTree.Supernodes.GetNumberOfValues()

  // Default Constructor
  VISKORES_EXEC_CONT
  SetTriangleSuperarcId(viskores::Id NumHypernodes, viskores::Id NumSupernodes)
    : numHypernodes(NumHypernodes)
    , numSupernodes(NumSupernodes)
  {
  }

  template <typename EndpointsArrayPortalType,
            typename InFieldArrayPortalType,
            typename InArrayPortalType,
            typename OutArrayPortalType>
  VISKORES_EXEC void operator()(const viskores::Id node,
                                const EndpointsArrayPortalType& endpointsPortal,
                                const InFieldArrayPortalType& fieldPortal,
                                const InFieldArrayPortalType& isovaluePortal,
                                const InArrayPortalType& sortOrder,
                                const InArrayPortalType& sortIndices,
                                const InArrayPortalType& contourTreeSuperparentsPortal,
                                const InArrayPortalType& contourTreeWhenTransferredPortal,
                                const InArrayPortalType& contourTreeHyperparentsPortal,
                                const InArrayPortalType& contourTreeHyperarcsPortal,
                                const InArrayPortalType& contourTreeHypernodesPortal,
                                const InArrayPortalType& contourTreeSupernodesPortal,
                                const InArrayPortalType& meshExtremaPeaksPortal,
                                const InArrayPortalType& meshExtremaPitsPortal,
                                const OutArrayPortalType& superarcIdsPortal) const
  {

    using namespace std;
    using namespace viskores;
    using namespace viskores::worklet::contourtree_augmented;

    // Unpack Data
    Float32 isovalue = isovaluePortal.Get(node);

    //viskores::Id edgeEndpointA = sortIndices.Get(triangle.representativeEdge[0]);
    //viskores::Id edgeEndpointB = sortIndices.Get(triangle.representativeEdge[1]);

    viskores::Id edgeEndpointA = sortIndices.Get(endpointsPortal.Get(node)[0]);
    viskores::Id edgeEndpointB = sortIndices.Get(endpointsPortal.Get(node)[1]);

    // Have to make sure that A is smaller than B, otherwise the path will have redudant edges
    // This is because we take the peak of A and pit of B, if we do it the other way we get incorrect labeling
    if (edgeEndpointA < edgeEndpointB)
    {
      viskores::Id temp = edgeEndpointA;
      edgeEndpointA = edgeEndpointB;
      edgeEndpointB = temp;
    }

    // we will need to prune top and bottom until one of them prunes past the node
    viskores::Id top = meshExtremaPeaksPortal.Get(edgeEndpointA);
    viskores::Id bottom = meshExtremaPitsPortal.Get(edgeEndpointB);

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

    //printf ("We are starting at node %llu for arc between %llu and %llu with top %llu, bottom %llu, topSuperparent %llu, bottomSuperparent %llu, topWhen %llu, bottomWhen %llu, topHyperparent %llu, bottomHyperparent %llu. \n", node, edgeEndpointA, edgeEndpointB, MaskedIndex(top), MaskedIndex(bottom), MaskedIndex(topSuperparent), MaskedIndex(bottomSuperparent), MaskedIndex(topWhen), MaskedIndex(bottomWhen), MaskedIndex(topHyperparent), MaskedIndex(bottomHyperparent));

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
        // @ORIGINAL
        //if (top < node)
        // @PETER HRISTOV
        if (fieldPortal.Get(sortOrder.Get(MaskedIndex(top))) < isovalue)
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
        // @ORIGINAL
        //if (bottom > node)
        // @PETER
        if (fieldPortal.Get(sortOrder.Get(MaskedIndex(bottom))) > isovalue)
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
      if (MaskedIndex(hyperparent) == numHypernodes - 1)
        highSupernode = numSupernodes - 1;
      // otherwise, take the supernode just before the next hypernode
      else
        highSupernode = contourTreeHypernodesPortal.Get(MaskedIndex(hyperparent) + 1) - 1;
      // now, the high supernode may be lower than the element, because the node belongs
      // between it and the high end of the hyperarc
      // @ORIGINAL
      //if (contourTreeSupernodesPortal.Get(highSupernode) < node)
      // @PETER
      if (fieldPortal.Get(sortOrder.Get(
            MaskedIndex(contourTreeSupernodesPortal.Get(MaskedIndex(highSupernode))))) < isovalue)
        // @ORIGINAL
        //contourTreeSuperparentsPortal.Set(node, highSupernode);
        // @PETER
        //triangle.superarcId = highSupernode;
        superarcIdsPortal.Set(node, highSupernode);
      // otherwise, we do a binary search of the superarcs
      else
      { // node between high & low
        // keep going until we span exactly
        while (highSupernode - lowSupernode > 1)
        { // binary search
          // find the midway supernode
          viskores::Id midSupernode = (lowSupernode + highSupernode) / 2;
          // test against the node
          // @ORIGINAL
          //if (contourTreeSupernodesPortal.Get(midSupernode) > node)
          // @PETER
          if (fieldPortal.Get(sortOrder.Get((contourTreeSupernodesPortal.Get((midSupernode))))) >
              isovalue)
            highSupernode = midSupernode;
          // == can't happen since node is regular
          else
            lowSupernode = midSupernode;
        } // binary search

        // now we can use the low node as the superparent
        // @ORIGINAL
        //contourTreeSuperparentsPortal.Set(node, lowSupernode);
        // @PETER
        //triangle.superarcId = lowSupernode;
        superarcIdsPortal.Set(node, lowSupernode);
      } // node between high & low
    }   // ascending hyperarc
    else
    { // descending hyperarc
      // the supernodes on the hyperarc are in sorted high-low order
      viskores::Id highSupernode = contourTreeHypernodesPortal.Get(hyperparent);
      viskores::Id lowSupernode;
      // if it's at the right hand end, take the last supernode in the array
      if (MaskedIndex(hyperparent) == numHypernodes - 1)
      { // last hyperarc
        lowSupernode = numSupernodes - 1;
      } // last hyperarc
      // otherwise, take the supernode just before the next hypernode
      else
      { // other hyperarc
        lowSupernode = contourTreeHypernodesPortal.Get(MaskedIndex(hyperparent) + 1) - 1;
      } // other hyperarc
      // now, the low supernode may be higher than the element, because the node belongs
      // between it and the low end of the hyperarc
      // @ORIGINAL
      //if (contourTreeSupernodesPortal.Get(lowSupernode) > node)
      // @PETER
      if (fieldPortal.Get(sortOrder.Get(
            MaskedIndex(contourTreeSupernodesPortal.Get(MaskedIndex(lowSupernode))))) > isovalue)
        // @ORIGINAL
        //contourTreeSuperparentsPortal.Set(node, lowSupernode);
        // @PETER
        //triangle.superarcId = lowSupernode;
        superarcIdsPortal.Set(node, lowSupernode);
      // otherwise, we do a binary search of the superarcs
      else
      { // node between low & high
        // keep going until we span exactly
        while (lowSupernode - highSupernode > 1)
        { // binary search
          // find the midway supernode
          viskores::Id midSupernode = (highSupernode + lowSupernode) / 2;
          // test against the node
          // @ORIGINAL
          //if (contourTreeSupernodesPortal.Get(midSupernode) > node)
          // @PETER
          if (fieldPortal.Get(sortOrder.Get(MaskedIndex(
                contourTreeSupernodesPortal.Get(MaskedIndex(midSupernode))))) > isovalue)
            highSupernode = midSupernode;
          // == can't happen since node is regular
          else
            lowSupernode = midSupernode;
        } // binary search
        // now we can use the high node as the superparent
        // @ORIGINAL
        //contourTreeSuperparentsPortal.Set(node, highSupernode);
        // @PETER
        //triangle.superarcId = highSupernode;
        superarcIdsPortal.Set(node, highSupernode);
      } // node between low & high
    }   // descending hyperarc

    //mcTriangles.Set(node, triangle);
  }
};

} // process_contourtree_inc
} // namespace contourtree_augmented
} // namespace worklet
} // namespace viskores

#endif
