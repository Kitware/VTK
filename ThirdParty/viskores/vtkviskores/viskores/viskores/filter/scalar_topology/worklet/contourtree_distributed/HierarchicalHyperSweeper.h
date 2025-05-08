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
//=======================================================================================
//
//	Parallel Peak Pruning v. 2.0
//
//	Started June 15, 2017
//
// Copyright Hamish Carr, University of Leeds
//
// HierarchicalHyperSweeper.h
//
//=======================================================================================
//
// COMMENTS:
//
//	This class encapsulates a hypersweep over the hierarchical contour tree.  It is a
//	separate class primarily to keep the post-processing separate from the main tree
//	construction, but it should also make it easier to generalise to arbitrary computations
//
//	Basically, the way that this operates is:
//	1.	First, we do a local (standard) hypersweep over the hierarchical tree
//	2.	We then fan-in one round at a time.  In each round,
//		a.	We trade the prefix of the array with our logical partner, then
//		b.	Combine the array prefix with our own
//
//	Tactically, when we do MPI, we can either embed it in this unit, or leave it in the
//	calling unit.  For ease of porting, we will leave all MPI in the calling unit, so
//	this unit only needs to do the combination.
//
//	Note that we could define an operator to be passed in, and probably want to template it
//	that way in the future, but for now, we'll do the first version directly with addition
//
//	By assumption, we need a commutative property, since we do not guarantee that we have
//	strict ordering along superarcs (which would require sharing a supernode sort with our
//	partner, if not globally)
//
//=======================================================================================

#ifndef viskores_worklet_contourtree_distributed_hierarchical_hyper_sweeper_h
#define viskores_worklet_contourtree_distributed_hierarchical_hyper_sweeper_h

#include <iomanip>
#include <string>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/PrintVectors.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/Types.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/data_set_mesh/IdRelabeler.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/HierarchicalContourTree.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/PrintGraph.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/hierarchical_hyper_sweeper/ComputeSuperarcDependentWeightsWorklet.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/hierarchical_hyper_sweeper/ComputeSuperarcTransferWeightsWorklet.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/hierarchical_hyper_sweeper/InitializeIntrinsicVertexCountComputeSuperparentIdsWorklet.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/hierarchical_hyper_sweeper/InitializeIntrinsicVertexCountInitalizeCountsWorklet.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/hierarchical_hyper_sweeper/InitializeIntrinsicVertexCountSubtractLowEndWorklet.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/hierarchical_hyper_sweeper/TransferTargetComperator.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/hierarchical_hyper_sweeper/TransferWeightsUpdateLHEWorklet.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/hierarchical_hyper_sweeper/TransferWeightsUpdateLHEWorkletRound2.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/hierarchical_hyper_sweeper/TransferWeightsUpdateRHEWorklet.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/hierarchical_hyper_sweeper/TransferWeightsUpdateRHEWorkletRound2.h>

#ifdef DEBUG_PRINT
#define DEBUG_PRINT_HYPER_SWEEPER
#define DEBUG_PRINT_HIERARCHICAL_CONTOUR_TREE
#endif // DEBUG_PRINT

namespace viskores
{
namespace worklet
{
namespace contourtree_distributed
{

// the class itself
template <typename SweepValueType, typename ContourTreeFieldType>
class HierarchicalHyperSweeper
{ // class HierarchicalHyperSweeper
public:
  // the tree that it hypersweeps over
  const HierarchicalContourTree<ContourTreeFieldType>& HierarchicalTree;

  // the Id of the base block (used for debug output)
  viskores::Id BlockId;

  // array of values being operated over (same size as supernode set)
  // keep both intrinsic & dependent values
  // the intrinsic values are just stored but not modifid here
  const viskores::cont::ArrayHandle<SweepValueType>& IntrinsicValues;
  // the dependent values are what is being sweeped and are updated here
  const viskores::cont::ArrayHandle<SweepValueType>& DependentValues;
  // and to avoid an extra log summation, store the number of logical nodes for the underlying block
  // (computed when initializing the regular vertex list)
  viskores::Id NumOwnedRegularVertices;


  // these are working arrays, lifted up here for ease of debug code
  // Subranges of these arrays will be reused in the rounds / iterations rather than being reallocated
  // an array for temporary storage of the prefix sums
  viskores::cont::ArrayHandle<SweepValueType> ValuePrefixSum;
  // two arrays for collecting targets of transfers
  viskores::worklet::contourtree_augmented::IdArrayType TransferTarget;
  viskores::worklet::contourtree_augmented::IdArrayType SortedTransferTarget;
  // an array for indirect sorting of sets of superarcs
  viskores::worklet::contourtree_augmented::IdArrayType SuperSortPermute;

  /// Constructor
  /// @param[in] blockId  The Id of the base block (used for debug output)
  /// @param[in] hierarchicalTree the tree that to hypersweeps over
  /// @param[in] intrinsicValues array of values of intrinisic nodes are just being stored here but not modified
  /// @param[in] dependentValues array of values being operated over (same size as supernode set)
  HierarchicalHyperSweeper<SweepValueType, ContourTreeFieldType>(
    viskores::Id blockId,
    const HierarchicalContourTree<ContourTreeFieldType>& hierarchicalTree,
    const viskores::cont::ArrayHandle<SweepValueType>& intrinsicValues,
    const viskores::cont::ArrayHandle<SweepValueType>& dependentValues);

