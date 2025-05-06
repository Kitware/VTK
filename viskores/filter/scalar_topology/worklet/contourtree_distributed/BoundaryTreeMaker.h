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

#ifndef viskores_worklet_contourtree_distributed_boundary_tree_maker_h
#define viskores_worklet_contourtree_distributed_boundary_tree_maker_h

// augmented contour tree includes
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/ContourTree.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/PrintVectors.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/Types.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/data_set_mesh/IdRelabeler.h>

// distibuted contour tree includes
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/BoundaryTree.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/HierarchicalContourTree.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/InteriorForest.h>

#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/boundary_tree_maker/AddTerminalFlagsToUpDownNeighboursWorklet.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/boundary_tree_maker/AugmentBoundaryWithNecessaryInteriorSupernodesAppendNecessarySupernodesWorklet.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/boundary_tree_maker/AugmentBoundaryWithNecessaryInteriorSupernodesUnsetBoundarySupernodesWorklet.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/boundary_tree_maker/BoundaryTreeNodeComparator.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/boundary_tree_maker/BoundaryVerticesPerSuperArcWorklets.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/boundary_tree_maker/CompressRegularisedNodesCopyNecessaryRegularNodesWorklet.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/boundary_tree_maker/CompressRegularisedNodesFillBoundaryTreeSuperarcsWorklet.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/boundary_tree_maker/CompressRegularisedNodesFindNewSuperarcsWorklet.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/boundary_tree_maker/CompressRegularisedNodesResolveRootWorklet.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/boundary_tree_maker/CompressRegularisedNodesTransferVerticesWorklet.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/boundary_tree_maker/ContourTreeNodeHyperArcComparator.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/boundary_tree_maker/FindBoundaryTreeSuperarcsSuperarcToWorklet.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/boundary_tree_maker/FindBoundaryVerticesIsNecessaryWorklet.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/boundary_tree_maker/FindNecessaryInteriorSetSuperparentNecessaryWorklet.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/boundary_tree_maker/FindNecessaryInteriorSupernodesFindNodesWorklet.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/boundary_tree_maker/HyperarcComparator.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/boundary_tree_maker/IdentifyRegularisedSupernodesStepOneWorklet.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/boundary_tree_maker/IdentifyRegularisedSupernodesStepTwoWorklet.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/boundary_tree_maker/NoSuchElementFunctor.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/boundary_tree_maker/PointerDoubleUpDownNeighboursWorklet.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/boundary_tree_maker/PropagateBoundaryCountsComputeGroupTotalsWorklet.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/boundary_tree_maker/PropagateBoundaryCountsSubtractDependentCountsWorklet.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/boundary_tree_maker/PropagateBoundaryCountsTransferCumulativeCountsWorklet.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/boundary_tree_maker/PropagateBoundaryCountsTransferDependentCountsWorklet.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/boundary_tree_maker/SetInteriorForestWorklet.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/boundary_tree_maker/SetUpAndDownNeighboursWorklet.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/boundary_tree_maker/SumFunctor.h>


// viskores includes
#include <viskores/Types.h>

// std includes
#include <sstream>
#include <string>
#include <utility>

// TODO: Change all pointers to std mangaged pointers or use references (the memory for those pointers should be managed outside in the filter)

namespace viskores
{
namespace worklet
{
namespace contourtree_distributed
{

/// \brief Class to compute the Boundary Restricted Augmented Contour Tree (BRACT), a.k.a., BoundaryTree
///
template <typename MeshType, typename MeshBoundaryExecObjType>
class BoundaryTreeMaker
{
public:
  // pointers to underlying data structures
  /// Pointer to the input mesh
  MeshType* Mesh;
  MeshBoundaryExecObjType& MeshBoundaryExecutionObject;

  /// Pointer to the contour tree for the mesh
  viskores::worklet::contourtree_augmented::ContourTree& ContourTree;
  /// Data structure for storing the results from this class
  BoundaryTree* BoundaryTreeData;
  /// Data structure for the interior forest of a data block, i.e, the contourtree of a block minus the BoundaryTree (also called the residue or BRACT)
  InteriorForest* InteriorForestData;

  /// how many vertices ARE on the boundary
  viskores::Id NumBoundary;

  /// how many interior vertices are necessary
  viskores::Id NumNecessary;

  /// how many vertices are kept in the BRACT
  viskores::Id NumKept;

  /// arrays for computation - stored here to simplify debug print

  //  arrays sized to all regular vertices - this may not be necessary, but is robust
  /// array for ID in boundary tree
  viskores::worklet::contourtree_augmented::IdArrayType BoundaryTreeId;

  // arrays sized to number of boundary vertices
  /// the regular IDs of the boundary vertices (a conservative over-estimate, needed for hierarchical computation)
  viskores::worklet::contourtree_augmented::IdArrayType BoundaryVertexSuperset;
  /// their sort indices (may be redundant, but . . .)
  viskores::worklet::contourtree_augmented::IdArrayType BoundaryIndices;
  /// the superparents for each boundary vertex
  viskores::worklet::contourtree_augmented::IdArrayType BoundarySuperparents;

  // arrays sized to number of supernodes/superarcs
  /// these are essentially the same as the transfer/intrinsic/dependent weights
  /// probably about time to refactor and do a generic hyperarc propagation routine (owch!)

  /// mapping from tree super ID to bract superset ID (could potentially be combined with isNecessary)
  viskores::worklet::contourtree_augmented::IdArrayType TreeToSuperset;

  /// count of boundary nodes on each superarc
  // TODO: May be able to reuse Petar's Hypersweep, which uses less storage AND/OR use Petar's generic hypersweep to simplify code
  viskores::worklet::contourtree_augmented::IdArrayType SuperarcIntrinsicBoundaryCount;
  /// count of boundary nodes being transferred at eah supernode
  viskores::worklet::contourtree_augmented::IdArrayType SupernodeTransferBoundaryCount;
  /// count of dependent boundary nodes for each superarc
  viskores::worklet::contourtree_augmented::IdArrayType SuperarcDependentBoundaryCount;
  /// count of dependent boundary nodes for each hyperarc
  viskores::worklet::contourtree_augmented::IdArrayType HyperarcDependentBoundaryCount;

  // vectors needed for collapsing out "regular" supernodes
  // up- and down- neighbours in the tree (unique for regular nodes)
  // sized to BRACT
  viskores::worklet::contourtree_augmented::IdArrayType UpNeighbour;
  viskores::worklet::contourtree_augmented::IdArrayType DownNeighbour;
  /// array needed for compression
  viskores::worklet::contourtree_augmented::IdArrayType NewVertexId;

  VISKORES_CONT
  BoundaryTreeMaker(MeshType* inputMesh,
                    MeshBoundaryExecObjType& meshBoundaryExecObj,
                    viskores::worklet::contourtree_augmented::ContourTree& inputTree,
                    BoundaryTree* boundaryTree,
                    InteriorForest* interiorTree)
    : Mesh(inputMesh)
    , MeshBoundaryExecutionObject(meshBoundaryExecObj)
    , ContourTree(inputTree)
    , BoundaryTreeData(boundaryTree)
    , InteriorForestData(interiorTree)
    , NumBoundary(0)
    , NumNecessary(0)
  {
  }

  /// computes a BRACT from a contour tree for a known block
  /// note the block ID for debug purposes
  /// @param[in] localToGlobalIdRelabeler IdRelabeler for the mesh needed to call
  ///            this->Mesh->GetGlobalIdsFromMeshIndices(...)  used by the
  ///            this->SetInteriorForest function (default=nullptr).
  /// @param[in] boundaryCritical If True then use only boundary critical points
  ///            in the BoundaryTree, otherwise use the full boundary between blocks.
  ///            (default=true)
  VISKORES_CONT
  void Construct(const viskores::worklet::contourtree_augmented::mesh_dem::IdRelabeler*
                   localToGlobalIdRelabeler = nullptr,
                 bool boundaryCritical = true);

  /// routine to find the set of boundary vertices
  /// @param[in] boundaryCritical If True then use only boundary critical points
  ///            in the BoundaryTree, otherwise use the full boundary between blocks.
  ///            (default=true)
  VISKORES_CONT
  void FindBoundaryVertices(bool boundaryCritical);

