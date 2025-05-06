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

#ifndef viskores_worklet_contourtree_augmented_contourtreemaker_h
#define viskores_worklet_contourtree_augmented_contourtreemaker_h

#include <iomanip>

// local includes
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/ArrayTransforms.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/ContourTree.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/DataSetMesh.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/MergeTree.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/MeshExtrema.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/PrintVectors.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/Types.h>

// contourtree_maker_inc includes
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/contourtreemaker/AugmentMergeTrees_InitNewJoinSplitIDAndSuperparents.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/contourtreemaker/AugmentMergeTrees_SetAugmentedMergeArcs.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/contourtreemaker/CompressTrees_Step.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/contourtreemaker/ComputeHyperAndSuperStructure_HypernodesSetFirstSuperchild.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/contourtreemaker/ComputeHyperAndSuperStructure_PermuteArcs.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/contourtreemaker/ComputeHyperAndSuperStructure_ResetHyperparentsId.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/contourtreemaker/ComputeHyperAndSuperStructure_SetFirstSupernodePerIterationWorklet.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/contourtreemaker/ComputeHyperAndSuperStructure_SetNewHypernodesAndArcs.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/contourtreemaker/ComputeRegularStructure_LocateSuperarcs.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/contourtreemaker/ComputeRegularStructure_SetArcs.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/contourtreemaker/ContourTreeNodeComparator.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/contourtreemaker/ContourTreeSuperNodeComparator.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/contourtreemaker/FindDegrees_FindRHE.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/contourtreemaker/FindDegrees_ResetUpAndDowndegree.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/contourtreemaker/FindDegrees_SubtractLHE.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/contourtreemaker/MoveNoSuchElementToBackComparator.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/contourtreemaker/TransferLeafChains_CollapsePastRegular.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/contourtreemaker/TransferLeafChains_InitInAndOutbound.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/contourtreemaker/TransferLeafChains_TransferToContourTree.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/contourtreemaker/WasNotTransferred.h>

#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/activegraph/SuperArcNodeComparator.h>


//VISKORES includes
#include <viskores/Types.h>
#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayHandleCast.h>
#include <viskores/cont/ArrayHandleConstant.h>
#include <viskores/cont/ArrayHandleImplicit.h>
#include <viskores/cont/ArrayHandleIndex.h>
#include <viskores/cont/ArrayHandlePermutation.h>
#include <viskores/cont/ArrayHandleTransform.h>
#include <viskores/cont/Error.h>
#include <viskores/cont/Invoker.h>



namespace contourtree_maker_inc_ns =
  viskores::worklet::contourtree_augmented::contourtree_maker_inc;
namespace active_graph_inc_ns = viskores::worklet::contourtree_augmented::active_graph_inc;

namespace viskores
{
namespace worklet
{
namespace contourtree_augmented
{


class ContourTreeMaker
{ // class MergeTree
public:
  viskores::cont::Invoker Invoke;

  // the contour tree, join tree & split tree to use
  ContourTree& ContourTreeResult;
  MergeTree& JoinTree;
  MergeTree& SplitTree;

  // vectors of up and down degree kept during the computation
  IdArrayType Updegree;
  IdArrayType Downdegree;

  // vectors for tracking merge superarcs
  IdArrayType AugmentedJoinSuperarcs;
  IdArrayType AugmentedSplitSuperarcs;

  // vector for the active set of supernodes
  IdArrayType ActiveSupernodes;

  // constructor does the real work does the real work but mostly, it just calls the following two routines
  ContourTreeMaker(ContourTree& theContourTree, MergeTree& joinTree, MergeTree& splitTree);

  // computes the hyperarcs in the contour tree
  void ComputeHyperAndSuperStructure();

  // computes the regular arcs in the contour tree. Augment the contour tree with all regular vertices.
  void ComputeRegularStructure(MeshExtrema& meshExtrema);

  // compute the parital regular arcs by augmenting the contour tree with the relevant vertices on the boundary
  template <class Mesh, class MeshBoundaryExecObj>
  void ComputeBoundaryRegularStructure(MeshExtrema& meshExtrema,
                                       const Mesh& mesh,
                                       const MeshBoundaryExecObj& meshBoundary);

  // routine that augments the join & split tree with each other's supernodes
  //              the augmented trees will be stored in the joinSuperarcs / mergeSuperarcs arrays
  //              the sort IDs will be stored in the ContourTree's arrays, &c.
  void AugmentMergeTrees();

  // routine to transfer leaf chains to contour tree
  void TransferLeafChains(bool isJoin);

  // routine to collapse regular vertices
  void CompressTrees();

  // compresses active set of supernodes
  void CompressActiveSupernodes();

  // finds the degree of each supernode from the merge trees
  void FindDegrees();

