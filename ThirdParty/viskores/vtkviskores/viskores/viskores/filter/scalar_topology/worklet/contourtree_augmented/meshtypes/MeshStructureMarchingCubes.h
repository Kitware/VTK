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

#ifndef viskores_worklet_contourtree_augmented_meshtypes_MeshStructureMarchingCubes_h
#define viskores_worklet_contourtree_augmented_meshtypes_MeshStructureMarchingCubes_h

#include <viskores/Pair.h>
#include <viskores/Types.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleGroupVec.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/Types.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/data_set_mesh/MeshStructure3D.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/meshtypes/marchingcubes_3D/Types.h>

namespace viskores
{
namespace worklet
{
namespace contourtree_augmented
{

// Worklet for computing the sort indices from the sort order
class MeshStructureMarchingCubes : public data_set_mesh::MeshStructure3D
{
public:
  // EdgeBoundaryDetectionMasks types
  using EdgeBoundaryDetectionMasksPortalType =
    m3d_marchingcubes::EdgeBoundaryDetectionMasksType::ReadPortalType;

  // Sort indicies types
  using SortIndicesPortalType = IdArrayType::ReadPortalType;

  // CubeVertexPermutations types
  using CubeVertexPermutationsPortalType =
    m3d_marchingcubes::CubeVertexPermutationsType::ReadPortalType;

  // linkVertexConnection types
  using LinkVertexConnectionsPortalType =
    m3d_marchingcubes::LinkVertexConnectionsType::ReadPortalType;
  // inCubeConnection types

  using InCubeConnectionsPortalType = m3d_marchingcubes::InCubeConnectionsType::ReadPortalType;

  // Default constructor needed to make the CUDA build work
  VISKORES_EXEC_CONT
  MeshStructureMarchingCubes()
    : data_set_mesh::MeshStructure3D()
    , GetMax(false)
  {
  }

  // Main constructore used in the code
  MeshStructureMarchingCubes(
    viskores::Id3 meshSize,
    bool getmax,
    const IdArrayType& sortIndices,
    const IdArrayType& sortOrder,
    const m3d_marchingcubes::EdgeBoundaryDetectionMasksType& EdgeBoundaryDetectionMasksIn,
    const m3d_marchingcubes::CubeVertexPermutationsType& CubeVertexPermutationsIn,
    const m3d_marchingcubes::LinkVertexConnectionsType& LinkVertexConnectionsSixIn,
    const m3d_marchingcubes::LinkVertexConnectionsType& LinkVertexConnectionsEighteenIn,
    const m3d_marchingcubes::InCubeConnectionsType& InCubeConnectionsSixIn,
    const m3d_marchingcubes::InCubeConnectionsType& InCubeConnectionsEighteenIn,
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token)
    : data_set_mesh::MeshStructure3D(meshSize)
    , GetMax(getmax)
  {
    this->SortIndicesPortal = sortIndices.PrepareForInput(device, token);
    this->SortOrderPortal = sortOrder.PrepareForInput(device, token);
    this->EdgeBoundaryDetectionMasksPortal =
      EdgeBoundaryDetectionMasksIn.PrepareForInput(device, token);
    this->CubeVertexPermutationsPortal = CubeVertexPermutationsIn.PrepareForInput(device, token);
    this->LinkVertexConnectionsSixPortal =
      LinkVertexConnectionsSixIn.PrepareForInput(device, token);
    this->LinkVertexConnectionsEighteenPortal =
      LinkVertexConnectionsEighteenIn.PrepareForInput(device, token);
    this->InCubeConnectionsSixPortal = InCubeConnectionsSixIn.PrepareForInput(device, token);
    this->InCubeConnectionsEighteenPortal =
      InCubeConnectionsEighteenIn.PrepareForInput(device, token);
  }

  VISKORES_EXEC
  viskores::Id GetMaxNumberOfNeighbours() const { return m3d_marchingcubes::N_FACE_NEIGHBOURS; }

