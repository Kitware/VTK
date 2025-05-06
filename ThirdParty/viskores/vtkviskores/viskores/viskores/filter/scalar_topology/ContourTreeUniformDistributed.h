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


#ifndef viskores_filter_scalar_topology_ContourTreeUniformDistributed_h
#define viskores_filter_scalar_topology_ContourTreeUniformDistributed_h

#include <viskores/Types.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/ContourTree.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/DataSetMesh.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/BoundaryTree.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/HierarchicalContourTree.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/InteriorForest.h>

#include <memory>
#include <viskores/filter/Filter.h>
#include <viskores/filter/scalar_topology/viskores_filter_scalar_topology_export.h>

namespace viskores
{
namespace filter
{
namespace scalar_topology
{
/// \brief Construct the Contour Tree for a 2D or 3D regular mesh
///
/// This filter implements the parallel peak pruning algorithm. In contrast to
/// the ContourTreeUniform filter, this filter is optimized to allow for the
/// computation of the augmented contour tree, i.e., the contour tree including
/// all regular mesh vertices. Augmentation with regular vertices is used in
/// practice to compute statistics (e.g., volume), to segment the input mesh,
/// facilitate iso-value selection, enable localization of all verticies of a
/// mesh in the tree among others.
///
/// In addition to single-block computation, the filter also supports multi-block
/// regular grids. The blocks are processed in parallel using DIY and then the
/// tree are merged progressively using a binary-reduction scheme to compute the
/// final contour tree. I.e., in the multi-block context, the final tree is
/// constructed on rank 0.
class VISKORES_FILTER_SCALAR_TOPOLOGY_EXPORT ContourTreeUniformDistributed
  : public viskores::filter::Filter
{
public:
  VISKORES_CONT bool CanThread() const override
  {
    // tons of shared mutable states
    return false;
  }

  ContourTreeUniformDistributed(
    viskores::cont::LogLevel timingsLogLevel = viskores::cont::LogLevel::Perf,
    viskores::cont::LogLevel treeLogLevel = viskores::cont::LogLevel::Info);

  VISKORES_CONT void SetUseBoundaryExtremaOnly(bool useBoundaryExtremaOnly)
  {
    this->UseBoundaryExtremaOnly = useBoundaryExtremaOnly;
  }

  VISKORES_CONT bool GetUseBoundaryExtremaOnly() { return this->UseBoundaryExtremaOnly; }

  VISKORES_CONT void SetUseMarchingCubes(bool useMarchingCubes)
  {
    this->UseMarchingCubes = useMarchingCubes;
  }

  VISKORES_CONT bool GetUseMarchingCubes() { return this->UseMarchingCubes; }

  VISKORES_CONT void SetAugmentHierarchicalTree(bool augmentHierarchicalTree)
  {
    this->AugmentHierarchicalTree = augmentHierarchicalTree;
  }

  VISKORES_CONT void SetPresimplifyThreshold(viskores::Id presimplifyThreshold)
  {
    this->PresimplifyThreshold = presimplifyThreshold;
  }

  VISKORES_CONT void SetBlockIndices(
    viskores::Id3 blocksPerDim,
    const viskores::cont::ArrayHandle<viskores::Id3>& localBlockIndices)
  {
    this->BlocksPerDimension = blocksPerDim;
    viskores::cont::ArrayCopy(localBlockIndices, this->LocalBlockIndices);
  }

  VISKORES_CONT bool GetAugmentHierarchicalTree() { return this->AugmentHierarchicalTree; }

  VISKORES_CONT viskores::Id GetPresimplifyThreshold() { return this->PresimplifyThreshold; }

  VISKORES_CONT void SetSaveDotFiles(bool saveDotFiles) { this->SaveDotFiles = saveDotFiles; }

  VISKORES_CONT bool GetSaveDotFiles() { return this->SaveDotFiles; }

  template <typename T, typename StorageType>
  VISKORES_CONT void ComputeLocalTree(
    const viskores::Id blockIndex,
    const viskores::cont::DataSet& input,
    const viskores::cont::ArrayHandle<T, StorageType>& fieldArray);

  /// Implement per block contour tree computation after the MeshType has been discovered
  template <typename T, typename StorageType, typename MeshType, typename MeshBoundaryExecType>
  VISKORES_CONT void ComputeLocalTreeImpl(const viskores::Id blockIndex,
                                          const viskores::cont::DataSet& input,
                                          const viskores::cont::ArrayHandle<T, StorageType>& field,
                                          MeshType& mesh,
                                          MeshBoundaryExecType& meshBoundaryExecObject);

private:
  VISKORES_CONT viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& input) override;
  VISKORES_CONT viskores::cont::PartitionedDataSet DoExecutePartitions(
    const viskores::cont::PartitionedDataSet& input) override;