  // debug routine
  void DebugPrint(const char* message, const char* fileName, long lineNum);

}; // class ContourTreeMaker


// TODO we should add an Init function to move the heavy-weight computions out of the constructor
// constructor
inline ContourTreeMaker::ContourTreeMaker(ContourTree& contourTree,
                                          MergeTree& joinTree,
                                          MergeTree& splitTree)
  : ContourTreeResult(contourTree)
  , JoinTree(joinTree)
  , SplitTree(splitTree)
  , Updegree()
  , Downdegree()
  , AugmentedJoinSuperarcs()
  , AugmentedSplitSuperarcs()
  , ActiveSupernodes()
{ // constructor
} //MakeContourTree()


inline void ContourTreeMaker::ComputeHyperAndSuperStructure()
{ // ComputeHyperAndSuperStructure()

  // augment the merge trees & establish the list of supernodes
  AugmentMergeTrees();

  // track how many iterations it takes
  this->ContourTreeResult.NumIterations = 0;

  // loop until no arcs remaining to be found
  // tree can end with either 0 or 1 vertices unprocessed
  // 0 means the last edge was pruned from both ends
  // 1 means that there were two final edges meeting at a vertex
  viskores::Id maxNumIterations = this->ActiveSupernodes.GetNumberOfValues();
  while (this->ActiveSupernodes.GetNumberOfValues() > 1)
  { // loop until no active vertices remaining
    // recompute the vertex degrees
    FindDegrees();

    // alternate iterations between upper & lower
    if (this->ContourTreeResult.NumIterations % 2 == 0)
      TransferLeafChains(true);
    else
      TransferLeafChains(false);

    // compress join & split trees
    CompressTrees();
    // compress the active list of supernodes
    CompressActiveSupernodes();
    this->ContourTreeResult.NumIterations++;

    // Check to make sure we are not iterating too long
    // this can happen if we are given a bad mesh that defines
    // a forest of contour trees, rather than a single tree.
    // Raise error if we have done more itertions than there are active nodes to remove
    if (this->ContourTreeResult.NumIterations >= maxNumIterations)
    {
      throw new viskores::cont::ErrorInternal(
        "Bad iteration. This can happen if the input mesh "
        "defines a contour forest rather than a simple tree.");
    }
  } // loop until no active vertices remaining

  // test for final edges meeting
  if (this->ActiveSupernodes.GetNumberOfValues() == 1)
  { // meet at a vertex
    viskores::Id superID = ArrayGetValue(0, this->ActiveSupernodes);
    this->ContourTreeResult.Superarcs.WritePortal().Set(superID,
                                                        static_cast<viskores::Id>(NO_SUCH_ELEMENT));
    this->ContourTreeResult.Hyperarcs.WritePortal().Set(superID,
                                                        static_cast<viskores::Id>(NO_SUCH_ELEMENT));
    this->ContourTreeResult.Hyperparents.WritePortal().Set(superID, superID);
    this->ContourTreeResult.WhenTransferred.WritePortal().Set(
      superID, this->ContourTreeResult.NumIterations | IS_HYPERNODE);
  } // meet at a vertex
#ifdef DEBUG_PRINT
  DebugPrint("Contour Tree Constructed. Now Swizzling", __FILE__, __LINE__);
#endif

  // next, we have to set up the hyper and super structure arrays one at a time
  // at present, all superarcs / Hyperarcs are expressed in terms of supernode IDs
  // but we will want to move supernodes around.
  // the first step is therefore to find the new order of supernodes by sorting
  // we will use the hypernodes array for this, as we will want a copy to end up there

  // create linear sequence of numbers 0, 1, .. NumSupernodes
  viskores::cont::ArrayHandleIndex initContourTreeHypernodes(
    this->ContourTreeResult.Supernodes.GetNumberOfValues());
  viskores::cont::Algorithm::Copy(initContourTreeHypernodes, this->ContourTreeResult.Hypernodes);

  // now we sort hypernodes array with a comparator
  viskores::cont::Algorithm::Sort(this->ContourTreeResult.Hypernodes,
                                  contourtree_maker_inc_ns::ContourTreeSuperNodeComparator(
                                    this->ContourTreeResult.Hyperparents,
                                    this->ContourTreeResult.Supernodes,
                                    this->ContourTreeResult.WhenTransferred));

  // we have to permute a bunch of arrays, so let's have some temporaries to store them
  IdArrayType permutedHyperparents;
  PermuteArrayWithMaskedIndex<viskores::Id>(
    this->ContourTreeResult.Hyperparents, this->ContourTreeResult.Hypernodes, permutedHyperparents);
  IdArrayType permutedSupernodes;
  PermuteArrayWithMaskedIndex<viskores::Id>(
    this->ContourTreeResult.Supernodes, this->ContourTreeResult.Hypernodes, permutedSupernodes);
  IdArrayType permutedSuperarcs;
  PermuteArrayWithMaskedIndex<viskores::Id>(
    this->ContourTreeResult.Superarcs, this->ContourTreeResult.Hypernodes, permutedSuperarcs);

  // now we establish the reverse index array
  IdArrayType superSortIndex;
  superSortIndex.Allocate(this->ContourTreeResult.Supernodes.GetNumberOfValues());
  // The following copy is equivalent to
  // for (viskores::Id supernode = 0; supernode < this->ContourTreeResult.Supernodes.size(); supernode++)
  //   superSortIndex[this->ContourTreeResult.Hypernodes[supernode]] = supernode;

  //typedef viskores::cont::ArrayHandlePermutation<IdArrayType, viskores::cont::ArrayHandleIndex> PermuteArrayWithMaskedIndexHandleIndex;
  viskores::cont::ArrayHandlePermutation<IdArrayType, IdArrayType> permutedSuperSortIndex(
    this->ContourTreeResult.Hypernodes, // index array
    superSortIndex);                    // value array
  viskores::cont::Algorithm::Copy(
    viskores::cont::ArrayHandleIndex(
      this->ContourTreeResult.Supernodes.GetNumberOfValues()), // source value array
    permutedSuperSortIndex);                                   // target array

  // we then copy the supernodes & hyperparents back to the main array
  viskores::cont::Algorithm::Copy(permutedSupernodes, this->ContourTreeResult.Supernodes);
  viskores::cont::Algorithm::Copy(permutedHyperparents, this->ContourTreeResult.Hyperparents);


  // we need an extra permutation to get the superarcs correct
  contourtree_maker_inc_ns::ComputeHyperAndSuperStructure_PermuteArcs permuteSuperarcsWorklet;
  this->Invoke(permuteSuperarcsWorklet,
               permutedSuperarcs,                  // (input)
               superSortIndex,                     // (input)
               this->ContourTreeResult.Superarcs); // (output)

  // we will permute the hyperarcs & copy them back with the new supernode target IDs
  IdArrayType permutedHyperarcs;
  PermuteArrayWithMaskedIndex<viskores::Id>(
    this->ContourTreeResult.Hyperarcs, this->ContourTreeResult.Hypernodes, permutedHyperarcs);
  contourtree_maker_inc_ns::ComputeHyperAndSuperStructure_PermuteArcs permuteHyperarcsWorklet;
  this->Invoke(
    permuteHyperarcsWorklet, permutedHyperarcs, superSortIndex, this->ContourTreeResult.Hyperarcs);

  // now swizzle the WhenTransferred value
  IdArrayType permutedWhenTransferred;
  PermuteArrayWithMaskedIndex<viskores::Id>(this->ContourTreeResult.WhenTransferred,
                                            this->ContourTreeResult.Hypernodes,
                                            permutedWhenTransferred);
  viskores::cont::Algorithm::Copy(permutedWhenTransferred, this->ContourTreeResult.WhenTransferred);

  // now we compress both the hypernodes & Hyperarcs
  IdArrayType newHypernodePosition;
  OneIfHypernode oneIfHypernodeFunctor;
  auto oneIfHypernodeArrayHandle =
    viskores::cont::ArrayHandleTransform<IdArrayType, OneIfHypernode>(
      this->ContourTreeResult.WhenTransferred, oneIfHypernodeFunctor);
  viskores::cont::Algorithm::ScanExclusive(oneIfHypernodeArrayHandle, newHypernodePosition);

  viskores::Id nHypernodes =
    ArrayGetValue(newHypernodePosition.GetNumberOfValues() - 1, newHypernodePosition) +
    oneIfHypernodeFunctor(
      ArrayGetValue(this->ContourTreeResult.WhenTransferred.GetNumberOfValues() - 1,
                    this->ContourTreeResult.WhenTransferred));

  IdArrayType newHypernodes;
  newHypernodes.Allocate(nHypernodes);
  IdArrayType newHyperarcs;
  newHyperarcs.Allocate(nHypernodes);

  contourtree_maker_inc_ns::ComputeHyperAndSuperStructure_SetNewHypernodesAndArcs
    setNewHypernodesAndArcsWorklet;
  this->Invoke(setNewHypernodesAndArcsWorklet,
               this->ContourTreeResult.Supernodes,
               this->ContourTreeResult.WhenTransferred,
               this->ContourTreeResult.Hypernodes,
               this->ContourTreeResult.Hyperarcs,
               newHypernodePosition,
               newHypernodes,
               newHyperarcs);
  // swap in the new computed arrays.
  // viskores ArrayHandles are smart so we can just swap the new data in here rather than copy
  //viskores::cont::Algorithm::Copy(newHypernodes, this->ContourTreeResult.Hypernodes);
  //viskores::cont::Algorithm::Copy(newHyperarcs, this->ContourTreeResult.Hyperarcs);
  this->ContourTreeResult.Hypernodes.ReleaseResources();
  this->ContourTreeResult.Hypernodes = newHypernodes;
  this->ContourTreeResult.Hyperarcs.ReleaseResources();
  this->ContourTreeResult.Hyperarcs = newHyperarcs;


  // now reuse the superSortIndex array for hypernode IDs
  // The following copy is equivalent to
  // for (viskores::Id hypernode = 0; hypernode < this->ContourTreeResult.Hypernodes.size(); hypernode++)
  //            superSortIndex[this->ContourTreeResult.Hypernodes[hypernode]] = hypernode;
  // source data array is a simple linear index from 0 to #Hypernodes
  viskores::cont::ArrayHandleIndex tempHypernodeIndexArray(
    this->ContourTreeResult.Hypernodes.GetNumberOfValues());
  // target data array for the copy operation is superSortIndex permuted by this->ContourTreeResult.Hypernodes
  permutedSuperSortIndex = viskores::cont::ArrayHandlePermutation<IdArrayType, IdArrayType>(
    this->ContourTreeResult.Hypernodes, superSortIndex);
  viskores::cont::Algorithm::Copy(tempHypernodeIndexArray, permutedSuperSortIndex);

  // loop through the hyperparents array, setting the first one for each
  contourtree_maker_inc_ns::ComputeHyperAndSuperStructure_HypernodesSetFirstSuperchild
    hypernodesSetFirstSuperchildWorklet;
  this->Invoke(hypernodesSetFirstSuperchildWorklet,
               this->ContourTreeResult.Hyperparents,
               superSortIndex,
               this->ContourTreeResult.Hypernodes);

  // do a separate loop to reset the hyperparent's ID
  // This does the following
  // for (viskores::Id supernode = 0; supernode < this->ContourTreeResult.Supernodes.size(); supernode++)
  //    this->ContourTreeResult.Hyperparents[supernode] = superSortIndex[MaskedIndex(this->ContourTreeResult.Hyperparents[supernode])];
  contourtree_maker_inc_ns::ComputeHyperAndSuperStructure_ResetHyperparentsId
    resetHyperparentsIdWorklet;
  this->Invoke(resetHyperparentsIdWorklet, superSortIndex, this->ContourTreeResult.Hyperparents);

  // set up the array which tracks which supernodes to deal with on which iteration:
  // it's plus 2 because there's an "extra" iteration for the root
  // and it's useful to store the size as one beyond that
  // initalize with 0's to be safe
  viskores::cont::Algorithm::Copy(
    viskores::cont::ArrayHandleConstant<viskores::Id>(0, this->ContourTreeResult.NumIterations + 2),
    this->ContourTreeResult.FirstSupernodePerIteration);
  {
    contourtree_maker_inc_ns::ComputeHyperAndSuperStructure_SetFirstSupernodePerIterationWorklet
      setFirstSupernodePerIterationWorklet;
    auto tempSupernodesIndex =
      viskores::cont::ArrayHandleIndex(this->ContourTreeResult.Supernodes.GetNumberOfValues());
    this->Invoke(setFirstSupernodePerIterationWorklet,
                 tempSupernodesIndex,                               // loopindex
                 this->ContourTreeResult.WhenTransferred,           // input
                 this->ContourTreeResult.FirstSupernodePerIteration // output
    );
  }

  // TODO The following loop should be safe in parallel since there should never be two zeros in sequence, i.e., the next
  // entry after a zero will always be valid, regardless of execution order. Because this is safe if could be implemented
  // as a worklet. The number of iterations in the loop is small, so it may not be necessary for performance.
  auto firstSupernodePerIterationPortal =
    this->ContourTreeResult.FirstSupernodePerIteration.WritePortal();
  for (viskores::Id iteration = 1; iteration < this->ContourTreeResult.NumIterations; ++iteration)
  {
    if (firstSupernodePerIterationPortal.Get(iteration) == 0)
    {
      firstSupernodePerIterationPortal.Set(iteration,
                                           firstSupernodePerIterationPortal.Get(iteration + 1));
    }
  }
  // set the sentinels at the end of the array
  firstSupernodePerIterationPortal.Set(this->ContourTreeResult.NumIterations,
                                       this->ContourTreeResult.Supernodes.GetNumberOfValues() - 1);
  firstSupernodePerIterationPortal.Set(this->ContourTreeResult.NumIterations + 1,
                                       this->ContourTreeResult.Supernodes.GetNumberOfValues());

  // now use that array to construct a similar array for hypernodes: it's plus 2 because there's an "extra" iteration for the root
  // and it's useful to store the size as one beyond that
  this->ContourTreeResult.FirstHypernodePerIteration.Allocate(
    this->ContourTreeResult.NumIterations + 2);
  {
    // permute the ContourTree.Hyperpartens by the ContourTreeFirstSupernodePerIteration
    auto tempContourTreeHyperparentsPermuted = viskores::cont::make_ArrayHandlePermutation(
      this->ContourTreeResult.FirstSupernodePerIteration, this->ContourTreeResult.Hyperparents);
    viskores::cont::Algorithm::CopySubRange(
      tempContourTreeHyperparentsPermuted,
      0,                                                 // start index
      this->ContourTreeResult.NumIterations,             // stop index
      this->ContourTreeResult.FirstHypernodePerIteration // target
    );
  }

  this->ContourTreeResult.FirstHypernodePerIteration.WritePortal().Set(
    this->ContourTreeResult.NumIterations,
    this->ContourTreeResult.Hypernodes.GetNumberOfValues() - 1);
  this->ContourTreeResult.FirstHypernodePerIteration.WritePortal().Set(
    this->ContourTreeResult.NumIterations + 1,
    this->ContourTreeResult.Hypernodes.GetNumberOfValues());
#ifdef DEBUG_PRINT
  DebugPrint("Contour Tree Super Structure Constructed", __FILE__, __LINE__);
#endif
} // ComputeHyperAndSuperStructure()


// computes the regular arcs in the contour tree
inline void ContourTreeMaker::ComputeRegularStructure(MeshExtrema& meshExtrema)
{ // ComputeRegularStructure()
  // First step - use the superstructure to set the superparent for all supernodes
  auto supernodesIndex = viskores::cont::ArrayHandleIndex(
    this->ContourTreeResult.Supernodes
      .GetNumberOfValues()); // Counting array of length #supernodes to
  auto permutedSuperparents = viskores::cont::make_ArrayHandlePermutation(
    this->ContourTreeResult.Supernodes,
    this->ContourTreeResult.Superparents); // superparents array permmuted by the supernodes array
  viskores::cont::Algorithm::Copy(supernodesIndex, permutedSuperparents);
  // The above copy is equivlant to
  // for (indexType supernode = 0; supernode < this->ContourTreeResult.supernodes.size(); supernode++)
  //    this->ContourTreeResult.superparents[this->ContourTreeResult.Supernodes[supernode]] = supernode;

  // Second step - for all remaining (regular) nodes, locate the superarc to which they belong
  contourtree_maker_inc_ns::ComputeRegularStructure_LocateSuperarcs locateSuperarcsWorklet(
    this->ContourTreeResult.Hypernodes.GetNumberOfValues(),
    this->ContourTreeResult.Supernodes.GetNumberOfValues());
  this->Invoke(locateSuperarcsWorklet,
               this->ContourTreeResult.Superparents,    // (input/output)
               this->ContourTreeResult.WhenTransferred, // (input)
               this->ContourTreeResult.Hyperparents,    // (input)
               this->ContourTreeResult.Hyperarcs,       // (input)
               this->ContourTreeResult.Hypernodes,      // (input)
               this->ContourTreeResult.Supernodes,      // (input)
               meshExtrema.Peaks,                       // (input)
               meshExtrema.Pits);                       // (input)

  // We have now set the superparent correctly for each node, and need to sort them to get the correct regular arcs
  viskores::cont::Algorithm::Copy(
    viskores::cont::ArrayHandleIndex(this->ContourTreeResult.Arcs.GetNumberOfValues()),
    this->ContourTreeResult.Nodes);

  viskores::cont::Algorithm::Sort(
    this->ContourTreeResult.Nodes,
    contourtree_maker_inc_ns::ContourTreeNodeComparator(this->ContourTreeResult.Superparents,
                                                        this->ContourTreeResult.Superarcs));

  // now set the arcs based on the array
  contourtree_maker_inc_ns::ComputeRegularStructure_SetArcs setArcsWorklet(
    this->ContourTreeResult.Arcs.GetNumberOfValues());
  this->Invoke(setArcsWorklet,
               this->ContourTreeResult.Nodes,        // (input) arcSorter array
               this->ContourTreeResult.Superparents, // (input)
               this->ContourTreeResult.Superarcs,    // (input)
               this->ContourTreeResult.Supernodes,   // (input)
               this->ContourTreeResult.Arcs);        // (output)
#ifdef DEBUG_PRINT
  DebugPrint("Regular Structure Computed", __FILE__, __LINE__);
#endif
} // ComputeRegularStructure()

struct ContourTreeNoSuchElementSuperParents
{
  template <typename T>
  VISKORES_EXEC_CONT bool operator()(const T& x) const
  {
    return (!NoSuchElement(x));
  }
};

inline void InitIdArrayTypeNoSuchElement(IdArrayType& idArray, viskores::Id size)
{
  idArray.Allocate(size);

  viskores::cont::ArrayHandleConstant<viskores::Id> noSuchElementArray(
    (viskores::Id)NO_SUCH_ELEMENT, size);
  viskores::cont::Algorithm::Copy(noSuchElementArray, idArray);
}

template <class Mesh, class MeshBoundaryExecObj>
inline void ContourTreeMaker::ComputeBoundaryRegularStructure(
  MeshExtrema& meshExtrema,
  const Mesh& mesh,
  const MeshBoundaryExecObj& meshBoundaryExecObj)
{ // ComputeRegularStructure()
  // First step - use the superstructure to set the superparent for all supernodes
  auto supernodesIndex =
    viskores::cont::ArrayHandleIndex(this->ContourTreeResult.Supernodes.GetNumberOfValues());
  IdArrayType superparents;
  InitIdArrayTypeNoSuchElement(superparents, mesh.GetNumberOfVertices());
  // superparents array permmuted by the supernodes array
  auto permutedSuperparents =
    viskores::cont::make_ArrayHandlePermutation(this->ContourTreeResult.Supernodes, superparents);
  viskores::cont::Algorithm::Copy(supernodesIndex, permutedSuperparents);
  // The above copy is equivlant to
  // for (indexType supernode = 0; supernode < this->ContourTreeResult.Supernodes.size(); supernode++)
  //    superparents[this->ContourTreeResult.Supernodes[supernode]] = supernode;

  // Second step - for all remaining (regular) nodes, locate the superarc to which they belong
  contourtree_maker_inc_ns::ComputeRegularStructure_LocateSuperarcsOnBoundary
    locateSuperarcsOnBoundaryWorklet(this->ContourTreeResult.Hypernodes.GetNumberOfValues(),
                                     this->ContourTreeResult.Supernodes.GetNumberOfValues());
  this->Invoke(locateSuperarcsOnBoundaryWorklet,
               superparents,                            // (input/output)
               this->ContourTreeResult.WhenTransferred, // (input)
               this->ContourTreeResult.Hyperparents,    // (input)
               this->ContourTreeResult.Hyperarcs,       // (input)
               this->ContourTreeResult.Hypernodes,      // (input)
               this->ContourTreeResult.Supernodes,      // (input)
               meshExtrema.Peaks,                       // (input)
               meshExtrema.Pits,                        // (input)
               mesh.SortOrder,                          // (input)
               meshBoundaryExecObj);                    // (input)

  // We have now set the superparent correctly for each node, and need to sort them to get the correct regular arcs
  // DAVID "ContourTreeMaker.h" line 338
  IdArrayType node;
  viskores::cont::Algorithm::Copy(
    viskores::cont::ArrayHandleIndex(superparents.GetNumberOfValues()),
    this->ContourTreeResult.Augmentnodes);
  viskores::cont::Algorithm::Copy(
    viskores::cont::ArrayHandleIndex(superparents.GetNumberOfValues()), node);
  viskores::cont::Algorithm::CopyIf(node,
                                    superparents,
                                    this->ContourTreeResult.Augmentnodes,
                                    ContourTreeNoSuchElementSuperParents());

  IdArrayType toCompressed;
  InitIdArrayTypeNoSuchElement(toCompressed, superparents.GetNumberOfValues());
  viskores::cont::Algorithm::Copy(
    viskores::cont::ArrayHandleIndex(this->ContourTreeResult.Augmentnodes.GetNumberOfValues()),
    node);
  auto permutedToCompressed =
    viskores::cont::make_ArrayHandlePermutation(this->ContourTreeResult.Augmentnodes, // index array
                                                toCompressed);                        // value array
  viskores::cont::Algorithm::Copy(node,                  // source value array
                                  permutedToCompressed); // target array

  // Make superparents correspond to nodes
  IdArrayType tmpsuperparents;
  viskores::cont::Algorithm::CopyIf(
    superparents, superparents, tmpsuperparents, ContourTreeNoSuchElementSuperParents());
  viskores::cont::Algorithm::Copy(tmpsuperparents, superparents);

  // Create array for sorting
  IdArrayType augmentnodes_sorted;
  viskores::cont::Algorithm::Copy(
    viskores::cont::ArrayHandleIndex(this->ContourTreeResult.Augmentnodes.GetNumberOfValues()),
    augmentnodes_sorted);

  // use a comparator to do the sort
  viskores::cont::Algorithm::Sort(augmentnodes_sorted,
                                  contourtree_maker_inc_ns::ContourTreeNodeComparator(
                                    superparents, this->ContourTreeResult.Superarcs));
  // now set the arcs based on the array
  InitIdArrayTypeNoSuchElement(this->ContourTreeResult.Augmentarcs,
                               this->ContourTreeResult.Augmentnodes.GetNumberOfValues());
  contourtree_maker_inc_ns::ComputeRegularStructure_SetAugmentArcs setAugmentArcsWorklet(
    this->ContourTreeResult.Augmentarcs.GetNumberOfValues());
  this->Invoke(setAugmentArcsWorklet,
               augmentnodes_sorted,                  // (input) arcSorter array
               superparents,                         // (input)
               this->ContourTreeResult.Superarcs,    // (input)
               this->ContourTreeResult.Supernodes,   // (input)
               toCompressed,                         // (input)
               this->ContourTreeResult.Augmentarcs); // (output)
#ifdef DEBUG_PRINT
  DebugPrint("Regular Boundary Structure Computed", __FILE__, __LINE__);
#endif
} // ComputeRegularStructure()


// routine that augments the join & split tree with each other's supernodes
// the augmented trees will be stored in the joinSuperarcs / mergeSuperarcs arrays
// the sort IDs will be stored in the ContourTree's arrays, &c.
inline void ContourTreeMaker::AugmentMergeTrees()
{ // ContourTreeMaker::AugmentMergeTrees()
  // in this version, we know that only connectivity-critical points are used
  // so we want to combine the lists of supernodes.
  // but they are not in sorted order, so some juggling is required.

  // NOTE: The following code block is a direct port from the original PPP2 code using std::set_union
  //       In the main code below  we replaced steps 1-4 with a combination of VISKORES copy, sort, and union operators instead
  /*
    // 1. Allocate an array that is guaranteed to be big enough
    //  - the sum of the sizes of the trees or the total size of the data
    viskores::Id nJoinSupernodes = this->JoinTree.Supernodes.GetNumberOfValues();
    viskores::Id nSplitSupernodes = this->SplitTree.Supernodes.GetNumberOfValues();
    viskores::Id nSupernodes = nJoinSupernodes + nSplitSupernodes;
    if (nSupernodes > this->JoinTree.Arcs.GetNumberOfValues())
      nSupernodes = this->JoinTree.Arcs.GetNumberOfValues();
    this->ContourTreeResult.Supernodes.Allocate(nSupernodes);

    // 2. Make copies of the lists of join & split supernodes & sort them
    IdArrayType joinSort;
    joinSort.Allocate(nJoinSupernodes);
    viskores::cont::Algorithm::Copy(this->JoinTree.Supernodes, joinSort);
    viskores::cont::Algorithm::Sort(joinSort);
    IdArrayType splitSort;
    splitSort.Allocate(nSplitSupernodes);
    viskores::cont::Algorithm::Copy(this->SplitTree.Supernodes, splitSort);
    viskores::cont::Algorithm::Sort(splitSort);

    // 3. Use set_union to combine the lists
    auto contTreeSuperNodesBegin = viskores::cont::ArrayPortalToIteratorBegin(this->ContourTreeResult.Supernodes.WritePortal());
    auto tail = std::set_union(viskores::cont::ArrayPortalToIteratorBegin(joinSort.WritePortal()),
                               viskores::cont::ArrayPortalToIteratorEnd(joinSort.WritePortal()),
                               viskores::cont::ArrayPortalToIteratorBegin(splitSort.WritePortal()),
                               viskores::cont::ArrayPortalToIteratorEnd(splitSort.WritePortal()),
                               contTreeSuperNodesBegin);
    // compute the true number of supernodes
    nSupernodes = tail - contTreeSuperNodesBegin;
    // and release the memory
    joinSort.ReleaseResources();
    splitSort.ReleaseResources();

    // 4. Resize the supernode array accordingly
    this->ContourTreeResult.Supernodes.Allocate(nSupernodes, viskores::CopyFlag::On);
    */

  // 1. Allocate an array that is guaranteed to be big enough
  //  - the sum of the sizes of the trees or the total size of the data
  viskores::Id nJoinSupernodes = this->JoinTree.Supernodes.GetNumberOfValues();
  viskores::Id nSplitSupernodes = this->SplitTree.Supernodes.GetNumberOfValues();
  viskores::Id nSupernodes = nJoinSupernodes + nSplitSupernodes;

  // TODO Check whether this replacement for Step 2 to 4 is a problem in terms of performance
  //  Step 2 - 4 in original PPP2. Create a sorted list of all unique supernodes from the Join and Split tree.
  this->ContourTreeResult.Supernodes.Allocate(nSupernodes);
  viskores::cont::Algorithm::CopySubRange(
    this->JoinTree.Supernodes, 0, nJoinSupernodes, this->ContourTreeResult.Supernodes, 0);
  viskores::cont::Algorithm::CopySubRange(this->SplitTree.Supernodes,
                                          0,
                                          nSplitSupernodes,
                                          this->ContourTreeResult.Supernodes,
                                          nJoinSupernodes);

  // Need to sort before Unique because VISKORES only guarantees to find neighboring duplicates
  // TODO/FIXME: It would be more efficient to do a merge of two sorted lists here, but that operation
  // is currently missing in viskores
  AssertArrayHandleNoFlagsSet(this->ContourTreeResult.Supernodes);
  viskores::cont::Algorithm::Sort(this->ContourTreeResult.Supernodes);
  viskores::cont::Algorithm::Unique(this->ContourTreeResult.Supernodes);
  nSupernodes = this->ContourTreeResult.Supernodes.GetNumberOfValues();

  // 5. Create lookup arrays for the join & split supernodes' new IDs
  IdArrayType newJoinID;
  newJoinID.Allocate(nJoinSupernodes);
  IdArrayType newSplitID;
  newSplitID.Allocate(nSplitSupernodes);

  // 6. Each supernode is listed by it's regular ID, so we can use the regular arrays
  //    to look up the corresponding supernode IDs in the merge trees, and to transfer
  //    the superparent for each
  IdArrayType joinSuperparents;
  joinSuperparents.Allocate(nSupernodes);
  IdArrayType splitSuperparents;
  splitSuperparents.Allocate(nSupernodes);

  contourtree_maker_inc_ns::AugmentMergeTrees_InitNewJoinSplitIDAndSuperparents
    initNewJoinSplitIDAndSuperparentsWorklet;
  this->Invoke(initNewJoinSplitIDAndSuperparentsWorklet,
               this->ContourTreeResult.Supernodes, //input
               this->JoinTree.Superparents,        //input
               this->SplitTree.Superparents,       //input
               this->JoinTree.Supernodes,          //input
               this->SplitTree.Supernodes,         //input
               joinSuperparents,                   //output
               splitSuperparents,                  //output
               newJoinID,                          //output
               newSplitID);                        //output

  // 7. use the active supernodes array for sorting
  // create linear sequence of numbers 0, 1, .. nSupernodes
  viskores::cont::ArrayHandleIndex initActiveSupernodes(nSupernodes);
  viskores::cont::Algorithm::Copy(initActiveSupernodes, this->ActiveSupernodes);

  // 8. Once we have got the superparent for each, we can sort by superparents and set
  //      the augmented superarcs. We start with the join superarcs
  viskores::cont::Algorithm::Sort(
    this->ActiveSupernodes,
    active_graph_inc_ns::SuperArcNodeComparator(joinSuperparents, this->JoinTree.IsJoinTree));

  // 9.   Set the augmented join superarcs
  this->AugmentedJoinSuperarcs.Allocate(nSupernodes);
  contourtree_maker_inc_ns::AugmentMergeTrees_SetAugmentedMergeArcs setAugmentedJoinArcsWorklet;
  this->Invoke(setAugmentedJoinArcsWorklet,
               this->ActiveSupernodes,        // (input domain)
               joinSuperparents,              // (input)
               this->JoinTree.Superarcs,      // (input)
               newJoinID,                     // (input)
               this->AugmentedJoinSuperarcs); // (output)

  // 10. Now we repeat the process for the split superarcs
  viskores::cont::Algorithm::Copy(initActiveSupernodes, this->ActiveSupernodes);
  // now sort by the split superparent
  viskores::cont::Algorithm::Sort(
    this->ActiveSupernodes,
    active_graph_inc_ns::SuperArcNodeComparator(splitSuperparents, this->SplitTree.IsJoinTree));

  // 11.  Set the augmented split superarcs
  this->AugmentedSplitSuperarcs.Allocate(nSupernodes);
  contourtree_maker_inc_ns::AugmentMergeTrees_SetAugmentedMergeArcs setAugmentedSplitArcsWorklet;
  this->Invoke(setAugmentedSplitArcsWorklet,
               this->ActiveSupernodes,         // (input domain)
               splitSuperparents,              // (input)
               this->SplitTree.Superarcs,      // (input)
               newSplitID,                     // (input)
               this->AugmentedSplitSuperarcs); // (output)

  // 12. Lastly, we can initialise all of the remaining arrays
  viskores::cont::ArrayHandleConstant<viskores::Id> noSuchElementArray(
    (viskores::Id)NO_SUCH_ELEMENT, nSupernodes);
  viskores::cont::Algorithm::Copy(noSuchElementArray, this->ContourTreeResult.Superarcs);
  viskores::cont::Algorithm::Copy(noSuchElementArray, this->ContourTreeResult.Hyperparents);
  viskores::cont::Algorithm::Copy(noSuchElementArray, this->ContourTreeResult.Hypernodes);
  viskores::cont::Algorithm::Copy(noSuchElementArray, this->ContourTreeResult.Hyperarcs);
  viskores::cont::Algorithm::Copy(noSuchElementArray, this->ContourTreeResult.WhenTransferred);

  // TODO We should only need to allocate the this->Updegree/Downdegree arrays. We initialize them with 0 here to ensure consistency of debug output
  //this->Updegree.Allocate(nSupernodes);
  //this->Downdegree.Allocate(nSupernodes);
  viskores::cont::Algorithm::Copy(viskores::cont::ArrayHandleConstant<viskores::Id>(0, nSupernodes),
                                  this->Updegree);
  viskores::cont::Algorithm::Copy(viskores::cont::ArrayHandleConstant<viskores::Id>(0, nSupernodes),
                                  this->Downdegree);
#ifdef DEBUG_PRINT
  DebugPrint("Supernodes Found", __FILE__, __LINE__);
#endif
} // ContourTreeMaker::AugmentMergeTrees()



namespace details
{

struct LeafChainsToContourTree
{
  LeafChainsToContourTree(const viskores::Id nIterations,
                          const bool isJoin,
                          const IdArrayType& outdegree,
                          const IdArrayType& indegree,
                          const IdArrayType& outbound,
                          const IdArrayType& inbound,
                          const IdArrayType& inwards)
    : NumIterations(nIterations)
    , IsJoin(isJoin)
    , Outdegree(outdegree)
    , Indegree(indegree)
    , Outbound(outbound)
    , Inbound(inbound)
    , Inwards(inwards)
  {
  }