  VISKORES_EXEC
  inline viskores::Id GetNeighbourIndex(viskores::Id sortIndex, viskores::Id nbrNo) const
  {
    using namespace m3d_marchingcubes;
    viskores::Id meshIndex = this->SortOrderPortal.Get(sortIndex);
    const viskores::Id3 strides{ 1, this->MeshSize[0], this->MeshSize[0] * this->MeshSize[1] };

    // GetNeighbourIndex
    switch (nbrNo)
    {
      // Edge connected neighbours
      case 0: // {  0,  0, -1 }
        return SortIndicesPortal.Get(meshIndex - strides[2]);
      case 1: // {  0, -1,  0 }
        return SortIndicesPortal.Get(meshIndex - strides[1]);
      case 2: // { -1,  0,  0 }
        return SortIndicesPortal.Get(meshIndex - strides[0]);
      case 3: // {  1,  0,  0 }
        return SortIndicesPortal.Get(meshIndex + strides[0]);
      case 4: // {  0,  1,  0 }
        return SortIndicesPortal.Get(meshIndex + strides[1]);
      case 5: // {  0,  0,  1 }
        return SortIndicesPortal.Get(meshIndex + strides[2]);
      // Face connected neighbours
      case 6: // {  0, -1, -1 }
        return SortIndicesPortal.Get(meshIndex - strides[2] - strides[1]);
      case 7: // { -1,  0, -1 }
        return SortIndicesPortal.Get(meshIndex - strides[2] - strides[0]);
      case 8: // {  1,  0, -1 }
        return SortIndicesPortal.Get(meshIndex - strides[2] + strides[0]);
      case 9: // {  0,  1, -1 }
        return SortIndicesPortal.Get(meshIndex - strides[2] + strides[1]);
      case 10: // { -1, -1,  0 }
        return SortIndicesPortal.Get(meshIndex - strides[1] - strides[0]);
      case 11: // {  1, -1,  0 }
        return SortIndicesPortal.Get(meshIndex - strides[1] + strides[0]);
      case 12: // { -1,  1,  0 }
        return SortIndicesPortal.Get(meshIndex + strides[1] - strides[0]);
      case 13: // {  1,  1,  0 }
        return SortIndicesPortal.Get(meshIndex + strides[1] + strides[0]);
      case 14: // {  0, -1,  1 }
        return SortIndicesPortal.Get(meshIndex + strides[2] - strides[1]);
      case 15: // { -1,  0,  1 }
        return SortIndicesPortal.Get(meshIndex + strides[2] - 1);
      case 16: // {  1,  0,  1 }
        return SortIndicesPortal.Get(meshIndex + strides[2] + 1);
      case 17: // {  0,  1,  1 }
        return SortIndicesPortal.Get(meshIndex + strides[2] + strides[1]);
      // Diagonal connected neighbours
      case 18: // { -1, -1, -1 }
        return SortIndicesPortal.Get(meshIndex - strides[2] - strides[1] - strides[0]);
      case 19: // {  1, -1, -1 }
        return SortIndicesPortal.Get(meshIndex - strides[2] - strides[1] + strides[0]);
      case 20: // { -1,  1, -1 }
        return SortIndicesPortal.Get(meshIndex - strides[2] + strides[1] - strides[0]);
      case 21: // {  1,  1, -1 }
        return SortIndicesPortal.Get(meshIndex - strides[2] + strides[1] + strides[0]);
      case 22: // { -1, -1,  1 }
        return SortIndicesPortal.Get(meshIndex + strides[2] - strides[1] - strides[0]);
      case 23: // {  1, -1,  1 }
        return SortIndicesPortal.Get(meshIndex + strides[2] - strides[1] + strides[0]);
      case 24: // { -1,  1,  1 }
        return SortIndicesPortal.Get(meshIndex + strides[2] + strides[1] - strides[0]);
      case 25: // {  1,  1,  1 }
        return SortIndicesPortal.Get(meshIndex + strides[2] + strides[1] + strides[0]);
      default:
        VISKORES_ASSERT(false);
        // TODO/FIXME: Should probaly return an invalid value or throw an exception instead
        return meshIndex; // Need to error out here
    }
  } // GetNeighbourIndex

// Disable conversion warnings for Add, Subtract, Multiply, Divide on GCC only.
// GCC creates false positive warnings for signed/unsigned char* operations.
// This occurs because the values are implicitly casted up to int's for the
// operation, and than  casted back down to char's when return.
// This causes a false positive warning, even when the values is within
// the value types range
#if (defined(VISKORES_GCC) || defined(VISKORES_CLANG))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#endif // gcc || clang

