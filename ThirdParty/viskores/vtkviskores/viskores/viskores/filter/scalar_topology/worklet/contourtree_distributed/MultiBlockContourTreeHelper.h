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

#ifndef viskores_worklet_contourtree_distributed_multiblockcontourtreehelper_h
#define viskores_worklet_contourtree_distributed_multiblockcontourtreehelper_h

#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/Types.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/meshtypes/ContourTreeMesh.h>

#include <viskores/Types.h>
#include <viskores/cont/BoundsCompute.h>
#include <viskores/cont/BoundsGlobalCompute.h>
#include <viskores/cont/ErrorFilterExecution.h>
#include <viskores/cont/PartitionedDataSet.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/data_set_mesh/IdRelabeler.h>

namespace viskores
{
namespace worklet
{
namespace contourtree_distributed
{

//--- Helper class to help with the contstuction of the GlobalContourTree
class MultiBlockContourTreeHelper
{
public:
  VISKORES_CONT
  MultiBlockContourTreeHelper(viskores::Id3 blocksPerDim,
                              const viskores::cont::ArrayHandle<viskores::Id3>& localBlockIndices)
    : BlocksPerDimension(blocksPerDim)
    , LocalBlockIndices(localBlockIndices)
    , LocalContourTrees(static_cast<size_t>(localBlockIndices.GetNumberOfValues()))
    , LocalSortOrders(static_cast<size_t>(localBlockIndices.GetNumberOfValues()))
  {
  }

  VISKORES_CONT
  MultiBlockContourTreeHelper(const viskores::cont::PartitionedDataSet& input)
    : BlocksPerDimension(-1, -1, -1)
    , LocalBlockIndices()
    , LocalContourTrees(static_cast<size_t>(input.GetNumberOfPartitions()))
    , LocalSortOrders(static_cast<size_t>(input.GetNumberOfPartitions()))
  {
  }

  VISKORES_CONT
  ~MultiBlockContourTreeHelper(void)
  {
    // FIXME: This shouldn't be necessary as arrays will get deleted anyway
    LocalContourTrees.clear();
    LocalSortOrders.clear();
  }

  inline static viskores::Bounds GetGlobalBounds(const viskores::cont::PartitionedDataSet& input)
  {
    // Get the  spatial bounds  of a multi -block  data  set
    viskores::Bounds bounds = viskores::cont::BoundsGlobalCompute(input);
    return bounds;
  }

  inline static viskores::Bounds GetLocalBounds(const viskores::cont::PartitionedDataSet& input)
  {
    // Get the spatial bounds  of a multi -block  data  set
    viskores::Bounds bounds = viskores::cont::BoundsCompute(input);
    return bounds;
  }

  inline viskores::Id GetLocalNumberOfBlocks() const
  {
    return static_cast<viskores::Id>(this->LocalContourTrees.size());
  }

  inline viskores::Id GetGlobalNumberOfBlocks() const
  {
    return this->BlocksPerDimension[0] * this->BlocksPerDimension[1] * this->BlocksPerDimension[2];
  }

  // Used to compute the local contour tree mesh in after DoExecute. I.e., the function is
  // used in PostExecute to construct the initial set of local ContourTreeMesh blocks for
  // DIY. Subsequent construction of updated ContourTreeMeshes is handled separately.
  template <typename T>
  inline static viskores::worklet::contourtree_augmented::ContourTreeMesh<T>*
  ComputeLocalContourTreeMesh(
    const viskores::Id3 localBlockOrigin,
    const viskores::Id3 localBlockSize,
    const viskores::Id3 globalSize,
    const viskores::cont::ArrayHandle<T>& field,
    const viskores::worklet::contourtree_augmented::ContourTree& contourTree,
    const viskores::worklet::contourtree_augmented::IdArrayType& sortOrder,
    unsigned int computeRegularStructure)

  {
    // compute the global mesh index and initalize the local contour tree mesh
    if (computeRegularStructure == 1)
    {
      // Compute the global mesh index
      viskores::worklet::contourtree_augmented::IdArrayType localGlobalMeshIndex;
      auto transformedIndex = viskores::cont::ArrayHandleTransform<
        viskores::worklet::contourtree_augmented::IdArrayType,
        viskores::worklet::contourtree_augmented::mesh_dem::IdRelabeler>(
        sortOrder,
        viskores::worklet::contourtree_augmented::mesh_dem::IdRelabeler(
          localBlockOrigin, localBlockSize, globalSize));
      viskores::cont::Algorithm::Copy(transformedIndex, localGlobalMeshIndex);
      // Compute the local contour tree mesh
      auto localContourTreeMesh = new viskores::worklet::contourtree_augmented::ContourTreeMesh<T>(
        contourTree.Arcs, sortOrder, field, localGlobalMeshIndex);
      return localContourTreeMesh;
    }
    else if (computeRegularStructure == 2)
    {
      // Compute the global mesh index for the partially augmented contour tree. I.e., here we
      // don't need the global mesh index for all nodes, but only for the augmented nodes from the
      // tree. We, hence, permute the sortOrder by contourTree.augmentednodes and then compute the
      // GlobalMeshIndex by tranforming those indices with our IdRelabler
      viskores::worklet::contourtree_augmented::IdArrayType localGlobalMeshIndex;
      viskores::cont::ArrayHandlePermutation<viskores::worklet::contourtree_augmented::IdArrayType,
                                             viskores::worklet::contourtree_augmented::IdArrayType>
        permutedSortOrder(contourTree.Augmentnodes, sortOrder);
      auto transformedIndex = viskores::cont::make_ArrayHandleTransform(
        permutedSortOrder,
        viskores::worklet::contourtree_augmented::mesh_dem::IdRelabeler(
          localBlockOrigin, localBlockSize, globalSize));
      viskores::cont::Algorithm::Copy(transformedIndex, localGlobalMeshIndex);
      // Compute the local contour tree mesh
      auto localContourTreeMesh = new viskores::worklet::contourtree_augmented::ContourTreeMesh<T>(
        contourTree.Augmentnodes, contourTree.Augmentarcs, sortOrder, field, localGlobalMeshIndex);
      return localContourTreeMesh;
    }
    else
    {
      // We should not be able to get here
      throw viskores::cont::ErrorFilterExecution(
        "Parallel contour tree requires at least parial boundary augmentation");
    }
  }

  viskores::Id3 BlocksPerDimension;
  viskores::cont::ArrayHandle<viskores::Id3> LocalBlockIndices;
  std::vector<viskores::worklet::contourtree_augmented::ContourTree> LocalContourTrees;
  std::vector<viskores::worklet::contourtree_augmented::IdArrayType> LocalSortOrders;
}; // end MultiBlockContourTreeHelper

} // namespace contourtree_distributed
} // namespace worklet
} // namespace viskores

#endif
