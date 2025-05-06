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
//  Copyright (c) 2016, Los Alamos National Security, LLC
//  All rights reserved.
//
//  Copyright 2016. Los Alamos National Security, LLC.
//  This software was produced under U.S. Government contract DE-AC52-06NA25396
//  for Los Alamos National Laboratory (LANL), which is operated by
//  Los Alamos National Security, LLC for the U.S. Department of Energy.
//  The U.S. Government has rights to use, reproduce, and distribute this
//  software.  NEITHER THE GOVERNMENT NOR LOS ALAMOS NATIONAL SECURITY, LLC
//  MAKES ANY WARRANTY, EXPRESS OR IMPLIED, OR ASSUMES ANY LIABILITY FOR THE
//  USE OF THIS SOFTWARE.  If software is modified to produce derivative works,
//  such modified software should be clearly marked, so as not to confuse it
//  with the version available from LANL.
//
//  Additionally, redistribution and use in source and binary forms, with or
//  without modification, are permitted provided that the following conditions
//  are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//  3. Neither the name of Los Alamos National Security, LLC, Los Alamos
//     National Laboratory, LANL, the U.S. Government, nor the names of its
//     contributors may be used to endorse or promote products derived from
//     this software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY LOS ALAMOS NATIONAL SECURITY, LLC AND
//  CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
//  BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
//  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL LOS ALAMOS
//  NATIONAL SECURITY, LLC OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
//  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
//  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
//  USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
//  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//============================================================================

//  This code is based on the algorithm presented in the paper:
//  “Parallel Peak Pruning for Scalable SMP Contour Tree Computation.”
//  Hamish Carr, Gunther Weber, Christopher Sewell, and James Ahrens.
//  Proceedings of the IEEE Symposium on Large Data Analysis and Visualization
//  (LDAV), October 2016, Baltimore, Maryland.

//=======================================================================================
//
// COMMENTS:
//
// This functor identifies for each vertex which edges to keep. For arbitrary meshes,
// this should use reductions. For regular meshes, this way is faster due to low bounded
// updegree.
//
// Any vector needed by the functor for lookup purposes will be passed as a parameter to
// the constructor and saved, with the actual function call being the operator ()
//
// Vectors marked I/O are intrinsically risky unless there is an algorithmic guarantee
// that the read/writes are completely independent - which for our case actually occurs
// The I/O vectors should therefore be justified in comments both here & in caller
//
//=======================================================================================

#ifndef viskores_worklet_contourtree_active_edge_transferrer_h
#define viskores_worklet_contourtree_active_edge_transferrer_h

#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace worklet
{
namespace contourtree
{

// Worklet: set initial chain maximum value
class ActiveEdgeTransferrer : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn vertexID,          // (input) active vertex ID
                                FieldIn newPosition,       // (input) new position of edge in array
                                FieldIn newOutdegree,      // (input) the new updegree computed
                                WholeArrayIn activeEdges,  // (input) active edges
                                WholeArrayIn prunesTo,     // (input) where a vertex prunes to
                                WholeArrayInOut firstEdge, // (i/o) first edge of each active vertex
                                WholeArrayInOut outdegree, // (i/o) existing vertex updegrees
                                WholeArrayInOut chainExtremum, // (i/o) chain extremum for vertices
                                WholeArrayInOut edgeFar,       // (i/o) high end of each edge
                                WholeArrayOut newActiveEdges); // (output) new active edge list
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10);
  using InputDomain = _1;

  // WARNING: POTENTIAL RISK FOR I/O
  // chainMaximum is safe for I/O here because:
  // 		we have previously eliminated maxima from the active vertex list
  //		we lookup chainMaximum of edgeHigh, which is guaranteed to be a maximum
  //		therefore, the chainMaximum entries edited are *NEVER* also accessed & v.v.
  //		edgeHigh is safe to edit, because each element only accesses it's own, and
  //		reads the current value before writing to it
  //		the same is true of firstEdge and updegree

  template <typename InFieldPortalType, typename InOutFieldPortalType, typename OutFieldPortalType>
  VISKORES_EXEC void operator()(const viskores::Id& vertexID,
                                const viskores::Id& newPosition,
                                const viskores::Id& newOutdegree,
                                const InFieldPortalType& activeEdges,
                                const InFieldPortalType& prunesTo,
                                const InOutFieldPortalType& firstEdge,
                                const InOutFieldPortalType& outdegree,
                                const InOutFieldPortalType& chainExtremum,
                                const InOutFieldPortalType& edgeFar,
                                const OutFieldPortalType& newActiveEdges) const
  {
    // retrieve actual vertex ID & first edge
    viskores::Id edgeFirst = firstEdge.Get(vertexID);

    // internal counter for # of edges
    viskores::Id whichEdge = newPosition;

    // walk through the vertex edges, counting as we go
    for (viskores::Id edge = 0; edge < outdegree.Get(vertexID); edge++)
    {
      // compute the index and edge ID of this edge
      viskores::Id edgeIndex = edgeFirst + edge;
      viskores::Id edgeID = activeEdges.Get(edgeIndex);

      // retrieve the vertex ID for the high end & update for pruning
      viskores::Id highEnd = prunesTo.Get(chainExtremum.Get(edgeFar.Get(edgeID)));

      // we want to ignore edges that lead back to this vertex
      if (highEnd != vertexID)
      {
        // reset the high end of the edge, copying downwards
        edgeFar.Set(edgeID, highEnd);

        // and keep the edge around
        newActiveEdges.Set(whichEdge++, edgeID);

        // and reset the chain maximum for good measure
        chainExtremum.Set(vertexID, highEnd);
      }
    }

    // now reset the firstEdge variable for this vertex
    outdegree.Set(vertexID, newOutdegree);
    firstEdge.Set(vertexID, newPosition);
  }
}; // ActiveEdgeTransferrer
}
}
}

#endif
