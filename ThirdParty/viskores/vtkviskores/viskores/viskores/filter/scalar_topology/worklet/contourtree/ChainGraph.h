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
//  Copyright (c) 2016, Los Alamos National Security, LLC
//  All rights reserved.
//
//  Copyright 2016. Los Alamos National Security, LLC.
//  This software was produced under U.S. Government contract DE-AC52-06NA25396
//  for Los Alamos National Laboratory (LANL), which is operated by
//  Los Alamos National Security, LLC for the U.S. Department of Energy.
//  The U.S. Government has rights to use, reproduce, and distribute this
//  software.  NEITHER THE GOVERNMENT NOR LOS ALAMOS NATIONAL SECURITY, LLC
//  MAKES ANY WARRANTY, EXPRESS OR IMPLIED, OR ASSUMES ANY LIABILITY FOR THE
//  USE OF THIS SOFTWARE.  If software is modified to produce derivative works,
//  such modified software should be clearly marked, so as not to confuse it
//  with the version available from LANL.
//
//  Additionally, redistribution and use in source and binary forms, with or
//  without modification, are permitted provided that the following conditions
//  are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//  3. Neither the name of Los Alamos National Security, LLC, Los Alamos
//     National Laboratory, LANL, the U.S. Government, nor the names of its
//     contributors may be used to endorse or promote products derived from
//     this software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY LOS ALAMOS NATIONAL SECURITY, LLC AND
//  CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
//  BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
//  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL LOS ALAMOS
//  NATIONAL SECURITY, LLC OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
//  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
//  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
//  USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
//  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//============================================================================

//  This code is based on the algorithm presented in the paper:
//  “Parallel Peak Pruning for Scalable SMP Contour Tree Computation.”
//  Hamish Carr, Gunther Weber, Christopher Sewell, and James Ahrens.
//  Proceedings of the IEEE Symposium on Large Data Analysis and Visualization
//  (LDAV), October 2016, Baltimore, Maryland.

//=======================================================================================
//
// COMMENTS:
//
//  The old ChainGraph has been abstracted a little further - it still does the same job
//  of carrying most of the intermediate stages.  However, since the chain building is
//  also needed by the mesh to set up the initial graph input, it has been moved (for now
//  to Types.h)
//
//	There will be no explicit constructor - instead, it's the mesh's job to initialise
//	a valid object of this type
//
//=======================================================================================

#ifndef viskores_worklet_contourtree_chaingraph_h
#define viskores_worklet_contourtree_chaingraph_h

#include <viskores/filter/scalar_topology/worklet/contourtree/ActiveEdgeTransferrer.h>
#include <viskores/filter/scalar_topology/worklet/contourtree/ChainDoubler.h>
#include <viskores/filter/scalar_topology/worklet/contourtree/EdgePeakComparator.h>
#include <viskores/filter/scalar_topology/worklet/contourtree/GoverningSaddleFinder.h>
#include <viskores/filter/scalar_topology/worklet/contourtree/JoinTreeTransferrer.h>
#include <viskores/filter/scalar_topology/worklet/contourtree/PrintVectors.h>
#include <viskores/filter/scalar_topology/worklet/contourtree/RegularPointTransferrer.h>
#include <viskores/filter/scalar_topology/worklet/contourtree/SaddleAscentFunctor.h>
#include <viskores/filter/scalar_topology/worklet/contourtree/SaddleAscentTransferrer.h>
#include <viskores/filter/scalar_topology/worklet/contourtree/TrunkBuilder.h>
#include <viskores/filter/scalar_topology/worklet/contourtree/VertexDegreeUpdater.h>

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayGetValues.h>
#include <viskores/cont/ArrayHandlePermutation.h>
#include <viskores/worklet/DispatcherMapField.h>

//#define DEBUG_PRINT 1
//#define DEBUG_FUNCTION_ENTRY 1
//#define DEBUG_TIMING 1

