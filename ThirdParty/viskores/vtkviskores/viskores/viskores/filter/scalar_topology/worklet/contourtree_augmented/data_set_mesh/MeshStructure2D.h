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

#ifndef viskores_worklet_contourtree_augmented_data_set_mesh_execution_object_mesh_2d_h
#define viskores_worklet_contourtree_augmented_data_set_mesh_execution_object_mesh_2d_h

#include <viskores/Types.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/data_set_mesh/IdRelabeler.h>

namespace viskores
{
namespace worklet
{
namespace contourtree_augmented
{
namespace data_set_mesh
{

// Worklet for computing the sort indices from the sort order
class MeshStructure2D
{
public:
  VISKORES_EXEC_CONT
  MeshStructure2D()
    : MeshSize{ 0, 0 }
  {
  }

  VISKORES_EXEC_CONT
  MeshStructure2D(viskores::Id2 meshSize)
    : MeshSize(meshSize)
  {
  }

  /// Get the number of mesh vertices
  VISKORES_EXEC_CONT
  viskores::Id GetNumberOfVertices() const { return (this->MeshSize[0] * this->MeshSize[1]); }

  /// Get the (x,y) position of the vertex based on its index
  VISKORES_EXEC
  inline viskores::Id2 VertexPos(viskores::Id v) const
  {
    return viskores::Id2{ v % this->MeshSize[0], v / this->MeshSize[0] };
  }

  ///vertex ID - row * ncols + col
  VISKORES_EXEC
  inline viskores::Id VertexId(viskores::Id2 pos) const
  {
    return pos[1] * this->MeshSize[0] + pos[0];
  }

  /// determine if the vertex is owned by this mesh block or not
  /// The function returns NO_SUCH_ELEMENT if the vertex is not owned by the block and
  /// otherwise it returns global id of the vertex as determined via the IdRelabeler
  VISKORES_EXEC_CONT
  inline viskores::Id GetVertexOwned(
    const viskores::Id& meshIndex,
    const viskores::worklet::contourtree_augmented::mesh_dem::IdRelabeler& localToGlobalIdRelabeler)
    const
  {
    // Get the vertex position
    viskores::Id2 pos = this->VertexPos(meshIndex);
    // now test - the low ID boundary belongs to this block
    // the high ID boundary belongs to the next block if there is one
    if (((pos[1] == this->MeshSize[1] - 1) &&
         (pos[1] + localToGlobalIdRelabeler.LocalBlockOrigin[1] !=
          localToGlobalIdRelabeler.GlobalSize[1] - 1)) ||
        ((pos[0] == this->MeshSize[0] - 1) &&
         (pos[0] + localToGlobalIdRelabeler.LocalBlockOrigin[0] !=
          localToGlobalIdRelabeler.GlobalSize[0] - 1)))
    {
      return viskores::worklet::contourtree_augmented::NO_SUCH_ELEMENT;
    }
    else
    {
      return localToGlobalIdRelabeler(meshIndex);
    }
  }

  viskores::Id2 MeshSize;

}; // MeshStructure2D

} // namespace mesh_dem
} // namespace contourtree_augmented
} // namespace worklet
} // namespace viskores

#endif
