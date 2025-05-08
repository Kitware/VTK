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

#ifndef viskores_worklet_contourtree_augmented_meshtypes_MeshStructureFreudenthal3D_h
#define viskores_worklet_contourtree_augmented_meshtypes_MeshStructureFreudenthal3D_h

#include <viskores/Pair.h>
#include <viskores/Types.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleGroupVec.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/Types.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/data_set_mesh/MeshStructure3D.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/meshtypes/freudenthal_3D/Types.h>


namespace viskores
{
namespace worklet
{
namespace contourtree_augmented
{

// Worklet for computing the sort indices from the sort order
class MeshStructureFreudenthal3D : public data_set_mesh::MeshStructure3D
{
public:
  using SortIndicesPortalType = IdArrayType::ReadPortalType;

  using EdgeBoundaryDetectionMasksPortalType =
    m3d_freudenthal::EdgeBoundaryDetectionMasksType::ReadPortalType;

  using NeighbourOffsetsPortalType = m3d_freudenthal::NeighbourOffsetsType::ReadPortalType;

  using LinkComponentCaseTablePortalType =
    m3d_freudenthal::LinkComponentCaseTableType::ReadPortalType;

  // Default constructor needed to make the CUDA build work
  VISKORES_EXEC_CONT
  MeshStructureFreudenthal3D()
    : data_set_mesh::MeshStructure3D()
    , GetMax(false)
    , NumIncidentEdge(m3d_freudenthal::N_INCIDENT_EDGES)
  {
  }

  // Main constructore used in the code
  MeshStructureFreudenthal3D(
    viskores::Id3 meshSize,
    viskores::Id nincident_edges,
    bool getmax,
    const IdArrayType& sortIndices,
    const IdArrayType& sortOrder,
    const m3d_freudenthal::EdgeBoundaryDetectionMasksType& edgeBoundaryDetectionMasksIn,
    const m3d_freudenthal::NeighbourOffsetsType& neighbourOffsetsIn,
    const m3d_freudenthal::LinkComponentCaseTableType& linkComponentCaseTableIn,
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token)
    : data_set_mesh::MeshStructure3D(meshSize)
    , GetMax(getmax)
    , NumIncidentEdge(nincident_edges)
  {
    this->SortIndicesPortal = sortIndices.PrepareForInput(device, token);
    this->SortOrderPortal = sortOrder.PrepareForInput(device, token);
    this->EdgeBoundaryDetectionMasksPortal =
      edgeBoundaryDetectionMasksIn.PrepareForInput(device, token);
    this->NeighbourOffsetsPortal = neighbourOffsetsIn.PrepareForInput(device, token);
    this->LinkComponentCaseTablePortal = linkComponentCaseTableIn.PrepareForInput(device, token);
  }

  VISKORES_EXEC
  viskores::Id GetMaxNumberOfNeighbours() const { return m3d_freudenthal::N_INCIDENT_EDGES; }