  /// routine to compute the initial dependent counts (i.e. along each superarc)
  /// in preparation for hyper-propagation
  VISKORES_CONT
  void ComputeDependentBoundaryCounts();

  /// routine for hyper-propagation to compute dependent boundary counts
  VISKORES_CONT
  void PropagateBoundaryCounts();

  /// routine to find the necessary interior supernodes for the BRACT
  VISKORES_CONT
  void FindNecessaryInteriorSupernodes();

  /// routine to add the necessary interior supernodes to the boundary array
  VISKORES_CONT
  void AugmentBoundaryWithNecessaryInteriorSupernodes();

  /// routine that sorts on hyperparent to find BRACT superarcs
  VISKORES_CONT
  void FindBoundaryTreeSuperarcs();

  /// compresses out supernodes in the interior that have become regular in the BRACT
  VISKORES_CONT
  void SuppressRegularisedInteriorSupernodes();

  /// routine to find *AN* up/down neighbour for each vertex
  /// this is deliberately non-canonical and exploits write-conflicts
  VISKORES_CONT
  void SetUpAndDownNeighbours();

  /// routine to set a flag for each vertex that has become regular in the interior of the BRACT
  VISKORES_CONT
  void IdentifyRegularisedSupernodes();

  /// this routine sets a flag on every up/down neighbour that points to a critical point
  /// to force termination of pointer-doubling
  VISKORES_CONT
  void AddTerminalFlagsToUpDownNeighbours();

  /// routine that uses pointer-doubling to collapse regular nodes in the BRACT
  VISKORES_CONT
  void PointerDoubleUpDownNeighbours();

  /// routine that compresses the regular nodes out of the BRACT
  VISKORES_CONT
  void CompressRegularisedNodes();

  /// sets the arrays in the InteriorForest (i.e., the residue) that need to be passed to the grafting stage.
  /// @param[in] localToGlobalIdRelabeler IdRelabeler for the mesh needed to call
  ///            this->Mesh->GetGlobalIdsFromMeshIndices(...)  (default=nullptr).
  VISKORES_CONT
  void SetInteriorForest(const viskores::worklet::contourtree_augmented::mesh_dem::IdRelabeler*
                           localToGlobalIdRelabeler = nullptr);

  /// prints the contents of the restrictor object in a standard format
  VISKORES_CONT
  std::string DebugPrint(const char* message, const char* fileName, long lineNum) const;

private:
  /// Used internally to Invoke worklets
  viskores::cont::Invoker Invoke;

}; // BoundaryTreeMaker class


template <typename MeshType, typename MeshBoundaryExecObjType>
void BoundaryTreeMaker<MeshType, MeshBoundaryExecObjType>::Construct(
  const viskores::worklet::contourtree_augmented::mesh_dem::IdRelabeler* localToGlobalIdRelabeler,
  bool boundaryCritical)
{ // Construct

  // 0.  Retrieve the number of iterations used to construct the contour tree
  //    NB: There may be sense in reusing transfer & dependent weight arrays,
  //    and nIterations, and the arrays for the super/hyper arc subsegments per pass
  //    but for now, burn extra memory
  //    The +1 is because this is 0-indexed

  // Step I: Initialise the bract to hold the set of boundary vertices
  //        & save how many for later
  this->FindBoundaryVertices(boundaryCritical);

  // Step II: For each supernode / superarc, compute the dependent boundary counts
  this->ComputeDependentBoundaryCounts();

  // Step III: We now have initial weights and do the standard inward propagation through
  // the hyperstructure to compute properly over the entire tree
  this->PropagateBoundaryCounts();

  // Step IV:  We now use the dependent weight to identify which internal supernodes are necessary
  this->FindNecessaryInteriorSupernodes();

  // Step V: Add the necessary interior nodes to the end of the boundary set
  this->AugmentBoundaryWithNecessaryInteriorSupernodes();

  // Step VI: Use the hyperparents to sort these vertices into contiguous chunks as usual
  //          We will store the BRACT ID for the superarc target this to simplify the next step
  this->FindBoundaryTreeSuperarcs();

  // Step VII: Suppress interior supernodes that are regular in the BRACT
  //           At the end, we will reset the superarc target from BRACT ID to block ID
  this->SuppressRegularisedInteriorSupernodes();

  // Step VIII: Set the Residue for passing to the TreeGrafter
  this->SetInteriorForest(localToGlobalIdRelabeler);

#ifdef DEBUG_PRINT
  VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                 this->BoundaryTreeData->DebugPrint("All Completed\n", __FILE__, __LINE__));
#endif
} // Construct


/// routine to find the set of boundary vertices
///
/// Side-effects: This function updates:
///   - this->BoundaryVertexSuperset
///   - this->BoundaryIndices
template <typename MeshType, typename MeshBoundaryExecObjType>
void BoundaryTreeMaker<MeshType, MeshBoundaryExecObjType>::FindBoundaryVertices(
  bool boundaryCritical)
{ // FindBoundaryVertices

  // ask the mesh to give us a list of boundary verticels (with their regular indices)
  this->Mesh->GetBoundaryVertices(
    this->BoundaryVertexSuperset, this->BoundaryIndices, &(this->MeshBoundaryExecutionObject));
  // pull a local copy of the size (they can diverge)
  this->BoundaryTreeData->NumBoundary = this->BoundaryVertexSuperset.GetNumberOfValues();
  // Identify the points that are boundary critical and update this->BoundaryVertexSuperset,
  // and this->BoundaryIndices accordingly by removing all boundary vertices that are
  // not boundary cirtical, and hence, are not neccessary for merging neighboring data blocks
  if (boundaryCritical)
  {
    viskores::worklet::contourtree_distributed::bract_maker::FindBoundaryVerticesIsNecessaryWorklet
      isNecessaryWorklet;
    viskores::cont::ArrayHandle<bool> isBoundaryCritical;
    this->Invoke(isNecessaryWorklet,
                 this->BoundaryVertexSuperset,
                 &(this->MeshBoundaryExecutionObject),
                 isBoundaryCritical);
    viskores::worklet::contourtree_augmented::IdArrayType necessaryBoundaryVertexSuperset;
    viskores::worklet::contourtree_augmented::IdArrayType necessaryBoundaryIndices;
    viskores::cont::Algorithm::CopyIf(
      this->BoundaryVertexSuperset, isBoundaryCritical, necessaryBoundaryVertexSuperset);
    this->BoundaryVertexSuperset = necessaryBoundaryVertexSuperset;
    viskores::cont::Algorithm::CopyIf(
      this->BoundaryIndices, isBoundaryCritical, necessaryBoundaryIndices);
    this->BoundaryIndices = necessaryBoundaryIndices;
  }

  this->NumBoundary = this->BoundaryVertexSuperset.GetNumberOfValues();
  this->BoundaryTreeData->NumBoundaryUsed = this->NumBoundary;

#ifdef DEBUG_PRINT
  VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                 this->DebugPrint("Boundary Vertices Set", __FILE__, __LINE__));
#endif
} // FindBoundaryVertices


/// routine to compute the initial dependent counts (i.e. along each superarc)
/// in preparation for hyper-propagation
///
/// Side-effects: This function updates:
///   - this->BoundarySuperparents
///   - this->SuperarcIntrinsicBoundaryCount
///   - this->BoundarySuperparents
template <typename MeshType, typename MeshBoundaryExecObjType>
void BoundaryTreeMaker<MeshType, MeshBoundaryExecObjType>::ComputeDependentBoundaryCounts()
{ // ComputeDependentBoundaryCounts
  // 1.  copy in the superparent from the regular arrays in the contour tree
  auto permutedContourTreeSuperparents = viskores::cont::make_ArrayHandlePermutation(
    this->BoundaryIndices, this->ContourTree.Superparents);
  viskores::cont::Algorithm::Copy(permutedContourTreeSuperparents, this->BoundarySuperparents);

#ifdef DEBUG_PRINT
  VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                 this->DebugPrint("Superparents Set", __FILE__, __LINE__));
#endif

