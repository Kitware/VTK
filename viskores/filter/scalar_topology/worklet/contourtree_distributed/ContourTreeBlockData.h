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

#ifndef viskores_worklet_contourtree_distributed_contourtreeblockdata_h
#define viskores_worklet_contourtree_distributed_contourtreeblockdata_h

#include <viskores/Types.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/Types.h>

// clang-format off
VISKORES_THIRDPARTY_PRE_INCLUDE
#include <viskores/thirdparty/diy/diy.h>
VISKORES_THIRDPARTY_POST_INCLUDE
// clang-format on


namespace viskores
{
namespace worklet
{
namespace contourtree_distributed
{
template <typename FieldType>
struct ContourTreeBlockData
{
  static void* create() { return new ContourTreeBlockData<FieldType>; }
  static void destroy(void* b) { delete static_cast<ContourTreeBlockData<FieldType>*>(b); }

  // ContourTreeMesh data
  viskores::Id NumVertices;
  viskores::cont::ArrayHandle<FieldType> SortedValue;
  viskores::worklet::contourtree_augmented::IdArrayType GlobalMeshIndex;
  viskores::worklet::contourtree_augmented::IdArrayType NeighborConnectivity;
  viskores::worklet::contourtree_augmented::IdArrayType NeighborOffsets;
  viskores::Id MaxNeighbors;

  // Block metadata
  viskores::Id3 BlockOrigin;            // Origin of the data block
  viskores::Id3 BlockSize;              // Extends of the data block
  viskores::Id3 GlobalSize;             // Extends of the global mesh
  unsigned int ComputeRegularStructure; // pass through augmentation setting
};
} // namespace contourtree_distributed
} // namespace worklet
} // namespace viskores


namespace viskoresdiy
{

// Struct to serialize ContourBlockData objects (i.e., load/save) needed in parralle for DIY
template <typename FieldType>
struct Serialization<viskores::worklet::contourtree_distributed::ContourTreeBlockData<FieldType>>
{
  static void save(
    viskoresdiy::BinaryBuffer& bb,
    const viskores::worklet::contourtree_distributed::ContourTreeBlockData<FieldType>& block)
  {
    viskoresdiy::save(bb, block.NumVertices);
    viskoresdiy::save(bb, block.SortedValue);
    viskoresdiy::save(bb, block.GlobalMeshIndex);
    viskoresdiy::save(bb, block.NeighborConnectivity);
    viskoresdiy::save(bb, block.NeighborOffsets);
    viskoresdiy::save(bb, block.MaxNeighbors);
    viskoresdiy::save(bb, block.BlockOrigin);
    viskoresdiy::save(bb, block.BlockSize);
    viskoresdiy::save(bb, block.GlobalSize);
    viskoresdiy::save(bb, block.ComputeRegularStructure);
  }

  static void load(
    viskoresdiy::BinaryBuffer& bb,
    viskores::worklet::contourtree_distributed::ContourTreeBlockData<FieldType>& block)
  {
    viskoresdiy::load(bb, block.NumVertices);
    viskoresdiy::load(bb, block.SortedValue);
    viskoresdiy::load(bb, block.GlobalMeshIndex);
    viskoresdiy::load(bb, block.NeighborConnectivity);
    viskoresdiy::load(bb, block.NeighborOffsets);
    viskoresdiy::load(bb, block.MaxNeighbors);
    viskoresdiy::load(bb, block.BlockOrigin);
    viskoresdiy::load(bb, block.BlockSize);
    viskoresdiy::load(bb, block.GlobalSize);
    viskoresdiy::load(bb, block.ComputeRegularStructure);
  }
};

} // namespace mangled_viskoresdiy_namespace


#endif
