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
//  Parallel Peak Pruning v. 2.0
//
//  Started June 15, 2017
//
// Copyright Hamish Carr, University of Leeds
//
// HierarchicalContourTree.cpp - Hierarchical version of contour tree that captures all of the
// superarcs relevant for a particular block.  It is constructed by grafting missing edges
// into the tree at all levels
//
//=======================================================================================
//
// COMMENTS:
//
//  There are several significant differences from the ContourTree class, in particular the
// semantics of storage:
// i.  Hyper arcs are processed inside to outside instead of outside to inside
//     This is to allow the superarcs in higher blocks to be a prefix of those in lower blocks
//    We can do this by inverting the loop order and processing each level separately, so
//    we don't need to renumber (whew!)
// ii.  If the superarc is -1, it USED to mean the root of the tree. Now it can also mean
//    the root of a lower-level subtree. in this case, the superparent will show which
//    existing superarc it inserts into.
//
//=======================================================================================

#ifndef viskores_worklet_contourtree_distributed_hierarchical_contour_tree_h
#define viskores_worklet_contourtree_distributed_hierarchical_contour_tree_h

#define VOLUME_PRINT_WIDTH 8

#include <viskores/Types.h>
#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/DataSet.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/ContourTree.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/Types.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/meshtypes/ContourTreeMesh.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/hierarchical_contour_tree/FindRegularByGlobal.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/hierarchical_contour_tree/FindSuperArcBetweenNodes.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/hierarchical_contour_tree/FindSuperArcForUnknownNode.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/hierarchical_contour_tree/InitalizeSuperchildrenWorklet.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/hierarchical_contour_tree/PermuteComparator.h>

namespace viskores
{
namespace worklet
{
namespace contourtree_distributed
{


/// \brief Hierarchical Contour Tree data structure
///
/// This class contains all the structures to construct/store the HierarchicalContourTree.
/// Functions used on the Device are then implemented in the HierarchicalContourTreeDeviceData
/// class which stores the prepared array portals we need for those functions.
///
template <typename FieldType>
class HierarchicalContourTree
{
public:
  VISKORES_CONT
  HierarchicalContourTree();

  // REGULAR arrays: i.e. over all nodes in the tree, including regular
  // the full list of global IDs for the regular nodes
  viskores::worklet::contourtree_augmented::IdArrayType RegularNodeGlobalIds;
  // we will also need to track the data values
  viskores::cont::ArrayHandle<FieldType> DataValues;

  // an array to support searching by global ID
  // given a global ID, find its position in the regular node index
  // To do so, we keep an index by global ID of their positions in the array
  viskores::worklet::contourtree_augmented::IdArrayType RegularNodeSortOrder;
  // the supernode ID for each regular node: for most, this will be NO_SUCH_ELEMENT
  // but this makes lookups for supernode ID a lot easier
  viskores::worklet::contourtree_augmented::IdArrayType Regular2Supernode;
  // the superparent for each regular node
  viskores::worklet::contourtree_augmented::IdArrayType Superparents;

  // SUPER arrays: i.e. over all supernodes in the tree
  // the ID in the globalID array
  viskores::worklet::contourtree_augmented::IdArrayType Supernodes;
  // where the supernode connects to
  viskores::worklet::contourtree_augmented::IdArrayType Superarcs;
  // the hyperparent for each supernode
  viskores::worklet::contourtree_augmented::IdArrayType Hyperparents;
  // the hypernode ID for each supernode: often NO_SUCH_ELEMENT
  // but it makes lookups easier
  viskores::worklet::contourtree_augmented::IdArrayType Super2Hypernode;

  // which iteration & round the vertex is transferred in
  // the second of these is the same as "whenTransferred", but inverted order
  viskores::worklet::contourtree_augmented::IdArrayType WhichRound;
  viskores::worklet::contourtree_augmented::IdArrayType WhichIteration;

  // HYPER arrays: i.e. over all hypernodes in the tree
  // the ID in the supernode array
  viskores::worklet::contourtree_augmented::IdArrayType Hypernodes;
  // where the hypernode connects to
  viskores::worklet::contourtree_augmented::IdArrayType Hyperarcs;
  // the number of child supernodes on the superarc (including the start node)
  // and not including any inserted in the hierarchy
  viskores::worklet::contourtree_augmented::IdArrayType Superchildren;

  // how many rounds of fan-in were used to construct it
  viskores::Id NumRounds;

  // use for debugging? -> This makes more sense in hyper sweeper?
  // viskores::Id NumOwnedRegularVertices;

  // The following arrays store the numbers of reg/super/hyper nodes at each level of the hierarchy
  // They are filled in from the top down, and are fundamentally CPU side control variables
  // They will be needed for hypersweeps.

  // May be for hypersweeps later on. SHOULD be primarily CPU side
  /// arrays holding the logical size of the arrays at each level
  viskores::worklet::contourtree_augmented::IdArrayType NumRegularNodesInRound;
  viskores::worklet::contourtree_augmented::IdArrayType NumSupernodesInRound;
  viskores::worklet::contourtree_augmented::IdArrayType NumHypernodesInRound;

  /// how many iterations needed for the hypersweep at each level
  viskores::worklet::contourtree_augmented::IdArrayType NumIterations;

  /// vectors tracking the segments used in each iteration of the hypersweep
  // TODO/FIXME: Consider using ArrayHandleGroupVecVariable instead of an STL vector
  // of ArrayHandles. (Though that may be a bit tricky with dynamic resizing.)
  std::vector<viskores::worklet::contourtree_augmented::IdArrayType> FirstSupernodePerIteration;
  std::vector<viskores::worklet::contourtree_augmented::IdArrayType> FirstHypernodePerIteration;

  /// routine to create a FindRegularByGlobal object that we can use as an input for worklets to call the function
  VISKORES_CONT
  FindRegularByGlobal GetFindRegularByGlobal() const
  {
    return FindRegularByGlobal(this->RegularNodeSortOrder, this->RegularNodeGlobalIds);
  }

  /// routine to create a FindSuperArcForUnknownNode object that we can use as an input for worklets to call the function
  VISKORES_CONT
  FindSuperArcForUnknownNode<FieldType> GetFindSuperArcForUnknownNode()
  {
    return FindSuperArcForUnknownNode<FieldType>(this->Superparents,
                                                 this->Supernodes,
                                                 this->Superarcs,
                                                 this->Superchildren,
                                                 this->WhichRound,
                                                 this->WhichIteration,
                                                 this->Hyperparents,
                                                 this->Hypernodes,
                                                 this->Hyperarcs,
                                                 this->RegularNodeGlobalIds,
                                                 this->DataValues);
  }