  // 2. Sort this set & count by superarc to set initial intrinsic boundary counts
  //    Note that this is in the parallel style, but can be done more efficiently in serial
  //    a. Allocate space for the count & initialise to zero
  viskores::cont::Algorithm::Copy(
    viskores::cont::make_ArrayHandleConstant(0, this->ContourTree.Superarcs.GetNumberOfValues()),
    this->SuperarcIntrinsicBoundaryCount);
  //   b. sort the superparents
  viskores::worklet::contourtree_augmented::AssertArrayHandleNoFlagsSet(this->BoundarySuperparents);
  viskores::cont::Algorithm::Sort(this->BoundarySuperparents);

  // c.  Now compute the number of boundary vertices per superarc
  //    This *could* be done with a prefix sum, but it's cheaper to do it this way with 2 passes
  //    NB: first superarc's beginning is always 0, so we can omit it, which simplifies the IF logic
  //    i. Start by detecting the high end of the range and then ii) setting the last element explicitly
  bract_maker::BoundaryVerticiesPerSuperArcStepOneWorklet tempWorklet1(this->NumBoundary);
  this->Invoke(tempWorklet1,
               this->BoundarySuperparents,
               this->SuperarcIntrinsicBoundaryCount // output
  );

  // iii.Repeat to subtract and compute the extent lengths (i.e. the counts)
  //     Note that the 0th element will subtract 0 and can be omitted
  bract_maker::BoundaryVerticiesPerSuperArcStepTwoWorklet tempWorklet2;
  this->Invoke(tempWorklet2,
               this->BoundarySuperparents,
               this->SuperarcIntrinsicBoundaryCount // output
  );

  // resize local array
  this->BoundarySuperparents.ReleaseResources();

#ifdef DEBUG_PRINT
  VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                 this->DebugPrint("Initial Counts Set", __FILE__, __LINE__));
#endif
} // ComputeDependentBoundaryCounts


/// routine for hyper-propagation to compute dependent boundary counts
///
///  Side-effects: This function updates:
///    - this->SupernodeTransferBoundaryCount
///    - this->SuperarcDependentBoundaryCount
///    - this->HyperarcDependentBoundaryCount
template <typename MeshType, typename MeshBoundaryExecObjType>
void BoundaryTreeMaker<MeshType, MeshBoundaryExecObjType>::PropagateBoundaryCounts()
{ // PropagateBoundaryCounts
  //   1.  Propagate boundary counts inwards along super/hyperarcs (same as ComputeWeights)
  //    a.  Initialise arrays for transfer & dependent counts
  viskores::cont::Algorithm::Copy(
    viskores::cont::make_ArrayHandleConstant(0, this->ContourTree.Supernodes.GetNumberOfValues()),
    this->SupernodeTransferBoundaryCount);
  viskores::cont::Algorithm::Copy(
    viskores::cont::make_ArrayHandleConstant(0, this->ContourTree.Superarcs.GetNumberOfValues()),
    this->SuperarcDependentBoundaryCount);
  viskores::cont::Algorithm::Copy(
    viskores::cont::make_ArrayHandleConstant(0, this->ContourTree.Hyperarcs.GetNumberOfValues()),
    this->HyperarcDependentBoundaryCount);

#ifdef DEBUG_PRINT
  VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                 this->DebugPrint("Arrays initialised", __FILE__, __LINE__));
#endif

  // b.  Iterate, propagating counts inwards
  for (viskores::Id iteration = 0; iteration < this->ContourTree.NumIterations; iteration++)
  { // b. per iteration
#ifdef DEBUG_PRINT
    VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                   this->DebugPrint("Top of Loop:", __FILE__, __LINE__));
#endif
    // i. Pull the array bounds into register
    viskores::Id firstSupernode =
      viskores::cont::ArrayGetValue(iteration, this->ContourTree.FirstSupernodePerIteration);
    viskores::Id lastSupernode =
      viskores::cont::ArrayGetValue(iteration + 1, this->ContourTree.FirstSupernodePerIteration);

    if (lastSupernode == firstSupernode)
    {
#ifdef DEBUG_PRINT
      VISKORES_LOG_S(
        viskores::cont::LogLevel::Info,
        "BoundaryTreeMaker::PropagateBoundaryCounts(): lastSupernode == firstSupernode "
        " -> Skipping iteration");
#endif
      continue;
    }

    viskores::Id firstHypernode =
      viskores::cont::ArrayGetValue(iteration, this->ContourTree.FirstHypernodePerIteration);
    viskores::Id lastHypernode =
      viskores::cont::ArrayGetValue(iteration + 1, this->ContourTree.FirstHypernodePerIteration);

    //  ii.  Add xfer + int & store in dependent count
    //      Compute the sum of this->SupernodeTransferBoundaryCount and this->SuperarcIntrinsicBoundaryCount
    //      for the [firstSupernodex, lastSupernode) subrange and copy to the this->SuperarcDependentBoundaryCount
    { // make local context so fancyTempSumArray gets deleted
      auto fancyTempZippedArray = viskores::cont::make_ArrayHandleZip(
        this->SupernodeTransferBoundaryCount, this->SuperarcIntrinsicBoundaryCount);
      auto fancyTempSumArray =
        viskores::cont::make_ArrayHandleTransform(fancyTempZippedArray, bract_maker::SumFunctor{});

      viskores::cont::Algorithm::CopySubRange(
        fancyTempSumArray,                    // input array
        firstSupernode,                       // start index for the copy
        lastSupernode - firstSupernode,       // number of values to copy
        this->SuperarcDependentBoundaryCount, // target output array
        firstSupernode // index in the output array where we start writing values to
      );
    } // end local context for step ii

#ifdef DEBUG_PRINT
    VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                   this->DebugPrint("After Transfer", __FILE__, __LINE__));
#endif
    // iii.Perform prefix sum on dependent count range
    { // make local context so tempArray and fancyRangeArraySuperarcDependentBoundaryCountget  deleted
      auto fancyRangeArraySuperarcDependentBoundaryCount = make_ArrayHandleView(
        this->SuperarcDependentBoundaryCount, firstSupernode, lastSupernode - firstSupernode);
      // Write to temporary array first as it is not clear whether ScanInclusive is safe to read and write
      // to the same array and range
      viskores::worklet::contourtree_augmented::IdArrayType tempArray;
      viskores::cont::Algorithm::ScanInclusive(fancyRangeArraySuperarcDependentBoundaryCount,
                                               tempArray);
      viskores::cont::Algorithm::CopySubRange(
        tempArray,
        0,
        tempArray.GetNumberOfValues(), // copy all of tempArray
        this->SuperarcDependentBoundaryCount,
        firstSupernode // to the SuperarcDependentBoundaryCound starting at firstSupernode
      );
    } // end local context for step iii

#ifdef DEBUG_PRINT
    VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                   this->DebugPrint("After Prefix Sum", __FILE__, __LINE__));
#endif
    //  iv.  Subtract out the dependent count of the prefix to the entire hyperarc
    { // make local context for iv so newSuperArcDependentBoundaryCount and the worklet gets deleted
      // Storage for the vector portion that will be modified
      viskores::worklet::contourtree_augmented::IdArrayType newSuperArcDependentBoundaryCount;
      viskores::cont::Algorithm::CopySubRange(this->SuperarcDependentBoundaryCount,
                                              firstSupernode,
                                              lastSupernode - firstSupernode,
                                              newSuperArcDependentBoundaryCount);
      auto subtractDependentCountsWorklet =
        bract_maker::PropagateBoundaryCountsSubtractDependentCountsWorklet(firstSupernode,
                                                                           firstHypernode);
      // per supenode
      this->Invoke(
        subtractDependentCountsWorklet,
        // use backwards counting array for consistency with original code, but if forward or backward doesn't matter
        viskores::cont::ArrayHandleCounting<viskores::Id>(
          lastSupernode - 1, -1, lastSupernode - firstSupernode - 1), // input
        this->ContourTree.Hyperparents,                               // input
        this->ContourTree.Hypernodes,                                 // input
        this->SuperarcDependentBoundaryCount,                         // input
        newSuperArcDependentBoundaryCount                             // (input/output)
      );
      // copy the results back into our main array
      viskores::cont::Algorithm::CopySubRange(newSuperArcDependentBoundaryCount,
                                              0,
                                              newSuperArcDependentBoundaryCount.GetNumberOfValues(),
                                              this->SuperarcDependentBoundaryCount,
                                              firstSupernode);
    } // end local context of iv
#ifdef DEBUG_PRINT
    VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                   this->DebugPrint("After Hyperarc Subtraction", __FILE__, __LINE__));
