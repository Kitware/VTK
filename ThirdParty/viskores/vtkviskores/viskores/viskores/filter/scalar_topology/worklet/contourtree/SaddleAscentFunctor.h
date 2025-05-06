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
// Any vector needed by the functor for lookup purposes will be passed as a parameter to
// the constructor and saved, with the actual function call being the operator ()
//
// Vectors marked I/O are intrinsically risky unless there is an algorithmic guarantee
// that the read/writes are completely independent - which for our case actually occurs
// The I/O vectors should therefore be justified in comments both here & in caller
//
//=======================================================================================

#ifndef viskores_worklet_contourtree_saddle_ascent_functor_h
#define viskores_worklet_contourtree_saddle_ascent_functor_h

#include <viskores/filter/scalar_topology/worklet/contourtree/Types.h>
#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace worklet
{
namespace contourtree
{

// Worklet for setting initial chain maximum value
class SaddleAscentFunctor : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn vertexID,       // (input) index into active vertices
                                WholeArrayIn firstEdge, // (input) first edge for each active vertex
                                WholeArrayIn outdegree, // (input) updegree of vertex
                                WholeArrayIn activeEdges,  // (input) active edges
                                WholeArrayIn chainExtemum, // (input) chain extemum for vertices
                                WholeArrayInOut edgeFar,   // (input) high ends of edges
                                FieldOut newOutdegree);    // (output) new updegree of vertex
  using ExecutionSignature = _7(_1, _2, _3, _4, _5, _6);
  using InputDomain = _1;

  // Constructor
  VISKORES_EXEC_CONT
  SaddleAscentFunctor() {}

  template <typename InFieldPortalType, typename InOutFieldPortalType>
  VISKORES_EXEC viskores::Id operator()(const viskores::Id& vertexID,
                                        const InFieldPortalType& firstEdge,
                                        const InFieldPortalType& outdegree,
                                        const InFieldPortalType& activeEdges,
                                        const InFieldPortalType& chainExtremum,
                                        const InOutFieldPortalType& edgeFar) const
  {
    viskores::Id newOutdegree;

    // first ascent found
    viskores::Id firstMax = NO_VERTEX_ASSIGNED;
    bool isGenuineSaddle = false;

    // loop through edges
    for (viskores::Id edge = 0; edge < outdegree.Get(vertexID); edge++)
    {
      // retrieve the edge ID and the high end of the edge
      viskores::Id edgeID = activeEdges.Get(firstEdge.Get(vertexID) + edge);
      viskores::Id nbrHigh = chainExtremum.Get(edgeFar.Get(edgeID));
      edgeFar.Set(edgeID, nbrHigh);

      // test for first one found
      if (firstMax == NO_VERTEX_ASSIGNED)
        firstMax = nbrHigh;
      else // otherwise, check for whether we have an actual join saddle
        if (firstMax != nbrHigh)
        { // first non-matching
          isGenuineSaddle = true;
        } // first non-matching
    }     // per edge

    // if it's not a genuine saddle, ignore the edges by setting updegree to 0
    if (!isGenuineSaddle)
      newOutdegree = 0;
    else
      newOutdegree = outdegree.Get(vertexID);
    return newOutdegree;
  }
}; // SaddleAscentFunctor
}
}
}

#endif
