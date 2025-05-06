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

#ifndef viskores_worklet_contourtree_augmented_active_graph_compact_active_edges_new_vertex_outdegree_h
#define viskores_worklet_contourtree_augmented_active_graph_compact_active_edges_new_vertex_outdegree_h

#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/Types.h>
#include <viskores/worklet/WorkletMapField.h>


namespace viskores
{
namespace worklet
{
namespace contourtree_augmented
{
namespace active_graph_inc
{


// Worklet for computing the sort indices from the sort order
class CompactActiveEdgesComputeNewVertexOutdegree : public viskores::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn activeVertices,
                                WholeArrayIn activeEdges,
                                WholeArrayIn edgeFar,
                                WholeArrayIn firstEdge,
                                WholeArrayIn outdegree,
                                WholeArrayInOut hyperarcs,
                                FieldOut newOutdegree);
  typedef _7 ExecutionSignature(_1, InputIndex, _2, _3, _4, _5, _6);
  using InputDomain = _1;

  // Default Constructor
  VISKORES_EXEC_CONT
  CompactActiveEdgesComputeNewVertexOutdegree() {}

  template <typename InFieldPortalType, typename InOutFieldPortalType>
  VISKORES_EXEC viskores::Id operator()(const viskores::Id& vertexId,
                                        // FIXME: Remove unused parameter?
                                        const viskores::Id /*activeVertex*/,
                                        const InFieldPortalType& activeEdgesPortal,
                                        const InFieldPortalType& edgeFarPortal,
                                        const InFieldPortalType& firstEdgePortal,
                                        const InFieldPortalType& outdegreePortal,
                                        const InOutFieldPortalType& hyperarcsPortal) const
  {
    viskores::Id edgeFirst = firstEdgePortal.Get(vertexId);
    // find the sentinel index
    viskores::Id edgeLast = edgeFirst + outdegreePortal.Get(vertexId);

    // retrieve the IS_SUPERNODE flag
    viskores::Id supernodeFlag = hyperarcsPortal.Get(vertexId) & IS_SUPERNODE;

    // we now recompute our monotone paths, starting with the default of ourself
    // with the terminal flag set, and preserving the is_supernode flag
    hyperarcsPortal.Set(vertexId, vertexId | TERMINAL_ELEMENT | supernodeFlag);

    // start with 0
    viskores::Id newOutdegreeValue = 0;

    // now loop through the vertex' edges [and count those not leading back to this vertex]
    for (viskores::Id edge = edgeFirst; edge < edgeLast; edge++)
    { // per edge
      // retrieve the edge ID
      viskores::Id edgeID = activeEdgesPortal.Get(edge);

      // retrieve the vertex ID for the far end & update for pruning
      viskores::Id farEnd = edgeFarPortal.Get(edgeID);

      // grab its hyperarc to see what pruning did
      viskores::Id hyperFar = hyperarcsPortal.Get(farEnd);

      // now get rid of the mask to find the real ID
      farEnd = MaskedIndex(hyperFar);

      // we want to ignore edges that lead back to this vertex
      if (farEnd != vertexId)
      { // far end is different from the vertex, so count it
        ++newOutdegreeValue;
      }
    } // per edge

    return newOutdegreeValue;

    // In serial this worklet implements the following operation
    /*
      for (indexType activeVertex = 0; activeVertex < activeVertices.size(); activeVertex++)
        { // per vertex
          // retrieve actual vertex ID & first edge
          indexType vertexID = activeVertices[activeVertex];
          indexType edgeFirst = firstEdge[vertexID];

          // find the sentinel index
          indexType edgeLast = edgeFirst + outdegree[vertexID];

          // retrieve the IS_SUPERNODE flag
          indexType supernodeFlag = hyperarcs[vertexID] & IS_SUPERNODE;

          // we now recompute our monotone paths, starting with the default of ourself
          // with the terminal flag set, and preserving the is_supernode flag
          hyperarcs[vertexID] = vertexID | TERMINAL_ELEMENT | supernodeFlag;

          // start with 0
          newOutdegree[activeVertex] = 0;

          // now loop through the vertex' edges [and count those not leading back to this vertex]
          for (indexType edge = edgeFirst; edge < edgeLast; edge++)
            { // per edge
              // retrieve the edge ID
              indexType edgeID = activeEdges[edge];

              // retrieve the vertex ID for the far end & update for pruning
              indexType farEnd = edgeFar[edgeID];

              // grab its hyperarc to see what pruning did
              indexType hyperFar = hyperarcs[farEnd];

              // now get rid of the mask to find the real ID
              farEnd = MaskedIndex(hyperFar);

              // we want to ignore edges that lead back to this vertex
              if (farEnd != vertexID)
                { // far end is different from the vertex, so count it
                  ++newOutdegree[activeVertex];
                }
            } // per edge
          } // per vertex
      */
  }


}; //  CompactActiveEdgesComputeNewVertexOutdegree


} // namespace active_graph_inc
} // namespace contourtree_augmented
} // namespace worklet
} // namespace viskores

#endif