  VISKORES_EXEC
  inline viskores::Id GetExtremalNeighbour(viskores::Id sortIndex) const
  {
    using namespace m3d_marchingcubes;
    // GetExtremalNeighbour()
    // convert to a sort index
    viskores::Id meshIndex = SortOrderPortal.Get(sortIndex);

    viskores::Id3 pos = this->VertexPos(meshIndex);
    viskores::Int8 boundaryConfig = ((pos[0] == 0) ? LeftBit : 0) |
      ((pos[0] == this->MeshSize[0] - 1) ? RightBit : 0) | ((pos[1] == 0) ? TopBit : 0) |
      ((pos[1] == this->MeshSize[1] - 1) ? BottomBit : 0) | ((pos[2] == 0) ? FrontBit : 0) |
      ((pos[2] == this->MeshSize[2] - 1) ? BackBit : 0);

    // in what follows, the boundary conditions always reset wasAscent
    // loop downwards so that we pick the same edges as previous versions
    const int nNeighbours = (!GetMax ? N_FACE_NEIGHBOURS : N_EDGE_NEIGHBOURS);
    for (viskores::Id nbrNo = 0; nbrNo < nNeighbours; ++nbrNo)
    {
      // only consider valid edges
      if (!(boundaryConfig & EdgeBoundaryDetectionMasksPortal.Get(nbrNo)))
      {
        viskores::Id nbrSortIndex = GetNeighbourIndex(sortIndex, nbrNo);
        // explicit test allows reversal between join and split trees
        if (GetMax ? (nbrSortIndex > sortIndex) : (nbrSortIndex < sortIndex))
        { // valid edge and outbound
          return nbrSortIndex;
        } // valid edge and outbound
      }
    } // per edge

    return sortIndex | TERMINAL_ELEMENT;
  } // GetExtremalNeighbour()