#endif

    //  v.  Transfer the dependent count to the hyperarc's target supernode
    { // local context of v.
      auto transferDependentCountsWorklet =
        bract_maker::PropagateBoundaryCountsTransferDependentCountsWorklet(
          this->ContourTree.Supernodes.GetNumberOfValues(),
          this->ContourTree.Hypernodes.GetNumberOfValues());
      this->Invoke(
        transferDependentCountsWorklet,
        // for (viskores::Id hypernode = firstHypernode; hypernode < lastHypernode; hypernode++)
        viskores::cont::ArrayHandleCounting<viskores::Id>(
          firstHypernode, 1, lastHypernode - firstHypernode), // input
        this->ContourTree.Hypernodes,                         // input
        this->SuperarcDependentBoundaryCount,                 // input
        this->HyperarcDependentBoundaryCount                  // output
      );
      // transferring the count is done as a separate reduction
    } // end local context for step iv
#ifdef DEBUG_PRINT
    VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                   this->DebugPrint("After Dependent Count transfer", __FILE__, __LINE__));
#endif

    // next we want to end up summing transfer count & storing in the target.
    // Unfortunately, there may be multiple hyperarcs in a given
    // pass that target the same supernode, so we have to do this separately.
    // 1. permute so that all hypernodes with the same target are contiguous
    { // local context for summing transfer count & storing in the target.
      viskores::worklet::contourtree_augmented::IdArrayType hyperarcTargetSortPermutation;
      viskores::cont::Algorithm::Copy(viskores::cont::ArrayHandleCounting<viskores::Id>(
                                        firstHypernode, 1, lastHypernode - firstHypernode),
                                      hyperarcTargetSortPermutation);

      // 2. sort the elements to cluster by hyperarc target, using a lambda function for comparator
      //    we need a reference that a lambda function can use
      auto hyperarcComparator = bract_maker::HyperarcComparator(this->ContourTree.Hyperarcs);
      viskores::cont::Algorithm::Sort(hyperarcTargetSortPermutation, hyperarcComparator);

      // 3. now compute the partial sum for the properly permuted boundary counts
      // total boundary count at each node
      viskores::worklet::contourtree_augmented::IdArrayType accumulatedBoundaryCount;
      auto permutedHyperarcDependentCount = viskores::cont::make_ArrayHandlePermutation(
        hyperarcTargetSortPermutation,       // id array
        this->HyperarcDependentBoundaryCount // value array
      );
      viskores::cont::Algorithm::ScanInclusive(permutedHyperarcDependentCount,
                                               accumulatedBoundaryCount);

      // 4. The partial sum is now over ALL hypertargets, so within each group we need to subtract the first from the last
      // To do so, the last hyperarc in each cluster copies its cumulative count to the output array
      auto transferCumulativeCountsWorklet =
        bract_maker::PropagateBoundaryCountsTransferCumulativeCountsWorklet();
      this->Invoke(transferCumulativeCountsWorklet,
                   hyperarcTargetSortPermutation,
                   this->ContourTree.Hyperarcs,
                   accumulatedBoundaryCount,
                   this->SupernodeTransferBoundaryCount);

#ifdef DEBUG_PRINT
      VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                     this->DebugPrint("After Tail Addition", __FILE__, __LINE__));
#endif

      // 5. Finally, we subtract the beginning of the group to get the total for each group
      // Note that we start the loop from 1, which means we don't need a special case if statement
      // because the prefix sum of the first element is the correct value anyway
      auto computeGroupTotalsWorklet =
        bract_maker::PropagateBoundaryCountsComputeGroupTotalsWorklet();
      this->Invoke(computeGroupTotalsWorklet,
                   hyperarcTargetSortPermutation,
                   this->ContourTree.Hyperarcs,
                   accumulatedBoundaryCount,
                   this->SupernodeTransferBoundaryCount);
    } // end  local context for summing transfer count & storing in the target.
#ifdef DEBUG_PRINT
    VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                   this->DebugPrint("After Hyperarc Transfer", __FILE__, __LINE__));
#endif
  } // b. per iteration

  // when we are done, we need to force the summation for the root node, JUST IN CASE it is a boundary node itself
  // BTW, the value *SHOULD* be the number of boundary nodes, anyway
  viskores::Id rootSuperId = this->ContourTree.Supernodes.GetNumberOfValues() - 1;
  viskores::worklet::contourtree_augmented::IdArraySetValue(
    rootSuperId,
    viskores::cont::ArrayGetValue(rootSuperId, this->SupernodeTransferBoundaryCount) +
      viskores::cont::ArrayGetValue(rootSuperId, this->SuperarcIntrinsicBoundaryCount),
    this->SuperarcDependentBoundaryCount);
  viskores::worklet::contourtree_augmented::IdArraySetValue(
    this->ContourTree.Hypernodes.GetNumberOfValues() - 1,
    viskores::cont::ArrayGetValue(rootSuperId, this->SuperarcDependentBoundaryCount),
    this->HyperarcDependentBoundaryCount);

#ifdef DEBUG_PRINT
  VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                 this->DebugPrint("Iterations Complete", __FILE__, __LINE__));
#endif
} // PropagateBoundaryCounts


/// routine to find the necessary interior supernodes for the BRACT
///
/// INVARIANT:
/// We have now computed the dependent weight for each supernode
///    For boundary nodes, we will ignore this
///    For non-boundary nodes, if the dependent weight is 0 or nBoundary
///    then all boundary nodes are in one direction, and the node is unnecessary
/// We have decided that if a superarc has any boundary nodes, the entire superarc
/// should be treated as necessary. This extends the criteria so that the superparent and superparent's
/// superarc of any boundary node are necessary
///
/// Side-effects: This function updates:
///    - this->InteriorForestData->IsNecessary
template <typename MeshType, typename MeshBoundaryExecObjType>
void BoundaryTreeMaker<MeshType, MeshBoundaryExecObjType>::FindNecessaryInteriorSupernodes()
{ // FindNecessaryInteriorSupernodes
  //  1. Identify the necessary supernodes (between two boundary points & still critical)
  //  1.A.  Start by setting all of them to "unnecessary"
  // Initalize isNecessary with False
  viskores::cont::Algorithm::Copy(viskores::cont::make_ArrayHandleConstant(
                                    false, this->ContourTree.Supernodes.GetNumberOfValues()),
                                  this->InteriorForestData->IsNecessary);
  // 1.B.  Our condition is that if the superarc dependent count is neither 0 nor the # of boundary
  //       points, the superarc target is necessary. Note that there may be write conflicts, but it's
  //       an OR operation, so it doesn't matter
  auto findNodesWorklet =
    bract_maker::FindNecessaryInteriorSupernodesFindNodesWorklet(this->NumBoundary);
  this->Invoke(findNodesWorklet,
               this->ContourTree.Superarcs,          // input
               this->SuperarcDependentBoundaryCount, // input
               this->InteriorForestData->IsNecessary // output
  );
#ifdef DEBUG_PRINT
  VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                 this->DebugPrint("Is Necessary Based on Dependency", __FILE__, __LINE__));
#endif

  // separate pass to set the superparent of every boundary node to be necessary
  auto setSuperparentNecessaryWorklet =
    bract_maker::FindNecessaryInteriorSetSuperparentNecessaryWorklet();
  this->Invoke(setSuperparentNecessaryWorklet,
               this->BoundaryIndices,
               this->ContourTree.Superparents,
               this->ContourTree.Superarcs,
               this->InteriorForestData->IsNecessary);
#ifdef DEBUG_PRINT
  VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                 this->DebugPrint("Is Necessary Set", __FILE__, __LINE__));
#endif
} // FindNecessaryInteriorSupernodes()