  /// Our routines to initialize the sweep need to be static (or externa)l if we are going to use the constructor
  /// to run the actual hypersweep
  /// @param[in] hierarchicalTree the tree that to hypersweeps over
  /// @param[in] baseBlock the underlying mesh base block  to initialize from
  /// @param[in] localToGlobalIdRelabeler Id relabeler used to compute global indices from local mesh indices
  /// @param[out] superarcRegularCounts   arrray for the output superarc regular counts
  template <typename MeshType>
  void InitializeIntrinsicVertexCount(
    const HierarchicalContourTree<ContourTreeFieldType>& hierarchicalTree,
    const MeshType& baseBlock,
    const viskores::worklet::contourtree_augmented::mesh_dem::IdRelabeler& localToGlobalIdRelabeler,
    viskores::worklet::contourtree_augmented::IdArrayType& superarcRegularCounts);

  /// routine to do the local hypersweep using addition / subtraction
  /// The funtion use the ComputeSuperarcDependentWeights, ComputeSuperarcTransferWeights,
  /// and TransferWeights functions to carry out the local hyper sweep
  void LocalHyperSweep();

  /// Debug routine to print contents of the HiearchicalHyperSweep
  /// @param[in] message Message to print along the debug output
  /// @param[in] fileName Name of the file the message is printed from. Usually set to __FILE__
  /// @param[in] lineNum Line number in the file where the message is printed from. Usually set to __LINE__
  std::string DebugPrint(std::string message, const char* fileName, long lineNum) const;

  /// Routine to save the HierarchicalContourTree of this HierarchicalHyperSweeper to a Dot file
  /// @param[in] message Message included in the file
  /// @param[in] outFileName The name of the file to write the
  void SaveHierarchicalContourTreeDot(std::string message, const char* outFileName) const;

protected:
  // Functions used internally be LocalHyperSweep to compute the local hyper sweep

  /// Routine to compute the correct weights dependent on each superarc in a subrange (defined by the round & iteration)
  void ComputeSuperarcDependentWeights(viskores::Id round,
                                       viskores::Id iteration,
                                       viskores::Id firstSupernode,
                                       viskores::Id lastSupernode);

  /// routine to compute the weights to transfer to superarcs (defined by the round & iteration)
  void ComputeSuperarcTransferWeights(viskores::Id round,
                                      viskores::Id iteration,
                                      viskores::Id firstSupernode,
                                      viskores::Id lastSupernode);