  ///@{
  /// when operating on viskores::cont::MultiBlock we want to
  /// do processing across ranks as well. Just adding pre/post handles
  /// for the same does the trick.
  VISKORES_CONT void PreExecute(const viskores::cont::PartitionedDataSet& input);

  VISKORES_CONT void PostExecute(const viskores::cont::PartitionedDataSet& input,
                                 viskores::cont::PartitionedDataSet& output);


  template <typename FieldType>
  VISKORES_CONT void ComputeVolumeMetric(
    viskoresdiy::Master& inputContourTreeMaster,
    viskoresdiy::DynamicAssigner& assigner,
    viskoresdiy::RegularSwapPartners& partners,
    const FieldType&, // dummy parameter to get the type
    std::stringstream& timingsStream,
    const viskores::cont::PartitionedDataSet& input,
    bool useAugmentedTree,
    std::vector<viskores::cont::ArrayHandle<viskores::Id>>& intrinsicVolumes,
    std::vector<viskores::cont::ArrayHandle<viskores::Id>>& dependentVolumes);

  ///
  /// Internal helper function that implements the actual functionality of PostExecute
  ///
  /// In the case we operate on viskores::cont::MultiBlock we need to merge the trees
  /// computed on the block to compute the final contour tree.
  template <typename T>
  VISKORES_CONT void DoPostExecute(const viskores::cont::PartitionedDataSet& input,
                                   viskores::cont::PartitionedDataSet& output);
  ///@}

  /// Use only boundary critical points in the parallel merge to reduce communication.
  /// Disabling this should only be needed for performance testing.
  bool UseBoundaryExtremaOnly;

  /// Use marching cubes connectivity for computing the contour tree
  bool UseMarchingCubes;

  /// Augment hierarchical tree
  bool AugmentHierarchicalTree;

  /// Threshold to use for volume pre-simplification
  viskores::Id PresimplifyThreshold = 0;

  /// Save dot files for all tree computations
  bool SaveDotFiles;

  /// Log level to be used for outputting timing information. Default is viskores::cont::LogLevel::Perf
  viskores::cont::LogLevel TimingsLogLevel = viskores::cont::LogLevel::Perf;

  /// Log level to be used for outputting metadata about the trees. Default is viskores::cont::LogLevel::Info
  viskores::cont::LogLevel TreeLogLevel = viskores::cont::LogLevel::Info;

  /// Information about block decomposition TODO/FIXME: Remove need for this information
  // ... Number of blocks along each dimension
  viskores::Id3 BlocksPerDimension;
  // ... Index of the local blocks in x,y,z, i.e., in i,j,k mesh coordinates
  viskores::cont::ArrayHandle<viskores::Id3> LocalBlockIndices;

  /// Intermediate results (one per local data block)...
  /// ... local mesh information needed at end of fan out
  std::vector<viskores::worklet::contourtree_augmented::DataSetMesh> LocalMeshes;
  /// ... local contour trees etc. computed during fan in and used during fan out
  std::vector<viskores::worklet::contourtree_augmented::ContourTree> LocalContourTrees;
  std::vector<viskores::worklet::contourtree_distributed::BoundaryTree> LocalBoundaryTrees;
  std::vector<viskores::worklet::contourtree_distributed::InteriorForest> LocalInteriorForests;

  /// The hierarchical trees computed by the filter (array with one entry per block)
  // TODO/FIXME: We need to find a way to store the final hieararchical trees somewhere.
  // Currently we cannot do this here as it is a template on FieldType
  //
  //std::vector<viskores::worklet::contourtree_distributed::HierarchicalContourTree> HierarchicalContourTrees;
  /// Number of iterations used to compute the contour tree
  viskores::Id NumIterations;
};
} // namespace scalar_topology
} // namespace filter
} // namespace viskores

#endif // viskores_filter_scalar_topology_ContourTreeUniformDistributed_h
