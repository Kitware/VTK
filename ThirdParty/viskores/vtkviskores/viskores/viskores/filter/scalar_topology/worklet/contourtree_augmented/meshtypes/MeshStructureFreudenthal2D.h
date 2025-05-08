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

#ifndef viskores_worklet_contourtree_augmented_meshtypes_MeshStructureFreudenthal2D_h
#define viskores_worklet_contourtree_augmented_meshtypes_MeshStructureFreudenthal2D_h

#include <viskores/Pair.h>
#include <viskores/Types.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/Types.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/data_set_mesh/MeshStructure2D.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/meshtypes/freudenthal_2D/Types.h>


namespace viskores
{
namespace worklet
{
namespace contourtree_augmented
{

// Worklet for computing the sort indices from the sort order
class MeshStructureFreudenthal2D : public data_set_mesh::MeshStructure2D
{
public:
  using SortIndicesPortalType = IdArrayType::ReadPortalType;
  using EdgeBoundaryDetectionMasksPortalType =
    m2d_freudenthal::EdgeBoundaryDetectionMasksType::ReadPortalType;

  // Default constucture. Needed for the CUDA built to work
  VISKORES_EXEC_CONT
  MeshStructureFreudenthal2D()
    : data_set_mesh::MeshStructure2D()
    , GetMax(false)
    , NumIncidentEdges(m2d_freudenthal::N_INCIDENT_EDGES)
  {
  }

  // Main constructor used in the code
  MeshStructureFreudenthal2D(
    viskores::Id2 meshSize,
    viskores::Int32 nincident_edges,
    bool getmax,
    const IdArrayType& sortIndices,
    const IdArrayType& SortOrder,
    const m2d_freudenthal::EdgeBoundaryDetectionMasksType& EdgeBoundaryDetectionMasksIn,
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token)
    : data_set_mesh::MeshStructure2D(meshSize)
    , GetMax(getmax)
    , NumIncidentEdges(nincident_edges)
  {
    this->SortIndicesPortal = sortIndices.PrepareForInput(device, token);
    this->SortOrderPortal = SortOrder.PrepareForInput(device, token);
    this->EdgeBoundaryDetectionMasksPortal =
      EdgeBoundaryDetectionMasksIn.PrepareForInput(device, token);
  }

  VISKORES_EXEC
  viskores::Id GetMaxNumberOfNeighbours() const { return m2d_freudenthal::N_INCIDENT_EDGES; }