  /// routine to transfer the weights
  void TransferWeights(viskores::Id round,
                       viskores::Id iteration,
                       viskores::Id firstSupernode,
                       viskores::Id lastSupernode);

private:
  /// Used internally to Invoke worklets
  viskores::cont::Invoker Invoke;

}; // class HierarchicalHyperSweeper

template <typename SweepValueType, typename ContourTreeFieldType>
HierarchicalHyperSweeper<SweepValueType, ContourTreeFieldType>::HierarchicalHyperSweeper(
  viskores::Id blockId,
  const HierarchicalContourTree<ContourTreeFieldType>& hierarchicalTree,
  const viskores::cont::ArrayHandle<SweepValueType>& intrinsicValues,
  const viskores::cont::ArrayHandle<SweepValueType>& dependentValues)
  : HierarchicalTree(hierarchicalTree)
  , BlockId(blockId)
  , IntrinsicValues(intrinsicValues)
  , DependentValues(dependentValues)
  , NumOwnedRegularVertices(viskores::Id{ 0 })
{ // constructor
  // Initalize arrays with 0s
  this->ValuePrefixSum.AllocateAndFill(this->HierarchicalTree.Supernodes.GetNumberOfValues(), 0);
  this->TransferTarget.AllocateAndFill(this->HierarchicalTree.Supernodes.GetNumberOfValues(), 0);
  this->SortedTransferTarget.AllocateAndFill(this->HierarchicalTree.Supernodes.GetNumberOfValues(),
                                             0);
  // Initialize the supersortPermute to the identity
  viskores::cont::ArrayHandleIndex tempIndexArray(
    this->HierarchicalTree.Supernodes.GetNumberOfValues());
  viskores::cont::Algorithm::Copy(tempIndexArray, this->SuperSortPermute);
} // constructor


// static function used to compute the initial superarc regular counts
template <typename SweepValueType, typename ContourTreeFieldType>
template <typename MeshType>
void HierarchicalHyperSweeper<SweepValueType, ContourTreeFieldType>::InitializeIntrinsicVertexCount(
  const HierarchicalContourTree<ContourTreeFieldType>& hierarchicalTree,
  const MeshType& baseBlock,
  const viskores::worklet::contourtree_augmented::mesh_dem::IdRelabeler& localToGlobalIdRelabeler,
  viskores::worklet::contourtree_augmented::IdArrayType& superarcRegularCounts)
{ // InitializeIntrinsicVertexCount()
  // I.  Call the mesh to get a list of all regular vertices belonging to the block by global Id
  viskores::worklet::contourtree_augmented::IdArrayType globalIds;
  // NOTE: Even though the virtual function on DataSetMesh was removed in commit
  // 93730495813f7b85e59d4a5dae2076977787fd78, this should call the correct function
  // since MeshType is templated and should have the appropriate type.
  baseBlock.GetOwnedVerticesByGlobalId(localToGlobalIdRelabeler, globalIds);
  // and store the size for later reference
  //hierarchicalTree.NumOwnedRegularVertices = globalIds.GetNumberOfValues();
  this->NumOwnedRegularVertices = globalIds.GetNumberOfValues();

#ifdef DEBUG_PRINT_HYPER_SWEEPER
  {
    std::stringstream debugStream;
    debugStream << std::endl << "Owned Regular Vertex List" << std::endl;
    viskores::worklet::contourtree_augmented::PrintHeader(globalIds.GetNumberOfValues(),
                                                          debugStream);
    viskores::worklet::contourtree_augmented::PrintIndices("GlobalId", globalIds, -1, debugStream);
    VISKORES_LOG_S(viskores::cont::LogLevel::Info, debugStream.str());
  }
#endif

  // II.  Look up the global Ids in the hierarchical tree & convert to superparent Ids
  viskores::worklet::contourtree_augmented::IdArrayType superparents;
  { // scope to make sure temporary variables are deleted
    auto findRegularByGlobal = hierarchicalTree.GetFindRegularByGlobal();
    auto computeSuperparentIdsWorklet = viskores::worklet::contourtree_distributed::
      hierarchical_hyper_sweeper::InitializeIntrinsicVertexCountComputeSuperparentIdsWorklet();
    Invoke(computeSuperparentIdsWorklet,       // worklet to run
           globalIds,                          // input
           findRegularByGlobal,                // input
           hierarchicalTree.Regular2Supernode, // input
           hierarchicalTree.Superparents,      // input
           superparents                        // output
    );
  }

#ifdef DEBUG_PRINT_HYPER_SWEEPER
  {
    std::stringstream debugStream;
    debugStream << std::endl;
    viskores::worklet::contourtree_augmented::PrintIndices(
      "Superparents", superparents, -1, debugStream);
    VISKORES_LOG_S(viskores::cont::LogLevel::Info, debugStream.str());
  }
#endif

  // III.  Sort the superparent Ids & count the copies of each
  viskores::cont::Algorithm ::Sort(superparents);

#ifdef DEBUG_PRINT_HYPER_SWEEPER
  {
    std::stringstream debugStream;
    debugStream << std::endl;
    viskores::worklet::contourtree_augmented::PrintIndices(
      "Sorted SP", superparents, -1, debugStream);
    VISKORES_LOG_S(viskores::cont::LogLevel::Info, debugStream.str());
  }
#endif

  // initialize the counts to zero.
  superarcRegularCounts.AllocateAndFill(this->HierarchicalTree.Supernodes.GetNumberOfValues(), 0);

  // set the count to the Id one off the high end of each range
  Invoke(viskores::worklet::contourtree_distributed::hierarchical_hyper_sweeper::
           InitializeIntrinsicVertexCountInitalizeCountsWorklet{},
         superparents,         // input domain
         superarcRegularCounts // output
  );

  // now repeat to subtract out the low end
  Invoke(viskores::worklet::contourtree_distributed::hierarchical_hyper_sweeper::
           InitializeIntrinsicVertexCountSubtractLowEndWorklet{},
         superparents,         // input domain
         superarcRegularCounts // output
  );
  // and that is that
#ifdef DEBUG_PRINT_HYPER_SWEEPER
  {
    std::stringstream debugStream;
    debugStream << std::endl;
    viskores::worklet::contourtree_augmented::PrintIndices(
      "SuperarcRegularCounts", superarcRegularCounts, -1, debugStream);
    VISKORES_LOG_S(viskores::cont::LogLevel::Info, debugStream.str());
  }
#endif
} // InitializeIntrinsicVertexCount()


// routine to do the local hypersweep using addition / subtraction
template <typename SweepValueType, typename ContourTreeFieldType>
void HierarchicalHyperSweeper<SweepValueType, ContourTreeFieldType>::LocalHyperSweep()
{ // LocalHyperSweep()
#ifdef DEBUG_PRINT_HYPER_SWEEPER
  VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                 DebugPrint(std::string("Hypersweep Block ") + std::to_string(BlockId) +
                              std::string(" Starting Local HyperSweep"),
                            __FILE__,
                            __LINE__));
#endif

  // I.  Iterate over all rounds of the hyperstructure
  for (viskores::Id round = 0; round <= this->HierarchicalTree.NumRounds; round++)
  { // per round
#ifdef DEBUG_PRINT_HYPER_SWEEPER
    VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                   DebugPrint(std::string("Hypersweep Block ") + std::to_string(BlockId) +
                                std::string(" Round ") + std::to_string(round) +
                                std::string(" Step 0 Starting Round"),
                              __FILE__,
                              __LINE__));
#endif
    //  A.  Iterate over all iterations of the round
    auto numIterationsPortal =
      this->HierarchicalTree.NumIterations
        .ReadPortal(); // TODO/FIXME: Use portal? Or something more efficient?
    for (viskores::Id iteration = 0; iteration < numIterationsPortal.Get(round); iteration++)
    { // per iteration
#ifdef DEBUG_PRINT_HYPER_SWEEPER
      VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                     DebugPrint(std::string("Hypersweep Block ") + std::to_string(BlockId) +
                                  std::string(" Round ") + std::to_string(round) +
                                  std::string(" Step 1 Iteration ") + std::to_string(iteration) +
                                  std::string(" Step A Starting Iteration"),
                                __FILE__,
                                __LINE__));
#endif
      //  1.  Establish the range of supernode Ids that we want to process
      //  TODO/FIXME: Use portal? Or is there a more efficient way?
      auto firstSupernodePerIterationPortal =
        this->HierarchicalTree.FirstSupernodePerIteration[round].ReadPortal();

