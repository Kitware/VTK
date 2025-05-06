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
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
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

#ifndef viskores_worklet_contourtree_augmented_mesh_dem_triangulation_contourtree_mesh_execution_obect_mesh_structure_h
#define viskores_worklet_contourtree_augmented_mesh_dem_triangulation_contourtree_mesh_execution_obect_mesh_structure_h

#include <viskores/Pair.h>
#include <viskores/Types.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/Types.h>



//Define namespace alias for the freudenthal types to make the code a bit more readable
namespace cpp2_ns = viskores::worklet::contourtree_augmented;

namespace viskores
{
namespace worklet
{
namespace contourtree_augmented
{
namespace mesh_dem_contourtree_mesh_inc
{

// Worklet for computing the sort indices from the sort order
class MeshStructureContourTreeMesh
{
public:
  using IdArrayPortalType = typename cpp2_ns::IdArrayType::ReadPortalType;

  // Default constucture. Needed for the CUDA built to work
  VISKORES_EXEC_CONT
  MeshStructureContourTreeMesh()
    : GetMax(false)
  {
  }

  // Main constructure used in the code
  VISKORES_CONT
  MeshStructureContourTreeMesh(const cpp2_ns::IdArrayType neighborConnectivity,
                               const cpp2_ns::IdArrayType neighborOffsets,
                               const viskores::Id maxNeighbors,
                               bool getMax,
                               viskores::cont::DeviceAdapterId device,
                               viskores::cont::Token& token)
    : MaxNeighbors(maxNeighbors)
    , GetMax(getMax)
  {
    this->NeighborConnectivityPortal = neighborConnectivity.PrepareForInput(device, token);
    this->NeighborOffsetsPortal = neighborOffsets.PrepareForInput(device, token);
  }

  VISKORES_EXEC
  viskores::Id GetNumberOfVertices() const
  {
    return this->NeighborOffsetsPortal.GetNumberOfValues() - 1;
  }

  VISKORES_EXEC
  viskores::Id GetMaxNumberOfNeighbours() const { return this->MaxNeighbors; }

  VISKORES_EXEC
  inline viskores::Id GetNeighbourIndex(viskores::Id sortIndex, viskores::Id neighborNo) const
  { // GetNeighbourIndex
    return NeighborConnectivityPortal.Get(NeighborOffsetsPortal.Get(sortIndex) + neighborNo);
  } // GetNeighbourIndex

  // sets outgoing paths for saddles
  VISKORES_EXEC
  inline viskores::Id GetExtremalNeighbour(viskores::Id sortIndex) const
  { // GetExtremalNeighbour()
    viskores::Id neighborsBeginIndex = NeighborOffsetsPortal.Get(sortIndex);
    viskores::Id neighborsEndIndex = NeighborOffsetsPortal.Get(sortIndex + 1) - 1;
    viskores::Id neighborsBegin = NeighborConnectivityPortal.Get(neighborsBeginIndex);
    viskores::Id neighborsEnd = NeighborConnectivityPortal.Get(neighborsEndIndex);

    if (neighborsBeginIndex == neighborsEndIndex + 1)
    { // Empty list of neighbors, this should never happen
      return sortIndex | TERMINAL_ELEMENT;
    }
    else
    {
      viskores::Id ret;
      if (this->GetMax)
      {
        ret = neighborsEnd;
        if (ret < sortIndex)
          ret = sortIndex | TERMINAL_ELEMENT;
      }
      else
      {
        ret = neighborsBegin;
        if (ret > sortIndex)
          ret = sortIndex | TERMINAL_ELEMENT;
      }
      return ret;
    }
  } // GetExtremalNeighbour()

  // NOTE/FIXME: The following also iterates over all values and could be combined with GetExtremalNeighbour(). However, the
  // results are needed at different places and splitting the two functions leads to a cleaner design
  VISKORES_EXEC
  inline viskores::Pair<viskores::Id, viskores::Id> GetNeighbourComponentsMaskAndDegree(
    viskores::Id sortIndex,
    bool getMaxComponents) const
  { // GetNeighbourComponentsMaskAndDegree()
    viskores::Id neighborsBeginIndex = NeighborOffsetsPortal.Get(sortIndex);
    viskores::Id neighborsEndIndex = NeighborOffsetsPortal.Get(sortIndex + 1);
    viskores::Id numNeighbours = neighborsEndIndex - neighborsBeginIndex;
    viskores::Id outDegree = 0;
    viskores::Id neighborComponentMask = 0;
    viskores::Id currNeighbour = 0;
    for (viskores::Id nbrNo = 0; nbrNo < numNeighbours; ++nbrNo)
    {
      currNeighbour = NeighborConnectivityPortal.Get(neighborsBeginIndex + nbrNo);
      if ((getMaxComponents && (currNeighbour > sortIndex)) ||
          (!getMaxComponents && (currNeighbour < sortIndex)))
      {
        ++outDegree;
        neighborComponentMask |= viskores::Id{ 1 } << nbrNo;
      }
    }
    return viskores::Pair<viskores::Id, viskores::Id>{ neighborComponentMask, outDegree };
  } // GetNeighbourComponentsMaskAndDegree()

private:
  IdArrayPortalType NeighborConnectivityPortal;
  IdArrayPortalType NeighborOffsetsPortal;
  viskores::Id MaxNeighbors;
  bool GetMax;

}; // ExecutionObjec_MeshStructure_3Dt

} // namespace mesh_dem_2d_freudenthal_inc
} // namespace contourtree_augmented
} // namespace worklet
} // namespace viskores

#endif