/// routine to add the necessary interior supernodes to the boundary array
///
/// Side effects: This function updates:
/// - this->NumNecessary
/// - this->BoundaryIndices,
/// - this->BoundaryVertexSuperset
template <typename MeshType, typename MeshBoundaryExecObjType>
void BoundaryTreeMaker<MeshType,
                       MeshBoundaryExecObjType>::AugmentBoundaryWithNecessaryInteriorSupernodes()
{ // AugmentBoundaryWithNecessaryInteriorSupernodes
  //  1.  Collect the necessary supernodes & boundary vertices & suppress duplicates
  viskores::worklet::contourtree_augmented::IdArrayType isNecessaryAndInterior;
  viskores::cont::Algorithm::Copy(this->InteriorForestData->IsNecessary, isNecessaryAndInterior);

  //  a.  To do this, first *UNSET* the necessary flags for all supernodes that are also on the boundary
  auto unsetBoundarySupernodesWorklet =
    bract_maker::AugmentBoundaryWithNecessaryInteriorSupernodesUnsetBoundarySupernodesWorklet();
  this->Invoke(unsetBoundarySupernodesWorklet,
               this->BoundaryIndices,          // input
               this->ContourTree.Superparents, // input
               this->ContourTree.Supernodes,   // input
               isNecessaryAndInterior          // output
  );

#ifdef DEBUG_PRINT
  DebugPrint("Flags Unset", __FILE__, __LINE__);
#endif

  //  b.  Now append all necessary supernodes to the boundary vertex array
  // first count how many are needed, then resize the arrays
  this->NumNecessary =
    viskores::cont::Algorithm::Reduce(isNecessaryAndInterior, static_cast<viskores::Id>(0));
  // We need to grow the arrays, without loosing our original data, so we need
  // to create new arrays of the approbriate size, copy our data in and then
  // assign. TODO: Would be great if Viskores allow resize without losing the values
  if (this->NumNecessary == 0)
  {
#ifdef DEBUG_PRINT
    VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                   "BoundaryTreeMaker::AugmentBoundaryWithNecessaryInteriorSupernodes():"
                   "No additional nodes necessary. Returning.");
#endif
    return;
  }
  {
    // Create a new resized array and copy the original values to the array
    viskores::worklet::contourtree_augmented::IdArrayType tempBoundaryVertexSuperset;
    tempBoundaryVertexSuperset.Allocate(this->NumBoundary + this->NumNecessary);
    viskores::cont::Algorithm::CopySubRange(
      this->BoundaryVertexSuperset, // input
      0,                            // start index
      this->NumBoundary, // number of values to copy. Same as size of this->BoundaryVertexSuperset
      tempBoundaryVertexSuperset, // copy to temp
      0                           // start inserting at 0
    );
    // Set the added values to 0
    // TODO: Check if it is really necessary to initalize the new values
    viskores::cont::Algorithm::CopySubRange(
      viskores::cont::make_ArrayHandleConstant<viskores::Id>(0, this->NumBoundary), // set to 0
      0,                                                                            // start index
      this->NumNecessary,         // number of values to copy
      tempBoundaryVertexSuperset, // copy to temp
      this->NumBoundary           // start where our original values end
    );
    // Set the BoundaryVertexSuperset to our new array
    this->BoundaryVertexSuperset.ReleaseResources();
    this->BoundaryVertexSuperset = tempBoundaryVertexSuperset;
  }
  // Now do the same for the BoundaryIndicies (i.e,. resize while keeping the original values)
  {
    // Create a new resized array and copy the original values to the array
    viskores::worklet::contourtree_augmented::IdArrayType tempBoundaryIndices;
    tempBoundaryIndices.Allocate(this->NumBoundary + this->NumNecessary);
    viskores::cont::Algorithm::CopySubRange(
      this->BoundaryIndices, // input
      0,                     // start index
      this->NumBoundary,     // number of values to copy. Same as size of this->BoundaryIndices
      tempBoundaryIndices,   // copy to temp
      0                      // start inserting at 0
    );
    // Set the added values to 0
    // TODO: Check if it is really necessary to initalize the new values
    viskores::cont::Algorithm::CopySubRange(
      viskores::cont::make_ArrayHandleConstant<viskores::Id>(0, this->NumBoundary), // set to 0
      0,                                                                            // start index
      this->NumNecessary,  // number of values to copy
      tempBoundaryIndices, // copy to temp
      this->NumBoundary    // start where our original values end
    );
    // Set the BoundaryVertexSuperset to our new array
    this->BoundaryIndices.ReleaseResources();
    this->BoundaryIndices = tempBoundaryIndices;
  }

  // create a temporary array for transfer IDs
  viskores::worklet::contourtree_augmented::IdArrayType boundaryNecessaryId;
  boundaryNecessaryId.Allocate(this->ContourTree.Supernodes.GetNumberOfValues());
  // and partial sum them in place
  viskores::cont::Algorithm::ScanInclusive(isNecessaryAndInterior, boundaryNecessaryId);

  // now do a parallel for loop to copy them
  auto appendNecessarySupernodesWorklet =
    bract_maker::AugmentBoundaryWithNecessaryInteriorSupernodesAppendNecessarySupernodesWorklet(
      this->NumBoundary);
  this->Invoke(appendNecessarySupernodesWorklet,
               this->ContourTree.Supernodes,
               isNecessaryAndInterior,
               boundaryNecessaryId,
               this->Mesh->SortOrder,
               this->BoundaryIndices,
               this->BoundaryVertexSuperset);

#ifdef DEBUG_PRINT
  VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                 this->DebugPrint("Necessary Appended", __FILE__, __LINE__));
#endif
} // AugmentBoundaryWithNecessaryInteriorSupernodes


/// routine that sorts on hyperparent to find BRACT superarcs
///
///  Side-effects: This function updates:
///  - this->TreeToSuperset
///  - this->BoundaryIndices
///  - this->BoundaryVertexSuperset
///  - this->BoundaryTreeData->Superarcs
///  - this->BoundaryTreeId
template <typename MeshType, typename MeshBoundaryExecObjType>
void BoundaryTreeMaker<MeshType, MeshBoundaryExecObjType>::FindBoundaryTreeSuperarcs()
{ // FindBoundaryTreeSuperarcs
  //  0.  Allocate memory for the tree2superset map
  viskores::cont::Algorithm::Copy(viskores::cont::make_ArrayHandleConstant(
                                    viskores::worklet::contourtree_augmented::NO_SUCH_ELEMENT,
                                    this->ContourTree.Supernodes.GetNumberOfValues()),
                                  this->TreeToSuperset);

  //  1.  Sort the boundary set by hyperparent
  auto contourTreeNodeHyperArcComparator = bract_maker::ContourTreeNodeHyperArcComparator(
    this->ContourTree.Superarcs, this->ContourTree.Superparents);
  viskores::cont::Algorithm::Sort(this->BoundaryIndices, contourTreeNodeHyperArcComparator);

#ifdef DEBUG_PRINT
  VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                 this->DebugPrint("Sorted by Superparent", __FILE__, __LINE__));
#endif

  //  2.  Reset the order of the vertices in the BRACT
  viskores::cont::Algorithm::Copy(
    viskores::cont::make_ArrayHandlePermutation(
      this->BoundaryIndices, // index array to permute with
      this->Mesh->SortOrder  // value array to be permuted
      ),
    this->BoundaryVertexSuperset // Copy the permuted Mesh->SortOrder to BoundaryVertexSuperset
  );
#ifdef DEBUG_PRINT
  VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                 this->DebugPrint("Vertices Reset", __FILE__, __LINE__));
