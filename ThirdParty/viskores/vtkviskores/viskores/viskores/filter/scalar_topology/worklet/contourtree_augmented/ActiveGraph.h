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

#ifndef viskores_worklet_contourtree_augmented_activegraph_h
#define viskores_worklet_contourtree_augmented_activegraph_h

#include <iomanip>
#include <numeric>

// local includes
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/ArrayTransforms.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/MergeTree.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/MeshExtrema.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/PrintVectors.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/Types.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/activegraph/BuildChainsWorklet.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/activegraph/BuildTrunkWorklet.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/activegraph/CompactActiveEdgesComputeNewVertexOutdegree.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/activegraph/CompactActiveEdgesTransferActiveEdges.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/activegraph/EdgePeakComparator.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/activegraph/FindGoverningSaddlesWorklet.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/activegraph/FindSuperAndHyperNodesWorklet.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/activegraph/HyperArcSuperNodeComparator.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/activegraph/InitializeActiveEdges.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/activegraph/InitializeActiveGraphVertices.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/activegraph/InitializeEdgeFarFromActiveIndices.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/activegraph/InitializeHyperarcsFromActiveIndices.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/activegraph/InitializeNeighbourhoodMasksAndOutDegrees.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/activegraph/SetArcsConnectNodes.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/activegraph/SetArcsSetSuperAndHypernodeArcs.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/activegraph/SetArcsSlideVertices.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/activegraph/SetHyperArcsWorklet.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/activegraph/SetSuperArcsSetTreeHyperparents.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/activegraph/SetSuperArcsSetTreeSuperarcs.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/activegraph/SuperArcNodeComparator.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/activegraph/TransferRegularPointsWorklet.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/activegraph/TransferSaddleStartsResetEdgeFar.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/activegraph/TransferSaddleStartsSetNewOutdegreeForSaddles.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/activegraph/TransferSaddleStartsUpdateEdgeSorter.h>


//VISKORES includes
#include <viskores/Types.h>
#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayGetValues.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleConstant.h>
#include <viskores/cont/ArrayHandleIndex.h>
#include <viskores/cont/ArrayHandlePermutation.h>
#include <viskores/cont/ArrayHandleTransform.h>
#include <viskores/cont/ArrayPortalToIterators.h>
#include <viskores/cont/Error.h>
#include <viskores/cont/Invoker.h>


namespace active_graph_inc_ns = viskores::worklet::contourtree_augmented::active_graph_inc;


namespace viskores
{
namespace worklet
{
namespace contourtree_augmented
{


class ActiveGraph
{ // class ActiveGraph

  template <typename T, typename S>
  static T GetLastValue(const viskores::cont::ArrayHandle<T, S>& ah)
  {
    return viskores::cont::ArrayGetValue(ah.GetNumberOfValues() - 1, ah);
  }

public:
  viskores::cont::Invoker Invoke;

  // we also need the orientation of the edges (i.e. is it join or split)
  bool IsJoinGraph;

  // we will store the number of iterations the computation took here
  viskores::Id NumIterations;

  // ARRAYS FOR NODES IN THE TOPOLOGY GRAPH

  // for each vertex, we need to know where it is in global sort order / mesh
  IdArrayType GlobalIndex;

  // the hyperarcs - i.e. the pseudoextremum defining the hyperarc the vertex is on
  IdArrayType Hyperarcs;

  // the first edge for each vertex
  IdArrayType FirstEdge;

  // the outdegree for each vertex
  IdArrayType Outdegree;

  // ARRAYS FOR EDGES IN THE TOPOLOGY GRAPH

  // we will also need to keep track of both near and far ends of each edge
  IdArrayType EdgeFar;
  IdArrayType EdgeNear;

  // these now track the active nodes, edges, &c.:
  IdArrayType ActiveVertices;
  IdArrayType ActiveEdges;

  // and an array for sorting edges
  IdArrayType EdgeSorter;

  // temporary arrays for super/hyper ID numbers
  IdArrayType SuperID;
  IdArrayType HyperID;

  // variables tracking size of super/hyper tree
  viskores::Id NumSupernodes;
  viskores::Id NumHypernodes;

  // BASIC ROUTINES: CONSTRUCTOR, PRINT, &c.

  // constructor takes necessary references
  ActiveGraph(bool IsJoinGraph);

  // initialises the active graph
  template <class Mesh>
  void Initialise(Mesh& mesh, const MeshExtrema& meshExtrema);

  // routine that computes the merge tree from the active graph
  // was previously Compute()
  void MakeMergeTree(MergeTree& tree, MeshExtrema& meshExtrema);

  // sorts saddle starts to find governing saddles
  void FindGoverningSaddles();

  // marks now regular points for removal
  void TransferRegularPoints();

  // compacts the active vertex list
  void CompactActiveVertices();

  // compacts the active edge list
  void CompactActiveEdges();

  // builds the chains for the new active vertices
  void BuildChains();

  // suppresses non-saddles for the governing saddles pass
  void TransferSaddleStarts();

  // sets all remaining active vertices
  void BuildTrunk();

  // finds all super and hyper nodes, numbers them & sets up arrays for lookup
  void FindSuperAndHyperNodes(MergeTree& tree);

  // uses active graph to set superarcs & hyperparents in merge tree
  void SetSuperArcs(MergeTree& tree);

  // uses active graph to set hypernodes in merge tree
  void SetHyperArcs(MergeTree& tree);

  // uses active graph to set arcs in merge tree
  void SetArcs(MergeTree& tree, MeshExtrema& meshExtrema);

