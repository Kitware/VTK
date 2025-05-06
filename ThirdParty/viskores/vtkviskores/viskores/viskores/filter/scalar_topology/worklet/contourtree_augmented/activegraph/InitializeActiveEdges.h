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

#ifndef viskores_worklet_contourtree_augmented_active_graph_initialize_active_edges_h
#define viskores_worklet_contourtree_augmented_active_graph_initialize_active_edges_h

#include <viskores/exec/arg/BasicArg.h>
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
template <class MeshClassType>
class InitializeActiveEdges : public viskores::worklet::WorkletMapField
{
public:
  /// Additional basic execution argument tags
  //struct _10 : viskores::exec::arg::BasicArg<10> {  };
  //struct _11 : viskores::exec::arg::BasicArg<11> {  };

  typedef void ControlSignature(
    FieldIn outdegree,               // (input) outdegree
    ExecObject meshStructure,        // (input) execution object with the mesh structure
    FieldIn firstEdge,               // (input)
    FieldIn globalIndex,             // (input) ActiveGraph.GlobalIndex
    WholeArrayIn extrema,            // (input)
    WholeArrayIn neighbourhoodMasks, // (input)
    WholeArrayOut edgeNear,          // (output) edgeNear
    WholeArrayOut edgeFar,           // (output) edgeFar
    WholeArrayOut activeEdges);      // (output) activeEdges
  typedef void ExecutionSignature(_1, InputIndex, _2, _3, _4, _5, _6, _7, _8, _9);

  using InputDomain = _1;

  // Default Constructor
  VISKORES_EXEC_CONT
  InitializeActiveEdges() {}

  template <typename MeshStructureType, typename InFieldPortalType, typename OutFieldPortalType>
  VISKORES_EXEC void operator()(const viskores::Id& outdegree,
                                const viskores::Id activeIndex,
                                const MeshStructureType& meshStructure,
                                const viskores::Id& firstEdgeIndex,
                                const viskores::Id& sortIndex, // = GlobalIndex.Get(activeIndex)
                                const InFieldPortalType& extrema,
                                const InFieldPortalType& neighbourhoodMasks,
                                const OutFieldPortalType& edgeNear,
                                const OutFieldPortalType& edgeFar,
                                const OutFieldPortalType& activeEdges) const
  {
    if (outdegree != 0)
    {
      // temporary array for storing edges
      viskores::Id neigbourComponents[MeshClassType::MAX_OUTDEGREE];
      int currNbrNo = 0;
      for (viskores::Id nbrNo = 0; nbrNo < meshStructure.GetMaxNumberOfNeighbours(); ++nbrNo)
        if (neighbourhoodMasks.Get(sortIndex) & (static_cast<viskores::Id>(1) << nbrNo))
        {
          neigbourComponents[currNbrNo++] = meshStructure.GetNeighbourIndex(sortIndex, nbrNo);
        }


      // arcs stores the ID from the join tree - i.e. the chain extremum
      //              we cannot store the correct ID yet, because it may not have been assigned yet
      //              in serial, we could hack around this by processing the vertices in a given order
      //              but in parallel we can't, so we have two stages:
      //                      in this stage, we store the join tree ID (after suppressing flags)
      //                      in a later stage, we convert it to an active graph ID
      // firstEdge / outdegree / edgeNear / edgeFar are straightforward
      // as with earlier versions, the parallel equivalent will need to use stream compression
      // but the serial version can be expressed more simply.

      for (viskores::Id edge = 0; edge < outdegree; edge++)
      { // per edge
        // compute the edge index in the edge arrays
        viskores::Id edgeID = firstEdgeIndex + edge;

        // now set the low and high ends
        edgeNear.Set(edgeID, activeIndex);
        edgeFar.Set(edgeID, MaskedIndex(extrema.Get(neigbourComponents[edge])));

        // and save the edge itself
        activeEdges.Set(edgeID, edgeID);
      } // per edge
    }

    // This operator implements the following loop from the serial code
    /*for (indexType activeIndex = 0; activeIndex < nCriticalPoints; ++activeIndex)
      {
        indexType outdegree = outdegree[activeIndex];
        if (outdegree != 0)
          {
            indexType sortIndex = globalIndex[activeIndex];

            // temporary array for storing edges
            indexType neigbourComponents[Mesh::MAX_OUTDEGREE];
            int currNbrNo = 0;
            for (viskores::Int32 nbrNo = 0; nbrNo < mesh.GetMaxNumberOfNeighbours(); ++nbrNo)
              if (neighbourhoodMasks[sortIndex] & 1 << nbrNo)
                {
                   neigbourComponents[currNbrNo++] = mesh.GetNeighbourIndex(sortIndex, nbrNo);
                }


            // arcs stores the ID from the join tree - i.e. the chain extremum
            //              we cannot store the correct ID yet, because it may not have been assigned yet
            //              in serial, we could hack around this by processing the vertices in a given order
            //              but in parallel we can't, so we have two stages:
            //                      in this stage, we store the join tree ID (after suppressing flags)
            //                      in a later stage, we convert it to an active graph ID
            // firstEdge / outdegree / edgeNear / edgeFar are straightforward
            // as with earlier versions, the parallel equivalent will need to use stream compression
            // but the serial version can be expressed more simply.

            for (indexType edge = 0; edge < outdegree; edge++)
              { // per edge
                // compute the edge index in the edge arrays
                indexType edgeID = firstEdge[activeIndex] + edge;

                // now set the low and high ends
                edgeNear[edgeID] = activeIndex;
                edgeFar[edgeID] = MaskedIndex(extrema[neigbourComponents[edge]]);

                // and save the edge itself
                activeEdges[edgeID] = edgeID;
              } // per edge
          }
        } // per activeIndex*/
  }
}; // Mesh2D_DEM_VertexStarter


} // namespace mesh_dem_triangulation_worklets
} // namespace contourtree_augmented
} // namespace worklet
} // namespace viskores

#endif