#endif

  // allocate memory for the superarcs (same size as supernodes for now)
  viskores::cont::Algorithm::Copy(viskores::cont::make_ArrayHandleConstant(
                                    viskores::worklet::contourtree_augmented::NO_SUCH_ELEMENT,
                                    this->BoundaryVertexSuperset.GetNumberOfValues()),
                                  this->BoundaryTreeData->Superarcs);

  // We would like to connect vertices to their neighbour on the hyperarc as usual
  // The problem here is that the root of the tree may be unnecessary
  // and if that is the case we will need to adjust

  // The test will be:
  //     i.  if there is a "next", we will take it
  //    ii.  if we are dangling at the end of the hyperarc, two possibilities exist
  //      a.  the supernode target of the hyperarc is in the BRACT anyway
  //      b.  the supernode target is not in the BRACT

  // To resolve all this, we will need to have an array the size of all the regular nodes
  // in order to find the boundary ID of each vertex transferred
  {
    // Allocate this->BoundaryTreeId with NoSuchElemet
    viskores::cont::Algorithm::Copy(viskores::cont::make_ArrayHandleConstant(
                                      viskores::worklet::contourtree_augmented::NO_SUCH_ELEMENT,
                                      this->ContourTree.Nodes.GetNumberOfValues()),
                                    this->BoundaryTreeId);
    // Fill the relevant values
    auto tempPermutedBoundaryTreeId =
      viskores::cont::make_ArrayHandlePermutation(this->BoundaryVertexSuperset, // index array
                                                  this->BoundaryTreeId          // value array
      );
    viskores::cont::Algorithm::Copy(
      viskores::cont::ArrayHandleIndex(
        this->BoundaryVertexSuperset.GetNumberOfValues()), // copy 0,1, ... n
      tempPermutedBoundaryTreeId // copy to BoundaryTreeId permuted by BoundaryTreeVertexSuperset
    );
  }

  // We now compute the superarc "to" for every bract node
  auto superarcToWorklet = bract_maker::FindBoundaryTreeSuperarcsSuperarcToWorklet();
  this->Invoke(superarcToWorklet,
               this->BoundaryVertexSuperset,     // input
               this->BoundaryIndices,            // input
               this->BoundaryTreeId,             // input
               this->ContourTree.Superparents,   // input
               this->ContourTree.Hyperparents,   // input
               this->ContourTree.Hyperarcs,      // input
               this->ContourTree.Supernodes,     // input
               this->Mesh->SortOrder,            // input
               this->TreeToSuperset,             // output
               this->BoundaryTreeData->Superarcs // output
  );

#ifdef DEBUG_PRINT
  VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                 this->DebugPrint("Restricted to Boundary", __FILE__, __LINE__));
#endif
} // FindBoundaryTreeSuperarcs


/// compresses out supernodes in the interior that have become regular in the BRACT
///
/// Side-effects: This function has the cummulative side effects of
/// - this->SetUpAndDownNeighbours();
/// - this->IdentifyRegularisedSupernodes();
/// - this->AddTerminalFlagsToUpDownNeighbours();
/// - this->PointerDoubleUpDownNeighbours();
/// - this->CompressRegularisedNodes();
template <typename MeshType, typename MeshBoundaryExecObjType>
void BoundaryTreeMaker<MeshType, MeshBoundaryExecObjType>::SuppressRegularisedInteriorSupernodes()
{ // SuppressRegularisedInteriorSupernodes
  // 1.   We have to suppress regular vertices that were interior critical points
  //      We can't get rid of them earlier because we need them to connect super-/hyper- arcs

  //  STEP I:    Find a (non-canonical) up/down neighbour for each vertex
  SetUpAndDownNeighbours();

  //  STEP II:  Find the critical points
  IdentifyRegularisedSupernodes();

  //  STEP III:  Set flags to indicate which pointers are terminal
  AddTerminalFlagsToUpDownNeighbours();

  //  STEP IV:  Use pointer-doubling to collapse past regular nodes
  PointerDoubleUpDownNeighbours();

  //  STEP V:    Get rid of the now regular interior supernodes
  CompressRegularisedNodes();
} // SuppressRegularisedInteriorSupernodes


/// routine to find *AN* up/down neighbour for each vertex
/// this is deliberately non-canonical and exploits write-conflicts
///
/// Side effect: This function updates
/// - this->UpNeighbour
/// - this->DownNeighbour
template <typename MeshType, typename MeshBoundaryExecObjType>
void BoundaryTreeMaker<MeshType, MeshBoundaryExecObjType>::SetUpAndDownNeighbours()
{ // SetUpAndDownNeighbours
  //  So, we will set an up- and down-neighbour for each one (for critical points, non-canonical)
  { // local context
    auto tempNoSuchElementArray = viskores::cont::make_ArrayHandleConstant(
      viskores::worklet::contourtree_augmented::NO_SUCH_ELEMENT,
      this->BoundaryVertexSuperset.GetNumberOfValues());
    viskores::cont::Algorithm::Copy(tempNoSuchElementArray, this->UpNeighbour);
    viskores::cont::Algorithm::Copy(tempNoSuchElementArray, this->DownNeighbour);
  } // end local context

  auto setUpAndDownNeighboursWorklet = bract_maker::SetUpAndDownNeighboursWorklet();
  this->Invoke(setUpAndDownNeighboursWorklet,
               this->BoundaryVertexSuperset,      // input
               this->BoundaryTreeData->Superarcs, // input
               this->Mesh->SortIndices,           // input
               this->UpNeighbour,                 // output
               this->DownNeighbour                // output
  );

#ifdef DEBUG_PRINT
  VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                 this->DebugPrint("Initial Up/Down Neighbours Set", __FILE__, __LINE__));
#endif
} // SetUpAndDownNeighbours


/// routine to set a flag for each vertex that has become regular in the interior of the BRACT
///
/// Side effect: This function updates
/// - this->NewVertexId
template <typename MeshType, typename MeshBoundaryExecObjType>
void BoundaryTreeMaker<MeshType, MeshBoundaryExecObjType>::IdentifyRegularisedSupernodes()
{ // IdentifyRegularisedSupernodes
  // here, if any edge detects the up/down neighbours mismatch, we must have had a write conflict
  // it then follows we have a critical point and can set a flag accordingly
  // we will use a vector that stores NO_SUCH_ELEMENT for false, anything else for true
  // it gets reused as an ID
  viskores::cont::Algorithm::Copy(viskores::cont::make_ArrayHandleConstant(
                                    viskores::worklet::contourtree_augmented::NO_SUCH_ELEMENT,
                                    this->BoundaryVertexSuperset.GetNumberOfValues()),
                                  this->NewVertexId);

  auto stepOneWorklet = bract_maker::IdentifyRegularisedSupernodesStepOneWorklet();
  this->Invoke(stepOneWorklet,
               this->BoundaryVertexSuperset,      // input
               this->BoundaryTreeData->Superarcs, // input
               this->Mesh->SortIndices,           // input
               this->UpNeighbour,                 // input
               this->DownNeighbour,               // input
               this->NewVertexId                  // output
  );

  //  c.  We also want to flag the leaves and boundary nodes as necessary
  auto stepTwoWorklet = bract_maker::IdentifyRegularisedSupernodesStepTwoWorklet();
  this->Invoke(stepTwoWorklet,
               this->BoundaryVertexSuperset,      // input
               this->UpNeighbour,                 // input
               this->DownNeighbour,               // input
               this->MeshBoundaryExecutionObject, // input
               this->NewVertexId                  // output
  );

#ifdef DEBUG_PRINT
  VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                 this->DebugPrint("Boundaries & Leaves Set", __FILE__, __LINE__));
#endif
} // IdentifyRegularisedSupernodes


/// this routine sets a flag on every up/down neighbour that points to a critical point
/// to force termination of pointer-doubling
///
/// Side effects: This function updates
/// - this->UpNeighbour
/// - this->DownNeighbour
template <typename MeshType, typename MeshBoundaryExecObjType>
void BoundaryTreeMaker<MeshType, MeshBoundaryExecObjType>::AddTerminalFlagsToUpDownNeighbours()
{ //
  //  d.  Now that we know which vertices are necessary, we can set the upNeighbour & downNeighbour flags
  auto addTerminalFlagsToUpDownNeighboursWorklet =
    bract_maker::AddTerminalFlagsToUpDownNeighboursWorklet();
  this->Invoke(addTerminalFlagsToUpDownNeighboursWorklet,
               this->NewVertexId,  // input
               this->UpNeighbour,  // output
               this->DownNeighbour // output
  );

#ifdef DEBUG_PRINT
  VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                 this->DebugPrint("Up/Down Neighbours Terminated", __FILE__, __LINE__));
#endif
} //


/// routine that uses pointer-doubling to collapse regular nodes in the BRACT
///
/// Side effects: This functions updates
/// - this->UpNeighbour,
/// - this->DownNeighbour
template <typename MeshType, typename MeshBoundaryExecObjType>
void BoundaryTreeMaker<MeshType, MeshBoundaryExecObjType>::PointerDoubleUpDownNeighbours()
{ // PointerDoubleUpDownNeighbours
  // e. Use pointer-doubling to eliminate regular nodes
  // 1. Compute the number of log steps
  viskores::Id nLogSteps = 1;
  for (viskores::Id shifter = this->BoundaryVertexSuperset.GetNumberOfValues(); shifter != 0;
       shifter >>= 1)
  {
    nLogSteps++;
  }

  //  2. loop that many times to do the compression
  for (viskores::Id iteration = 0; iteration < nLogSteps; iteration++)
  { // per iteration
    // loop through the vertices, updating both ends
    auto pointerDoubleUpDownNeighboursWorklet = bract_maker::PointerDoubleUpDownNeighboursWorklet();
    this->Invoke(pointerDoubleUpDownNeighboursWorklet, this->UpNeighbour, this->DownNeighbour);
  } // per iteration
#ifdef DEBUG_PRINT
  VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                 this->DebugPrint("Pointer Doubling Done", __FILE__, __LINE__));
