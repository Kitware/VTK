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


#ifndef viskores_worklet_contourtree_augmented_active_graph_initialize_active_graph_vertices_h
#define viskores_worklet_contourtree_augmented_active_graph_initialize_active_graph_vertices_h

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
class InitializeActiveGraphVertices : public viskores::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn sortIndex,         // (input) sort index
                                WholeArrayIn outDegrees,   // (input) outDegress for each vertex
                                WholeArrayIn inverseIndex, // (input) inverse index for each vertex
                                WholeArrayIn extrema,      // (input) extrema array
                                WholeArrayOut activeIndices,   // (output) active indices
                                WholeArrayOut globalIndex,     // (output) global index
                                WholeArrayOut outdegree,       // (output) out degree
                                WholeArrayOut hyperarcs,       // (output) hyperacrs
                                WholeArrayOut activeVertices); // (output) activeVertices
  typedef void ExecutionSignature(_1, _2, _3, _4, InputIndex, _5, _6, _7, _8, _9);
  using InputDomain = _1;

  // Default Constructor
  VISKORES_EXEC_CONT
  InitializeActiveGraphVertices() {}

  template <typename InFieldPortalType, typename OutFieldPortalType>
  VISKORES_EXEC void operator()(
    const viskores::Id& sortIndex,
    const InFieldPortalType& outDegrees,
    const InFieldPortalType& inverseIndex,
    const InFieldPortalType& extrema,
    const viskores::Id /*vertexIndex*/, // FIXME: Remove unused parameter?
    const OutFieldPortalType& activeIndices,
    const OutFieldPortalType& globalIndex,
    const OutFieldPortalType& outdegree,
    const OutFieldPortalType& hyperarcs,
    const OutFieldPortalType& activeVertices) const
  {
    if (outDegrees.Get(sortIndex) != 1)
    {
      viskores::Id activeIndex = inverseIndex.Get(sortIndex);
      // store it so we can look it up
      activeIndices.Set(sortIndex, activeIndex);
      // add the vertex to the active graph
      globalIndex.Set(activeIndex, sortIndex);
      // set the first edge and outDegrees for it
      outdegree.Set(activeIndex, outDegrees.Get(sortIndex));
      // store the vertex as a merge tree ID, remembering to suppress flags
      hyperarcs.Set(activeIndex, MaskedIndex(extrema.Get(sortIndex)));
      // and store the vertex in the active vertex array
      activeVertices.Set(activeIndex, activeIndex);
    }
    // This operator implements the following loop from the serial code
    //       for (indexType vertex = 0; vertex < mesh.SortIndices.size(); ++vertex)
    //             {
    //             indexType sortIndex = mesh.SortIndices[vertex];
    //             if (outDegrees[sortIndex] != 1)
    //                     {
    //                     indexType activeIndex = inverseIndex[sortIndex];
    //                     // store it so we can look it up
    //                     activeIndices[sortIndex] = activeIndex;
    //                     // add the vertex to the active graph
    //                     globalIndex[activeIndex] = sortIndex;
    //                     // set the first edge and outDegrees for it
    //                     outdegree[activeIndex] = outDegrees[sortIndex];
    //                     // store the vertex as a merge tree ID, remembering to suppress flags
    //                     hyperarcs[activeIndex] = MaskedIndex(extrema[sortIndex]);
    //                     // and store the vertex in the active vertex array
    //                     activeVertices[activeIndex] = activeIndex;
    //                     }
    //             }
  }

}; // Mesh2D_DEM_VertexStarter


} // namespace mesh_dem_triangulation_worklets
} // namespace contourtree_augmented
} // namespace worklet
} // namespace viskores

#endif