namespace viskores
{
namespace worklet
{
namespace contourtree
{

#define DEBUG_STRING_TRANSFER_GOVERNING_SADDLES "Extrema should now be assigned"
#define DEBUG_STRING_TRANSFER_SADDLE_STARTS "Transfer Saddle Starts "
#define DEBUG_STRING_TRANSFERRED_SADDLE_STARTS "Saddle Starts Transferred"
#define DEBUG_STRING_TRANSFER_TO_MERGE_TREE "Transfer to Merge Tree"
#define DEBUG_STRING_OUTDEGREE "Outdegree"
#define DEBUG_STRING_CHAINEXT "Chain Ext"
#define DEBUG_STRING_ACTIVE_OUTDEGREE "Active Outdegree"
#define DEBUG_STRING_ACTIVE_CHAINEXT "Active Chain Ext"
#define DEBUG_STRING_FAR_ID "Far"
#define DEBUG_STRING_FAR_INDEX "Far Index"
#define DEBUG_STRING_FAR_VALUE "Far Value"
#define DEBUG_STRING_NEAR_ID "Near"
#define DEBUG_STRING_NEAR_INDEX "Near Index"
#define DEBUG_STRING_NEAR_VALUE "Near Value"
#define DEBUG_STRING_EDGE_FAR_ID "Edge Far"
#define DEBUG_STRING_EDGE_NEAR_ID "Edge Near"
#define DEBUG_STRING_EDGE_NEAR_INDEX "Edge Near Index"
#define DEBUG_STRING_EDGE_NEAR_VALUE "Edge Near Value"
#define DEBUG_STRING_SORTED_NEAR_ID "Sorted Near"
#define DEBUG_STRING_SORTED_NEAR_INDEX "Sorted Near Index"
#define DEBUG_STRING_SORTED_NEAR_VALUE "Sorted Near Value"
#define DEBUG_STRING_SORTED_FAR_ID "Sorted Far"

template <typename T, typename StorageType>
class ChainGraph
{
public:
  // we will want a reference to the original data array
  const viskores::cont::ArrayHandle<T, StorageType>& values;

  // we will also want a reference to the arc array where we write the output
  viskores::cont::ArrayHandle<viskores::Id>& arcArray;

  // for each vertex, we need to know where it is in the original data array
  viskores::cont::ArrayHandle<viskores::Id> valueIndex;

  // and we also need the orientation of the edges (i.e. is it join or split)
  bool isJoinGraph;

  // and we will store the number of iterations the computation took here
  viskores::Id nIterations;

  // array recording pruning sequence
  // pseudo-extrema prune to pseudo-saddles
  // all others prune to pseudo-extrema
  viskores::cont::ArrayHandle<viskores::Id> prunesTo;

  // we also want to keep track of the first edge for each vertex
  viskores::cont::ArrayHandle<viskores::Id> firstEdge;

  // and the outdegree for each vertex
  viskores::cont::ArrayHandle<viskores::Id> outdegree;

  // finally, we need to keep track of the chain extremum for each vertex
  viskores::cont::ArrayHandle<viskores::Id> chainExtremum;

  // we will also need to keep track of both near and far ends of each edge
  viskores::cont::ArrayHandle<viskores::Id> edgeFar;
  viskores::cont::ArrayHandle<viskores::Id> edgeNear;

  // we will also keep track of the currently active set of vertices and edges
  viskores::cont::ArrayHandle<viskores::Id> activeVertices;
  viskores::cont::ArrayHandle<viskores::Id> activeEdges;

  // and an array for sorting edges
  viskores::cont::ArrayHandle<viskores::Id> edgeSorter;

  // constructor takes necessary references
  ChainGraph(const viskores::cont::ArrayHandle<T, StorageType>& Values,
             viskores::cont::ArrayHandle<viskores::Id>& ArcArray,
             bool IsJoinGraph)
    : values(Values)
    , arcArray(ArcArray)
    , isJoinGraph(IsJoinGraph)
  {
  }

  // sets initial size of vertex arrays
  void AllocateVertexArrays(viskores::Id Size);