#endif
} // PointerDoubleUpDownNeighbours


/// routine that compresses the regular nodes out of the BRACT
///
/// Side effects: This function updates:
///  - this->NewVertexId
///  - this->NumKept
///  - this->BoundaryTreeData->VertexIndex
///  - this->BoundaryTreeData->Superarcs
template <typename MeshType, typename MeshBoundaryExecObjType>
void BoundaryTreeMaker<MeshType, MeshBoundaryExecObjType>::CompressRegularisedNodes()
{ // CompressRegularisedNodes
  //  f.  Compress the regular nodes out of the tree
  //  1.  Assign new indices
  //      Copy the necessary ones only - this is a compression call in parallel
  // HAC: to use a different array, keeping isNecessary indexed on everything
  //      We now reset it to the size of the return tree
  viskores::worklet::contourtree_augmented::IdArrayType keptInBoundaryTree;
  //    Start by creating the ID #s with a partial sum (these will actually start from 1, not 0
  viskores::cont::Algorithm::ScanInclusive(
    viskores::cont::make_ArrayHandleTransform(this->NewVertexId,
                                              bract_maker::NoSuchElementFunctor()),
    keptInBoundaryTree);
  // Update newVertexID, i.e., for each element set:
  // if (!noSuchElement(newVertexID[returnIndex]))
  //      newVertexID[returnIndex] = keptInBoundaryTree[returnIndex]-1;
  auto copyNecessaryRegularNodesWorklet =
    bract_maker::CompressRegularisedNodesCopyNecessaryRegularNodesWorklet();
  this->Invoke(copyNecessaryRegularNodesWorklet,
               this->NewVertexId, // input/output
               keptInBoundaryTree // input
  );

#ifdef DEBUG_PRINT
  VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                 this->DebugPrint("Compressed IDs Computed", __FILE__, __LINE__));
#endif
  //    2.  Work out the new superarcs, which is slightly tricky, since they point inbound.
  //      For each necessary vertex N, the inbound vertex I in the original contour tree can point to:
  //      i.    Another necessary vertex (in which case we keep the superarc)
  //      ii.    Nothing (in the case of the root) - again, we keep it, since we already know it's necessar
  //      iii.  An unnecessary vertex (i.e. any other case).  Here, the treatment is more complex.
  //          In this case, we know that the pointer-doubling has forced the up/down neighbours of I
  //          to point to necessary vertices (or nothing).  And since we know that there is an inbound
  //          edge from N, N must therefore be either the up or down neighbour of I after doubling.
  //          We therefore go the other way to find which necessary vertex V that N must connect to.
  //
  //      Just to make it interesting, the root vertex can become unnecessary. If this is the case, we end up with two
  //       necessary vertices each with a superarc to the other.  In serial, we can deal with this by checking to see whether
  //      the far end has already been set to ourself, but this doesn't parallelise properly.  So we will have an extra pass
  //      just to get this correct.
  //  We will compute the new superarcs directly with the new size
  //
  //  To do this in parallel, we compute for every vertex

  // first create the array: start by observing that the last entry is guaranteed
  // to hold the total number of necessary vertices
  this->NumKept =
    viskores::cont::ArrayGetValue(keptInBoundaryTree.GetNumberOfValues() - 1, keptInBoundaryTree);
  // create an array to store the new superarc Ids and initalize it with NO_SUCH_ELEMENT
  viskores::worklet::contourtree_augmented::IdArrayType newSuperarc;
  viskores::cont::Algorithm::Copy(
    viskores::cont::make_ArrayHandleConstant(
      viskores::worklet::contourtree_augmented::NO_SUCH_ELEMENT, this->NumKept),
    newSuperarc);
  auto findNewSuperarcsWorklet = bract_maker::CompressRegularisedNodesFindNewSuperarcsWorklet();
  this->Invoke(findNewSuperarcsWorklet,
               this->NewVertexId,                 //input
               this->BoundaryTreeData->Superarcs, //input
               this->UpNeighbour,                 //input
               this->DownNeighbour,               //input
               newSuperarc                        //output
  );

#ifdef DEBUG_PRINT
  VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                 this->DebugPrint("New Superarcs Found", __FILE__, __LINE__));
#endif

  //  3.  Now do the pass to resolve the root: choose the direction with decreasing index
  auto resolveRootWorklet = bract_maker::CompressRegularisedNodesResolveRootWorklet();
  this->Invoke(resolveRootWorklet,
               viskores::cont::ArrayHandleIndex(this->NumKept), // input
               newSuperarc                                      // output
  );

#ifdef DEBUG_PRINT
  VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                 this->DebugPrint("Root Resolved", __FILE__, __LINE__));
#endif

  // 4.  Now transfer the vertices & resize
  viskores::worklet::contourtree_augmented::IdArrayType newVertexIndex;
  newVertexIndex.Allocate(this->NumKept);
  auto transferVerticesWorklet = bract_maker::CompressRegularisedNodesTransferVerticesWorklet();
  this->Invoke(transferVerticesWorklet,
               this->BoundaryVertexSuperset, // input
               this->NewVertexId,            // input
               newVertexIndex                // output
  );

  //viskores::worklet::contourtree_augmented::PrintHeader(newVertexIndex.GetNumberOfValues(), std::cerr);
  //viskores::worklet::contourtree_augmented::PrintIndices("New Vertex Index", newVertexIndex, -1, std::cerr);

#ifdef DEBUG_PRINT
  VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                 this->DebugPrint("Vertices Transferred", __FILE__, __LINE__));
#endif

  // 5.  Create an index array and sort it indirectly by sortOrder
  viskores::worklet::contourtree_augmented::IdArrayType vertexSorter;
  viskores::cont::Algorithm::Copy(viskores::cont::ArrayHandleIndex(this->NumKept), vertexSorter);
  auto bractNodeComparator =
    bract_maker::BoundaryTreeNodeComparator(newVertexIndex, this->Mesh->SortIndices);
  viskores::cont::Algorithm::Sort(vertexSorter, bractNodeComparator);
  // 5.1. Compute the reverseSorter
  viskores::worklet::contourtree_augmented::IdArrayType reverseSorter;
  reverseSorter.Allocate(
    this->NumKept); // Reserve space since we need to permute the array for copy
  {
    auto permutedReverseSorter =
      viskores::cont::make_ArrayHandlePermutation(vertexSorter, reverseSorter);
    viskores::cont::Algorithm::Copy(viskores::cont::ArrayHandleIndex(this->NumKept),
                                    permutedReverseSorter);
  }

#ifdef DEBUG_PRINT
  VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                 this->DebugPrint("Indirect Sort Complete", __FILE__, __LINE__));
#endif

  //viskores::worklet::contourtree_augmented::PrintHeader(vertexSorter.GetNumberOfValues(), std::cerr);
  //viskores::worklet::contourtree_augmented::PrintIndices("Vertex Sorter", vertexSorter, -1, std::cerr);
  //viskores::worklet::contourtree_augmented::PrintIndices("Reverse Sorter", reverseSorter, -1, std::cerr);

  //    6.  Resize both vertex IDs and superarcs, and copy in by sorted order
  // copy the vertex index with indirection, and using the sort order NOT the regular ID
  // the following Copy operation is equivilant to
  // bract->vertexIndex[bractID] = mesh->SortIndex(newVertexIndex[vertexSorter[bractID]]);
  viskores::cont::Algorithm::Copy(
    viskores::cont::make_ArrayHandlePermutation(
      viskores::cont::make_ArrayHandlePermutation(vertexSorter, newVertexIndex),
      this->Mesh->SortIndices),
    this->BoundaryTreeData->VertexIndex);

  // now copy the this->BoundaryTreeData->Superarcs
  this->BoundaryTreeData->Superarcs.Allocate(this->NumKept, viskores::CopyFlag::On);
  auto fillBoundaryTreeSuperarcsWorklet =
    bract_maker::CompressRegularisedNodesFillBoundaryTreeSuperarcsWorklet();
  this->Invoke(fillBoundaryTreeSuperarcsWorklet,
               newSuperarc,
               reverseSorter,
               vertexSorter,
               this->BoundaryTreeData->Superarcs);