  // Allocate the vertex array
  void AllocateVertexArrays(viskores::Id nElems);

  // Allocate the edge array
  void AllocateEdgeArrays(viskores::Id nElems);

  // releases temporary arrays
  void ReleaseTemporaryArrays();

  // prints the contents of the active graph in a standard format
  void DebugPrint(const char* message, const char* fileName, long lineNum);

}; // class ActiveGraph


// constructor takes necessary references
inline ActiveGraph::ActiveGraph(bool isJoinGraph)
  : Invoke()
  , IsJoinGraph(isJoinGraph)
{ // constructor
  this->NumIterations = 0;
  this->NumSupernodes = 0;
  this->NumHypernodes = 0;
} // constructor



// initialises the active graph
template <class Mesh>
inline void ActiveGraph::Initialise(Mesh& mesh, const MeshExtrema& meshExtrema)
{ // InitialiseActiveGraph()
  // reference to the correct array in the extrema
  const IdArrayType& extrema = this->IsJoinGraph ? meshExtrema.Peaks : meshExtrema.Pits;

  // For every vertex, work out whether it is critical
  // We do so by computing outdegree in the mesh & suppressing the vertex if outdegree is 1
  // All vertices of outdegree 0 must be extrema
  // Saddle points must be at least outdegree 2, so this is a correct test
  // BUT it is possible to overestimate the degree of a non-extremum,
  // The test is therefore necessary but not sufficient, and extra vertices are put in the active graph

  // Neighbourhood mask (one bit set per connected component in neighbourhood
  IdArrayType neighbourhoodMasks;
  neighbourhoodMasks.Allocate(mesh.NumVertices);
  IdArrayType outDegrees; // TODO Should we change this to an unsigned type
  outDegrees.Allocate(mesh.NumVertices);

  // Initialize the nerighborhoodMasks and outDegrees arrays
  mesh.SetPrepareForExecutionBehavior(this->IsJoinGraph);
  viskores::cont::ArrayHandleIndex sortIndexArray(mesh.NumVertices);
  active_graph_inc_ns::InitializeNeighbourhoodMasksAndOutDegrees initNeighMasksAndOutDegWorklet(
    this->IsJoinGraph);

  this->Invoke(initNeighMasksAndOutDegWorklet,
               sortIndexArray,
               mesh,
               neighbourhoodMasks, // output
               outDegrees);        // output

  // next, we compute where each vertex lands in the new array
  // it needs to be one place offset, hence the +/- 1
  // this should automatically parallelise
  // The following commented code block is variant ported directly from PPP2 using std::partial_sum. This has been replaced here with viskores's ScanExclusive.
  /*auto oneIfCritical = [](unsigned x) { return x!= 1 ? 1 : 0; };

    // we need a temporary inverse index to change vertex IDs
    IdArrayType inverseIndex;
    inverseIndex.Allocate(mesh.NumVertices);
    inverseIndex.WritePortal().Set(0,0);

    std::partial_sum(
            boost::make_transform_iterator(viskores::cont::ArrayPortalToIteratorBegin(outDegrees.WritePortal()), oneIfCritical),
            boost::make_transform_iterator(viskores::cont::ArrayPortalToIteratorEnd(outDegrees.WritePortal())-1, oneIfCritical),
            viskores::cont::ArrayPortalToIteratorBegin(inverseIndex.WritePortal()) + 1);
    */
  IdArrayType inverseIndex;
  OneIfCritical oneIfCriticalFunctor;
  auto oneIfCriticalArrayHandle = viskores::cont::ArrayHandleTransform<IdArrayType, OneIfCritical>(
    outDegrees, oneIfCriticalFunctor);
  viskores::cont::Algorithm::ScanExclusive(oneIfCriticalArrayHandle, inverseIndex);

  // now we can compute how many critical points we carry forward
  viskores::Id nCriticalPoints =
    this->GetLastValue(inverseIndex) + oneIfCriticalFunctor(this->GetLastValue(outDegrees));

  // we need to keep track of what the index of each vertex is in the active graph
  // for most vertices, this should have the NO_SUCH_VERTEX flag set
  AllocateVertexArrays(
    nCriticalPoints); // allocates outdegree, GlobalIndex, Hyperarcs, ActiveVertices

  // our processing now depends on the degree of the vertex
  // but basically, we want to set up the arrays for this vertex:
  // activeIndex gets the next available ID in the active graph (was called nearIndex before)
  // GlobalIndex stores the index in the join tree for later access
  IdArrayType activeIndices;
  activeIndices.Allocate(mesh.NumVertices);
  viskores::cont::ArrayHandleConstant<viskores::Id> noSuchElementArray(
    static_cast<viskores::Id>(NO_SUCH_ELEMENT), mesh.NumVertices);
  viskores::cont::Algorithm::Copy(noSuchElementArray, activeIndices);

  active_graph_inc_ns::InitializeActiveGraphVertices initActiveGraphVerticesWorklet;
  this->Invoke(initActiveGraphVerticesWorklet,
               sortIndexArray,
               outDegrees,
               inverseIndex,
               extrema,
               activeIndices,
               this->GlobalIndex,
               this->Outdegree,
               this->Hyperarcs,
               this->ActiveVertices);

  // now we need to compute the FirstEdge array from the outDegrees
  this->FirstEdge.Allocate(nCriticalPoints);
  // STD Version of the prefix sum
  //this->FirstEdge.WritePortal().Set(0, 0);
  //std::partial_sum(viskores::cont::ArrayPortalToIteratorBegin(this->Outdegree.GetPortalControl()),
  //                 viskores::cont::ArrayPortalToIteratorEnd(this->Outdegree.GetPortalControl()) - 1,
  //                 viskores::cont::ArrayPortalToIteratorBegin(this->firstEdge.GetPortalControl()) + 1);
  // VISKORES Version of the prefix sum
  viskores::cont::Algorithm::ScanExclusive(this->Outdegree, this->FirstEdge);
  // Compute the number of critical edges

  viskores::Id nCriticalEdges =
    this->GetLastValue(this->FirstEdge) + this->GetLastValue(this->Outdegree);

  AllocateEdgeArrays(nCriticalEdges);

  active_graph_inc_ns::InitializeActiveEdges<Mesh> initActiveEdgesWorklet;
  this->Invoke(initActiveEdgesWorklet,
               this->Outdegree,
               mesh,
               this->FirstEdge,
               this->GlobalIndex,
               extrema,
               neighbourhoodMasks,
               this->EdgeNear,
               this->EdgeFar,
               this->ActiveEdges);

  // now we have to go through and set the far ends of the new edges using the
  // inverse index array
  active_graph_inc_ns::InitializeEdgeFarFromActiveIndices initEdgeFarWorklet;
  this->Invoke(initEdgeFarWorklet, this->EdgeFar, extrema, activeIndices);

  DebugPrint("Active Graph Started", __FILE__, __LINE__);

  // then we loop through the active vertices to convert their indices to active graph indices
  active_graph_inc_ns::InitializeHyperarcsFromActiveIndices initHyperarcsWorklet;
  this->Invoke(initHyperarcsWorklet, this->Hyperarcs, activeIndices);

  // finally, allocate and initialise the edgeSorter array
  this->EdgeSorter.Allocate(this->ActiveEdges.GetNumberOfValues());
  viskores::cont::Algorithm::Copy(this->ActiveEdges, this->EdgeSorter);

  //DebugPrint("Active Graph Initialised", __FILE__, __LINE__);
} // InitialiseActiveGraph()


// routine that computes the merge tree from the active graph
// was previously Compute()
inline void ActiveGraph::MakeMergeTree(MergeTree& tree, MeshExtrema& meshExtrema)
{ // MakeMergeTree()
  DebugPrint("Active Graph Computation Starting", __FILE__, __LINE__);

  // loop until we run out of active edges
  viskores::Id maxNumIterations = this->EdgeSorter.GetNumberOfValues();
  this->NumIterations = 0;
  while (true)
  { // main loop
    // choose the subset of edges for the governing saddles
    TransferSaddleStarts();

    // test whether there are any left (if not, we're on the trunk)
    if (this->EdgeSorter.GetNumberOfValues() <= 0)
    {
      break;
    }
    // test whether we are in a bad infinite loop due to bad input data. Usually
    // this is not an issue for the merge tree (only for the contour tree), but
    // we check just to make absolutely sure we won't get stuck in an infinite loop
    if (this->NumIterations >= maxNumIterations)
    {
      throw new viskores::cont::ErrorInternal(
        "Bad iteration. Merge tree unable to process all edges.");
    }

    // find & label the extrema with their governing saddles
    FindGoverningSaddles();

    // label the regular points
    TransferRegularPoints();

    // compact the active set of vertices & edges
    CompactActiveVertices();
    CompactActiveEdges();

    // rebuild the chains
    BuildChains();

    // increment the iteration count
    this->NumIterations++;
  } // main loop

  // final pass to label the trunk vertices
  BuildTrunk();

  // transfer results to merge tree
  FindSuperAndHyperNodes(tree);
  SetSuperArcs(tree);
  SetHyperArcs(tree);
  SetArcs(tree, meshExtrema);

  // we can now release many of the arrays to free up space
  ReleaseTemporaryArrays();

  DebugPrint("Merge Tree Computed", __FILE__, __LINE__);
} // MakeMergeTree()


// suppresses non-saddles for the governing saddles pass
inline void ActiveGraph::TransferSaddleStarts()
{ // TransferSaddleStarts()
  // update all of the edges so that the far end resets to the result of the ascent in the previous step

  active_graph_inc_ns::TransferSaddleStartsResetEdgeFar transferSaddleResetWorklet;
  this->Invoke(transferSaddleResetWorklet, this->ActiveEdges, this->Hyperarcs, this->EdgeFar);

  // in parallel, we need to create a vector to count the first edge for each vertex
  IdArrayType newOutdegree;
  newOutdegree.Allocate(this->ActiveVertices.GetNumberOfValues());

  // this will be a stream compaction later, but for now we'll do it the serial way
  active_graph_inc_ns::TransferSaddleStartsSetNewOutdegreeForSaddles transferOutDegree;
  this->Invoke(transferOutDegree,
               this->ActiveVertices,
               this->FirstEdge,
               this->Outdegree,
               this->ActiveEdges,
               this->Hyperarcs,
               this->EdgeFar,
               newOutdegree);

  // now do a parallel prefix sum using the offset partial sum trick.
  IdArrayType newFirstEdge;
  newFirstEdge.Allocate(this->ActiveVertices.GetNumberOfValues());
  // STD version of the prefix sum
  // newFirstEdge.WritePortal().Set(0, 0);
  // std::partial_sum(viskores::cont::ArrayPortalToIteratorBegin(newOutdegree.WritePortal()),
  //                 viskores::cont::ArrayPortalToIteratorEnd(newOutdegree.WritePortal()) - 1,
  //                 viskores::cont::ArrayPortalToIteratorBegin(newFirstEdge.WritePortal()) + 1);
  // VTK:M version of the prefix sum
  viskores::cont::Algorithm::ScanExclusive(newOutdegree, newFirstEdge);

  viskores::Id nEdgesToSort = this->GetLastValue(newFirstEdge) + this->GetLastValue(newOutdegree);

  // now we write only the active saddle edges to the sorting array
  this->EdgeSorter
    .ReleaseResources(); // TODO is there a single way to resize an array handle without calling ReleaseResources followed by Allocate
  this->EdgeSorter.Allocate(nEdgesToSort);

  // this will be a stream compaction later, but for now we'll do it the serial way
  active_graph_inc_ns::TransferSaddleStartsUpdateEdgeSorter updateEdgeSorterWorklet;
  this->Invoke(updateEdgeSorterWorklet,
               this->ActiveVertices,
               this->ActiveEdges,
               this->FirstEdge,
               newFirstEdge,
               newOutdegree,
               this->EdgeSorter);

  DebugPrint("Saddle Starts Transferred", __FILE__, __LINE__);
} // TransferSaddleStarts()


// sorts saddle starts to find governing saddles
inline void ActiveGraph::FindGoverningSaddles()
{ // FindGoverningSaddles()
  // sort with the comparator
  viskores::cont::Algorithm::Sort(
    this->EdgeSorter,
    active_graph_inc_ns::EdgePeakComparator(this->EdgeFar, this->EdgeNear, this->IsJoinGraph));

  // DebugPrint("After Sorting", __FILE__, __LINE__);

  // now loop through the edges to find the governing saddles
  active_graph_inc_ns::FindGoverningSaddlesWorklet findGovSaddlesWorklet;
  viskores::cont::ArrayHandleIndex edgeIndexArray(this->EdgeSorter.GetNumberOfValues());

  this->Invoke(findGovSaddlesWorklet,
               edgeIndexArray,
               this->EdgeSorter,
               this->EdgeFar,
               this->EdgeNear,
               this->Hyperarcs,
               this->Outdegree);

  DebugPrint("Governing Saddles Set", __FILE__, __LINE__);
} // FindGoverningSaddles()


// marks now regular points for removal
inline void ActiveGraph::TransferRegularPoints()
{ // TransferRegularPointsWorklet
  // we need to label the regular points that have been identified
  active_graph_inc_ns::TransferRegularPointsWorklet transRegPtWorklet(this->IsJoinGraph);
  this->Invoke(transRegPtWorklet, this->ActiveVertices, this->Hyperarcs, this->Outdegree);

  DebugPrint("Regular Points Should Now Be Labelled", __FILE__, __LINE__);
} // TransferRegularPointsWorklet()


// compacts the active vertex list
inline void ActiveGraph::CompactActiveVertices()
{ // CompactActiveVertices()
  using PermuteIndexType = viskores::cont::ArrayHandlePermutation<IdArrayType, IdArrayType>;

  // create a temporary array the same size
  viskores::cont::ArrayHandle<viskores::Id> newActiveVertices;

  // Use only the current this->ActiveVertices this->Outdegree to match size on CopyIf
  viskores::cont::ArrayHandle<viskores::Id> outdegreeLookup;
  viskores::cont::Algorithm::Copy(PermuteIndexType(this->ActiveVertices, this->Outdegree),
                                  outdegreeLookup);

  // compact the this->ActiveVertices array to keep only the ones of interest
  viskores::cont::Algorithm::CopyIf(this->ActiveVertices, outdegreeLookup, newActiveVertices);

  this->ActiveVertices.ReleaseResources();
  viskores::cont::Algorithm::Copy(newActiveVertices, this->ActiveVertices);

  DebugPrint("Active Vertex List Compacted", __FILE__, __LINE__);
} // CompactActiveVertices()


// compacts the active edge list
inline void ActiveGraph::CompactActiveEdges()
{ // CompactActiveEdges()
  // grab the size of the array for easier reference
  viskores::Id nActiveVertices = this->ActiveVertices.GetNumberOfValues();

  // first, we have to work out the first edge for each active vertex
  // we start with a temporary new outdegree
  IdArrayType newOutdegree;
  newOutdegree.Allocate(nActiveVertices);

  // Run workflet to compute newOutdegree for each vertex
  active_graph_inc_ns::CompactActiveEdgesComputeNewVertexOutdegree computeNewOutdegreeWorklet;
  this->Invoke(computeNewOutdegreeWorklet,
               this->ActiveVertices, // (input)
               this->ActiveEdges,    // (input)
               this->EdgeFar,        // (input)
               this->FirstEdge,      // (input)
               this->Outdegree,      // (input)
               this->Hyperarcs,      // (input/output)
               newOutdegree          // (output)
  );

  // now we do a reduction to compute the offsets of each vertex
  viskores::cont::ArrayHandle<viskores::Id> newPosition;
  // newPosition.Allocate(nActiveVertices);   // Not necessary. ScanExclusive takes care of this.
  viskores::cont::Algorithm::ScanExclusive(newOutdegree, newPosition);

  viskores::Id nNewEdges = viskores::cont::ArrayGetValue(nActiveVertices - 1, newPosition) +
    viskores::cont::ArrayGetValue(nActiveVertices - 1, newOutdegree);

  // create a temporary vector for copying
  IdArrayType newActiveEdges;
  newActiveEdges.Allocate(nNewEdges);
  // overwriting Hyperarcs in parallel is safe, as the worst than can happen is
  // that another valid ascent is found; for comparison and validation purposes
  // however it makes sense to have a `canoical' computation. To achieve this
  // canonical computation, we need to write into a new array during computation
  // ensuring that we always use the same information. The following is left in
  // commented out for future debugging and validation

  //DebugPrint("Active Edges Counted", __FILE__, __LINE__);

  // now copy the relevant edges into the active edge array
  active_graph_inc_ns::CompactActiveEdgesTransferActiveEdges transferActiveEdgesWorklet;
  this->Invoke(transferActiveEdgesWorklet,
               this->ActiveVertices,
               newPosition,       // (input)
               newOutdegree,      // (input)
               this->ActiveEdges, // (input)
               newActiveEdges,    // (output)
               this->EdgeFar,     // (input/output)
               this->FirstEdge,   // (input/output)
               this->Outdegree,   // (input/output)
               this->Hyperarcs    // (input/output)
  );

  // resize the original array and recopy
  //viskores::cont::Algorithm::::Copy(newActiveEdges, this-ActiveEdges);
  this->ActiveEdges.ReleaseResources();
  // viskores ArrayHandles are smart, so we can just swap it in without having to copy
  this->ActiveEdges = newActiveEdges;

  // for canonical computation: swap in newly computed hyperarc array
  //      this->Hyperarcs.swap(newHyperarcs);

  DebugPrint("Active Edges Now Compacted", __FILE__, __LINE__);
} // CompactActiveEdges()



// builds the chains for the new active vertices
inline void ActiveGraph::BuildChains()
{ // BuildChains()
  // 1. compute the number of log steps required in this pass
  viskores::Id numLogSteps = 1;
  for (viskores::Id shifter = this->ActiveVertices.GetNumberOfValues(); shifter != 0; shifter >>= 1)
    numLogSteps++;

  // 2.   Use path compression / step doubling to collect vertices along chains
  //              until every vertex has been assigned to *an* extremum
  for (viskores::Id logStep = 0; logStep < numLogSteps; logStep++)
  { // per log step
    active_graph_inc_ns::BuildChainsWorklet buildChainsWorklet;
    this->Invoke(buildChainsWorklet, this->ActiveVertices, this->Hyperarcs);
  } // per log step
  DebugPrint("Chains Built", __FILE__, __LINE__);
} // BuildChains()


// sets all remaining active vertices
inline void ActiveGraph::BuildTrunk()
{ //BuildTrunk
  // all remaining vertices belong to the trunk
  active_graph_inc_ns::BuildTrunkWorklet buildTrunkWorklet;
  this->Invoke(buildTrunkWorklet, this->ActiveVertices, this->Hyperarcs);

  DebugPrint("Trunk Built", __FILE__, __LINE__);
} //BuildTrunk


// finds all super and hyper nodes, numbers them & sets up arrays for lookup
inline void ActiveGraph::FindSuperAndHyperNodes(MergeTree& tree)
{ // FindSuperAndHyperNodes()
  // allocate memory for nodes
  this->HyperID.ReleaseResources();
  this->HyperID.Allocate(this->GlobalIndex.GetNumberOfValues());

  // compute new node positions
  // The following commented code block is variant ported directly from PPP2 using std::partial_sum. This has been replaced here with viskores's ScanExclusive.
  /*auto oneIfSupernode = [](viskores::Id v) { return IsSupernode(v) ? 1 : 0; };
    IdArrayType newSupernodePosition;
    newSupernodePosition.Allocate(this->Hyperarcs.GetNumberOfValues());
    newSupernodePosition.WritePortal().Set(0, 0);

    std::partial_sum(
            boost::make_transform_iterator(viskores::cont::ArrayPortalToIteratorBegin(this->Hyperarcs.WritePortal()), oneIfSupernode),
            boost::make_transform_iterator(viskores::cont::ArrayPortalToIteratorEnd(this->Hyperarcs.WritePortal()) - 1, oneIfSupernode),
            viskores::cont::ArrayPortalToIteratorBegin(newSupernodePosition.GetPortalControl()) + 1);*/

  IdArrayType newSupernodePosition;
  OneIfSupernode oneIfSupernodeFunctor;
  auto oneIfSupernodeArrayHandle =
    viskores::cont::ArrayHandleTransform<IdArrayType, OneIfSupernode>(this->Hyperarcs,
                                                                      oneIfSupernodeFunctor);
  viskores::cont::Algorithm::ScanExclusive(oneIfSupernodeArrayHandle, newSupernodePosition);

  this->NumSupernodes = this->GetLastValue(newSupernodePosition) +
    oneIfSupernodeFunctor(this->GetLastValue(this->Hyperarcs));

  tree.Supernodes.ReleaseResources();
  tree.Supernodes.Allocate(this->NumSupernodes);

  // The following commented code block is variant ported directly from PPP2 using std::partial_sum. This has been replaced here with viskores's ScanExclusive.
  /*
    auto oneIfHypernode = [](viskores::Id v) { return IsHypernode(v) ? 1 : 0; };
    IdArrayType newHypernodePosition;
    newHypernodePosition.Allocate(this->Hyperarcs.GetNumberOfValues());
    newHypernodePosition.WritePortal().Set(0, 0);
    std::partial_sum(
            boost::make_transform_iterator(viskores::cont::ArrayPortalToIteratorBegin(this->Hyperarcs.WritePortal()), oneIfHypernode),
            boost::make_transform_iterator(viskores::cont::ArrayPortalToIteratorEnd(this->Hyperarcs.WritePortal()) - 1, oneIfHypernode),
             viskores::cont::ArrayPortalToIteratorBegin(newHypernodePosition.GetPortalControl()) + 1);
    */
  IdArrayType newHypernodePosition;
  OneIfHypernode oneIfHypernodeFunctor;
  auto oneIfHypernodeArrayHandle =
    viskores::cont::ArrayHandleTransform<IdArrayType, OneIfHypernode>(this->Hyperarcs,
                                                                      oneIfHypernodeFunctor);
  viskores::cont::Algorithm::ScanExclusive(oneIfHypernodeArrayHandle, newHypernodePosition);

  this->NumHypernodes = this->GetLastValue(newHypernodePosition) +
    oneIfHypernodeFunctor(this->GetLastValue(this->Hyperarcs));

  tree.Hypernodes.ReleaseResources();
  tree.Hypernodes.Allocate(this->GlobalIndex.GetNumberOfValues());

  // perform stream compression
  active_graph_inc_ns::FindSuperAndHyperNodesWorklet findSuperAndHyperNodesWorklet;
  viskores::cont::ArrayHandleIndex graphVertexIndex(this->GlobalIndex.GetNumberOfValues());
  this->Invoke(findSuperAndHyperNodesWorklet,
               graphVertexIndex,
               this->Hyperarcs,
               newHypernodePosition,
               newSupernodePosition,
               this->HyperID,
               tree.Hypernodes,
               tree.Supernodes);

  DebugPrint("Super/Hypernodes Found", __FILE__, __LINE__);
  tree.DebugPrint("Super/Hypernodes Found", __FILE__, __LINE__);
} // FindSuperAndHyperNodes()


// uses active graph to set superarcs & hyperparents in merge tree
inline void ActiveGraph::SetSuperArcs(MergeTree& tree)
{ // SetSuperArcs()
  using PermutedIdArrayType = viskores::cont::ArrayHandlePermutation<IdArrayType, IdArrayType>;

  //      1.      set the hyperparents
  // allocate space for the hyperparents
  tree.Hyperparents.ReleaseResources();
  tree.Hyperparents.Allocate(this->NumSupernodes);

  // execute the worklet to set the hyperparents
  active_graph_inc_ns::SetSuperArcsSetTreeHyperparents setTreeHyperparentsWorklet;
  this->Invoke(setTreeHyperparentsWorklet, tree.Supernodes, this->Hyperarcs, tree.Hyperparents);

  tree.DebugPrint("Hyperparents Set", __FILE__, __LINE__);
  //      a.      And the super ID array needs setting up
  this->SuperID.ReleaseResources();
  viskores::cont::Algorithm::Copy(viskores::cont::make_ArrayHandleConstant(
                                    NO_SUCH_ELEMENT, this->GlobalIndex.GetNumberOfValues()),
                                  this->SuperID);
  viskores::cont::ArrayHandleIndex supernodeIndex(this->NumSupernodes);
  PermutedIdArrayType permutedSuperID(tree.Supernodes, this->SuperID);
  viskores::cont::Algorithm::Copy(supernodeIndex, permutedSuperID);

  //      2.      Sort the supernodes into segments according to hyperparent
  //              See comparator for details
  viskores::cont::Algorithm::Sort(tree.Supernodes,
                                  active_graph_inc_ns::HyperArcSuperNodeComparator(
                                    tree.Hyperparents, this->SuperID, tree.IsJoinTree));

  //      3.      Now update the other arrays to match
  IdArrayType hyperParentsTemp;
  hyperParentsTemp.Allocate(this->NumSupernodes);
  auto permutedTreeHyperparents = viskores::cont::make_ArrayHandlePermutation(
    viskores::cont::make_ArrayHandlePermutation(tree.Supernodes, this->SuperID), tree.Hyperparents);

  viskores::cont::Algorithm::Copy(permutedTreeHyperparents, hyperParentsTemp);
  viskores::cont::Algorithm::Copy(hyperParentsTemp, tree.Hyperparents);
  hyperParentsTemp.ReleaseResources();
  //      a.      And the super ID array needs setting up // TODO Check if we really need this?
  viskores::cont::Algorithm::Copy(supernodeIndex, permutedSuperID);

  DebugPrint("Supernodes Sorted", __FILE__, __LINE__);
  tree.DebugPrint("Supernodes Sorted", __FILE__, __LINE__);

  //      4.      Allocate memory for superarcs
  tree.Superarcs.ReleaseResources();
  tree.Superarcs.Allocate(this->NumSupernodes);
  tree.FirstSuperchild.ReleaseResources();
  tree.FirstSuperchild.Allocate(this->NumHypernodes);

  //      5.      Each supernode points to its neighbour in the list, except at the end of segments
  // execute the worklet to set the tree.Hyperparents and tree.FirstSuperchild
  active_graph_inc_ns::SetSuperArcsSetTreeSuperarcs setTreeSuperarcsWorklet;
  this->Invoke(setTreeSuperarcsWorklet,
               tree.Supernodes,     // (input)
               this->Hyperarcs,     // (input)
               tree.Hyperparents,   // (input)
               this->SuperID,       // (input)
               this->HyperID,       // (input)
               tree.Superarcs,      // (output)
               tree.FirstSuperchild // (output)
  );

  // 6.   Now we can reset the supernodes to mesh IDs
  PermutedIdArrayType permuteGlobalIndex(tree.Supernodes, this->GlobalIndex);
  viskores::cont::Algorithm::Copy(permuteGlobalIndex, tree.Supernodes);

  // 7.   and the hyperparent to point to a hyperarc rather than a graph index
  PermutedIdArrayType permuteHyperID(tree.Hyperparents, this->HyperID);
  viskores::cont::Algorithm::Copy(permuteHyperID, tree.Hyperparents);

  tree.DebugPrint("Superarcs Set", __FILE__, __LINE__);
} // SetSuperArcs()


// uses active graph to set hypernodes in merge tree
inline void ActiveGraph::SetHyperArcs(MergeTree& tree)
{ // SetHyperArcs()
  //      1.      Allocate memory for hypertree
  tree.Hypernodes.Allocate(
    this->NumHypernodes,     // Has been allocated previously.
    viskores::CopyFlag::On); // The values are needed but the size may be too large.
  tree.Hyperarcs.ReleaseResources();
  tree.Hyperarcs.Allocate(this->NumHypernodes); // Has not been allocated yet

  //      2.      Use the superIDs already set to fill in the Hyperarcs array
  active_graph_inc_ns::SetHyperArcsWorklet setHyperArcsWorklet;
  this->Invoke(
    setHyperArcsWorklet, tree.Hypernodes, tree.Hyperarcs, this->Hyperarcs, this->SuperID);

  // Debug output
  DebugPrint("Hyperarcs Set", __FILE__, __LINE__);
  tree.DebugPrint("Hyperarcs Set", __FILE__, __LINE__);
} // SetHyperArcs()


// uses active graph to set arcs in merge tree
inline void ActiveGraph::SetArcs(MergeTree& tree, MeshExtrema& meshExtrema)
{ // SetArcs()
  using PermuteIndexType = viskores::cont::ArrayHandlePermutation<IdArrayType, IdArrayType>;

  // reference to the correct array in the extrema
  const IdArrayType& extrema = this->IsJoinGraph ? meshExtrema.Peaks : meshExtrema.Pits;

  // 1.   Set the arcs for the super/hypernodes based on where they prune to
  active_graph_inc_ns::SetArcsSetSuperAndHypernodeArcs setSuperAndHypernodeArcsWorklet;
  this->Invoke(setSuperAndHypernodeArcsWorklet,
               this->GlobalIndex,
               this->Hyperarcs,
               this->HyperID,
               tree.Arcs,
               tree.Superparents);

  DebugPrint("Sliding Arcs Set", __FILE__, __LINE__);
  tree.DebugPrint("Sliding Arcs Set", __FILE__, __LINE__);

  // 2.   Loop through all vertices to slide down Hyperarcs
  active_graph_inc_ns::SetArcsSlideVertices slideVerticesWorklet(
    this->IsJoinGraph, this->NumSupernodes, this->NumHypernodes);
  this->Invoke(slideVerticesWorklet,
               tree.Arcs,            // (input)
               extrema,              // (input)  i.e,. meshExtrema.Peaks or meshExtrema.Pits
               tree.FirstSuperchild, // (input)
               tree.Supernodes,      // (input)
               tree.Superparents);   // (input/output)

  tree.DebugPrint("Sliding Finished", __FILE__, __LINE__);

  // 3.   Now set the superparents correctly for the supernodes
  PermuteIndexType permuteTreeSuperparents(tree.Supernodes, tree.Superparents);
  viskores::cont::ArrayHandleIndex supernodesIndex(this->NumSupernodes);
  viskores::cont::Algorithm::Copy(supernodesIndex, permuteTreeSuperparents);

  tree.DebugPrint("Superparents Set", __FILE__, __LINE__);

  // 4.   Finally, sort all of the vertices onto their superarcs
  IdArrayType nodes;
  viskores::cont::ArrayHandleIndex nodesIndex(tree.Arcs.GetNumberOfValues());
  viskores::cont::Algorithm::Copy(nodesIndex, nodes);

  //  5.  Sort the nodes into segments according to superparent
  //      See comparator for details
  viskores::cont::Algorithm::Sort(
    nodes, active_graph_inc_ns::SuperArcNodeComparator(tree.Superparents, tree.IsJoinTree));

  //  6. Connect the nodes to each other
  active_graph_inc_ns::SetArcsConnectNodes connectNodesWorklet;
  this->Invoke(connectNodesWorklet,
               tree.Arcs,         // (input/output)
               nodes,             // (input)
               tree.Superparents, // (input)
               tree.Superarcs,    // (input)
               tree.Supernodes);  // (input)

  tree.DebugPrint("Arcs Set", __FILE__, __LINE__);
} // SetArcs()


// Allocate the vertex array
inline void ActiveGraph::AllocateVertexArrays(viskores::Id nElems)
{
  this->GlobalIndex.Allocate(nElems);
  this->Outdegree.Allocate(nElems);
  this->Hyperarcs.Allocate(nElems);
  this->ActiveVertices.Allocate(nElems);
}


// Allocate the edge array
inline void ActiveGraph::AllocateEdgeArrays(viskores::Id nElems)
{
  this->ActiveEdges.Allocate(nElems);
  this->EdgeNear.Allocate(nElems);
  this->EdgeFar.Allocate(nElems);
}


// releases temporary arrays
inline void ActiveGraph::ReleaseTemporaryArrays()
{
  this->GlobalIndex.ReleaseResources();
  this->FirstEdge.ReleaseResources();
  this->Outdegree.ReleaseResources();
  this->EdgeNear.ReleaseResources();
  this->EdgeFar.ReleaseResources();
  this->ActiveEdges.ReleaseResources();
  this->ActiveVertices.ReleaseResources();
  this->EdgeSorter.ReleaseResources();
  this->Hyperarcs.ReleaseResources();
  this->HyperID.ReleaseResources();
  this->SuperID.ReleaseResources();
}


// prints the contents of the active graph in a standard format
inline void ActiveGraph::DebugPrint(const char* message, const char* fileName, long lineNum)
{ // DebugPrint()
#ifdef DEBUG_PRINT
  std::cout << "------------------------------------------------------" << std::endl;
  std::cout << std::setw(30) << std::left << fileName << ":" << std::right << std::setw(4)
            << lineNum << std::endl;
  std::cout << std::left << std::string(message) << std::endl;
  std::cout << "Active Graph Contains:                                " << std::endl;
  std::cout << "------------------------------------------------------" << std::endl;

  std::cout << "Is Join Graph? " << (this->IsJoinGraph ? "T" : "F") << std::endl;
  std::cout << "NumIterations    " << this->NumIterations << std::endl;
  std::cout << "nSupernodes    " << this->NumSupernodes << std::endl;
  std::cout << "nHypernodes    " << this->NumHypernodes << std::endl;

  // Full Vertex Arrays
  std::cout << "Full Vertex Arrays - Size:  " << this->GlobalIndex.GetNumberOfValues() << std::endl;
  PrintHeader(this->GlobalIndex.GetNumberOfValues());
  PrintIndices("Global Index", this->GlobalIndex);
  PrintIndices("First Edge", this->FirstEdge);
  PrintIndices("Outdegree", this->Outdegree);
  PrintIndices("Hyperarc ID", this->Hyperarcs);
  PrintIndices("Hypernode ID", this->HyperID);
  PrintIndices("Supernode ID", this->SuperID);
  std::cout << std::endl;

  // Active Vertex Arrays
  IdArrayType activeIndices;
  PermuteArrayWithMaskedIndex<viskores::Id>(this->GlobalIndex, this->ActiveVertices, activeIndices);
  IdArrayType activeFirst;
  PermuteArrayWithMaskedIndex<viskores::Id>(this->FirstEdge, this->ActiveVertices, activeFirst);
  IdArrayType activeOutdegree;
  PermuteArrayWithMaskedIndex<viskores::Id>(this->Outdegree, this->ActiveVertices, activeOutdegree);
  IdArrayType activeHyperarcs;
  PermuteArrayWithMaskedIndex<viskores::Id>(this->Hyperarcs, this->ActiveVertices, activeHyperarcs);
  std::cout << "Active Vertex Arrays - Size: " << this->ActiveVertices.GetNumberOfValues()
            << std::endl;
  PrintHeader(this->ActiveVertices.GetNumberOfValues());
  PrintIndices("Active Vertices", this->ActiveVertices);
  PrintIndices("Active Indices", activeIndices);
  PrintIndices("Active First Edge", activeFirst);
  PrintIndices("Active Outdegree", activeOutdegree);
  PrintIndices("Active Hyperarc ID", activeHyperarcs);
  std::cout << std::endl;

  // Full Edge Arrays
  IdArrayType farIndices;
  PermuteArrayWithMaskedIndex<viskores::Id>(this->GlobalIndex, this->EdgeFar, farIndices);
  IdArrayType nearIndices;
  PermuteArrayWithMaskedIndex<viskores::Id>(this->GlobalIndex, this->EdgeNear, nearIndices);
  std::cout << "Full Edge Arrays - Size:     " << this->EdgeNear.GetNumberOfValues() << std::endl;
  PrintHeader(this->EdgeFar.GetNumberOfValues());
  PrintIndices("Near", this->EdgeNear);
  PrintIndices("Far", this->EdgeFar);
  PrintIndices("Near Index", nearIndices);
  PrintIndices("Far Index", farIndices);
  std::cout << std::endl;

  // Active Edge Arrays
  IdArrayType activeFarIndices;
  PermuteArrayWithMaskedIndex<viskores::Id>(this->EdgeFar, this->ActiveEdges, activeFarIndices);
  IdArrayType activeNearIndices;
  PermuteArrayWithMaskedIndex<viskores::Id>(this->EdgeNear, this->ActiveEdges, activeNearIndices);
  std::cout << "Active Edge Arrays - Size:   " << this->ActiveEdges.GetNumberOfValues()
            << std::endl;
  PrintHeader(this->ActiveEdges.GetNumberOfValues());
  PrintIndices("Active Edges", this->ActiveEdges);
  PrintIndices("Edge Near Index", activeNearIndices);
  PrintIndices("Edge Far Index", activeFarIndices);
  std::cout << std::endl;

  // Edge Sorter Array
  IdArrayType sortedFarIndices;
  PermuteArrayWithMaskedIndex<viskores::Id>(this->EdgeFar, this->EdgeSorter, sortedFarIndices);
  IdArrayType sortedNearIndices;
  PermuteArrayWithMaskedIndex<viskores::Id>(this->EdgeNear, this->EdgeSorter, sortedNearIndices);
  std::cout << "Edge Sorter - Size:          " << this->EdgeSorter.GetNumberOfValues() << std::endl;
  PrintHeader(this->EdgeSorter.GetNumberOfValues());
  PrintIndices("Edge Sorter", this->EdgeSorter);
  PrintIndices("Sorted Near Index", sortedNearIndices);
  PrintIndices("Sorted Far Index", sortedFarIndices);
  std::cout << std::endl;

  std::cout << "---------------------------" << std::endl;
  std::cout << std::endl;
#else
  // Prevent unused parameter warning
  (void)message;
  (void)fileName;
  (void)lineNum;
#endif
} // DebugPrint()



} // namespace contourtree_augmented
} // worklet
} // viskores

#endif
