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
//	If we have computed the merge max & merge saddles correctly, we have substantially
//	computed the merge tree already. However, it is not in the same format as we have
//	previously represented it - in particular, we have yet to define all the merge arcs
//      and the superarcs we have collected are not the same as before - i.e. they are already
//	partially collapsed, but not according to the same rule as branch decomposition
//	This unit is therefore to get the same result out as before so we can set up an
//	automated crosscheck on the computation
//
//	Compared to earlier versions, we have made a significant change - the merge tree
// 	is only computed on critical points, not on the full array.  We therefore have a
//	final step: to extend it to the full array. To do this, we will keep the initial
//	mergeArcs array which records a maximum for each vertex, as we need the information
//
//	Each maximum is now labelled with the saddle it is mapped to, or to the global min
//	We therefore transfer this information back to the mergeArcs array, so that maxima
//	(including saddles) are marked with the (lower) vertex that is the low end of their
//	arc

//	BIG CHANGE: we can actually reuse the mergeArcs array for the final merge arc, for the
//	chain maximum for each (regular) point, and for the merge saddle for maxima.  This is
//	slightly tricky and has some extra memory traffic, but it avoids duplicating arrays
//	unnecessarily
//
//	Initially, mergeArcs will be set to an outbound neighbour (or self for extrema), as the
//	chainMaximum array used to be.
//
//	After chains are built, then it will hold *AN* accessible extremum for each vertex.
//
//	During the main processing, when an extremum is assigned a saddle, it will be stored
//	here. Regular points will still store pointers to an extremum.
//
//	After this is done, if the mergeArc points lower/higher, it is pointing to a saddle.
//      Otherwise it is pointing to an extremum.
//
//	And after the final pass, it will always point to the next along superarcs.
//
//=======================================================================================

#ifndef viskores_worklet_contourtree_mergetree_h
#define viskores_worklet_contourtree_mergetree_h

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleConstant.h>
#include <viskores/cont/DataSet.h>

#include <viskores/filter/scalar_topology/worklet/contourtree/ChainDoubler.h>
#include <viskores/filter/scalar_topology/worklet/contourtree/JoinArcConnector.h>
#include <viskores/filter/scalar_topology/worklet/contourtree/JoinSuperArcFinder.h>
#include <viskores/filter/scalar_topology/worklet/contourtree/PrintVectors.h>
#include <viskores/filter/scalar_topology/worklet/contourtree/VertexMergeComparator.h>

//#define DEBUG_PRINT 1
//#define DEBUG_FUNCTION_ENTRY 1
//#define DEBUG_TIMING 1

namespace viskores
{
namespace worklet
{
namespace contourtree
{

template <typename T, typename StorageType>
class MergeTree
{
public:
  // original data array
  const viskores::cont::ArrayHandle<T, StorageType>& values;

  // size of mesh
  viskores::Id nRows, nCols, nSlices, NumVertices, nLogSteps;

  // whether it is join or split tree
  bool isJoinTree;

  // vector of arcs representing the merge tree
  viskores::cont::ArrayHandle<viskores::Id> mergeArcs;

  // vector storing an extremum for each vertex
  viskores::cont::ArrayHandle<viskores::Id> extrema;

  // vector storing a saddle for each vertex
  viskores::cont::ArrayHandle<viskores::Id> saddles;

  // merge tree constructor
  MergeTree(const viskores::cont::ArrayHandle<T, StorageType>& Values,
            viskores::Id NRows,
            viskores::Id NCols,
            viskores::Id NSlices,
            bool IsJoinTree);

  // routine that does pointer-doubling in the mergeArc array
  void BuildRegularChains();

  // routine that computes the augmented merge tree superarcs from the merge graph
  void ComputeAugmentedSuperarcs();

  // routine that computes the augmented merge arcs from the superarcs
  // this is separate from the previous routine because it also gets called separately
  // once saddle & extrema are set for a given set of vertices, the merge arcs can be
  // computed for any subset of those vertices that contains all of the critical points
  void ComputeAugmentedArcs(viskores::cont::ArrayHandle<viskores::Id>& vertices);