      viskores::Id firstSupernode = firstSupernodePerIterationPortal.Get(iteration);
      viskores::Id lastSupernode = firstSupernodePerIterationPortal.Get(iteration + 1);

      // call the routine that computes the dependent weights for each superarc in that range
      this->ComputeSuperarcDependentWeights(round, iteration, firstSupernode, lastSupernode);

#ifdef DEBUG_PRINT_HYPER_SWEEPER
      VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                     DebugPrint(std::string("Hypersweep Block ") + std::to_string(BlockId) +
                                  std::string(" Round ") + std::to_string(round) +
                                  std::string(" Step 1 Iteration ") + std::to_string(iteration) +
                                  std::string(" Step B Dependent Weights Computed"),
                                __FILE__,
                                __LINE__));
#endif
      // now call the routine that computes the weights to be transferred and the superarcs to which they transfer
      this->ComputeSuperarcTransferWeights(round, iteration, firstSupernode, lastSupernode);

#ifdef DEBUG_PRINT_HYPER_SWEEPER
      VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                     DebugPrint(std::string("Hypersweep Block ") + std::to_string(BlockId) +
                                  std::string(" Round ") + std::to_string(round) +
                                  std::string(" Step 1 Iteration ") + std::to_string(iteration) +
                                  std::string(" Step C Transfer Weights Computed"),
                                __FILE__,
                                __LINE__));
#endif

      // transfer the weights
      this->TransferWeights(round, iteration, firstSupernode, lastSupernode);

#ifdef DEBUG_PRINT_HYPER_SWEEPER
      VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                     DebugPrint(std::string("Hypersweep Block ") + std::to_string(BlockId) +
                                  std::string(" Round ") + std::to_string(round) +
                                  std::string(" Step 1 Iteration ") + std::to_string(iteration) +
                                  std::string(" Step D Weights Transferred"),
                                __FILE__,
                                __LINE__));
#endif
    } // per iteration

#ifdef DEBUG_PRINT_HYPER_SWEEPER
    VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                   DebugPrint(std::string("Hypersweep Block ") + std::to_string(BlockId) +
                                std::string(" Round ") + std::to_string(round) +
                                std::string(" Step 2 Ending Round"),
                              __FILE__,
                              __LINE__));
#endif
  } // per round
} // LocalHyperSweep()


// routine to compute the correct weights dependent on each superarc in a subrange (defined by the round & iteration)
template <typename SweepValueType, typename ContourTreeFieldType>
void HierarchicalHyperSweeper<SweepValueType, ContourTreeFieldType>::
  ComputeSuperarcDependentWeights(
    viskores::Id round,
    viskores::Id, // iteration, // Kept parameter in case we need it for debugging.
    viskores::Id firstSupernode,
    viskores::Id lastSupernode)
{ // ComputeSuperarcDependentWeights()
  viskores::Id numSupernodesToProcess = lastSupernode - firstSupernode;
  //  2.  Use sorted prefix sum to compute the total weight to contribute to the super/hypertarget
  // Same as std::partial_sum(sweepValues.begin() + firstSupernode, sweepValues.begin() + lastSupernode, valuePrefixSum.begin() + firstSupernode);
  {
    // DependentValues[firstSuperNode, lastSupernode)
    viskores::cont::ArrayHandleView<viskores::worklet::contourtree_augmented::IdArrayType>
      dependentValuesView(this->DependentValues,   // subset DependentValues
                          firstSupernode,          // start at firstSupernode
                          numSupernodesToProcess); // until lastSuperNode (not inclued)
    // Target array
    viskores::cont::ArrayHandleView<viskores::worklet::contourtree_augmented::IdArrayType>
      valuePrefixSumView(this->ValuePrefixSum,    // subset ValuePrefixSum
                         firstSupernode,          // start at firstSupernode
                         numSupernodesToProcess); // until lastSuperNode (not inclued)
    // Compute the partial sum for DependentValues[firstSuperNode, lastSupernode) and write to ValuePrefixSum[firstSuperNode, lastSupernode)
    viskores::cont::Algorithm::ScanInclusive(dependentValuesView, // input
                                             valuePrefixSumView); // result of partial sum
  }
  // Since the prefix sum is over *all* supernodes in the iteration, we need to break it into segments
  // There are two cases we have to worry about:
  // a.  Hyperarcs made up of multiple supernodes
  // b.  Attachment points (which don't have a corresponding hyperarc)
  // and they can be mixed in any given iteration

  // Since we have the prefix sum in a separate array, we avoid read/write conflicts

  // 3.  Compute the segmented weights from the prefix sum array
  {
    // Create views of the subranges of the arrays we need to update
    viskores::cont::ArrayHandleCounting<viskores::Id> supernodeIndex(
      firstSupernode, viskores::Id{ 1 }, numSupernodesToProcess);
    viskores::cont::ArrayHandleView<viskores::worklet::contourtree_augmented::IdArrayType>
      hierarchicalTreeSuperarcsView(
        this->HierarchicalTree.Superarcs, firstSupernode, numSupernodesToProcess);
    viskores::cont::ArrayHandleView<viskores::worklet::contourtree_augmented::IdArrayType>
      hierarchicalTreeHyperparentsView(
        this->HierarchicalTree.Hyperparents, firstSupernode, numSupernodesToProcess);
    viskores::cont::ArrayHandleView<viskores::worklet::contourtree_augmented::IdArrayType>
      hierarchicalTreeHypernodesView(
        this->HierarchicalTree.Hypernodes, firstSupernode, numSupernodesToProcess);
    viskores::cont::ArrayHandleView<viskores::worklet::contourtree_augmented::IdArrayType>
      dependentValuesView(this->DependentValues, firstSupernode, numSupernodesToProcess);
    // create the worklet
    viskores::worklet::contourtree_distributed::hierarchical_hyper_sweeper::
      ComputeSuperarcDependentWeightsWorklet<SweepValueType>
        computeSuperarcDependentWeightsWorklet(
          firstSupernode, round, this->HierarchicalTree.NumRounds);
    // Execute the worklet
    this->Invoke(
      computeSuperarcDependentWeightsWorklet, // the worklet
      supernodeIndex,                // input counting index [firstSupernode, lastSupernode)
      hierarchicalTreeSuperarcsView, // input view of  hierarchicalTree.Superarcs[firstSupernode, lastSupernode)
      hierarchicalTreeHyperparentsView, // input view of  hierarchicalTree.Hyperparents[firstSupernode, lastSupernode)
      this->HierarchicalTree.Hypernodes, // input full hierarchicalTree.Hypernodes array
      this->ValuePrefixSum,              // input full ValuePrefixSum array
      dependentValuesView                // output view of sweepValues[firstSu
    );
  }
} // ComputeSuperarcDependentWeights()