  VISKORES_EXEC
  inline viskores::Pair<viskores::Id, viskores::Id> GetNeighbourComponentsMaskAndDegree(
    viskores::Id sortIndex,
    bool getMaxComponents) const
  {
    using namespace m3d_marchingcubes;
    // GetNeighbourComponentsMaskAndDegree()
    // convert to a sort index
    viskores::Id meshIndex = SortOrderPortal.Get(sortIndex);

    viskores::Id3 pos = this->VertexPos(meshIndex);
    viskores::Int8 boundaryConfig = ((pos[0] == 0) ? LeftBit : 0) |
      ((pos[0] == this->MeshSize[0] - 1) ? RightBit : 0) | ((pos[1] == 0) ? TopBit : 0) |
      ((pos[1] == this->MeshSize[1] - 1) ? BottomBit : 0) | ((pos[2] == 0) ? FrontBit : 0) |
      ((pos[2] == this->MeshSize[2] - 1) ? BackBit : 0);

    // Initialize "union find"
    int parentId[N_ALL_NEIGHBOURS];

    // Compute components of upper link
    for (int edgeNo = 0; edgeNo < N_ALL_NEIGHBOURS; ++edgeNo)
    {
      if (!(boundaryConfig & EdgeBoundaryDetectionMasksPortal.Get(edgeNo)))
      {
        viskores::Id nbrSortIndex = GetNeighbourIndex(sortIndex, edgeNo);

        if (getMaxComponents ? (sortIndex < nbrSortIndex) : (sortIndex > nbrSortIndex))
        {
          parentId[edgeNo] = edgeNo;
        }
        else
        {
          parentId[edgeNo] = -1;
        }
      } // inside grid
      else
      {
        parentId[edgeNo] = -1;
      }
    } // for each edge

    for (viskores::UInt8 permIndex = 0; permIndex < CubeVertexPermutations_NumPermutations;
         permIndex++)
    {
      // Combpute connection configuration in each of the eight cubes
      // surrounding a vertex
      viskores::UInt8 caseNo = 0;
      for (int vtxNo = 0; vtxNo < 7; ++vtxNo)
      {
        if (parentId[CubeVertexPermutationsPortal.Get(permIndex)[vtxNo]] != -1)
        {
          caseNo |= (viskores::UInt8)(1 << vtxNo);
        }
      }

      const auto& vertex_permutation = CubeVertexPermutationsPortal.Get(permIndex);
      if (getMaxComponents)
      {
        for (int edgeNo = 0; edgeNo < 3; ++edgeNo)
        {
          if (InCubeConnectionsSixPortal.Get(caseNo) & (static_cast<viskores::Id>(1) << edgeNo))
          {
            const auto& edge = LinkVertexConnectionsSixPortal.Get(edgeNo);
            viskores::IdComponent edge0 = edge[0];
            viskores::IdComponent edge1 = edge[1];
            VISKORES_ASSERT(0 <= edge0 && edge0 < CubeVertexPermutations_PermVecLength);
            VISKORES_ASSERT(0 <= edge1 && edge1 < CubeVertexPermutations_PermVecLength);
            int root0 = vertex_permutation[edge0];
            int root1 = vertex_permutation[edge1];
            VISKORES_ASSERT(0 <= root0 && root0 < N_ALL_NEIGHBOURS);
            VISKORES_ASSERT(0 <= root1 && root1 < N_ALL_NEIGHBOURS);
            while (parentId[root0] != root0)
            {
              root0 = parentId[root0];
              VISKORES_ASSERT(0 <= root0 && root0 < N_ALL_NEIGHBOURS);
            }
            while (parentId[root1] != root1)
            {
              root1 = parentId[root1];
              VISKORES_ASSERT(0 <= root1 && root1 < N_ALL_NEIGHBOURS);
            }
            if (root0 != root1)
            {
              VISKORES_ASSERT(0 <= root1 && root1 < N_ALL_NEIGHBOURS);
              parentId[root1] = root0;
            }
          }
        }
      }
      else
      {
        for (int edgeNo = 0; edgeNo < 15; ++edgeNo)
        {
          if (InCubeConnectionsEighteenPortal.Get(caseNo) &
              (static_cast<viskores::Id>(1) << edgeNo))
          {
            const auto& edge = LinkVertexConnectionsEighteenPortal.Get(edgeNo);
            viskores::IdComponent edge0 = edge[0];
            viskores::IdComponent edge1 = edge[1];
            VISKORES_ASSERT(0 <= edge0 && edge0 < CubeVertexPermutations_PermVecLength);
            VISKORES_ASSERT(0 <= edge1 && edge1 < CubeVertexPermutations_PermVecLength);
            int root0 = vertex_permutation[edge0];
            int root1 = vertex_permutation[edge1];
            VISKORES_ASSERT(0 <= root0 && root0 < N_ALL_NEIGHBOURS);
            VISKORES_ASSERT(0 <= root1 && root1 < N_ALL_NEIGHBOURS);
            while (parentId[root0] != root0)
            {
              root0 = parentId[root0];
              VISKORES_ASSERT(0 <= root0 && root0 < N_ALL_NEIGHBOURS);
            }
            while (parentId[root1] != root1)
            {
              root1 = parentId[root1];
              VISKORES_ASSERT(0 <= root1 && root1 < N_ALL_NEIGHBOURS);
            }
            if (root0 != root1)
            {
              VISKORES_ASSERT(0 <= root1 && root1 < N_ALL_NEIGHBOURS);
              parentId[root1] = root0;
            }
          }
        }
      }
    }
    // we now know which edges are ascents, so we count to get the updegree
    viskores::Id outDegree = 0;
    viskores::Id neighbourComponentMask = 0;

    // Find one representaant for each connected compomnent in "link"
    const int nNeighbours = getMaxComponents ? 6 : 18;
    for (int nbrNo = 0; nbrNo < nNeighbours; ++nbrNo)
    {
      if (parentId[nbrNo] == nbrNo)
      {
        outDegree++;
        neighbourComponentMask |= static_cast<viskores::Id>(1) << nbrNo;
      }
    }

    return viskores::make_Pair(neighbourComponentMask, outDegree);
  } // GetNeighbourComponentsMaskAndDegree()

#if (defined(VISKORES_GCC) || defined(VISKORES_CLANG))
#pragma GCC diagnostic pop
#endif // gcc || clang



private:
  SortIndicesPortalType SortIndicesPortal;
  SortIndicesPortalType SortOrderPortal;
  EdgeBoundaryDetectionMasksPortalType EdgeBoundaryDetectionMasksPortal;
  CubeVertexPermutationsPortalType CubeVertexPermutationsPortal;
  LinkVertexConnectionsPortalType LinkVertexConnectionsSixPortal;
  LinkVertexConnectionsPortalType LinkVertexConnectionsEighteenPortal;
  InCubeConnectionsPortalType InCubeConnectionsSixPortal;
  InCubeConnectionsPortalType InCubeConnectionsEighteenPortal;
  bool GetMax;


}; // MeshStructureMarchingCubes

} // namespace contourtree_augmented
} // namespace worklet
} // namespace viskores

#endif