  // debug routine
  void DebugPrint(const char* message);
};

// creates merge tree
template <typename T, typename StorageType>
MergeTree<T, StorageType>::MergeTree(const viskores::cont::ArrayHandle<T, StorageType>& Values,
                                     viskores::Id NRows,
                                     viskores::Id NCols,
                                     viskores::Id NSlices,
                                     bool IsJoinTree)
  : values(Values)
  , nRows(NRows)
  , nCols(NCols)
  , nSlices(NSlices)
  , isJoinTree(IsJoinTree)
{
  NumVertices = nRows * nCols * nSlices;
  nLogSteps = 1;
  for (viskores::Id shifter = NumVertices; shifter != 0; shifter >>= 1)
    nLogSteps++;

  viskores::cont::ArrayHandleConstant<viskores::Id> nullArray(0, NumVertices);

  mergeArcs.Allocate(NumVertices);
  extrema.Allocate(NumVertices);
  saddles.Allocate(NumVertices);

  viskores::cont::ArrayCopy(nullArray, mergeArcs);
  viskores::cont::ArrayCopy(nullArray, extrema);
  viskores::cont::ArrayCopy(nullArray, saddles);
}

// routine that does pointer-doubling in the saddles array
template <typename T, typename StorageType>
void MergeTree<T, StorageType>::BuildRegularChains()
{
#ifdef DEBUG_FUNCTION_ENTRY
  std::cout << std::endl;
  std::cout << "====================" << std::endl;
  std::cout << "Build Regular Chains" << std::endl;
  std::cout << "====================" << std::endl;
  std::cout << std::endl;
#endif
  // 2. Create a temporary array so that we can alternate writing between them
  viskores::cont::ArrayHandle<viskores::Id> temporaryArcs;
  temporaryArcs.Allocate(NumVertices);

  viskores::cont::ArrayHandleIndex vertexIndexArray(NumVertices);
  ChainDoubler chainDoubler;
  viskores::worklet::DispatcherMapField<ChainDoubler> chainDoublerDispatcher(chainDoubler);

  // 3. Apply pointer-doubling to build chains to maxima, rocking between two arrays
  for (viskores::Id logStep = 0; logStep < nLogSteps; logStep++)
  {
    chainDoublerDispatcher.Invoke(vertexIndexArray, // input
                                  extrema);         // i/o whole array
  }
} // BuildRegularChains()

// routine that computes the augmented merge tree from the merge graph
template <typename T, typename StorageType>
void MergeTree<T, StorageType>::ComputeAugmentedSuperarcs()
{
#ifdef DEBUG_FUNCTION_ENTRY
  std::cout << std::endl;
  std::cout << "=================================" << std::endl;
  std::cout << "Compute Augmented Merge Superarcs" << std::endl;
  std::cout << "=================================" << std::endl;
  std::cout << std::endl;
#endif

  // our first step is to assign every vertex to a pseudo-extremum based on how the
  // vertex ascends to a extremum, and the sequence of pruning for the extremum
  // to do this, we iterate as many times as pruning occurred

  // we run a little loop for each element until it finds its join superarc
  // expressed as a functor.
  viskores::Id nExtrema = extrema.GetNumberOfValues();

  JoinSuperArcFinder<T> joinSuperArcFinder(isJoinTree);
  viskores::worklet::DispatcherMapField<JoinSuperArcFinder<T>> joinSuperArcFinderDispatcher(
    joinSuperArcFinder);
  viskores::cont::ArrayHandleIndex vertexIndexArray(nExtrema);

  joinSuperArcFinderDispatcher.Invoke(vertexIndexArray, // input
                                      values,           // input (whole array)
                                      saddles,          // i/o (whole array)
                                      extrema);         // i/o (whole array)

// at the end of this, all vertices should have a pseudo-extremum in the extrema array
// and a pseudo-saddle in the saddles array
#ifdef DEBUG_PRINT
  DebugPrint("Merge Superarcs Set");
#endif
} // ComputeAugmentedSuperarcs()

// routine that computes the augmented merge arcs from the superarcs
// this is separate from the previous routine because it also gets called separately
// once saddle & extrema are set for a given set of vertices, the merge arcs can be
// computed for any subset of those vertices that contains all of the critical points
template <typename T, typename StorageType>
void MergeTree<T, StorageType>::ComputeAugmentedArcs(
  viskores::cont::ArrayHandle<viskores::Id>& vertices)
{
#ifdef DEBUG_FUNCTION_ENTRY
  std::cout << std::endl;
  std::cout << "============================" << std::endl;
  std::cout << "Compute Augmented Merge Arcs" << std::endl;
  std::cout << "============================" << std::endl;
  std::cout << std::endl;
#endif

  // create a vector of indices for sorting
  viskores::Id nCriticalVerts = vertices.GetNumberOfValues();
  viskores::cont::ArrayHandle<viskores::Id> vertexSorter;
  viskores::cont::ArrayCopy(vertices, vertexSorter);

  // We sort by pseudo-maximum to establish the extents
  viskores::cont::Algorithm::Sort(
    vertexSorter, VertexMergeComparator<T, StorageType>(values, extrema, isJoinTree));
#ifdef DEBUG_PRINT
  DebugPrint("Sorting Complete");
#endif

  viskores::cont::ArrayHandleConstant<viskores::Id> noVertArray(NO_VERTEX_ASSIGNED, NumVertices);
  viskores::cont::ArrayCopy(noVertArray, mergeArcs);

  viskores::cont::ArrayHandleIndex critVertexIndexArray(nCriticalVerts);
  JoinArcConnector joinArcConnector;
  viskores::worklet::DispatcherMapField<JoinArcConnector> joinArcConnectorDispatcher(
    joinArcConnector);

  joinArcConnectorDispatcher.Invoke(critVertexIndexArray, // input
                                    vertexSorter,         // input (whole array)
                                    extrema,              // input (whole array)
                                    saddles,              // input (whole array)
                                    mergeArcs);           // output (whole array)
#ifdef DEBUG_PRINT
  DebugPrint("Augmented Arcs Set");
#endif
} // ComputeAugmentedArcs()

// debug routine
template <typename T, typename StorageType>
void MergeTree<T, StorageType>::DebugPrint(const char* message)
{
  std::cout << "---------------------------" << std::endl;
  std::cout << std::string(message) << std::endl;
  std::cout << "---------------------------" << std::endl;
  std::cout << std::endl;

  PrintLabelledBlock("Values", values, nRows * nSlices, nCols);
  std::cout << std::endl;
  PrintLabelledBlock("MergeArcs", mergeArcs, nRows, nCols);
  std::cout << std::endl;
  PrintLabelledBlock("Extrema", extrema, nRows, nCols);
  std::cout << std::endl;
  PrintLabelledBlock("Saddles", saddles, nRows, nCols);
  std::cout << std::endl;
} // DebugPrint()
}
}
}
#endif
