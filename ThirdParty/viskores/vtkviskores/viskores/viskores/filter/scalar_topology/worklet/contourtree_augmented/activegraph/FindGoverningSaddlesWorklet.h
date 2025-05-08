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


#ifndef viskores_worklet_contourtree_augmented_active_graph_find_governing_saddles_worklet_h
#define viskores_worklet_contourtree_augmented_active_graph_find_governing_saddles_worklet_h

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
class FindGoverningSaddlesWorklet : public viskores::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn edgeNo,             // (input) edge number
                                WholeArrayInOut edgeSorter, // (input/output) edge sorter
                                WholeArrayIn edgeFar,       // (input) edge far
                                WholeArrayIn edgeNear,      // (input) edge near
                                WholeArrayOut hyperarcs,    // (output) hyperarcs
                                WholeArrayOut outdegree);   // (output) outdegree
  typedef void ExecutionSignature(_1, _2, _3, _4, _5, _6);
  using InputDomain = _1;

  // Default Constructor
  VISKORES_EXEC_CONT
  FindGoverningSaddlesWorklet() {}

  template <typename InOutFieldPortalType, typename InFieldPortalType, typename OutFieldPortalType>
  VISKORES_EXEC void operator()(const viskores::Id& edgeNo,
                                const InOutFieldPortalType& edgeSorterPortal,
                                const InFieldPortalType& edgeFarPortal,
                                const InFieldPortalType& edgeNearPortal,
                                const OutFieldPortalType& hyperarcsPortal,
                                const OutFieldPortalType& outdegreePortal) const
  {
    // default to true
    bool isBestSaddleEdge = true;
    // retrieve the edge ID
    viskores::Id edge = edgeSorterPortal.Get(edgeNo);

    // edge no. 0 is always best, so skip it
    if (edgeNo != 0)
    { // edge != 0
      // retrieve the previous edge
      viskores::Id prevEdge = edgeSorterPortal.Get(edgeNo - 1);
      // if the previous edge has the same far end
      if (edgeFarPortal.Get(prevEdge) == edgeFarPortal.Get(edge))
        isBestSaddleEdge = false;
    } // edge != 0

    if (isBestSaddleEdge)
    { // found an extremum
      // retrieve the near end as the saddle
      viskores::Id saddle = edgeNearPortal.Get(edge);
      // and the far end as the extremum
      viskores::Id extreme = edgeFarPortal.Get(edge);

      // we can now set the hyperarc for the extremum
      // we do this by setting it to point to the saddle
      // and setting it's IS_HYPERNODE and IS_SUPERNODE flags
      hyperarcsPortal.Set(extreme, saddle | IS_HYPERNODE | IS_SUPERNODE);

      // and set the outdegree to 0
      outdegreePortal.Set(extreme, 0);

      // NEW! NEW! NEW!
      // we now also know that the lower end is a supernode
      // so we set it's flag as well - this allows us to identify real saddles
      // versus mere candidates
      // NB: there may be write collisions, but they are all setting the
      // flag on, so the collision is acceptable
      hyperarcsPortal.Set(saddle, hyperarcsPortal.Get(saddle) | IS_SUPERNODE);
    } // found a extremum

    // This worklet implements the following from the OpenMP version
    /*for (indexType edgeNo = 0; edgeNo < edgeSorter.size(); edgeNo++)
                { // per edge
                // default to true
                bool isBestSaddleEdge = true;
                // retrieve the edge ID
                indexType edge = edgeSorter[edgeNo];

                // edge no. 0 is always best, so skip it
                if (edgeNo != 0)
                        { // edge != 0
                        // retrieve the previous edge
                        indexType prevEdge = edgeSorter[edgeNo-1];
                        // if the previous edge has the same far end
                        if (edgeFar[prevEdge] == edgeFar[edge])
                                isBestSaddleEdge = false;
                        } // edge != 0

                if (isBestSaddleEdge)
                        { // found an extremum
                        // retrieve the near end as the saddle
                        indexType saddle = edgeNear[edge];
                        // and the far end as the extremum
                        indexType extreme = edgeFar[edge];

                        // we can now set the hyperarc for the extremum
                        // we do this by setting it to point to the saddle
                        // and setting it's IS_HYPERNODE and IS_SUPERNODE flags
                        hyperarcs[extreme] = saddle | IS_HYPERNODE | IS_SUPERNODE;

                        // and set the outdegree to 0
                        outdegree[extreme] = 0;

                        // NEW! NEW! NEW!
                        // we now also know that the lower end is a supernode
                        // so we set it's flag as well - this allows us to identify real saddles
                        // versus mere candidates
                        // NB: there may be write collisions, but they are all setting the
                        // flag on, so the collision is acceptable
                        hyperarcs[saddle] |= IS_SUPERNODE;
                        } // found a extremum
                } // per edge */
  }
}; // FindGoverningSaddlesWorklet


} // namespace active_graph_inc
} // namespace contourtree_augmented
} // namespace worklet
} // namespace viskores

#endif