  /// routine to create a FindSuperArcBetweenNodes object that we can use as an input for worklets to call the function
  VISKORES_CONT
  FindSuperArcBetweenNodes GetFindSuperArcBetweenNodes() const
  {
    return FindSuperArcBetweenNodes(this->Superarcs);
  }

  ///  routine to initialize the hierarchical tree with the top level tree
  VISKORES_CONT
  void Initialize(viskores::Id numRounds,
                  viskores::worklet::contourtree_augmented::ContourTree& tree,
                  viskores::worklet::contourtree_augmented::ContourTreeMesh<FieldType>& mesh);

  /// utility routines for the path probes
  VISKORES_CONT
  std::string RegularString(const viskores::Id regularId) const;

  VISKORES_CONT
  std::string SuperString(const viskores::Id superId) const;

  VISKORES_CONT
  std::string HyperString(const viskores::Id hyperId) const;

  /// routine to probe a given node and trace it's hyperpath to the root
  VISKORES_CONT
  std::string ProbeHyperPath(const viskores::Id regularId, const viskores::Id maxLength = -1) const;

  /// routine to probe a given node and trace it's superpath to the root
  VISKORES_CONT
  std::string ProbeSuperPath(const viskores::Id regularId, const viskores::Id maxLength = -1) const;

  /// Outputs the Hierarchical Tree in Dot format for visualization
  VISKORES_CONT
  std::string PrintDotSuperStructure(const char* label) const;

  /// Print hierarchical tree construction stats, usually used for logging
  VISKORES_CONT
  std::string PrintTreeStats() const;

  /// debug routine
  VISKORES_CONT
  std::string DebugPrint(std::string message, const char* fileName, long lineNum) const;

  // modified version of dumpSuper() that also gives volume counts
  VISKORES_CONT
  static std::string DumpVolumes(
    const viskores::worklet::contourtree_augmented::IdArrayType& supernodes,
    const viskores::worklet::contourtree_augmented::IdArrayType& superarcs,
    const viskores::worklet::contourtree_augmented::IdArrayType& regularNodeGlobalIds,
    viskores::Id totalVolume,
    const viskores::worklet::contourtree_augmented::IdArrayType& intrinsicVolume,
    const viskores::worklet::contourtree_augmented::IdArrayType& dependentVolume);

  // Helper function to convert an STL vector of Viskores arrays into components and
  // group arrays (for packing into a viskores::cont::DataSet and using with an
  // ArrayHandleGroupVecVariable for access)
  // TODO/FIXME: Ultimately, we should get rid of the STL arrays and use an
  // ArrayHandleGroupVecVariable in this class.
  VISKORES_CONT
  static void ConvertSTLVecOfHandlesToVISKORESComponentsAndOffsetsArray(
    const std::vector<viskores::worklet::contourtree_augmented::IdArrayType>& inputVec,
    viskores::worklet::contourtree_augmented::IdArrayType& outputComponents,
    viskores::cont::ArrayHandle<viskores::Id>& outputOffsets);