// routine to compute the weights to transfer to superarcs (defined by the round & iteration)
template <typename SweepValueType, typename ContourTreeFieldType>
void HierarchicalHyperSweeper<SweepValueType, ContourTreeFieldType>::ComputeSuperarcTransferWeights(
  viskores::Id round,
  viskores::Id, // iteration, // Kept parameter in case we need it for debugging.
  viskores::Id firstSupernode,
  viskores::Id lastSupernode)
{ // ComputeSuperarcTransferWeights()
  // At this stage, we would otherwise transfer weights by hyperarc, but attachment points don't *have* hyperarcs
  // so we will do a transfer by superarc instead, making sure that we only transfer from the last superarc in each
  // hyperarc, plus for any attachment point
  viskores::Id numSupernodesToProcess = lastSupernode - firstSupernode;

  // 4.  Set the amount each superarc wants to transfer, reusing the valuePrefixSum array for the purpose
  //    and the transfer target
  { // scope ComputeSuperarcTransferWeightsWorklet to make sure temp variables are cleared
    // Create ArrayHandleViews of the subrange of values that we need to update
    viskores::cont::ArrayHandleCounting<viskores::Id> supernodeIndex(
      firstSupernode, viskores::Id{ 1 }, numSupernodesToProcess);
    viskores::cont::ArrayHandleView<viskores::worklet::contourtree_augmented::IdArrayType>
      hierarchicalTreeSupernodesView(
        this->HierarchicalTree.Supernodes, firstSupernode, numSupernodesToProcess);
    viskores::cont::ArrayHandleView<viskores::worklet::contourtree_augmented::IdArrayType>
      hierarchicalTreeSuperarcsView(
        this->HierarchicalTree.Superarcs, firstSupernode, numSupernodesToProcess);
    viskores::cont::ArrayHandleView<viskores::worklet::contourtree_augmented::IdArrayType>
      transferTargetView(this->TransferTarget, firstSupernode, numSupernodesToProcess);
    // instantiate the worklet
    viskores::worklet::contourtree_distributed::hierarchical_hyper_sweeper::
      ComputeSuperarcTransferWeightsWorklet computeSuperarcTransferWeightsWorklet(
        round, this->HierarchicalTree.NumRounds, lastSupernode);
    // call the worklet
    this->Invoke(
      computeSuperarcTransferWeightsWorklet, // worklet
      supernodeIndex,                        // input counting array [firstSupernode, lastSupernode)
      hierarchicalTreeSupernodesView, // input view of hierarchicalTree.supernodes[firstSupernode, lastSupernode)
      this->HierarchicalTree.Superparents, // input whole array of hierarchicalTree.superparents
      this->HierarchicalTree.Hyperparents, // input whole array of hierarchicalTree.hyperparents
      hierarchicalTreeSuperarcsView, // input/output view of hierarchicalTree.superarcs[firstSupernode, lastSupernode)
      transferTargetView // input view of transferTarget[firstSupernode, lastSupernode)
    );
  } // scope ComputeSuperarcTransferWeightsWorklet

  // 5. Now we need to sort the transfer targets into contiguous segments
  // NOTE 11/07/2023
  // We have now got a flag of ATTACHMENT_POINT_TRANSFER whose effect is to separate out transfers to
  // the superarc from transfers to the supernode
  {
    // create view of superSortPermute[firstSupernode, lastSupernode) for sorting
    viskores::cont::ArrayHandleView<viskores::worklet::contourtree_augmented::IdArrayType>
      superSortPermuteView(this->SuperSortPermute, firstSupernode, numSupernodesToProcess);
    // create comperator for the sort
    viskores::worklet::contourtree_distributed::hierarchical_hyper_sweeper::TransferTargetComperator
      transferTargetComperator(this->TransferTarget);
    // sort the subrange of our array
    viskores::cont::Algorithm::Sort(superSortPermuteView, transferTargetComperator);
  }

  // 6. The [first,last] subrange is now permuted, so we can copy the transfer targets and weights into arrays
  //    The following code block implements the following for loop using fancy array handles and copy
  //    for (viskores::Id supernode = firstSupernode; supernode < lastSupernode; supernode++)
  //    {
  //       sortedTransferTarget[supernode] = transferTarget[superSortPermute[supernode]];
  //       valuePrefixSum[supernode] = sweepValues[superSortPermute[supernode]];
  //    }
  {
    // copy transfer target in the sorted order
    viskores::cont::ArrayHandleView<viskores::worklet::contourtree_augmented::IdArrayType>
      sortedTransferTargetView(this->SortedTransferTarget, firstSupernode, numSupernodesToProcess);
    viskores::cont::ArrayHandleView<viskores::worklet::contourtree_augmented::IdArrayType>
      superSortPermuteView(this->SuperSortPermute, firstSupernode, numSupernodesToProcess);
    auto permutedTransferTarget =
      viskores::cont::make_ArrayHandlePermutation(superSortPermuteView,  // idArray
                                                  this->TransferTarget); // valueArray
    viskores::cont::Algorithm::Copy(permutedTransferTarget, sortedTransferTargetView);
    // Note that any values associated with NO_SUCH_ELEMENT will be ignored
    // copy transfer weight in the sorted order
    viskores::cont::ArrayHandleView<viskores::worklet::contourtree_augmented::IdArrayType>
      valuePrefixSumView(this->ValuePrefixSum, firstSupernode, numSupernodesToProcess);
    auto permutedDependentValues =
      viskores::cont::make_ArrayHandlePermutation(superSortPermuteView,   // idArray
                                                  this->DependentValues); // valueArray
    viskores::cont::Algorithm::Copy(permutedDependentValues, valuePrefixSumView);
  }
} // ComputeSuperarcTransferWeights()