  template <typename DeviceAdapter, typename... Args>
  inline bool operator()(DeviceAdapter device, Args&&... args) const
  {
    viskores::cont::Token token;
    contourtree_maker_inc_ns::TransferLeafChains_TransferToContourTree worklet(
      this->NumIterations, // (input)
      this->IsJoin,        // (input)
      this->Outdegree,     // (input)
      this->Indegree,      // (input)
      this->Outbound,      // (input)
      this->Inbound,       // (input)
      this->Inwards,       // (input)
      device,
      token);
    viskores::worklet::DispatcherMapField<decltype(worklet)> dispatcher(worklet);
    dispatcher.SetDevice(device);
    dispatcher.Invoke(std::forward<Args>(args)...);
    return true;
  }


  const viskores::Id NumIterations;
  const bool IsJoin;
  const IdArrayType& Outdegree;
  const IdArrayType& Indegree;
  const IdArrayType& Outbound;
  const IdArrayType& Inbound;
  const IdArrayType& Inwards;
};
}

// routine to transfer leaf chains to contour tree
inline void ContourTreeMaker::TransferLeafChains(bool isJoin)
{ // ContourTreeMaker::TransferLeafChains()
  // we need to compute the chains in both directions, so we have two vectors:
  // TODO below we initialize the outbound and inbound arrays with 0 to ensure consistency of debug output. Check if this is needed.
  IdArrayType outbound;
  viskores::cont::Algorithm::Copy(viskores::cont::ArrayHandleConstant<viskores::Id>(
                                    0, this->ContourTreeResult.Supernodes.GetNumberOfValues()),
                                  outbound);
  //outbound.Allocate(this->ContourTreeResult.Supernodes.GetNumberOfValues());
  IdArrayType inbound;
  //inbound.Allocate(this->ContourTreeResult.Supernodes.GetNumberOfValues());
  viskores::cont::Algorithm::Copy(viskores::cont::ArrayHandleConstant<viskores::Id>(
                                    0, this->ContourTreeResult.Supernodes.GetNumberOfValues()),
                                  inbound);


  // a reference for the inwards array we use to initialise
  IdArrayType& inwards = isJoin ? this->AugmentedJoinSuperarcs : this->AugmentedSplitSuperarcs;
  // and references for the degrees
  IdArrayType& indegree = isJoin ? this->Downdegree : this->Updegree;
  IdArrayType& outdegree = isJoin ? this->Updegree : this->Downdegree;

  // loop to each active node to copy join/split to outbound and inbound arrays
  contourtree_maker_inc_ns::TransferLeafChains_InitInAndOutbound initInAndOutboundWorklet;
  this->Invoke(initInAndOutboundWorklet,
               this->ActiveSupernodes, // (input)
               inwards,                // (input)
               outdegree,              // (input)
               indegree,               // (input)
               outbound,               // (output)
               inbound);               // (output)
#ifdef DEBUG_PRINT
  DebugPrint("Init in and outbound -- Step 1", __FILE__, __LINE__);
#endif
  // Compute the number of log steps required in this pass
  viskores::Id numLogSteps = 1;
  for (viskores::Id shifter = this->ActiveSupernodes.GetNumberOfValues(); shifter != 0;
       shifter >>= 1)
  {
    numLogSteps++;
  }


  // loop to find the now-regular vertices and collapse past them without altering
  // the existing join & split arcs
  for (viskores::Id iteration = 0; iteration < numLogSteps; iteration++)
  { // per iteration
    // loop through the vertices, updating outbound
    contourtree_maker_inc_ns::TransferLeafChains_CollapsePastRegular collapsePastRegularWorklet;
    this->Invoke(collapsePastRegularWorklet,
                 this->ActiveSupernodes, // (input)
                 outbound,               // (input/output)
                 inbound);               // (input/output)

  } // per iteration
#ifdef DEBUG_PRINT
  DebugPrint("Init in and outbound -- Step 2", __FILE__, __LINE__);
#endif
  // at this point, the outbound vector chains everything outwards to the leaf
  // any vertices on the last outbound leaf superarc point to the leaf
  // and the leaf itself will point to its saddle, identifying the hyperarc

  // what we want to do is:
  // a. for leaves (tested by degree),
  //              i.      we use inbound as the hyperarc
  //              ii.     we use inwards as the superarc
  //              iii.we use self as the hyperparent
  // b. for regular vertices pointing to a leaf (test by outbound's degree),
  //              i.      we use outbound as the hyperparent
  //              ii. we use inwards as the superarc
  // c. for all other vertics
  //              ignore


  // loop through the active vertices
  // Note: there are better and safer ways to pass these arrays (e.g. in/outdegree) to
  // a worklet. You could pass them as WholeArrayIn ControlSignature arguments. Or
  // you could build a subclass of viskores::cont::ExecutionObjectBase and pass that in
  // as an ExecObject.
  details::LeafChainsToContourTree task(this->ContourTreeResult.NumIterations, // (input)
                                        isJoin,                                // (input)
                                        outdegree,                             // (input)
                                        indegree,                              // (input)
                                        outbound,                              // (input)
                                        inbound,                               // (input)
                                        inwards);                              // (input)
  viskores::cont::TryExecute(task,
                             this->ActiveSupernodes,                   // (input)
                             this->ContourTreeResult.Hyperparents,     // (output)
                             this->ContourTreeResult.Hyperarcs,        // (output)
                             this->ContourTreeResult.Superarcs,        // (output)
                             this->ContourTreeResult.WhenTransferred); // (output)
#ifdef DEBUG_PRINT
  DebugPrint(isJoin ? "Upper Regular Chains Transferred" : "Lower Regular Chains Transferred",
             __FILE__,
             __LINE__);
#endif
} // ContourTreeMaker::TransferLeafChains()


// routine to compress trees by removing regular vertices as well as Hypernodes
inline void ContourTreeMaker::CompressTrees()
{ // ContourTreeMaker::CompressTrees()

  // Compute the number of log steps required in this pass
  viskores::Id numLogSteps = 1;
  for (viskores::Id shifter = this->ActiveSupernodes.GetNumberOfValues(); shifter != 0;
       shifter >>= 1)
  {
    numLogSteps++;
  }

  // loop to update the merge trees
  for (viskores::Id logStep = 0; logStep < numLogSteps; logStep++)
  { // iteration log times
    contourtree_maker_inc_ns::CompressTrees_Step compressTreesStepWorklet;
    this->Invoke(compressTreesStepWorklet,
                 this->ActiveSupernodes,            // (input)
                 this->ContourTreeResult.Superarcs, // (input)
                 this->AugmentedJoinSuperarcs,      // (input/output)
                 this->AugmentedSplitSuperarcs      // (input/output)
    );

  } // iteration log times
#ifdef DEBUG_PRINT
  DebugPrint("Trees Compressed", __FILE__, __LINE__);
#endif
} // ContourTreeMaker::CompressTrees()


// compresses trees to remove transferred vertices
inline void ContourTreeMaker::CompressActiveSupernodes()
{ // ContourTreeMaker::CompressActiveSupernodes()
  // copy only if this->ContourTreeResult.WhenTransferred has been set
  IdArrayType compressedActiveSupernodes;

  // Transform the WhenTransferred array to return 1 if the index was not transferred and 0 otherwise
  auto wasNotTransferred =
    viskores::cont::ArrayHandleTransform<IdArrayType, contourtree_maker_inc_ns::WasNotTransferred>(
      this->ContourTreeResult.WhenTransferred, contourtree_maker_inc_ns::WasNotTransferred());
  // Permute the wasNotTransferred array handle so that the lookup is based on the value of the indices in the active supernodes array
  auto notTransferredActiveSupernodes =
    viskores::cont::make_ArrayHandlePermutation(this->ActiveSupernodes, wasNotTransferred);
  // Keep only the indices of the active supernodes that have not been transferred yet
  viskores::cont::Algorithm::CopyIf(
    this->ActiveSupernodes, notTransferredActiveSupernodes, compressedActiveSupernodes);
  // Copy the data into the active supernodes
  ActiveSupernodes.ReleaseResources();
  ActiveSupernodes =
    compressedActiveSupernodes; // viskores ArrayHandles are smart, so we can just swap it in without having to copy
#ifdef DEBUG_PRINT
  DebugPrint("Active Supernodes Compressed", __FILE__, __LINE__);
#endif
} // ContourTreeMaker::CompressActiveSupernodes()


inline void ContourTreeMaker::FindDegrees()
{ // ContourTreeMaker::FindDegrees()
  using PermuteIndexArray = viskores::cont::ArrayHandlePermutation<IdArrayType, IdArrayType>;

  // retrieve the size to register for speed
  viskores::Id nActiveSupernodes = this->ActiveSupernodes.GetNumberOfValues();

  // reset the Updegree & Downdegree
  contourtree_maker_inc_ns::FindDegrees_ResetUpAndDowndegree resetUpAndDowndegreeWorklet;
  this->Invoke(
    resetUpAndDowndegreeWorklet, this->ActiveSupernodes, this->Updegree, this->Downdegree);
#ifdef DEBUG_PRINT
  DebugPrint("Degrees Set to 0", __FILE__, __LINE__);
#endif
  // now we loop through every join & split arc, updating degrees
  // to minimise memory footprint, we will do two separate loops
  // but we could combine the two into paired loops
  // first we establish an array of destination vertices (since outdegree is always 1)
  IdArrayType inNeighbour;
  //inNeighbour.Allocate(nActiveSupernodes);
  //PermuteIndexArray permuteInNeighbour(this->ActiveSupernodes, inNeighbour);
  PermuteIndexArray permuteAugmentedJoinSuperarcs(this->ActiveSupernodes,
                                                  this->AugmentedJoinSuperarcs);
  viskores::cont::Algorithm::Copy(permuteAugmentedJoinSuperarcs, inNeighbour);
  // now sort to group copies together
  viskores::cont::Algorithm::Sort(inNeighbour,
                                  contourtree_maker_inc_ns::MoveNoSuchElementToBackComparator{});

  // there's probably a smarter scatter-gather solution to this, but this should work
  // find the RHE of each segment
  contourtree_maker_inc_ns::FindDegrees_FindRHE joinFindRHEWorklet(nActiveSupernodes);
  this->Invoke(joinFindRHEWorklet, inNeighbour, this->Updegree);

  // now subtract the LHE to get the size
  contourtree_maker_inc_ns::FindDegrees_SubtractLHE joinSubractLHEWorklet;
  this->Invoke(joinSubractLHEWorklet, inNeighbour, this->Updegree);

  // now repeat the same process for the split neighbours
  PermuteIndexArray permuteAugmentedSplitSuperarcs(this->ActiveSupernodes,
                                                   this->AugmentedSplitSuperarcs);
  viskores::cont::Algorithm::Copy(permuteAugmentedSplitSuperarcs, inNeighbour);
  // now sort to group copies together
  viskores::cont::Algorithm::Sort(inNeighbour,
                                  contourtree_maker_inc_ns::MoveNoSuchElementToBackComparator{});

  // there's probably a smarter scatter-gather solution to this, but this should work
  // find the RHE of each segment
  contourtree_maker_inc_ns::FindDegrees_FindRHE splitFindRHEWorklet(nActiveSupernodes);
  this->Invoke(splitFindRHEWorklet, inNeighbour, this->Downdegree);

  // now subtract the LHE to get the size
  contourtree_maker_inc_ns::FindDegrees_SubtractLHE splitSubractLHEWorklet;
  this->Invoke(splitSubractLHEWorklet, inNeighbour, this->Downdegree);
#ifdef DEBUG_PRINT
  DebugPrint("Degrees Computed", __FILE__, __LINE__);
#endif
} // ContourTreeMaker::FindDegrees()



inline void ContourTreeMaker::DebugPrint(const char* message, const char* fileName, long lineNum)
{ // ContourTreeMaker::DebugPrint()
  std::string childString = std::string(message);
  std::cout
    << "==========================================================================================="
       "==============================================="
    << "=============================================="
    << //============================================================================================" <<
    "=============================================================================================="
    "============================================"
    << std::endl;
  std::cout
    << "==========================================================================================="
       "==============================================="
    << "=============================================="
    << //============================================================================================" <<
    "=============================================================================================="
    "============================================"
    << std::endl;
  std::cout
    << "==========================================================================================="
       "==============================================="
    << "=============================================="
    << //============================================================================================" <<
    "=============================================================================================="
    "============================================"
    << std::endl;
  std::cout << std::setw(30) << std::left << fileName << ":" << std::right << std::setw(4)
            << lineNum << std::endl;
  std::cout << std::left << std::string(message) << std::endl;

  // this->JoinTree.DebugPrint((childString + std::string(": Join Tree")).c_str(), fileName, lineNum);
  // this->SplitTree.DebugPrint((childString + std::string(": Split Tree")).c_str(), fileName, lineNum);
  std::cout << this->ContourTreeResult.DebugPrint(
    (childString + std::string(": Contour Tree")).c_str(), fileName, lineNum);
  std::cout
    << "==========================================================================================="
       "==============================================="
    << "=============================================="
    << //============================================================================================" <<
    "=============================================================================================="
    "============================================"
    << std::endl;

  // std::cout << "------------------------------------------------------" << std::endl;
  std::cout << std::setw(30) << std::left << fileName << ":" << std::right << std::setw(4)
            << lineNum << std::endl;
  std::cout << std::left << std::string(message) << std::endl;
  std::cout << "Contour Tree Maker Contains:                          " << std::endl;
  std::cout << "------------------------------------------------------" << std::endl;
  std::cout << "NumIterations: " << this->ContourTreeResult.NumIterations << std::endl;

  PrintHeader(this->Updegree.GetNumberOfValues());
  PrintIndices("Updegree", this->Updegree);
  PrintIndices("Downdegree", this->Downdegree);
  PrintIndices("Aug Join SArcs", this->AugmentedJoinSuperarcs);
  PrintIndices("Aug Split SArcs", this->AugmentedSplitSuperarcs);

  PrintHeader(this->ActiveSupernodes.GetNumberOfValues());
  PrintIndices("Active SNodes", this->ActiveSupernodes);
} // ContourTreeMaker::DebugPrint()


} // namespace contourtree_augmented
} // worklet
} // viskores

#endif
