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

#ifndef viskores_worklet_contourtree_augmented_active_graph_transfer_saddle_starts_update_edge_sorter_h
#define viskores_worklet_contourtree_augmented_active_graph_transfer_saddle_starts_update_edge_sorter_h

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


// Worklet to update all of the edges so that the far end resets to the result of the ascent in the previous step
class TransferSaddleStartsUpdateEdgeSorter : public viskores::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn activeVertices,    // (input) active activeVertices
                                WholeArrayIn activeEdges,  // (input) active edges
                                WholeArrayIn firstEdge,    // (input) first edge
                                WholeArrayIn newFirstEdge, // (input) new first edge
                                WholeArrayIn newOutDegree, // (input) new out degree
                                WholeArrayOut edgeSorter); // (output) edge sorter
  typedef void ExecutionSignature(_1, InputIndex, _2, _3, _4, _5, _6);
  using InputDomain = _1;

  // Default Constructor
  VISKORES_EXEC_CONT
  TransferSaddleStartsUpdateEdgeSorter() {}

  template <typename InFieldPortalType, typename OutFieldPortalType>
  VISKORES_EXEC void operator()(const viskores::Id& vertexId,
                                const viskores::Id vertex,
                                const InFieldPortalType& activeEdgesPortal,
                                const InFieldPortalType& firstEdgePortal,
                                const InFieldPortalType& newFirstEdgePortal,
                                const InFieldPortalType& newOutdegreePortal,
                                const OutFieldPortalType& edgeSorterPortal) const
  {
    viskores::Id activeEdgesIdx = firstEdgePortal.Get(vertexId);
    viskores::Id edgeSorterIndex = newFirstEdgePortal.Get(vertex);

    // loop through edges
    for (viskores::Id edge = 0; edge < newOutdegreePortal.Get(vertex); ++edge)
    { // per edge
      // retrieve the edge ID and the far end of the edge
      edgeSorterPortal.Set(edgeSorterIndex++, activeEdgesPortal.Get(activeEdgesIdx++));
    } // per edge


    // In serial this worklet implements the following operation
    // for (indexType vertex = 0; vertex < activeVertices.size(); vertex++)
    //    { // per vertex
    //    // retrieve the actual vertex ID
    //    indexType vertexID = activeVertices[vertex];
    //
    //    indexType activeEdgesIdx = firstEdge[vertexID];
    //    indexType edgeSorterIndex = newFirstEdge[vertex];
    //
    //    // loop through edges
    //    for (indexType edge = 0; edge < newOutdegree[vertex]; ++edge)
    //            { // per edge
    //            // retrieve the edge ID and the far end of the edge
    //            edgeSorter[edgeSorterIndex++] = activeEdges[activeEdgesIdx++];
    //            } // per edge
    //    } // per vertex
  }


}; //  TransferSaddleStartsUpdateEdgeSorter


} // namespace active_graph_inc
} // namespace contourtree_augmented
} // namespace worklet
} // namespace viskores

#endif