// routine to transfer the weights
template <typename SweepValueType, typename ContourTreeFieldType>
void HierarchicalHyperSweeper<SweepValueType, ContourTreeFieldType>::TransferWeights(
#ifdef DEBUG_PRINT_HYPER_SWEEPER
  viskores::Id round,     // Kept parameters in case we need it for debugging.
  viskores::Id iteration, // Kept parameters in case we need it for debugging.
#else
  viskores::Id,
  viskores::Id,
#endif
  viskores::Id firstSupernode,
  viskores::Id lastSupernode)
{ // TransferWeights()
  // WARNING 11/07/2023
  // This code was originally written on the assumption that the hierarchical tree had been augmented by the attachment points.
  // As a result, it assumed that no attachment points remained.
  // It is now being used for partially augmented versions due to pre-simplification, for which the correct treatment is to
  // transfer not as dependent weight, but as intrinsic weight. Note that this ONLY applies to attachment points: if the
  // subtree attaches at a proper supernode in the ancestor level, it should still be treated as dependent weight. The logic
  // behind this is that an attachment point is regular with respect to the superarc along which it inserts.  Adding it as
  // dependent weight means that it is treated as *OUTSIDE* the superarc in the reverse sweep (or equivalent computation)
  // Treating it as dependent weight means that both ends of the superarc end up with the correct value.

  // 7. Now perform a segmented prefix sum
  viskores::Id numSupernodesToProcess = lastSupernode - firstSupernode;
  // Same as std::partial_sum(valuePrefixSum.begin() + firstSupernode, valuePrefixSum.begin() + lastSupernode, valuePrefixSum.begin() + firstSupernode);
  {
    // ValuePrefixSum[firstSuperNode, lastSupernode)
    viskores::cont::ArrayHandleView<viskores::worklet::contourtree_augmented::IdArrayType>
      valuePrefixSumView(this->ValuePrefixSum,    // subset ValuePrefixSum
                         firstSupernode,          // start at firstSupernode
                         numSupernodesToProcess); // until lastSuperNode (not inclued)
    // For safety we store the values of our prefix sum in a temporary array
    // and then copy the values back into our valuePrefixSumView at the end
    viskores::worklet::contourtree_augmented::IdArrayType tempScanInclusiveTarget;
    tempScanInclusiveTarget.Allocate(numSupernodesToProcess);
    // Compute the partial sum for DependentValues[firstSuperNode, lastSupernode) and write to ValuePrefixSum[firstSuperNode, lastSupernode)
    viskores::cont::Algorithm::ScanInclusive(valuePrefixSumView,       // input
                                             tempScanInclusiveTarget); // result of partial sum
    // Now copy the values from our prefix sum back
    viskores::cont::Algorithm::Copy(tempScanInclusiveTarget, valuePrefixSumView);
  }

#ifdef DEBUG_PRINT_HYPER_SWEEPER
  VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                 DebugPrint(std::string("Hypersweep Block ") + std::to_string(this->BlockId) +
                              std::string(" Round ") + std::to_string(round) +
                              std::string(" Step 1 Iteration ") + std::to_string(iteration) +
                              std::string(" Starting Weight Transfer"),
                            __FILE__,
                            __LINE__));