  VISKORES_CONT
  void AddToVISKORESDataSet(viskores::cont::DataSet& ds) const;

private:
  /// Used internally to Invoke worklets
  viskores::cont::Invoker Invoke;
};

template <typename FieldType>
HierarchicalContourTree<FieldType>::HierarchicalContourTree()
//: NumOwnedRegularVertices(static_cast<viskores::Id>(0))
{ // constructor
  NumRegularNodesInRound.ReleaseResources();
  NumSupernodesInRound.ReleaseResources();
  NumHypernodesInRound.ReleaseResources();
  NumIterations.ReleaseResources();
} // constructor


///  routine to initialize the hierarchical tree with the top level tree
template <typename FieldType>
void HierarchicalContourTree<FieldType>::Initialize(
  viskores::Id numRounds,
  viskores::worklet::contourtree_augmented::ContourTree& tree,
  viskores::worklet::contourtree_augmented::ContourTreeMesh<FieldType>& mesh)
{ // Initialize(..)
  // TODO: If any other arrays are only copied in this function but will not be modified then we could just assign instead of copy them and make them const
  // set the initial logical size of the arrays: note that we need to keep level 0 separate, so have an extra level at the top
  this->NumRounds = numRounds;
  {
    auto tempZeroArray = viskores::cont::ArrayHandleConstant<viskores::Id>(0, this->NumRounds + 1);
    viskores::cont::Algorithm::Copy(tempZeroArray, this->NumIterations);
    viskores::cont::Algorithm::Copy(tempZeroArray, this->NumRegularNodesInRound);
    viskores::worklet::contourtree_augmented::IdArraySetValue(
      this->NumRounds, tree.Nodes.GetNumberOfValues(), this->NumRegularNodesInRound);
    viskores::cont::Algorithm::Copy(tempZeroArray, this->NumSupernodesInRound);
    viskores::worklet::contourtree_augmented::IdArraySetValue(
      this->NumRounds, tree.Supernodes.GetNumberOfValues(), this->NumSupernodesInRound);
    viskores::cont::Algorithm::Copy(tempZeroArray, this->NumHypernodesInRound);
    viskores::worklet::contourtree_augmented::IdArraySetValue(
      this->NumRounds, tree.Hypernodes.GetNumberOfValues(), this->NumHypernodesInRound);
  }
  // copy the iterations of the top level hypersweep - this is +1: one because we are counting inclusively
  // HAC JAN 15, 2020: In order to make this consistent with grafting rounds for hybrid hypersweeps, we add one to the logical number of
  // iterations instead of the prior version which stored an extra extra element (ie +2)
  // WARNING! WARNING! WARNING!  This is a departure from the treatment in the contour tree, where the last iteration to the NULL root was
  // treated as an implicit round.
  {
    viskores::Id tempSizeVal =
      viskores::cont::ArrayGetValue(this->NumRounds, this->NumIterations) + 1;
    viskores::worklet::contourtree_augmented::IdArraySetValue(
      this->NumRounds, tree.NumIterations + 1, this->NumIterations);
    this->FirstSupernodePerIteration.resize(static_cast<std::size_t>(this->NumRounds + 1));
    this->FirstSupernodePerIteration[static_cast<std::size_t>(this->NumRounds)].Allocate(
      tempSizeVal);
    this->FirstHypernodePerIteration.resize(static_cast<std::size_t>(this->NumRounds + 1));
    this->FirstHypernodePerIteration[static_cast<std::size_t>(this->NumRounds)].Allocate(
      tempSizeVal);
  }
  // now copy in the details. Use CopySubRagnge to ensure that the Copy does not shrink the size
  // of the array as the arrays are in this case allocated above to the approbriate size
  viskores::cont::Algorithm::CopySubRange(
    tree.FirstSupernodePerIteration,                     // copy this
    0,                                                   // start at index 0
    tree.FirstSupernodePerIteration.GetNumberOfValues(), // copy all values
    this->FirstSupernodePerIteration[static_cast<std::size_t>(this->NumRounds)]);
  viskores::cont::Algorithm::CopySubRange(
    tree.FirstHypernodePerIteration,
    0,                                                   // start at index 0
    tree.FirstHypernodePerIteration.GetNumberOfValues(), // copy all values
    this->FirstHypernodePerIteration[static_cast<std::size_t>(this->NumRounds)]);

  // set the sizes for the arrays
  this->RegularNodeGlobalIds.Allocate(tree.Nodes.GetNumberOfValues());
  this->DataValues.Allocate(mesh.SortedValues.GetNumberOfValues());
  this->RegularNodeSortOrder.Allocate(tree.Nodes.GetNumberOfValues());
  this->Superparents.Allocate(tree.Superparents.GetNumberOfValues());
  {
    auto tempNSE = viskores::cont::ArrayHandleConstant<viskores::Id>(
      viskores::worklet::contourtree_augmented::NO_SUCH_ELEMENT, tree.Nodes.GetNumberOfValues());
    viskores::cont::Algorithm::Copy(tempNSE, this->Regular2Supernode);
  }

  this->Supernodes.Allocate(tree.Supernodes.GetNumberOfValues());
  this->Superarcs.Allocate(tree.Superarcs.GetNumberOfValues());
  this->Hyperparents.Allocate(tree.Hyperparents.GetNumberOfValues());
  {
    auto tempNSE = viskores::cont::ArrayHandleConstant<viskores::Id>(
      viskores::worklet::contourtree_augmented::NO_SUCH_ELEMENT,
      tree.Supernodes.GetNumberOfValues());
    viskores::cont::Algorithm::Copy(tempNSE, this->Super2Hypernode);
  }
  this->WhichRound.Allocate(tree.Supernodes.GetNumberOfValues());
  this->WhichIteration.Allocate(tree.Supernodes.GetNumberOfValues());

  this->Hypernodes.Allocate(tree.Hypernodes.GetNumberOfValues());
  this->Hyperarcs.Allocate(tree.Hyperarcs.GetNumberOfValues());
  this->Superchildren.Allocate(tree.Hyperarcs.GetNumberOfValues());

  //copy the regular nodes
  viskores::cont::Algorithm::Copy(mesh.GlobalMeshIndex, this->RegularNodeGlobalIds);
  viskores::cont::Algorithm::Copy(mesh.SortedValues, this->DataValues);

  // we want to be able to search by global mesh index.  That means we need to have an index array, sorted indirectly on globalMeshIndex
  viskores::cont::Algorithm::Copy(
    viskores::cont::ArrayHandleIndex(RegularNodeSortOrder.GetNumberOfValues()),
    RegularNodeSortOrder);
  viskores::cont::Algorithm::Sort(RegularNodeSortOrder,
                                  PermuteComparator(this->RegularNodeGlobalIds));
  viskores::cont::Algorithm::Copy(tree.Superparents, this->Superparents);

  // copy in the supernodes
  viskores::cont::Algorithm::Copy(tree.Supernodes, this->Supernodes);
  viskores::cont::Algorithm::Copy(tree.Superarcs, this->Superarcs);
  viskores::cont::Algorithm::Copy(tree.Hyperparents, this->Hyperparents);

  viskores::cont::Algorithm::Copy(viskores::cont::ArrayHandleConstant<viskores::Id>(
                                    numRounds, this->WhichRound.GetNumberOfValues()),
                                  this->WhichRound);
  viskores::cont::Algorithm::Copy(tree.WhenTransferred, this->WhichIteration);

  // now set the regular to supernode array up: it's already been set to NO_SUCH_ELEMENT
  {
    auto regular2SupernodePermuted =
      viskores::cont::make_ArrayHandlePermutation(this->Supernodes, this->Regular2Supernode);
    viskores::cont::Algorithm::Copy(
      viskores::cont::ArrayHandleIndex(this->Supernodes.GetNumberOfValues()),
      regular2SupernodePermuted);
  }
  // copy in the hypernodes
  viskores::cont::Algorithm::Copy(tree.Hypernodes, this->Hypernodes);
  viskores::cont::Algorithm::Copy(tree.Hyperarcs, this->Hyperarcs);

  // now set the supernode to hypernode array up: it's already been set to NO_SUCH_ELEMENT
  {
    auto super2HypernodePermuted =
      viskores::cont::make_ArrayHandlePermutation(this->Hypernodes, this->Super2Hypernode);
    viskores::cont::Algorithm::Copy(
      viskores::cont::ArrayHandleIndex(this->Hypernodes.GetNumberOfValues()),
      super2HypernodePermuted);
  }
  {
    auto initalizeSuperchildrenWorklet = InitalizeSuperchildrenWorklet();
    this->Invoke(initalizeSuperchildrenWorklet,
                 this->Hyperarcs,    // Input
                 this->Hypernodes,   // Input
                 this->Superchildren // Output
    );
  }
} // Initialize(..)


/// utility routine for the path probes
template <typename FieldType>
std::string HierarchicalContourTree<FieldType>::RegularString(const viskores::Id regularId) const
{ // RegularString()
  std::stringstream resultStream;
  // this can get called before the regular ID is fully stored
  if (regularId >= this->DataValues.GetNumberOfValues())
  {
    resultStream << "Regular ID: ";
    viskores::worklet::contourtree_augmented::PrintIndexType(regularId, resultStream);
    resultStream << " Value: N/A Global ID: N/A Regular ID: ";
    viskores::worklet::contourtree_augmented::PrintIndexType(regularId, resultStream);
    resultStream << " SNode ID:    N/A Superparent: N/A";
  }
  else
  {
    resultStream << "Regular ID: ";
    viskores::worklet::contourtree_augmented::PrintIndexType(regularId, resultStream);
    resultStream << "  Value: " << viskores::cont::ArrayGetValue(regularId, this->DataValues);
    resultStream << " Global ID: ";
    viskores::worklet::contourtree_augmented::PrintIndexType(
      viskores::cont::ArrayGetValue(regularId, this->RegularNodeGlobalIds), resultStream);
    resultStream << " Regular ID: ";
    viskores::worklet::contourtree_augmented::PrintIndexType(regularId, resultStream);
    resultStream << " SNode ID: ";
    viskores::worklet::contourtree_augmented::PrintIndexType(
      viskores::cont::ArrayGetValue(regularId, this->Regular2Supernode), resultStream);
    resultStream << "Superparents: ";
    viskores::worklet::contourtree_augmented::PrintIndexType(
      viskores::cont::ArrayGetValue(regularId, this->Superparents));
  }
  return resultStream.str();
} // RegularString()


/// utility routine for the path probes
template <typename FieldType>
std::string HierarchicalContourTree<FieldType>::SuperString(const viskores::Id superId) const
{ // SuperString()
  std::stringstream resultStream;
  if (viskores::worklet::contourtree_augmented::NoSuchElement(superId))
  {
    resultStream << "Super ID:   ";
    viskores::worklet::contourtree_augmented::PrintIndexType(superId, resultStream);
  }
  else
  {
    viskores::Id unmaskedSuperId = viskores::worklet::contourtree_augmented::MaskedIndex(superId);
    viskores::Id tempSupernodeOfSuperId =
      viskores::cont::ArrayGetValue(unmaskedSuperId, this->Supernodes);
    resultStream << "Super ID:   ";
    viskores::worklet::contourtree_augmented::PrintIndexType(superId, resultStream);
    resultStream << "  Value: "
                 << viskores::cont::ArrayGetValue(tempSupernodeOfSuperId, this->DataValues);
    resultStream << " Global ID: ";
    viskores::worklet::contourtree_augmented::PrintIndexType(
      viskores::cont::ArrayGetValue(tempSupernodeOfSuperId, this->RegularNodeGlobalIds),
      resultStream);
    resultStream << " Regular Id: ";
    viskores::worklet::contourtree_augmented::PrintIndexType(tempSupernodeOfSuperId, resultStream);
    resultStream << " Superarc:    ";
    viskores::worklet::contourtree_augmented::PrintIndexType(
      viskores::cont::ArrayGetValue(unmaskedSuperId, this->Superarcs), resultStream);
    resultStream << " HNode ID: ";
    viskores::worklet::contourtree_augmented::PrintIndexType(
      viskores::cont::ArrayGetValue(unmaskedSuperId, this->Super2Hypernode), resultStream);
    resultStream << " Hyperparent:   ";
    viskores::worklet::contourtree_augmented::PrintIndexType(
      viskores::cont::ArrayGetValue(unmaskedSuperId, this->Hyperparents), resultStream);
    resultStream << " Round: ";
    viskores::worklet::contourtree_augmented::PrintIndexType(
      viskores::cont::ArrayGetValue(unmaskedSuperId, this->WhichRound), resultStream);
    resultStream << " Iteration: ";
    viskores::worklet::contourtree_augmented::PrintIndexType(
      viskores::cont::ArrayGetValue(unmaskedSuperId, this->WhichIteration), resultStream);
  }
  return resultStream.str();
} // SuperString()


/// utility routine for the path probes
template <typename FieldType>
std::string HierarchicalContourTree<FieldType>::HyperString(const viskores::Id hyperId) const
{ // HyperString()
  std::stringstream resultStream;
  if (viskores::worklet::contourtree_augmented::NoSuchElement(hyperId))
  {
    resultStream << "Hyper ID:   ";
    viskores::worklet::contourtree_augmented::PrintIndexType(hyperId, resultStream);
  }
  else
  {
    viskores::Id unmaskedHyperId = viskores::worklet::contourtree_augmented::MaskedIndex(hyperId);
    viskores::Id hypernodeOfHyperId =
      viskores::cont::ArrayGetValue(unmaskedHyperId, this->Hypernodes);
    viskores::Id supernodeOfHyperId =
      viskores::cont::ArrayGetValue(hypernodeOfHyperId, this->Supernodes);
    resultStream << "Hyper Id:    ";
    viskores::worklet::contourtree_augmented::PrintIndexType(hyperId, resultStream);
    resultStream << "  Value: "
                 << viskores::cont::ArrayGetValue(supernodeOfHyperId, this->DataValues);
    resultStream << " Global ID: ";
    viskores::worklet::contourtree_augmented::PrintIndexType(
      viskores::cont::ArrayGetValue(supernodeOfHyperId, this->RegularNodeGlobalIds), resultStream);
    resultStream << " Regular ID: ";
    viskores::worklet::contourtree_augmented::PrintIndexType(supernodeOfHyperId, resultStream);
    resultStream << " Super ID: ";
    viskores::worklet::contourtree_augmented::PrintIndexType(hypernodeOfHyperId, resultStream);
    resultStream << " Hyperarc: ";
    viskores::worklet::contourtree_augmented::PrintIndexType(
      viskores::cont::ArrayGetValue(unmaskedHyperId, this->Hyperarcs), resultStream);
    resultStream << " Superchildren: "
                 << viskores::cont::ArrayGetValue(unmaskedHyperId, this->Superchildren);
  }
  return resultStream.str();
} // HyperString()

/// routine to probe a given node and trace it's hyperpath to the root
template <typename FieldType>
std::string HierarchicalContourTree<FieldType>::ProbeHyperPath(const viskores::Id regularId,
                                                               const viskores::Id maxLength) const
{ // ProbeHyperPath()
  std::stringstream resultStream;
  resultStream << "Probing HyperPath\n";
  resultStream << "Node:        " << this->RegularString(regularId) << std::endl;

  // find the superparent
  viskores::Id superparent = viskores::cont::ArrayGetValue(regularId, this->Superparents);
  resultStream << "Superparent: " << SuperString(superparent) << std::endl;

  // and the hyperparent
  viskores::Id hyperparent = viskores::cont::ArrayGetValue(superparent, this->Hyperparents);

  // now trace the path inwards: terminate on last round when we have null hyperarc
  viskores::Id length = 0;
  while (true)
  { // loop inwards
    length++;
    if (length > maxLength && maxLength > 0)
    {
      break;
    }
    resultStream << "Hyperparent: " << this->HyperString(hyperparent) << std::endl;

    // retrieve the target of the hyperarc
    viskores::Id hypertarget = viskores::cont::ArrayGetValue(hyperparent, this->Hyperarcs);

    resultStream << "Hypertarget: "
                 << SuperString(viskores::worklet::contourtree_augmented::MaskedIndex(hypertarget))
                 << std::endl;

    // mask the hypertarget
    viskores::Id maskedHypertarget =
      viskores::worklet::contourtree_augmented::MaskedIndex(hypertarget);

    // test for null superarc: can only be root or attachment point
    if (viskores::worklet::contourtree_augmented::NoSuchElement(hypertarget))
    { // root or attachment point
      // we're done
      break;
    } // root or attachment point
    else
    { // ordinary supernode
      hyperparent = viskores::cont::ArrayGetValue(maskedHypertarget, this->Hyperparents);
    } // ordinary supernode

    // now take the new superparent's hyperparent/hypertarget
    hypertarget = viskores::cont::ArrayGetValue(hyperparent, this->Hyperarcs);
  } // loop inwards

  resultStream << "Probe Complete" << std::endl << std::endl;
  return resultStream.str();
} // ProbeHyperPath()


/// routine to probe a given node and trace it's superpath to the root
template <typename FieldType>
std::string HierarchicalContourTree<FieldType>::ProbeSuperPath(const viskores::Id regularId,
                                                               const viskores::Id maxLength) const
{
  std::stringstream resultStream;
  // find the superparent
  viskores::Id superparent = viskores::cont::ArrayGetValue(regularId, this->Superparents);
  // now trace the path inwards: terminate on last round when we have null hyperarc
  viskores::Id length = 0;
  while (true)
  { // loop inwards
    length++;
    if (length > maxLength && maxLength > 0)
    {
      break;
    }
    // retrieve the target of the superarc
    viskores::Id supertarget = viskores::cont::ArrayGetValue(superparent, this->Superarcs);

    resultStream << "Superparent: " << this->SuperString(superparent) << std::endl;
    resultStream << "Supertarget: "
                 << this->SuperString(
                      viskores::worklet::contourtree_augmented::MaskedIndex(supertarget))
                 << std::endl;

    // mask the supertarget
    viskores::Id maskedSupertarget =
      viskores::worklet::contourtree_augmented::MaskedIndex(supertarget);
    // and retrieve it's supertarget
    viskores::Id nextSupertarget =
      viskores::cont::ArrayGetValue(maskedSupertarget, this->Superarcs);
    viskores::Id maskedNextSupertarget =
      viskores::worklet::contourtree_augmented::MaskedIndex(nextSupertarget);
    resultStream << "Next target: " << this->SuperString(nextSupertarget) << std::endl;

    // test for null superarc: can only be root or attachment point
    if (viskores::worklet::contourtree_augmented::NoSuchElement(nextSupertarget))
    { // root or attachment point
      // test round: if it's the last one, only the root has a null edge
      if (viskores::cont::ArrayGetValue(maskedNextSupertarget, this->WhichRound) == this->NumRounds)
        // we're done
        break;
      else // attachment point
        superparent = maskedNextSupertarget;
    } // root or attachment point
    else
    { // ordinary supernode
      superparent = maskedSupertarget;
    } // ordinary supernode
  }   // loop inwards

  resultStream << "Probe Complete" << std::endl << std::endl;
  return resultStream.str();
}

/// Outputs the Hierarchical Tree in Dot format for visualization
template <typename FieldType>
std::string HierarchicalContourTree<FieldType>::PrintDotSuperStructure(const char* label) const
{ // PrintDotSuperStructure
  // make a copy of the label
  std::string filename("temp/");
  filename += label;

  // replace spaces with underscores
  for (unsigned int strChar = 0; strChar < filename.length(); strChar++)
    if (filename[strChar] == ' ')
      filename[strChar] = '_';

  // add the .gv suffix
  filename += ".gv";

  // generate an output stream
  std::ofstream outstream(filename);

  // print the header information
  outstream << "digraph RegularTree\n\t{\n";
  outstream << "\tsize=\"6.5, 9\"\n\tratio=\"fill\"\n";
  outstream << "\tlabel=\"" << label << "\"\n\tlabelloc=t\n\tfontsize=30\n";

  // create the NULL (root) node
  outstream << "\t// NULL node to use as a root for the tree\n";
  outstream << "\tNULL [style=filled,fillcolor=white,shape=point,label=\"NULL\"];\n";

  outstream << "\t// Supernodes\n";
  // loop through all supernodes
  // We use regular ReadPortals here since this requires access to many values anyways
  auto supernodesPortal = this->Supernodes.ReadPortal();
  auto hypernodesPortal = this->Hypernodes.ReadPortal();
  auto hyperparentsPortal = this->Hyperparents.ReadPortal();
  auto hyperarcsPortal = this->Hyperarcs.ReadPortal();
  auto regularNodeGlobalIdsPortal = this->RegularNodeGlobalIds.ReadPortal();
  auto whichIterationPortal = this->WhichIteration.ReadPortal();
  auto whichRoundPortal = this->WhichRound.ReadPortal();
  auto superarcsPortal = this->Superarcs.ReadPortal();
  auto superparentsPortal = this->Superparents.ReadPortal();
  for (viskores::Id supernode = 0; supernode < this->Supernodes.GetNumberOfValues(); supernode++)
  { // per supernode
    viskores::Id regularID = supernodesPortal.Get(supernode);
    // print the supernode, making hypernodes double octagons
    outstream
      << "    SN" << std::setw(1) << supernode
      << " [style=filled,fillcolor=white,shape=" << std::setw(1)
      << ((hypernodesPortal.Get(hyperparentsPortal.Get(supernode)) == supernode)
            ? "doublecircle"
            : "circle") // hypernodes are double-circles
      << ",label=\"sn" << std::setw(4) << supernode << "    h" << std::setw(1)
      << ((hypernodesPortal.Get(hyperparentsPortal.Get(supernode)) == supernode)
            ? "n"
            : "p") // hypernodes show "hn001" (their own ID), supernodes show "hp001" (their hyperparent)
      << std::setw(4) << hyperparentsPortal.Get(supernode) << "\\nm" << std::setw(1) << regularID
      << "    g" << std::setw(4) << regularNodeGlobalIdsPortal.Get(regularID) << "\\nrd"
      << std::setw(1) << whichIterationPortal.Get(supernode) << "    it" << std::setw(4)
      << contourtree_augmented::MaskedIndex(whichIterationPortal.Get(supernode)) << "\"];\n";
  } // per supernode

  outstream << "\t// Superarc nodes\n";
  // now repeat to create nodes for the middle of each superarc (to represent the superarcs themselves)
  for (viskores::Id superarc = 0; superarc < this->Superarcs.GetNumberOfValues(); superarc++)
  { // per superarc
    // print the superarc vertex
    outstream
      << "\tSA" << std::setw(1) << superarc
      << " [shape=circle,fillcolor=white,fixedsize=true,height=0.5,width=0.5,label=\"\"];\n";
  } // per superarc

  outstream << "\t// Superarc edges\n";
  // loop through all superarcs to draw them
  for (viskores::Id superarc = 0; superarc < this->Superarcs.GetNumberOfValues(); superarc++)
  { // per superarc
    // retrieve ID of target supernode
    viskores::Id superarcFrom = superarc;
    viskores::Id superarcTo = superarcsPortal.Get(superarcFrom);

    // if this is true, it may be the last pruned vertex
    if (contourtree_augmented::NoSuchElement(superarcTo))
    { // no superarc
      // if it occurred on the final round, it's the global root and is shown as the NULL node
      if (whichRoundPortal.Get(superarcFrom) == this->NumRounds)
      { // root node
        outstream << "\tSN" << std::setw(1) << superarcFrom << " -> SA" << std::setw(1) << superarc
                  << " [label=\"S" << std::setw(1) << superarc << "\",style=dotted]\n";
        outstream << "\tSN" << std::setw(1) << superarc << " -> NULL[label=\"S" << std::setw(1)
                  << superarc << "\",style=dotted]\n";
      } // root node
      else
      { // attachment point
        // otherwise, the target is actually a superarc vertex not a supernode vertex
        // so we use the regular ID to retrieve the superparent which tells us which superarc we insert into
        viskores::Id regularFrom = supernodesPortal.Get(superarcFrom);
        superarcTo = superparentsPortal.Get(regularFrom);

        // output a suitable edge
        outstream << "\tSN" << std::setw(1) << superarcFrom << " -> SA" << std::setw(1)
                  << superarcTo << "[label=\"S" << std::setw(1) << superarc << "\",style=dotted]\n";
      } // attachment point
    }   // no superarc

    else
    { // there is a superarc
      // retrieve the ascending flag
      bool ascendingSuperarc = contourtree_augmented::IsAscending(superarcTo);

      // strip out the flags
      superarcTo = contourtree_augmented::MaskedIndex(superarcTo);

      // how we print depends on whether the superarc ascends
      outstream << "\tSN" << std::setw(1) << (ascendingSuperarc ? superarcTo : superarcFrom)
                << " -> SA" << std::setw(1) << superarc << " [label=\"S" << std::setw(1) << superarc
                << "\"" << (ascendingSuperarc ? ",dir=\"back\"" : "") << ",arrowhead=\"none\"]\n";
      outstream << "\tSA" << std::setw(1) << superarc << " -> SN" << std::setw(1)
                << (ascendingSuperarc ? superarcFrom : superarcTo) << " [label=\"S" << std::setw(1)
                << superarc << "\"" << (ascendingSuperarc ? ",dir=\"back\"" : "")
                << ",arrowhead=\"none\"]\n";
    } // there is a superarc
  }   // per superarc

  outstream << "\t// Hyperarcs\n";
  // now loop through the hyperarcs to draw them
  for (viskores::Id hyperarc = 0; hyperarc < this->Hyperarcs.GetNumberOfValues(); hyperarc++)
  { // per hyperarc
    // retrieve ID of target hypernode
    viskores::Id hyperarcFrom = hypernodesPortal.Get(hyperarc);
    viskores::Id hyperarcTo = hyperarcsPortal.Get(hyperarc);

    // if this is true, it is the last pruned vertex & needs a hyperarc to the root
    if (contourtree_augmented::NoSuchElement(hyperarcTo))
      outstream << "\tSN" << std::setw(1) << hyperarcFrom << " -> NULL[label=\"H" << std::setw(1)
                << hyperarc << "\",penwidth=5.0,style=dotted]\n";
    else
    { // not the last one
      // otherwise, retrieve the ascending flag
      bool ascendingHyperarc = contourtree_augmented::IsAscending(hyperarcTo);

      // strip out the flags
      hyperarcTo = contourtree_augmented::MaskedIndex(hyperarcTo);

      // how we print depends on whether the hyperarc ascends
      outstream << "\tSN" << std::setw(1) << (ascendingHyperarc ? hyperarcTo : hyperarcFrom)
                << " -> SN" << std::setw(1) << (ascendingHyperarc ? hyperarcFrom : hyperarcTo)
                << "[label=\"H" << std::setw(1) << hyperarc << "\",penwidth=5.0,dir=\"back\"]\n";
    } // not the last one
  }   // per hyperarc

  // print the footer information
  outstream << "\t}\n";

  return std::string("HierarchicalContourTree<FieldType>::PrintDotSuperStructure() Complete");
} // PrintDotSuperStructure

/// debug routine
template <typename FieldType>
std::string HierarchicalContourTree<FieldType>::DebugPrint(std::string message,
                                                           const char* fileName,
                                                           long lineNum) const
{ // DebugPrint
  std::stringstream resultStream;
  resultStream << std::endl;
  resultStream << "[CUTHERE]-------------------------------" << std::endl;
  resultStream << std::setw(30) << std::left << fileName << ":" << std::right << std::setw(4)
               << lineNum << std::endl;
  resultStream << std::left << std::string(message) << std::endl;
  resultStream << "Hierarchical Contour Tree Contains:     " << std::endl;
  resultStream << "----------------------------------------" << std::endl;
  resultStream << std::endl;

  viskores::worklet::contourtree_augmented::PrintHeader(
    this->RegularNodeGlobalIds.GetNumberOfValues(), resultStream);
  viskores::worklet::contourtree_augmented::PrintIndices(
    "Regular Nodes (global ID)", this->RegularNodeGlobalIds, -1, resultStream);
  viskores::worklet::contourtree_augmented::PrintValues(
    "Data Values", this->DataValues, -1, resultStream);
  viskores::worklet::contourtree_augmented::PrintIndices(
    "Regular Node Sort Order", this->RegularNodeSortOrder, -1, resultStream);
  viskores::worklet::contourtree_augmented::PrintIndices(
    "Superparents (unsorted)", this->Superparents, -1, resultStream);
  viskores::worklet::contourtree_augmented::PrintIndices(
    "Supernode ID (if any)", this->Regular2Supernode, -1, resultStream);
  resultStream << std::endl;
  viskores::worklet::contourtree_augmented::PrintHeader(this->Supernodes.GetNumberOfValues(),
                                                        resultStream);
  viskores::worklet::contourtree_augmented::PrintIndices(
    "Supernodes (regular index)", this->Supernodes, -1, resultStream);
  viskores::worklet::contourtree_augmented::PrintIndices(
    "Superarcs (supernode index)", this->Superarcs, -1, resultStream);
  viskores::worklet::contourtree_augmented::PrintIndices(
    "Hyperparents (hypernode index)", this->Hyperparents, -1, resultStream);
  viskores::worklet::contourtree_augmented::PrintIndices(
    "Hypernode ID (if any)", this->Super2Hypernode, -1, resultStream);
  viskores::worklet::contourtree_augmented::PrintIndices(
    "Which Round", this->WhichRound, -1, resultStream);
  viskores::worklet::contourtree_augmented::PrintIndices(
    "Which Iteration", this->WhichIteration, -1, resultStream);
  resultStream << std::endl;
  viskores::worklet::contourtree_augmented::PrintHeader(this->Hypernodes.GetNumberOfValues(),
                                                        resultStream);
  viskores::worklet::contourtree_augmented::PrintIndices(
    "Hypernodes (supernode index)", this->Hypernodes, -1, resultStream);
  viskores::worklet::contourtree_augmented::PrintIndices(
    "Hyperarcs (supernode index)", this->Hyperarcs, -1, resultStream);
  viskores::worklet::contourtree_augmented::PrintIndices(
    "Superchildren", this->Superchildren, -1, resultStream);
  resultStream << std::endl;
  resultStream << "nRounds: " << this->NumRounds << std::endl;
  viskores::worklet::contourtree_augmented::PrintHeader(
    this->NumRegularNodesInRound.GetNumberOfValues(), resultStream);
  viskores::worklet::contourtree_augmented::PrintIndices(
    "nRegular Nodes In Round", this->NumRegularNodesInRound, -1, resultStream);
  viskores::worklet::contourtree_augmented::PrintIndices(
    "nSupernodes In Round", this->NumSupernodesInRound, -1, resultStream);
  viskores::worklet::contourtree_augmented::PrintIndices(
    "nHypernodes In Round", this->NumHypernodesInRound, -1, resultStream);
  //resultStream << "Owned Regular Vertices: " << this->NumOwnedRegularVertices << std::endl;
  viskores::worklet::contourtree_augmented::PrintHeader(this->NumIterations.GetNumberOfValues(),
                                                        resultStream);
  viskores::worklet::contourtree_augmented::PrintIndices(
    "nIterations", this->NumIterations, -1, resultStream);
  for (viskores::Id whichRound = 0; whichRound < this->NumIterations.GetNumberOfValues();
       whichRound++)
  { // per round
    resultStream << "Round " << whichRound << std::endl;
    viskores::worklet::contourtree_augmented::PrintHeader(
      this->FirstSupernodePerIteration[static_cast<std::size_t>(whichRound)].GetNumberOfValues(),
      resultStream);
    viskores::worklet::contourtree_augmented::PrintIndices(
      "First Supernode Per Iteration",
      this->FirstSupernodePerIteration[static_cast<std::size_t>(whichRound)],
      -1,
      resultStream);
    viskores::worklet::contourtree_augmented::PrintIndices(
      "First Hypernode Per Iteration",
      this->FirstHypernodePerIteration[static_cast<std::size_t>(whichRound)],
      -1,
      resultStream);
    resultStream << std::endl;
  } // per round
  return resultStream.str();
} // DebugPrint

template <typename FieldType>
std::string HierarchicalContourTree<FieldType>::PrintTreeStats() const
{ // PrintTreeStats
  std::stringstream resultStream;
  resultStream << std::setw(42) << std::left << "    NumRounds"
               << ": " << this->NumRounds << std::endl;
  viskores::worklet::contourtree_augmented::PrintIndices(
    "    NumIterations", this->NumIterations, -1, resultStream);
  viskores::worklet::contourtree_augmented::PrintIndices(
    "    NumRegularNodesInRound", this->NumRegularNodesInRound, -1, resultStream);
  viskores::worklet::contourtree_augmented::PrintIndices(
    "    NumSupernodesInRound", this->NumSupernodesInRound, -1, resultStream);
  viskores::worklet::contourtree_augmented::PrintIndices(
    "    NumHypernodesInRound", this->NumHypernodesInRound, -1, resultStream);

  return resultStream.str();
} // PrintTreeStats


// modified version of dumpSuper() that also gives volume counts
template <typename FieldType>
std::string HierarchicalContourTree<FieldType>::DumpVolumes(
  const viskores::worklet::contourtree_augmented::IdArrayType& supernodes,
  const viskores::worklet::contourtree_augmented::IdArrayType& superarcs,
  const viskores::worklet::contourtree_augmented::IdArrayType& regularNodeGlobalIds,
  viskores::Id totalVolume,
  const viskores::worklet::contourtree_augmented::IdArrayType& intrinsicVolume,
  const viskores::worklet::contourtree_augmented::IdArrayType& dependentVolume)
{ // DumpVolumes()
  // a local string stream to build the output
  std::stringstream outStream;

  // header info
  outStream << "============" << std::endl;
  outStream << "Contour Tree" << std::endl;

  // loop through all superarcs.
  // We use regular ReadPortals here since this requires access to many values anyways
  auto supernodesPortal = supernodes.ReadPortal();
  auto regularNodeGlobalIdsPortal = regularNodeGlobalIds.ReadPortal();
  auto superarcsPortal = superarcs.ReadPortal();
  auto intrinsicVolumePortal = intrinsicVolume.ReadPortal();
  auto dependentVolumePortal = dependentVolume.ReadPortal();
  for (viskores::Id supernode = 0; supernode < supernodes.GetNumberOfValues(); supernode++)
  { // per supernode
    // convert all the way down to global regular IDs
    viskores::Id fromRegular = supernodesPortal.Get(supernode);
    viskores::Id fromGlobal = regularNodeGlobalIdsPortal.Get(fromRegular);

    // retrieve the superarc target
    viskores::Id toSuper = superarcsPortal.Get(supernode);

    // if it is NO_SUCH_ELEMENT, it is the root or an attachment point
    // for an augmented tree, it can only be the root
    // in any event, we don't want to print them
    if (viskores::worklet::contourtree_augmented::NoSuchElement(toSuper))
    {
      continue;
    }
    // now break out the ascending flag & the underlying ID
    bool superarcAscends = viskores::worklet::contourtree_augmented::IsAscending(toSuper);
    toSuper = viskores::worklet::contourtree_augmented::MaskedIndex(toSuper);
    viskores::Id toRegular = supernodesPortal.Get(toSuper);
    viskores::Id toGlobal = regularNodeGlobalIdsPortal.Get(toRegular);

    // compute the weights
    viskores::Id weight = dependentVolumePortal.Get(supernode);
    // -1 because the validation output does not count the supernode for the superarc
    viskores::Id arcWeight = intrinsicVolumePortal.Get(supernode) - 1;
    viskores::Id counterWeight = totalVolume - weight + arcWeight;

    // orient with high end first
    if (superarcAscends)
    { // ascending superarc
      outStream << "H: " << std::setw(VOLUME_PRINT_WIDTH) << toGlobal;
      outStream << " L: " << std::setw(VOLUME_PRINT_WIDTH) << fromGlobal;
      outStream << " VH: " << std::setw(VOLUME_PRINT_WIDTH) << weight;
      outStream << " VR: " << std::setw(VOLUME_PRINT_WIDTH) << arcWeight;
      outStream << " VL: " << std::setw(VOLUME_PRINT_WIDTH) << counterWeight;
      outStream << std::endl;
    } // ascending superarc
    else
    { // descending superarc
      outStream << "H: " << std::setw(VOLUME_PRINT_WIDTH) << fromGlobal;
      outStream << " L: " << std::setw(VOLUME_PRINT_WIDTH) << toGlobal;
      outStream << " VH: " << std::setw(VOLUME_PRINT_WIDTH) << counterWeight;
      outStream << " VR: " << std::setw(VOLUME_PRINT_WIDTH) << arcWeight;
      outStream << " VL: " << std::setw(VOLUME_PRINT_WIDTH) << weight;
      outStream << std::endl;
    } // descending superarc

  } // per supernode
  // return the string
  return outStream.str();
} // DumpVolumes()

template <typename FieldType>
void HierarchicalContourTree<FieldType>::ConvertSTLVecOfHandlesToVISKORESComponentsAndOffsetsArray(
  const std::vector<viskores::worklet::contourtree_augmented::IdArrayType>& inputVec,
  viskores::worklet::contourtree_augmented::IdArrayType& outputComponents,
  viskores::cont::ArrayHandle<viskores::Id>& outputOffsets)
{
  // Compute num components vector
  viskores::cont::ArrayHandle<viskores::IdComponent> numComponents;
  numComponents.Allocate(inputVec.size());
  auto numComponentsWritePortal = numComponents.WritePortal();

  for (viskores::Id i = 0; i < static_cast<viskores::Id>(inputVec.size()); ++i)
  {
    numComponentsWritePortal.Set(
      i, static_cast<viskores::IdComponent>(inputVec[i].GetNumberOfValues()));
  }

  // Convert to offsets and store in output array
  viskores::Id componentsArraySize;
  viskores::cont::ConvertNumComponentsToOffsets(numComponents, outputOffsets, componentsArraySize);

  // Copy data to components array
  auto outputOffsetsReadPortal = outputOffsets.ReadPortal();
  outputComponents.Allocate(componentsArraySize);
  auto numComponentsReadPortal = numComponents.ReadPortal();
  for (viskores::Id i = 0; i < static_cast<viskores::Id>(inputVec.size()); ++i)
  {
    auto outputView = viskores::cont::make_ArrayHandleView(
      outputComponents, outputOffsetsReadPortal.Get(i), numComponentsReadPortal.Get(i));
    viskores::cont::ArrayCopy(inputVec[i], outputView);
  }
}

template <typename FieldType>
void HierarchicalContourTree<FieldType>::AddToVISKORESDataSet(viskores::cont::DataSet& ds) const
{
  // Create data set from output
  viskores::cont::Field regularNodeGlobalIdsField("RegularNodeGlobalIds",
                                                  viskores::cont::Field::Association::WholeDataSet,
                                                  this->RegularNodeGlobalIds);
  ds.AddField(regularNodeGlobalIdsField);
  viskores::cont::Field dataValuesField(
    "DataValues", viskores::cont::Field::Association::WholeDataSet, this->DataValues);
  ds.AddField(dataValuesField);
  viskores::cont::Field regularNodeSortOrderField("RegularNodeSortOrder",
                                                  viskores::cont::Field::Association::WholeDataSet,
                                                  this->RegularNodeSortOrder);
  ds.AddField(regularNodeSortOrderField);
  viskores::cont::Field regular2SupernodeField(
    "Regular2Supernode", viskores::cont::Field::Association::WholeDataSet, this->Regular2Supernode);
  ds.AddField(regular2SupernodeField);
  viskores::cont::Field superparentsField(
    "Superparents", viskores::cont::Field::Association::WholeDataSet, this->Superparents);
  ds.AddField(superparentsField);
  viskores::cont::Field supernodesField(
    "Supernodes", viskores::cont::Field::Association::WholeDataSet, this->Supernodes);
  ds.AddField(supernodesField);
  viskores::cont::Field superarcsField(
    "Superarcs", viskores::cont::Field::Association::WholeDataSet, this->Superarcs);
  ds.AddField(superarcsField);
  viskores::cont::Field superchildrenField(
    "Superchildren", viskores::cont::Field::Association::WholeDataSet, this->Superchildren);
  ds.AddField(superchildrenField);
  viskores::cont::Field hyperparentsField(
    "Hyperparents", viskores::cont::Field::Association::WholeDataSet, this->Hyperparents);
  ds.AddField(hyperparentsField);
  viskores::cont::Field hypernodesField(
    "Hypernodes", viskores::cont::Field::Association::WholeDataSet, this->Hypernodes);
  ds.AddField(hypernodesField);
  viskores::cont::Field hyperarcsField(
    "Hyperarcs", viskores::cont::Field::Association::WholeDataSet, this->Hyperarcs);
  ds.AddField(hyperarcsField);
  viskores::cont::Field super2HypernodeField(
    "Super2Hypernode", viskores::cont::Field::Association::WholeDataSet, this->Super2Hypernode);
  ds.AddField(super2HypernodeField);
  viskores::cont::Field whichRoundField(
    "WhichRound", viskores::cont::Field::Association::WholeDataSet, this->WhichRound);
  ds.AddField(whichRoundField);
  viskores::cont::Field whichIterationField(
    "WhichIteration", viskores::cont::Field::Association::WholeDataSet, this->WhichIteration);
  ds.AddField(whichIterationField);
  // TODO/FIXME: See what other fields we need to add
  viskores::worklet::contourtree_augmented::IdArrayType firstSupernodePerIterationComponents;
  viskores::cont::ArrayHandle<viskores::Id> firstSupernodePerIterationOffsets;
  ConvertSTLVecOfHandlesToVISKORESComponentsAndOffsetsArray(FirstSupernodePerIteration,
                                                            firstSupernodePerIterationComponents,
                                                            firstSupernodePerIterationOffsets);
  viskores::cont::Field firstSupernodePerIterationComponentsField(
    "FirstSupernodePerIterationComponents",
    viskores::cont::Field::Association::WholeDataSet,
    firstSupernodePerIterationComponents);
  ds.AddField(firstSupernodePerIterationComponentsField);
  viskores::cont::Field firstSupernodePerIterationOffsetsField(
    "FirstSupernodePerIterationOffsets",
    viskores::cont::Field::Association::WholeDataSet,
    firstSupernodePerIterationOffsets);
  ds.AddField(firstSupernodePerIterationOffsetsField);
  // TODO/FIXME: It seems we may only need the counts for the first iteration, so check, which
  // information we actually need.
  // Add the number of rounds as an array of length 1
  viskores::cont::ArrayHandle<viskores::Id> tempNumRounds;
  tempNumRounds.Allocate(1);
  viskores::worklet::contourtree_augmented::IdArraySetValue(0, this->NumRounds, tempNumRounds);
  viskores::cont::Field numRoundsField(
    "NumRounds", viskores::cont::Field::Association::WholeDataSet, tempNumRounds);
  ds.AddField(numRoundsField);
}

} // namespace contourtree_distributed
} // namespace worklet
} // namespace viskores


#endif