  // sets initial size of edge arrays
  void AllocateEdgeArrays(viskores::Id Size);

  // routine that builds the merge graph once the initial vertices & edges are set
  void Compute(viskores::cont::ArrayHandle<viskores::Id>& saddles);

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

  // transfers partial results to merge tree array
  void TransferToMergeTree(viskores::cont::ArrayHandle<viskores::Id>& saddles);

  // prints the contents of the topology graph in a standard format
  void DebugPrint(const char* message);
}; // class ChainGraph

// sets initial size of vertex arrays
template <typename T, typename StorageType>
void ChainGraph<T, StorageType>::AllocateVertexArrays(viskores::Id Size)
{
  valueIndex.Allocate(Size);
  prunesTo.Allocate(Size);
  firstEdge.Allocate(Size);
  outdegree.Allocate(Size);
  chainExtremum.Allocate(Size);
  activeVertices.Allocate(Size);
} // AllocateVertexArrays()

// sets initial size of edge arrays
template <typename T, typename StorageType>
void ChainGraph<T, StorageType>::AllocateEdgeArrays(viskores::Id Size)
{
  edgeFar.Allocate(Size);
  edgeNear.Allocate(Size);
  activeEdges.Allocate(Size);
} // AllocateEdgeArrays()

// routine that builds the merge graph once the initial vertices & edges are set
template <typename T, typename StorageType>
void ChainGraph<T, StorageType>::Compute(viskores::cont::ArrayHandle<viskores::Id>& saddles)
{
#ifdef DEBUG_FUNCTION_ENTRY
  std::cout << std::endl;
  std::cout << "===================" << std::endl;
  std::cout << "Compute Chain Graph" << std::endl;
  std::cout << "===================" << std::endl;
  std::cout << std::endl;
#endif

#ifdef DEBUG_PRINT
  DebugPrint("Chain Graph Computation Starting");
#endif

  // loop until we run out of active edges
  nIterations = 0;
  while (edgeSorter.GetNumberOfValues() > 0)
  {
    // find & label the extrema with their governing saddles
    FindGoverningSaddles();

    // label the regular points
    TransferRegularPoints();

    // compact the active set of vertices & edges
    CompactActiveVertices();
    CompactActiveEdges();

    // rebuild the chains
    BuildChains();

    // choose the subset of edges for the governing saddles
    TransferSaddleStarts();

    // increment the iteration count
    nIterations++;
  } // main loop

  // final pass to label the trunk vertices
  BuildTrunk();

  // we can now release many of the arrays to free up space
  firstEdge.ReleaseResources();
  outdegree.ReleaseResources();
  edgeNear.ReleaseResources();
  edgeFar.ReleaseResources();
  activeEdges.ReleaseResources();
  activeVertices.ReleaseResources();
  edgeSorter.ReleaseResources();

  // and transfer results to mergearcs
  TransferToMergeTree(saddles);

  // then release the remaining memory
  chainExtremum.ReleaseResources();
  prunesTo.ReleaseResources();

#ifdef DEBUG_PRINT
  DebugPrint("Chain Graph Computed");
#endif
} // Compute()

// sorts saddle ascents to find governing saddles
template <typename T, typename StorageType>
void ChainGraph<T, StorageType>::FindGoverningSaddles()
{
#ifdef DEBUG_FUNCTION_ENTRY
  std::cout << std::endl;
  std::cout << "======================" << std::endl;
  std::cout << "Find Governing Saddles" << std::endl;
  std::cout << "======================" << std::endl;
  std::cout << std::endl;
#endif

  // sort with the comparator
  viskores::cont::Algorithm::Sort(edgeSorter,
                                  EdgePeakComparator<T, StorageType>(
                                    values, valueIndex, edgeFar, edgeNear, arcArray, isJoinGraph));

#ifdef DEBUG_PRINT
  DebugPrint("After Sorting");
#endif

  // now loop through the edges
  GoverningSaddleFinder governingSaddleFinder;
  viskores::worklet::DispatcherMapField<GoverningSaddleFinder> governingSaddleFinderDispatcher(
    governingSaddleFinder);
  viskores::Id nEdges = edgeSorter.GetNumberOfValues();
  viskores::cont::ArrayHandleIndex edgeIndexArray(nEdges);

  governingSaddleFinderDispatcher.Invoke(edgeIndexArray, // input
                                         edgeSorter,     // input (whole array)
                                         edgeFar,        // input (whole array)
                                         edgeNear,       // input (whole array)
                                         prunesTo,       // output (whole array)
                                         outdegree);     // output (whole array)
#ifdef DEBUG_PRINT
  DebugPrint(DEBUG_STRING_TRANSFER_GOVERNING_SADDLES);
#endif
} // FindGoverningSaddles()

// marks now regular points for removal
template <typename T, typename StorageType>
void ChainGraph<T, StorageType>::TransferRegularPoints()
{
#ifdef DEBUG_FUNCTION_ENTRY
  std::cout << std::endl;
  std::cout << "=======================" << std::endl;
  std::cout << "Transfer Regular Points" << std::endl;
  std::cout << "=======================" << std::endl;
  std::cout << std::endl;
#endif
  RegularPointTransferrer<T> regularPointTransferrer(isJoinGraph);
  viskores::worklet::DispatcherMapField<RegularPointTransferrer<T>>
    regularPointTransferrerDispatcher(regularPointTransferrer);

  regularPointTransferrerDispatcher.Invoke(activeVertices, // input
                                           chainExtremum,  // input (whole array)
                                           values,         // input (whole array)
                                           valueIndex,     // input (whole array)
                                           prunesTo,       // i/o (whole array)
                                           outdegree);     // output (whole array)
#ifdef DEBUG_PRINT
  DebugPrint("Regular Points Should Now Be Labelled");
#endif
} // TransferRegularPoints()

// compacts the active vertex list
template <typename T, typename StorageType>
void ChainGraph<T, StorageType>::CompactActiveVertices()
{
#ifdef DEBUG_FUNCTION_ENTRY
  std::cout << std::endl;
  std::cout << "=======================" << std::endl;
  std::cout << "Compact Active Vertices" << std::endl;
  std::cout << "=======================" << std::endl;
  std::cout << std::endl;
#endif
  using IdArrayType = viskores::cont::ArrayHandle<viskores::Id>;
  using PermuteIndexType = viskores::cont::ArrayHandlePermutation<IdArrayType, IdArrayType>;

  // create a temporary array the same size
  viskores::cont::ArrayHandle<viskores::Id> newActiveVertices;

  // Use only the current activeVertices outdegree to match size on CopyIf
  viskores::cont::ArrayHandle<viskores::Id> outdegreeLookup;
  viskores::cont::ArrayCopy(PermuteIndexType(activeVertices, outdegree), outdegreeLookup);

  // compact the activeVertices array to keep only the ones of interest
  viskores::cont::Algorithm::CopyIf(activeVertices, outdegreeLookup, newActiveVertices);

  activeVertices.ReleaseResources();
  viskores::cont::Algorithm::Copy(newActiveVertices, activeVertices);

#ifdef DEBUG_PRINT
  DebugPrint("Active Vertex List Compacted");
#endif
} // CompactActiveVertices()

// compacts the active edge list
template <typename T, typename StorageType>
void ChainGraph<T, StorageType>::CompactActiveEdges()
{
#ifdef DEBUG_FUNCTION_ENTRY
  std::cout << std::endl;
  std::cout << "====================" << std::endl;
  std::cout << "Compact Active Edges" << std::endl;
  std::cout << "====================" << std::endl;
  std::cout << std::endl;
#endif
  // grab the size of the array for easier reference
  viskores::Id nActiveVertices = activeVertices.GetNumberOfValues();

  // first, we have to work out the first edge for each active vertex
  // we start with a temporary new updegree
  viskores::cont::ArrayHandle<viskores::Id> newOutdegree;
  newOutdegree.Allocate(nActiveVertices);

  // do a parallel computation using the vertex degree updater
  // WARNING: Using chainMaximum for I/O in parallel loop
  // See functor description for algorithmic justification of safety
  VertexDegreeUpdater vertexDegreeUpdater;
  viskores::worklet::DispatcherMapField<VertexDegreeUpdater> vertexDegreeUpdaterDispatcher(
    vertexDegreeUpdater);

  vertexDegreeUpdaterDispatcher.Invoke(activeVertices, // input
                                       activeEdges,    // input (whole array)
                                       edgeFar,        // input (whole array)
                                       firstEdge,      // input (whole array)
                                       prunesTo,       // input (whole array)
                                       outdegree,      // input (whole array)
                                       chainExtremum,  // i/o (whole array)
                                       newOutdegree);  // output

  // now we do a reduction to compute the offsets of each vertex
  viskores::cont::ArrayHandle<viskores::Id> newPosition;
  viskores::cont::Algorithm::ScanExclusive(newOutdegree, newPosition);
  viskores::Id nNewEdges = viskores::cont::ArrayGetValue(nActiveVertices - 1, newPosition) +
    viskores::cont::ArrayGetValue(nActiveVertices - 1, newOutdegree);

  // create a temporary vector for copying
  viskores::cont::ArrayHandle<viskores::Id> newActiveEdges;
  newActiveEdges.Allocate(nNewEdges);

  // now copy the relevant edges into the active edge array
  // WARNING: Using chainMaximum, edgeHigh, firstEdge, updegree for I/O in parallel loop
  // See functor description for algorithmic justification of safety
  viskores::worklet::DispatcherMapField<ActiveEdgeTransferrer>().Invoke(
    activeVertices,  // input
    newPosition,     // input
    newOutdegree,    // input
    activeEdges,     // input (whole array)
    prunesTo,        // input (whole array)
    firstEdge,       // i/o (whole array)
    outdegree,       // i/o (whole array)
    chainExtremum,   // i/o (whole array)
    edgeFar,         // i/o (whole array)
    newActiveEdges); // output (whole array)

  // resize the original array and recopy
  viskores::cont::ArrayCopy(newActiveEdges, activeEdges);

#ifdef DEBUG_PRINT
  DebugPrint("Active Edges Now Compacted");
#endif
} // CompactActiveEdges()

// builds the chains for the new active vertices
template <typename T, typename StorageType>
void ChainGraph<T, StorageType>::BuildChains()
{
#ifdef DEBUG_FUNCTION_ENTRY
  std::cout << std::endl;
  std::cout << "============" << std::endl;
  std::cout << "Build Chains" << std::endl;
  std::cout << "============" << std::endl;
  std::cout << std::endl;
#endif
  // a temporary array the full size of the graph
  viskores::cont::ArrayHandle<viskores::Id> tempChainExtremum;
  tempChainExtremum.Allocate(edgeNear.GetNumberOfValues());

  // compute the number of log steps required in this pass
  viskores::Id nActiveVertices = activeVertices.GetNumberOfValues();
  viskores::Id nLogSteps = 1;
  for (viskores::Id shifter = nActiveVertices; shifter != 0; shifter >>= 1)
    nLogSteps++;

  ChainDoubler chainDoubler;
  viskores::worklet::DispatcherMapField<ChainDoubler> chainDoublerDispatcher(chainDoubler);

  // 2.	Use path compression / step doubling to collect vertices along ascending chains
  //		until every vertex has been assigned to *an* extremum
  //		Step two at a time, so that we rock between the original and the temp
  for (viskores::Id logStep = 0; logStep < nLogSteps; logStep++)
  {
    chainDoublerDispatcher.Invoke(activeVertices, // input
                                  chainExtremum); // i/o (whole array)
  }

#ifdef DEBUG_PRINT
  DebugPrint("Chains Built");
#endif
} // BuildChains()

// transfers saddle ascent edges into edge sorter
template <typename T, typename StorageType>
void ChainGraph<T, StorageType>::TransferSaddleStarts()
{
#ifdef DEBUG_FUNCTION_ENTRY
  std::cout << std::endl;
  std::cout << "=======================" << std::endl;
  std::cout << DEBUG_STRING_TRANSFER_SADDLE_STARTS << std::endl;
  std::cout << "=======================" << std::endl;
  std::cout << std::endl;
#endif

  // grab the size of the array for easier reference
  viskores::Id nActiveVertices = activeVertices.GetNumberOfValues();

  // reset number of edges to sort
  viskores::Id nEdgesToSort = 0;

  // in parallel, we need to create a vector to count the first edge for each vertex
  viskores::cont::ArrayHandle<viskores::Id> newFirstEdge;
  viskores::cont::ArrayHandle<viskores::Id> newOutdegree;
  newFirstEdge.Allocate(nActiveVertices);
  newOutdegree.Allocate(nActiveVertices);

  // 2. now test all active vertices to see if they have only one chain maximum
  SaddleAscentFunctor saddleAscentFunctor;
  viskores::worklet::DispatcherMapField<SaddleAscentFunctor> saddleAscentFunctorDispatcher(
    saddleAscentFunctor);

  saddleAscentFunctorDispatcher.Invoke(activeVertices, // input
                                       firstEdge,      // input (whole array)
                                       outdegree,      // input (whole array)
                                       activeEdges,    // input (whole array)
                                       chainExtremum,  // input (whole array)
                                       edgeFar,        // input (whole array)
                                       newOutdegree);  // output

  // 3. now compute the new offsets in the newFirstEdge array
  viskores::cont::Algorithm::ScanExclusive(newOutdegree, newFirstEdge);
  nEdgesToSort = viskores::cont::ArrayGetValue(nActiveVertices - 1, newFirstEdge) +
    viskores::cont::ArrayGetValue(nActiveVertices - 1, newOutdegree);

  edgeSorter.ReleaseResources();
  edgeSorter.Allocate(nEdgesToSort);

  SaddleAscentTransferrer saddleAscentTransferrer;
  viskores::worklet::DispatcherMapField<SaddleAscentTransferrer> saddleAscentTransferrerDispatcher(
    saddleAscentTransferrer);

  saddleAscentTransferrerDispatcher.Invoke(activeVertices, // input
                                           newOutdegree,   // input
                                           newFirstEdge,   // input
                                           activeEdges,    // input (whole array)
                                           firstEdge,      // input (whole array)
                                           edgeSorter);    // output (whole array)

#ifdef DEBUG_PRINT
  DebugPrint(DEBUG_STRING_TRANSFERRED_SADDLE_STARTS);
#endif
} // TransferSaddleStarts()

// sets all remaining active vertices
template <typename T, typename StorageType>
void ChainGraph<T, StorageType>::BuildTrunk()
{
#ifdef DEBUG_FUNCTION_ENTRY
  std::cout << std::endl;
  std::cout << "===========" << std::endl;
  std::cout << "Build Trunk" << std::endl;
  std::cout << "============" << std::endl;
  std::cout << std::endl;
#endif

  TrunkBuilder trunkBuilder;
  viskores::worklet::DispatcherMapField<TrunkBuilder> trunkBuilderDispatcher(trunkBuilder);

  trunkBuilderDispatcher.Invoke(activeVertices, // input
                                chainExtremum,  // input (whole array)
                                prunesTo);      // output (whole array)
#ifdef DEBUG_PRINT
  DebugPrint("Trunk Built");
#endif
} // BuildTrunk()

// transfers partial results to merge tree array
template <typename T, typename StorageType>
void ChainGraph<T, StorageType>::TransferToMergeTree(
  viskores::cont::ArrayHandle<viskores::Id>& saddles)
{
#ifdef DEBUG_FUNCTION_ENTRY
  std::cout << std::endl;
  std::cout << "=====================" << std::endl;
  std::cout << DEBUG_STRING_TRANSFER_TO_MERGE_TREE << std::endl;
  std::cout << "=====================" << std::endl;
  std::cout << std::endl;
#endif

  // first allocate memory for the target array
  saddles.ReleaseResources();

  // initialise it to the arcArray
  viskores::cont::ArrayCopy(arcArray, saddles);

  JoinTreeTransferrer joinTreeTransferrer;
  viskores::worklet::DispatcherMapField<JoinTreeTransferrer> joinTreeTransferrerDispatcher(
    joinTreeTransferrer);
  viskores::cont::ArrayHandleIndex valueIndexArray(valueIndex.GetNumberOfValues());

  joinTreeTransferrerDispatcher.Invoke(valueIndexArray, // input
                                       prunesTo,        // input
                                       valueIndex,      // input (whole array)
                                       chainExtremum,   // input (whole array)
                                       saddles,         // output (whole array)
                                       arcArray);       // output (whole array)
} // TransferToMergeTree()

// prints the contents of the topology graph in standard format
template <typename T, typename StorageType>
void ChainGraph<T, StorageType>::DebugPrint(const char* message)
{
  std::cout << "---------------------------" << std::endl;
  std::cout << std::string(message) << std::endl;
  std::cout << "---------------------------" << std::endl;
  std::cout << std::endl;

  using IdArrayType = viskores::cont::ArrayHandle<viskores::Id>;
  using ValueArrayType = viskores::cont::ArrayHandle<T>;
  using PermuteIndexType = viskores::cont::ArrayHandlePermutation<IdArrayType, IdArrayType>;
  using PermuteValueType = viskores::cont::ArrayHandlePermutation<IdArrayType, ValueArrayType>;

  // Full Vertex Arrays
  viskores::Id nValues = valueIndex.GetNumberOfValues();
  viskores::cont::ArrayHandle<T, StorageType> vertexValues;

  std::cout << "Full Vertex Arrays - Size:  " << nValues << std::endl;
  PrintHeader(nValues);
  PrintIndices("Index", valueIndex);
  viskores::cont::ArrayCopy(PermuteValueType(valueIndex, values), vertexValues);
  PrintValues("Value", vertexValues);
  PrintIndices("First Edge", firstEdge);
  PrintIndices("Outdegree", outdegree);
  PrintIndices("Chain Ext", chainExtremum);
  PrintIndices("Prunes To", prunesTo);
  std::cout << std::endl;

  // Active Vertex Arrays
  viskores::Id nActiveVertices = activeVertices.GetNumberOfValues();
  std::cout << "Active Vertex Arrays - Size: " << nActiveVertices << std::endl;
  if (nActiveVertices > 0)
  {
    viskores::cont::ArrayHandle<viskores::Id> tempIndex;
    viskores::cont::ArrayHandle<T> tempValue;

    PrintHeader(nActiveVertices);
    PrintIndices("Active Vertices", activeVertices);
    viskores::cont::ArrayCopy(PermuteIndexType(activeVertices, valueIndex), tempIndex);
    PrintIndices("Active Indices", tempIndex);
    viskores::cont::ArrayCopy(PermuteValueType(activeVertices, vertexValues), tempValue);
    PrintValues("Active Values", tempValue);
    viskores::cont::ArrayCopy(PermuteIndexType(activeVertices, firstEdge), tempIndex);
    PrintIndices("Active First Edge", tempIndex);
    viskores::cont::ArrayCopy(PermuteIndexType(activeVertices, outdegree), tempIndex);
    PrintIndices("Active Outdegree", tempIndex);
    viskores::cont::ArrayCopy(PermuteIndexType(activeVertices, chainExtremum), tempIndex);
    PrintIndices("Active Chain Ext", tempIndex);
    viskores::cont::ArrayCopy(PermuteIndexType(activeVertices, prunesTo), tempIndex);
    PrintIndices("Active Prunes To", tempIndex);
    std::cout << std::endl;
  }

  // Full Edge Arrays
  viskores::Id nEdges = edgeNear.GetNumberOfValues();
  std::cout << "Full Edge Arrays - Size:     " << nEdges << std::endl;
  viskores::cont::ArrayHandle<viskores::Id> farIndices;
  viskores::cont::ArrayHandle<viskores::Id> nearIndices;
  viskores::cont::ArrayHandle<T, StorageType> farValues;
  viskores::cont::ArrayHandle<T, StorageType> nearValues;
  if (nEdges > 0)
  {
    PrintHeader(nEdges);
    PrintIndices("Far", edgeFar);
    viskores::cont::ArrayCopy(PermuteIndexType(edgeFar, valueIndex), farIndices);
    PrintIndices("Far Index", farIndices);
    viskores::cont::ArrayCopy(PermuteValueType(farIndices, values), farValues);
    PrintValues("Far Value", farValues);

    PrintHeader(nEdges);
    PrintIndices("Near", edgeNear);
    viskores::cont::ArrayCopy(PermuteIndexType(edgeNear, valueIndex), nearIndices);
    PrintIndices("Near Index", nearIndices);
    viskores::cont::ArrayCopy(PermuteValueType(nearIndices, values), nearValues);
    PrintValues("Near Value", nearValues);
  }

  // Active Edge Arrays
  viskores::Id nActiveEdges = activeEdges.GetNumberOfValues();
  std::cout << "Active Edge Arrays - Size:   " << nActiveEdges << std::endl;
  if (nActiveEdges > 0)
  {
    viskores::cont::ArrayHandle<viskores::Id> activeFarIndices;
    viskores::cont::ArrayHandle<viskores::Id> activeNearIndices;
    viskores::cont::ArrayHandle<viskores::Id> activeNearLookup;
    viskores::cont::ArrayHandle<T, StorageType> activeNearValues;

    PrintHeader(nActiveEdges);
    PrintIndices("Active Edges", activeEdges);

    viskores::cont::ArrayCopy(PermuteIndexType(activeEdges, edgeFar), activeFarIndices);
    PrintIndices("Edge Far", activeFarIndices);
    viskores::cont::ArrayCopy(PermuteIndexType(activeEdges, edgeNear), activeNearIndices);
    PrintIndices("Edge Near", activeNearIndices);
    viskores::cont::ArrayCopy(PermuteIndexType(activeNearIndices, valueIndex), activeNearLookup);
    PrintIndices("Edge Near Index", activeNearLookup);
    viskores::cont::ArrayCopy(PermuteValueType(activeNearLookup, values), activeNearValues);
    PrintValues("Edge Near Value", activeNearValues);
    std::cout << std::endl;
  }

  // Edge Sorter Array
  viskores::Id nEdgeSorter = edgeSorter.GetNumberOfValues();
  std::cout << "Edge Sorter - Size:          " << nEdgeSorter << std::endl;
  if (nEdgeSorter > 0)
  {
    viskores::cont::ArrayHandle<viskores::Id> tempSortIndex;
    viskores::cont::ArrayHandle<T> tempSortValue;

    PrintHeader(nEdgeSorter);
    PrintIndices("Edge Sorter", edgeSorter);
    viskores::cont::ArrayCopy(PermuteIndexType(edgeSorter, edgeNear), tempSortIndex);
    PrintIndices("Sorted Near", tempSortIndex);
    viskores::cont::ArrayCopy(PermuteIndexType(edgeSorter, nearIndices), tempSortIndex);
    PrintIndices("Sorted Near Index", tempSortIndex);
    viskores::cont::ArrayCopy(PermuteIndexType(edgeSorter, edgeFar), tempSortIndex);
    PrintIndices("Sorted Far", tempSortIndex);
    viskores::cont::ArrayCopy(PermuteValueType(edgeSorter, nearValues), tempSortValue);
    PrintValues("Sorted Near Value", tempSortValue);
    std::cout << std::endl;
  }

  std::cout << "---------------------------" << std::endl;
  std::cout << std::endl;
} // DebugPrint()
}
}
}

#endif