#endif // DEBUG_PRINT_HYPER_SWEEPER

  // 7a. and 7b.
  {
    // WARNING 11/07/2023
    // Before dealing with attachment points, we just transferred by segments.  We now have
    // the possibility of transferring some weight at an attachment point,
    // and some not.  To avoid write conflicts, we treat this as two passes: one for attachment
    // points, one for all others. This means duplicating 7a/7b, sadly.

    // 7a. Find the RHE of each group and transfer the prefix sum weight
    // Note that we do not compute the transfer weight separately, we add it in place instead
    // Instantiate the worklet
    auto supernodeIndex = viskores::cont::make_ArrayHandleCounting(
      firstSupernode, viskores::Id{ 1 }, numSupernodesToProcess);
    VISKORES_ASSERT(firstSupernode + numSupernodesToProcess <=
                    this->ValuePrefixSum.GetNumberOfValues());
    auto valuePrefixSumView = viskores::cont::make_ArrayHandleView(
      this->ValuePrefixSum, firstSupernode, numSupernodesToProcess);

    viskores::worklet::contourtree_distributed::hierarchical_hyper_sweeper::
      TransferWeightsUpdateRHEWorklet transferWeightsUpdateRHEWorklet(lastSupernode);
    // Invoke the worklet
    this->Invoke(transferWeightsUpdateRHEWorklet, // worklet
                 supernodeIndex, // input counting array [firstSupernode, lastSupernode)
                 this->SortedTransferTarget,
                 valuePrefixSumView, // input view of valuePrefixSum[firstSupernode, lastSupernode)
                 this->DependentValues);
  }

#ifdef DEBUG_PRINT_HYPER_SWEEPER
  VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                 DebugPrint(std::string("Hypersweep Block ") + std::to_string(this->BlockId) +
                              std::string(" Round ") + std::to_string(round) +
                              std::string(" Step 1 Iteration ") + std::to_string(iteration) +
                              std::string(" Non-Attachment LHE Completed"),
                            __FILE__,
                            __LINE__));
#endif // DEBUG_PRINT_HYPER_SWEEPER

  {
    VISKORES_ASSERT(firstSupernode + 1 + numSupernodesToProcess - 1 <=
                    this->SortedTransferTarget.GetNumberOfValues());
    auto sortedTransferTargetView = viskores::cont::make_ArrayHandleView(
      this->SortedTransferTarget, firstSupernode + 1, numSupernodesToProcess - 1);
    VISKORES_ASSERT(firstSupernode + 1 + numSupernodesToProcess - 1 <=
                    this->SortedTransferTarget.GetNumberOfValues());
    auto sortedTransferTargetShiftedView = viskores::cont::make_ArrayHandleView(
      this->SortedTransferTarget, firstSupernode, numSupernodesToProcess - 1);
    auto valuePrefixSumPreviousValueView = viskores::cont::make_ArrayHandleView(
      this->ValuePrefixSum, firstSupernode, numSupernodesToProcess - 1);

    // 7b (non-attachment). Now find the LHE of each group and subtract out the prior weight.
    viskores::worklet::contourtree_distributed::hierarchical_hyper_sweeper::
      TransferWeightsUpdateLHEWorklet transferWeightsUpdateLHEWorklet;
    this->Invoke(transferWeightsUpdateLHEWorklet,
                 sortedTransferTargetView,
                 sortedTransferTargetShiftedView,
                 valuePrefixSumPreviousValueView,
                 this->DependentValues);
  }

#ifdef DEBUG_PRINT_HYPER_SWEEPER
  VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                 DebugPrint(std::string("Hypersweep Block ") + std::to_string(this->BlockId) +
                              std::string(" Round ") + std::to_string(round) +
                              std::string(" Step 1 Iteration ") + std::to_string(iteration) +
                              std::string(" Non-Attachment RHE Completed"),
                            __FILE__,
                            __LINE__));
#endif

  // 7a (attachment). Find the RHE of each group and transfer the prefix sum weight
  // Note that we do not compute the transfer weight separately, we add it in place instead
  {
    auto supernodeIndex = viskores::cont::make_ArrayHandleCounting(
      firstSupernode, viskores::Id{ 1 }, numSupernodesToProcess);
    auto valuePrefixSumView = viskores::cont::make_ArrayHandleView(
      this->ValuePrefixSum, firstSupernode, numSupernodesToProcess);

    viskores::worklet::contourtree_distributed::hierarchical_hyper_sweeper::
      TransferWeightsUpdateRHEWorkletRound2 transferWeightsUpdateRHEWorkletRound2(lastSupernode);
    // Invoke the worklet
    this->Invoke(transferWeightsUpdateRHEWorkletRound2, // worklet
                 supernodeIndex, // input counting array [firstSupernode, lastSupernode)
                 this->SortedTransferTarget,
                 valuePrefixSumView, // input view of valuePrefixSum[firstSupernode, lastSupernode)
                 this->IntrinsicValues,
                 this->DependentValues);
  }

