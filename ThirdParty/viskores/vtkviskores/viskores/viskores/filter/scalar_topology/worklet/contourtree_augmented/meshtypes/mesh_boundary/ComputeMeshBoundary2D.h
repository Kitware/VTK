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

#ifndef viskores_worklet_contourtree_augmented_mesh_dem_mesh_types_mesh_boundary_compute_mesh_boundary_2D_h
#define viskores_worklet_contourtree_augmented_mesh_dem_mesh_types_mesh_boundary_compute_mesh_boundary_2D_h

#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/Types.h>
#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace worklet
{
namespace contourtree_augmented
{

// Worklet to collapse past regular vertices by updating inbound and outbound as part
// loop to find the now-regular vertices and collapse past them without altering
// the existing join & split arcs
class ComputeMeshBoundary2D : public viskores::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn boundaryId,             // (input)
                                WholeArrayIn sortIndices,       // (input)
                                ExecObject meshBoundary,        // (input)
                                FieldOut boundaryVertexArray,   // output
                                FieldOut boundarySortIndexArray // output
  );
  typedef void ExecutionSignature(_1, _2, _3, _4, _5);
  using InputDomain = _1;


  // Default Constructor
  VISKORES_EXEC_CONT
  ComputeMeshBoundary2D() {}

  template <typename InFieldPortalType, typename MeshBoundaryType>
  VISKORES_EXEC void operator()(const viskores::Id& boundaryId,
                                const InFieldPortalType sortIndicesPortal,
                                const MeshBoundaryType& meshBoundary,
                                viskores::Id& boundaryVertex,
                                viskores::Id& boundarySortIndex) const
  {
    auto meshStructure2D = meshBoundary.GetMeshStructure();
    viskores::Id numBoundary =
      2 * meshStructure2D.MeshSize[1] + 2 * meshStructure2D.MeshSize[0] - 4;

    // For comments: [0] -> column, [1] -> row
    // Define the boundaryVertex result
    if (boundaryId < meshStructure2D.MeshSize[0])
    {
      boundaryVertex = meshStructure2D.VertexId(viskores::Id2{ boundaryId, 0 });
    }
    // then bottom row
    else if (boundaryId > numBoundary - meshStructure2D.MeshSize[0] - 1)
    {
      boundaryVertex = meshStructure2D.VertexId(viskores::Id2{
        boundaryId + meshStructure2D.MeshSize[0] - numBoundary, meshStructure2D.MeshSize[1] - 1 });
    }
    // then the row ends
    else
    { // row ends
      boundaryVertex = meshStructure2D.VertexId(viskores::Id2{
        ((boundaryId - meshStructure2D.MeshSize[0]) % 2) ? (meshStructure2D.MeshSize[0] - 1) : 0,
        ((boundaryId - meshStructure2D.MeshSize[0]) / 2) + 1 });
    } // row ends
    // and fill in the sort index array as well
    boundarySortIndex = sortIndicesPortal.Get(boundaryVertex);

    /* TODO/FIXME: Delete this comment after code review and tests
    // compute how many elements are needed
    indexType nBoundary = 2 * nRows + 2 * nCols - 4;

    boundaryVertexArray.resize(nBoundary);
    boundarySortIndexArray.resize(nBoundary);

    // loop to add in the vertices
    // NB: the arithmetic here is chosen to guarantee that the vertex indices
    // are in sorted order in the output - I'm not sure that this is necessary, but . . .
    for (indexType boundaryId = 0; boundaryId < nBoundary; boundaryId++)
        { // loop through indices
        // do top row first
        if (boundaryId < nCols)
            boundaryVertexArray[boundaryId] = vertexId(0, boundaryId);
        // then bottom row
        else if (boundaryId > nBoundary - nCols - 1)
            boundaryVertexArray[boundaryId] = vertexId(nRows - 1, boundaryId + nCols - nBoundary);
        // then the row ends
        else
            { // row ends
            indexType row = ((boundaryId - nCols) / 2) + 1;
            indexType col = ((boundaryId - nCols) % 2) ? (nCols - 1) : 0;
            boundaryVertexArray[boundaryId] = vertexId(row, col);
            } // row ends
        // and fill in the index array as well
        boundarySortIndexArray[boundaryId] = sortIndices[boundaryVertexArray[boundaryId]];
        } // loop through indices
    */
  }

}; // ComputeMeshBoundary2D

} // namespace contourtree_augmented
} // namespace worklet
} // namespace viskores

#endif
