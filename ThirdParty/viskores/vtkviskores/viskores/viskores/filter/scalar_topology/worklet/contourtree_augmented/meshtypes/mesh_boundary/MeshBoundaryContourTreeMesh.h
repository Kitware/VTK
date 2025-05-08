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

// This header contains a collection of classes used to describe the boundary
// of a mesh, for each main mesh type (i.e., 2D, 3D, and ContourTreeMesh).
// For each mesh type, there are two classes, the actual boundary desriptor
// class and an ExectionObject class with the PrepareForInput function that
// Viskores expects to generate the object for the execution environment.

#ifndef viskores_worklet_contourtree_augmented_mesh_boundary_mesh_boundary_contour_tree_mesh_h
#define viskores_worklet_contourtree_augmented_mesh_boundary_mesh_boundary_contour_tree_mesh_h

#include <cstdlib>

#include <viskores/cont/ExecutionObjectBase.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/Types.h>

namespace viskores
{
namespace worklet
{
namespace contourtree_augmented
{

class MeshBoundaryContourTreeMesh
{
public:
  using IndicesPortalType = IdArrayType::ReadPortalType;

  VISKORES_EXEC_CONT
  MeshBoundaryContourTreeMesh() {}

  VISKORES_CONT
  MeshBoundaryContourTreeMesh(const IdArrayType& globalMeshIndex,
                              viskores::Id3 globalSize,
                              viskores::Id3 minIdx,
                              viskores::Id3 maxIdx,
                              viskores::cont::DeviceAdapterId device,
                              viskores::cont::Token& token)
    : GlobalSize(globalSize)
    , MinIdx(minIdx)
    , MaxIdx(maxIdx)
  {
    assert(this->GlobalSize[0] > 0 && this->GlobalSize[1] > 0);
    this->GlobalMeshIndexPortal = globalMeshIndex.PrepareForInput(device, token);
  }

  VISKORES_EXEC_CONT
  bool LiesOnBoundary(const viskores::Id index) const
  {
    viskores::Id global_idx = this->GlobalMeshIndexPortal.Get(index);
    viskores::Id3 mesh_idx{
      viskores::Id(global_idx % this->GlobalSize[0]),
      viskores::Id((global_idx % (this->GlobalSize[0] * this->GlobalSize[1])) /
                   this->GlobalSize[0]),
      viskores::Id(global_idx / (this->GlobalSize[0] * this->GlobalSize[1]))
    };

    // FIXME: Probably better communicate n_dims in constructor or make it a template parameter
    // Or at least be more consistent in setting this in MinIdx/MaxIdx. Currently MinIdx[2] is 0
    // and MaxIdx[2] is -1 for a 2D data set.
    const auto n_dims = (MaxIdx[2] == -1) ? 2 : 3;
    for (int d = 0; d < n_dims; ++d)
    {
      if (this->MinIdx[d] != this->MaxIdx[d] &&
          (mesh_idx[d] == this->MinIdx[d] || mesh_idx[d] == this->MaxIdx[d]))
      {
        return true;
      }
    }
    return false;
  }

  VISKORES_EXEC_CONT
  bool IsNecessary(const viskores::Id idx) const { return this->LiesOnBoundary(idx); }

private:
  // mesh block parameters
  viskores::Id3 GlobalSize;
  viskores::Id3 MinIdx;
  viskores::Id3 MaxIdx;
  IndicesPortalType GlobalMeshIndexPortal;
};


class MeshBoundaryContourTreeMeshExec : public viskores::cont::ExecutionObjectBase
{
public:
  VISKORES_EXEC_CONT
  MeshBoundaryContourTreeMeshExec(const IdArrayType& globalMeshIndex,
                                  viskores::Id3 globalSize,
                                  viskores::Id3 minIdx,
                                  viskores::Id3 maxIdx)
    : GlobalMeshIndex(globalMeshIndex)
    , GlobalSize(globalSize)
    , MinIdx(minIdx)
    , MaxIdx(maxIdx)
  {
  }

  VISKORES_CONT MeshBoundaryContourTreeMesh
  PrepareForExecution(viskores::cont::DeviceAdapterId device, viskores::cont::Token& token) const
  {
    return MeshBoundaryContourTreeMesh(
      this->GlobalMeshIndex, this->GlobalSize, this->MinIdx, this->MaxIdx, device, token);
  }

private:
  const IdArrayType& GlobalMeshIndex;
  viskores::Id3 GlobalSize;
  viskores::Id3 MinIdx;
  viskores::Id3 MaxIdx;
};


} // namespace contourtree_augmented
} // worklet
} // viskores

#endif