  VISKORES_EXEC
  inline viskores::Id GetNeighbourIndex(viskores::Id sortIndex, viskores::Id edgeNo) const
  { // GetNeighbourIndex
    viskores::Id meshIndex = SortOrderPortal.Get(sortIndex);
    // NOTE: Offsets are stored in "reversed" zyx [2][1][0] order (remaining artifact from
    // using slices, rows, columns instead of xyz/[0][1][2])
    return SortIndicesPortal.Get(meshIndex +
                                 (NeighbourOffsetsPortal.Get(edgeNo)[0] * this->MeshSize[1] +
                                  NeighbourOffsetsPortal.Get(edgeNo)[1]) *
                                   this->MeshSize[0] +
                                 NeighbourOffsetsPortal.Get(edgeNo)[2]);
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

  // sets outgoing paths for saddles
  VISKORES_EXEC
  inline viskores::Id GetExtremalNeighbour(viskores::Id sortIndex) const
  { //  GetExtremalNeighbour()
    // convert to a mesh index
    using namespace m3d_freudenthal;
    viskores::Id meshIndex = SortOrderPortal.Get(sortIndex);

    viskores::Id3 pos = this->VertexPos(meshIndex);
    viskores::Int8 boundaryConfig = ((pos[0] == 0) ? LeftBit : 0) |
      ((pos[0] == this->MeshSize[0] - 1) ? RightBit : 0) | ((pos[1] == 0) ? TopBit : 0) |
      ((pos[1] == this->MeshSize[1] - 1) ? BottomBit : 0) | ((pos[2] == 0) ? FrontBit : 0) |
      ((pos[2] == this->MeshSize[2] - 1) ? BackBit : 0);

    // in what follows, the boundary conditions always reset wasAscent
    // loop downwards so that we pick the same edges as previous versions
    for (viskores::Id nbrNo = 0; nbrNo < NumIncidentEdge; ++nbrNo)
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


  // NOTE/FIXME: The following also iterates over all values and could be combined with GetExtremalNeighbour(). However, the
  // results are needed at different places and splitting the two functions leads to a cleaner design
  VISKORES_EXEC
  inline viskores::Pair<viskores::Id, viskores::Id> GetNeighbourComponentsMaskAndDegree(
    viskores::Id sortIndex,
    bool getMaxComponents) const
  { // GetNeighbourComponentsMaskAndDegree()
    // convert to a meshIndex
    using namespace m3d_freudenthal;
    viskores::Id meshIndex = SortOrderPortal.Get(sortIndex);

    // get the row and column
    viskores::Id3 pos = this->VertexPos(meshIndex);
    viskores::Int8 boundaryConfig = ((pos[0] == 0) ? LeftBit : 0) |
      ((pos[0] == this->MeshSize[0] - 1) ? RightBit : 0) | ((pos[1] == 0) ? TopBit : 0) |
      ((pos[1] == this->MeshSize[1] - 1) ? BottomBit : 0) | ((pos[2] == 0) ? FrontBit : 0) |
      ((pos[2] == this->MeshSize[2] - 1) ? BackBit : 0);

    // Initialize "union find"
    viskores::Id caseNo = 0;

    // Compute components of upper link
    for (int edgeNo = 0; edgeNo < N_INCIDENT_EDGES; ++edgeNo)
    {
      if (!(boundaryConfig & EdgeBoundaryDetectionMasksPortal.Get(edgeNo)))
      {
        viskores::Id nbrSortIndex = GetNeighbourIndex(sortIndex, edgeNo);
        if (getMaxComponents ? (sortIndex < nbrSortIndex) : (sortIndex > nbrSortIndex))
        {
          caseNo |= viskores::Id{ 1 } << edgeNo;
        }
      } // inside grid
    }   // for each edge

    // we now know which edges are ascents, so we count to get the updegree
    viskores::Id outDegree = 0;
    viskores::Id neighbourComponentMask = 0;

    for (int nbrNo = 0; nbrNo < N_INCIDENT_EDGES; ++nbrNo)
      if (LinkComponentCaseTablePortal.Get(caseNo) & (1 << nbrNo))
      {
        outDegree++;
        neighbourComponentMask |= viskores::Id{ 1 } << nbrNo;
      }

    viskores::Pair<viskores::Id, viskores::Id> re(neighbourComponentMask, outDegree);
    return re;
  } // GetNeighbourComponentsMaskAndDegree()


#if (defined(VISKORES_GCC) || defined(VISKORES_CLANG))
#pragma GCC diagnostic pop
#endif // gcc || clang


private:
  SortIndicesPortalType SortIndicesPortal;
  SortIndicesPortalType SortOrderPortal;
  EdgeBoundaryDetectionMasksPortalType EdgeBoundaryDetectionMasksPortal;
  NeighbourOffsetsPortalType NeighbourOffsetsPortal;
  LinkComponentCaseTablePortalType LinkComponentCaseTablePortal;
  bool GetMax;
  viskores::Id NumIncidentEdge;

}; // ExecutionObjec_MeshStructure_3Dt

} // namespace contourtree_augmented
} // namespace worklet
} // namespace viskores

#endif