  VISKORES_EXEC
  inline viskores::Id GetNeighbourIndex(viskores::Id sortIndex, viskores::Id edgeNo) const
  { // GetNeighbourIndex
    viskores::Id meshIndex = this->SortOrderPortal.Get(sortIndex);
    switch (edgeNo)
    {
      case 0:
        return this->SortIndicesPortal.Get(meshIndex + 1); // [1]    , [0] + 1
      case 1:
        return this->SortIndicesPortal.Get(meshIndex + this->MeshSize[0] + 1); // [1] + 1, [0] + 1
      case 2:
        return this->SortIndicesPortal.Get(meshIndex + this->MeshSize[0]); // [1] + 1, [0]
      case 3:
        return this->SortIndicesPortal.Get(meshIndex - 1); // [1]    , [0] - 1
      case 4:
        return this->SortIndicesPortal.Get(meshIndex - this->MeshSize[0] - 1); // [1] - 1, [0] - 1
      case 5:
        return this->SortIndicesPortal.Get(meshIndex - this->MeshSize[0]); // [1] - 1, [0]
      default:
        return -1; // TODO How to generate a meaningful error message from a device (in particular when using CUDA?)
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

  // sets outgoing paths for saddles
  VISKORES_EXEC
  inline viskores::Id GetExtremalNeighbour(viskores::Id sortIndex) const
  { // GetExtremalNeighbour()
    using namespace m2d_freudenthal;
    // convert to a mesh index
    viskores::Id meshIndex = SortOrderPortal.Get(sortIndex);

    // get the row and column
    viskores::Id2 pos = this->VertexPos(meshIndex);
    viskores::Int8 boundaryConfig = ((pos[0] == 0) ? LeftBit : 0) |
      ((pos[0] == this->MeshSize[0] - 1) ? RightBit : 0) | ((pos[1] == 0) ? TopBit : 0) |
      ((pos[1] == this->MeshSize[1] - 1) ? BottomBit : 0);

    // in what follows, the boundary conditions always reset wasAscent
    for (viskores::Id edgeNo = 0; edgeNo < this->NumIncidentEdges; edgeNo++)
    { // per edge
      // ignore if at edge of data
      if (!(boundaryConfig & EdgeBoundaryDetectionMasksPortal.Get(edgeNo)))
      {
        // calculate neighbour's ID and sort order
        viskores::Id nbrSortIndex = GetNeighbourIndex(sortIndex, edgeNo);

        // if it's not a valid destination, ignore it
        if (GetMax ? (nbrSortIndex > sortIndex) : (nbrSortIndex < sortIndex))
        {
          // and save the destination
          return nbrSortIndex;
        }
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
    using namespace m2d_freudenthal;
    // get data portals
    // convert to a mesh index
    viskores::Id meshIndex = SortOrderPortal.Get(sortIndex);

    // get the row and column
    viskores::Id2 pos = this->VertexPos(meshIndex);
    viskores::Int8 boundaryConfig = ((pos[0] == 0) ? LeftBit : 0) |
      ((pos[0] == this->MeshSize[0] - 1) ? RightBit : 0) | ((pos[1] == 0) ? TopBit : 0) |
      ((pos[1] == this->MeshSize[1] - 1) ? BottomBit : 0);

    // and initialise the mask
    viskores::Id neighbourhoodMask = 0;
    // in what follows, the boundary conditions always reset wasAscent
    for (viskores::Id edgeNo = 0; edgeNo < N_INCIDENT_EDGES; edgeNo++)
    { // per edge
      // ignore if at edge of data
      if (!(boundaryConfig & EdgeBoundaryDetectionMasksPortal.Get(edgeNo)))
      {
        // calculate neighbour's ID and sort order
        viskores::Id nbrSortIndex = GetNeighbourIndex(sortIndex, edgeNo);

        // if it's not a valid destination, ignore it
        if (getMaxComponents ? (nbrSortIndex > sortIndex) : (nbrSortIndex < sortIndex))
        {
          // now set the flag in the neighbourhoodMask
          neighbourhoodMask |= viskores::Id{ 1 } << edgeNo;
        }
      }
    } // per edge

    // we now know which edges are outbound, so we count to get the outdegree
    viskores::Id outDegree = 0;
    viskores::Id neighbourComponentMask = 0;
    // special case for local minimum
    if (neighbourhoodMask == 0x3F)
    {
      outDegree = 1;
    }
    else
    { // not a local minimum
      if ((neighbourhoodMask & 0x30) == 0x20)
      {
        ++outDegree;
        neighbourComponentMask |= viskores::Id{ 1 } << 5;
      }
      if ((neighbourhoodMask & 0x18) == 0x10)
      {
        ++outDegree;
        neighbourComponentMask |= viskores::Id{ 1 } << 4;
      }
      if ((neighbourhoodMask & 0x0C) == 0x08)
      {
        ++outDegree;
        neighbourComponentMask |= viskores::Id{ 1 } << 3;
      }
      if ((neighbourhoodMask & 0x06) == 0x04)
      {
        ++outDegree;
        neighbourComponentMask |= viskores::Id{ 1 } << 2;
      }
      if ((neighbourhoodMask & 0x03) == 0x02)
      {
        ++outDegree;
        neighbourComponentMask |= viskores::Id{ 1 } << 1;
      }
      if ((neighbourhoodMask & 0x21) == 0x01)
      {
        ++outDegree;
        neighbourComponentMask |= viskores::Id{ 1 } << 0;
      }
    } // not a local minimum
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
  bool GetMax;
  viskores::Id NumIncidentEdges;

}; // ExecutionObjec_MeshStructure_3Dt

} // namespace contourtree_augmented
} // namespace worklet
} // namespace viskores

#endif