#ifdef DEBUG_PRINT
  VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                 this->DebugPrint("Regularised Nodes Compressed", __FILE__, __LINE__));
#endif
} // CompressRegularisedNodes


/// sets the arrays in the InteriorForest (i.e., the residue) that need to be passed to the grafting stage. In the original this function was called  SetResidue()
///
/// Side effects: This function updates:
/// - this->InteriorForestData->Above
/// - this->InteriorForestData->Below
/// - this->InteriorForestData->BoundaryTreeMeshIndices
template <typename MeshType, typename MeshBoundaryExecObjType>
void BoundaryTreeMaker<MeshType, MeshBoundaryExecObjType>::SetInteriorForest(
  const viskores::worklet::contourtree_augmented::mesh_dem::IdRelabeler* localToGlobalIdRelabeler)
{ // SetInteriorForest()
  // allocate memory for the residue arrays
  auto tempNoSuchElementArray = viskores::cont::make_ArrayHandleConstant(
    viskores::worklet::contourtree_augmented::NO_SUCH_ELEMENT,
    this->ContourTree.Supernodes.GetNumberOfValues());
  viskores::cont::Algorithm::Copy(tempNoSuchElementArray, this->InteriorForestData->Above);
  viskores::cont::Algorithm::Copy(tempNoSuchElementArray, this->InteriorForestData->Below);

  // now fill them in
  auto meshGlobalIds =
    this->Mesh
      ->template GetGlobalIdsFromMeshIndices<viskores::worklet::contourtree_augmented::IdArrayType>(
        this->BoundaryVertexSuperset, localToGlobalIdRelabeler);
  auto setInteriorForestWorklet =
    viskores::worklet::contourtree_distributed::bract_maker::SetInteriorForestWorklet();
  // NOTE: We don't need this->BoundaryVertexSuperset as input since meshGlobalIds is already transformed accordingly
  this->Invoke(setInteriorForestWorklet,
               this->ContourTree.Supernodes,          // input
               this->InteriorForestData->IsNecessary, // input
               this->TreeToSuperset,                  // input,
               meshGlobalIds,                         // input
               this->UpNeighbour,                     // input
               this->DownNeighbour,                   // input
               this->InteriorForestData->Above,       // output
               this->InteriorForestData->Below        // output
  );

  // now copy the mesh indices of the BRACT's vertices for the TreeGrafter to use
  InteriorForestData->BoundaryTreeMeshIndices.Allocate(
    this->BoundaryTreeData->VertexIndex.GetNumberOfValues());
  // per vertex in the bract, convert it to a sort ID and then mesh ID and copy to the InteriorForestData
  viskores::cont::Algorithm::Copy(viskores::cont::make_ArrayHandlePermutation(
                                    this->BoundaryTreeData->VertexIndex, this->Mesh->SortOrder),
                                  InteriorForestData->BoundaryTreeMeshIndices);
} // SetInteriorForest()



/// prints the contents of the restrictor object in a standard format
template <typename MeshType, typename MeshBoundaryExecObjType>
std::string BoundaryTreeMaker<MeshType, MeshBoundaryExecObjType>::DebugPrint(const char* message,
                                                                             const char* fileName,
                                                                             long lineNum) const
{ // DebugPrint
  std::stringstream resultStream;
  resultStream << std::setw(30) << std::left << fileName << ":" << std::right << std::setw(4)
               << lineNum << " ";
  resultStream << std::left << std::string(message) << std::endl;

  resultStream << "------------------------------------------------------" << std::endl;
  resultStream << "BRACT Contains:                                       " << std::endl;
  resultStream << "------------------------------------------------------" << std::endl;
  viskores::worklet::contourtree_augmented::PrintHeader(
    this->BoundaryTreeData->VertexIndex.GetNumberOfValues(), resultStream);
  viskores::worklet::contourtree_augmented::PrintIndices(
    "BRACT Vertices", this->BoundaryTreeData->VertexIndex, -1, resultStream);
  viskores::worklet::contourtree_augmented::PrintIndices(
    "BRACT Superarcs", this->BoundaryTreeData->Superarcs, -1, resultStream);
  resultStream << "------------------------------------------------------" << std::endl;
  resultStream << "BRACT Maker Contains:                                 " << std::endl;
  resultStream << "------------------------------------------------------" << std::endl;
  resultStream << "nBoundary:  " << this->NumBoundary << std::endl;
  resultStream << "nNecessary: " << this->NumNecessary << std::endl;

  // Regular Vertex Arrays
  viskores::worklet::contourtree_augmented::PrintHeader(this->BoundaryTreeId.GetNumberOfValues(),
                                                        resultStream);
  viskores::worklet::contourtree_augmented::PrintIndices(
    "ID in Boundary Tree", this->BoundaryTreeId, -1, resultStream);
  resultStream << std::endl;

  // Boundary Vertex Arrays
  viskores::worklet::contourtree_augmented::PrintHeader(this->BoundaryIndices.GetNumberOfValues(),
                                                        resultStream);
  viskores::worklet::contourtree_augmented::PrintIndices(
    "Boundary Sort Indices", this->BoundaryIndices, -1, resultStream);
  viskores::worklet::contourtree_augmented::PrintIndices(
    "Boundary Vertex Superset", this->BoundaryVertexSuperset, -1, resultStream);
  viskores::worklet::contourtree_augmented::PrintIndices(
    "Boundary Superparents", this->BoundarySuperparents, -1, resultStream);
  resultStream << std::endl;

  // Per Supernode Arrays
  viskores::worklet::contourtree_augmented::PrintHeader(
    this->SupernodeTransferBoundaryCount.GetNumberOfValues(), resultStream);
  viskores::worklet::contourtree_augmented::PrintIndices(
    "Supernode Transfer Count", this->SupernodeTransferBoundaryCount, -1, resultStream);
  viskores::worklet::contourtree_augmented::PrintIndices(
    "Superarc Intrinsic Count", this->SuperarcIntrinsicBoundaryCount, -1, resultStream);
  viskores::worklet::contourtree_augmented::PrintIndices(
    "Superarc Dependent Count", this->SuperarcDependentBoundaryCount, -1, resultStream);
  // Print IsNecessary as bool
  viskores::worklet::contourtree_augmented::PrintValues(
    "isNecessary", this->InteriorForestData->IsNecessary, -1, resultStream);
  viskores::worklet::contourtree_augmented::PrintIndices(
    "Tree To Superset", this->TreeToSuperset, -1, resultStream);
  resultStream << std::endl;

  // Per Hypernode Arrays
  viskores::worklet::contourtree_augmented::PrintHeader(
    this->HyperarcDependentBoundaryCount.GetNumberOfValues(), resultStream);
  viskores::worklet::contourtree_augmented::PrintIndices(
    "Hyperarc Dependent Count", this->HyperarcDependentBoundaryCount, -1, resultStream);
  resultStream << std::endl;

  // BRACT sized Arrays
  viskores::worklet::contourtree_augmented::PrintHeader(this->NewVertexId.GetNumberOfValues(),
                                                        resultStream);
  viskores::worklet::contourtree_augmented::PrintIndices(
    "New Vertex ID", this->NewVertexId, -1, resultStream);

  // arrays with double use & different sizes
  viskores::worklet::contourtree_augmented::PrintHeader(this->UpNeighbour.GetNumberOfValues(),
                                                        resultStream);
  viskores::worklet::contourtree_augmented::PrintIndices(
    "Up Neighbour", this->UpNeighbour, -1, resultStream);
  viskores::worklet::contourtree_augmented::PrintIndices(
    "Down Neighbour", this->DownNeighbour, -1, resultStream);

  resultStream << "------------------------------------------------------" << std::endl;
  resultStream << std::endl;
  resultStream << std::flush;
  return resultStream.str();
} // DebugPrint


} // namespace contourtree_distributed
} // namespace worklet
} // namespace viskores

#endif