#ifdef DEBUG_PRINT_HYPER_SWEEPER
  VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                 DebugPrint(std::string("Hypersweep Block ") + std::to_string(this->BlockId) +
                              std::string(" Round ") + std::to_string(round) +
                              std::string(" Step 1 Iteration ") + std::to_string(iteration) +
                              std::string(" Attachment LHE Completed"),
                            __FILE__,
                            __LINE__));
#endif

  // 7b (i). Now find the LHE of each group and subtract out the prior weight.
  {
    auto sortedTransferTargetView = viskores::cont::make_ArrayHandleView(
      this->SortedTransferTarget, firstSupernode + 1, numSupernodesToProcess - 1);
    auto sortedTransferTargetShiftedView = viskores::cont::make_ArrayHandleView(
      this->SortedTransferTarget, firstSupernode, numSupernodesToProcess - 1);
    auto valuePrefixSumPreviousValueView = viskores::cont::make_ArrayHandleView(
      this->ValuePrefixSum, firstSupernode, numSupernodesToProcess - 1);

    viskores::worklet::contourtree_distributed::hierarchical_hyper_sweeper::
      TransferWeightsUpdateLHEWorkletRound2 transferWeightsUpdateLHEWorkletRound2;
    this->Invoke(transferWeightsUpdateLHEWorkletRound2,
                 sortedTransferTargetView,
                 sortedTransferTargetShiftedView,
                 valuePrefixSumPreviousValueView,
                 this->IntrinsicValues,
                 this->DependentValues);
  }

#ifdef DEBUG_PRINT_HYPER_SWEEPER
  VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                 DebugPrint(std::string("Hypersweep Block ") + std::to_string(this->BlockId) +
                              std::string(" Round ") + std::to_string(round) +
                              std::string(" Step 1 Iteration ") + std::to_string(iteration) +
                              std::string(" Attachment RHE Completed"),
                            __FILE__,
                            __LINE__));
#endif
} // TransferWeights()


// debug routine
template <typename SweepValueType, typename ContourTreeFieldType>
std::string HierarchicalHyperSweeper<SweepValueType, ContourTreeFieldType>::DebugPrint(
  std::string message,
  const char* fileName,
  long lineNum) const
{ // DebugPrint()
  std::stringstream resultStream;
  resultStream << std::endl;
  resultStream << "----------------------------------------" << std::endl;
  resultStream << std::setw(30) << std::left << fileName << ":" << std::right << std::setw(4)
               << lineNum << std::endl;
  resultStream << std::left << message << std::endl;

#ifdef DEBUG_PRINT_HIERARCHICAL_CONTOUR_TREE
  resultStream << this->HierarchicalTree.DebugPrint(
    (message + std::string(" Hierarchical Tree")).c_str(), fileName, lineNum);
#endif

  resultStream << "Hypersweep Value Array Contains:        " << std::endl;
  resultStream << "----------------------------------------" << std::endl;
  resultStream << std::endl;

  viskores::worklet::contourtree_augmented::PrintHeader(this->DependentValues.GetNumberOfValues(),
                                                        resultStream);
  viskores::worklet::contourtree_augmented::PrintIndices(
    "Intrinsic", this->IntrinsicValues, -1, resultStream);
  viskores::worklet::contourtree_augmented::PrintIndices(
    "Dependent", this->DependentValues, -1, resultStream);
  viskores::worklet::contourtree_augmented::PrintIndices(
    "Prefix Sum", this->ValuePrefixSum, -1, resultStream);
  viskores::worklet::contourtree_augmented::PrintIndices(
    "Transfer To", this->TransferTarget, -1, resultStream);
  viskores::worklet::contourtree_augmented::PrintIndices(
    "Sorted Transfer", this->SortedTransferTarget, -1, resultStream);
  viskores::worklet::contourtree_augmented::PrintIndices(
    "Sort Permute", this->SuperSortPermute, -1, resultStream);
  return resultStream.str();
} // DebugPrint()


// Routine to save the hierarchical tree to file
template <typename SweepValueType, typename ContourTreeFieldType>
void HierarchicalHyperSweeper<SweepValueType, ContourTreeFieldType>::SaveHierarchicalContourTreeDot(
  std::string message,
  const char* outFileName) const
{ // SaveHierarchicalContourTreeDot()
  std::string hierarchicalTreeDotString =
    HierarchicalContourTreeDotGraphPrint<viskores::worklet::contourtree_augmented::IdArrayType>(
      message,
      this->HierarchicalTree,
      SHOW_SUPER_STRUCTURE | SHOW_HYPER_STRUCTURE | SHOW_ALL_IDS | SHOW_ALL_SUPERIDS |
        SHOW_ALL_HYPERIDS | SHOW_EXTRA_DATA, //|GV_NODE_NAME_USES_GLOBAL_ID
      this->BlockId,
      this->DependentValues);
  std::ofstream hierarchicalTreeFile(outFileName);
  hierarchicalTreeFile << hierarchicalTreeDotString;
} // SaveHierarchicalContourTreeDot


} // namespace contourtree_distributed
} // namespace worklet
} // namespace viskores

#endif
