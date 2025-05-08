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

#ifndef viskores_worklet_contourtree_augmented_mesh_dem_mesh_types_mesh_boundary_compute_mesh_boundary_3D_h
#define viskores_worklet_contourtree_augmented_mesh_dem_mesh_types_mesh_boundary_compute_mesh_boundary_3D_h

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
class ComputeMeshBoundary3D : public viskores::worklet::WorkletMapField
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
  ComputeMeshBoundary3D() {}

  template <typename InFieldPortalType, typename MeshBoundaryType>
  VISKORES_EXEC void operator()(const viskores::Id& boundaryId,
                                const InFieldPortalType sortIndicesPortal,
                                const MeshBoundaryType& meshBoundary,
                                viskores::Id& boundaryVertex,
                                viskores::Id& boundarySortIndex) const
  {
    auto meshStructure3D = meshBoundary.GetMeshStructure();
    viskores::Id3 meshSize = meshStructure3D.MeshSize;
    // for comments [0]/x -> column, [1]/y -> row, [2]/z -> slice
    // calculate the number of boundary elements - all of the two xy faces
    viskores::Id nBoundary = 2 * meshSize[1] * meshSize[0] // xy faces
      + 2 * meshSize[1] * (meshSize[2] - 2)                // yz faces - excluding vertices on xy
      + 2 * (meshSize[0] - 2) * (meshSize[2] - 2);         // xz face interiors

    viskores::Id3 pos{ 0, 0, 0 };
    viskores::Id sliceSize = meshSize[1] * meshSize[0];
    viskores::Id sliceBoundarySize = 2 * meshSize[1] + 2 * meshSize[0] - 4;
    // do top plane first
    if (boundaryId < sliceSize)
    { // top plane
      pos[0] = boundaryId % meshSize[0];
      pos[1] = boundaryId / meshSize[0];
      pos[2] = 0;
    } // top plane
    // then bottom plane
    else if (boundaryId >= nBoundary - sliceSize)
    { // bottom plane
      pos[0] = (boundaryId - (nBoundary - sliceSize)) % meshSize[0];
      pos[1] = (boundaryId - (nBoundary - sliceSize)) / meshSize[0];
      pos[2] = meshSize[2] - 1;
    } // bottom plane
    // now we have to deal with the exterior of the remaining slices
    else
    { // slice exteriors
      // first we subtract the size of the first slice
      viskores::Id offsetBoundaryid = boundaryId - sliceSize;
      // now we can compute the slice id
      pos[2] = 1 + offsetBoundaryid / sliceBoundarySize;
      // compute the local id on the slice
      viskores::Id sliceBoundaryId = offsetBoundaryid % sliceBoundarySize;
      // now test for the first and last pos[1]
      if (sliceBoundaryId < meshSize[0])
      { // first row
        pos[0] = sliceBoundaryId;
        pos[1] = 0;
      } // first row
      else if (sliceBoundaryId >= (sliceBoundarySize - meshSize[0]))
      { // last row
        pos[0] = sliceBoundaryId - (sliceBoundarySize - meshSize[0]);
        pos[1] = meshSize[1] - 1;
      } // last row
      else
      { // any other row
        pos[0] = ((sliceBoundaryId - meshSize[0]) % 2) ? (meshSize[0] - 1) : 0;
        pos[1] = ((sliceBoundaryId - meshSize[0]) / 2) + 1;
      } // any other row
    }   // slice exteriors
    // now we have row, col, slice all set, compute the actual ID
    boundaryVertex = meshStructure3D.VertexId(pos);
    // and fill in the index array as well
    boundarySortIndex = sortIndicesPortal.Get(boundaryVertex);

    /*
    { // GetBoundaryVertices()
    // calculate the number of boundary elements - all of the two xy faces
    indexType nBoundary = 	  2 * nRows * nCols						// xy faces
                            + 2 * nRows * (nSlices - 2)				// yz faces - excluding vertices on xy
                            + 2 * (nCols - 2) * (nSlices - 2)		// xz face interiors
                            ;

    // resize the arrays accordingly
    boundaryVertexArray.resize(nBoundary);
    boundarySortIndexArray.resize(nBoundary);

    // loop to add in the vertices
    for (indexType boundaryID = 0; boundaryID < nBoundary; boundaryID++)
        { // loop through indices
        indexType row = 0, col = 0, slice = 0;
        indexType sliceSize = nRows * nCols;
        indexType sliceBoundarySize = 2 * nRows + 2 * nCols - 4;
        // do top plane first
        if (boundaryID < sliceSize)
            { // top plane
            row = boundaryID / nCols;
            col = boundaryID % nCols;
            slice = 0;
            } // top plane
        // then bottom plane
        else if (boundaryID >= nBoundary - sliceSize)
            { // bottom plane
            row = (boundaryID - (nBoundary - sliceSize)) / nCols;
            col = (boundaryID - (nBoundary - sliceSize)) % nCols;
            slice = nSlices - 1;
            } // bottom plane
        // now we have to deal with the exterior of the remaining slices
        else
            { // slice exteriors
            // first we subtract the size of the first slice
            indexType offsetBoundaryID = boundaryID - sliceSize;
            // now we can compute the slice ID
            slice = 1 + offsetBoundaryID / sliceBoundarySize;
            // compute the local ID on the slice
            indexType sliceBoundaryID = offsetBoundaryID % sliceBoundarySize;
            // now test for the first and last row
            if (sliceBoundaryID < nCols)
                { // first row
                row = 0;
                col = sliceBoundaryID;
                } // first row
            else if (sliceBoundaryID >= (sliceBoundarySize - nCols))
                { // last row
                row = nRows - 1;
                col = sliceBoundaryID - (sliceBoundarySize - nCols);
                } // last row
            else
                { // any other row
                row = ((sliceBoundaryID - nCols) / 2) + 1;
                col = ((sliceBoundaryID - nCols) % 2) ? (nCols - 1) : 0;
                } // any other row
            } // slice exteriors
        // now we have row, col, slice all set, compute the actual ID
        boundaryVertexArray[boundaryID] = vertexId(slice, row, col);
        // and fill in the index array as well
        boundarySortIndexArray[boundaryID] = sortIndices[boundaryVertexArray[boundaryID]];
        } // loop through indices
    } // GetBoundaryVertices()
    */
  }

}; // ComputeMeshBoundary3D

} // namespace contourtree_augmented
} // namespace worklet
} // namespace viskores

#endif
