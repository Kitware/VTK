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
//	Essentially, a vector of data values. BUT we will want them sorted to simplify
//	processing - i.e. it's the robust way of handling simulation of simplicity
//
//	On the other hand, once we have them sorted, we can discard the original data since
//	only the sort order matters
//
//	Since we've been running into memory issues, we'll start being more careful.
//	Clearly, we can eliminate the values if we sort, but in this iteration we are
//	deferring doing a full sort, so we need to keep the values.
//
//=======================================================================================

#ifndef viskores_worklet_contourtree_mesh2d_dem_triangulation_h
#define viskores_worklet_contourtree_mesh2d_dem_triangulation_h

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayGetValues.h>
#include <viskores/cont/ArrayHandleCounting.h>
#include <viskores/cont/ArrayHandleIndex.h>
#include <viskores/cont/ArrayHandleZip.h>
#include <viskores/worklet/DispatcherMapField.h>

#include <viskores/filter/scalar_topology/worklet/contourtree/ChainGraph.h>
#include <viskores/filter/scalar_topology/worklet/contourtree/Mesh2D_DEM_SaddleStarter.h>
#include <viskores/filter/scalar_topology/worklet/contourtree/Mesh2D_DEM_VertexOutdegreeStarter.h>
#include <viskores/filter/scalar_topology/worklet/contourtree/Mesh2D_DEM_VertexStarter.h>
#include <viskores/filter/scalar_topology/worklet/contourtree/PrintVectors.h>
#include <viskores/filter/scalar_topology/worklet/contourtree/Types.h>

//#define DEBUG_PRINT 1
//#define DEBUG_TIMING 1

namespace viskores
{
namespace worklet
{
namespace contourtree
{

template <typename T, typename StorageType>
class Mesh2D_DEM_Triangulation
{
public:
  // original data array
  const viskores::cont::ArrayHandle<T, StorageType>& values;

  // size of the mesh
  viskores::Id nRows, nCols, NumVertices, nLogSteps;

  // Array with neighbourhood masks
  viskores::cont::ArrayHandle<viskores::Id> neighbourhoodMask;

  // constructor
  Mesh2D_DEM_Triangulation(const viskores::cont::ArrayHandle<T, StorageType>& Values,
                           viskores::Id NRows,
                           viskores::Id NCols);

  // sets all vertices to point along an outgoing edge (except extrema)
  void SetStarts(viskores::cont::ArrayHandle<viskores::Id>& chains, bool descending);

  // sets outgoing paths for saddles
  void SetSaddleStarts(ChainGraph<T, StorageType>& mergeGraph, bool descending);
};

// sets outgoing paths for saddles
template <typename T, typename StorageType>
void Mesh2D_DEM_Triangulation<T, StorageType>::SetStarts(
  viskores::cont::ArrayHandle<viskores::Id>& chains,
  bool ascending)
{
  // create the neighbourhood mask
  neighbourhoodMask.Allocate(NumVertices);

  // For each vertex set the next vertex in the chain
  viskores::cont::ArrayHandleIndex vertexIndexArray(NumVertices);
  Mesh2D_DEM_VertexStarter<T> vertexStarter(nRows, nCols, ascending);
  viskores::worklet::DispatcherMapField<Mesh2D_DEM_VertexStarter<T>> vertexStarterDispatcher(
    vertexStarter);

  vertexStarterDispatcher.Invoke(vertexIndexArray,   // input
                                 values,             // input (whole array)
                                 chains,             // output
                                 neighbourhoodMask); // output
} // SetStarts()

// creates input mesh
template <typename T, typename StorageType>
Mesh2D_DEM_Triangulation<T, StorageType>::Mesh2D_DEM_Triangulation(
  const viskores::cont::ArrayHandle<T, StorageType>& Values,
  viskores::Id NRows,
  viskores::Id NCols)
  : values(Values)
  , nRows(NRows)
  , nCols(NCols)
{
  NumVertices = nRows * nCols;

  // compute the number of log-jumping steps (i.e. lg_2 (NumVertices))
  nLogSteps = 1;
  for (viskores::Id shifter = NumVertices; shifter > 0; shifter >>= 1)
    nLogSteps++;
}

// sets outgoing paths for saddles
template <typename T, typename StorageType>
void Mesh2D_DEM_Triangulation<T, StorageType>::SetSaddleStarts(
  ChainGraph<T, StorageType>& mergeGraph,
  bool ascending)
{
  // we need a temporary inverse index to change vertex IDs
  viskores::cont::ArrayHandle<viskores::Id> inverseIndex;
  viskores::cont::ArrayHandle<viskores::Id> isCritical;
  viskores::cont::ArrayHandle<viskores::Id> outdegree;
  inverseIndex.Allocate(NumVertices);
  isCritical.Allocate(NumVertices);
  outdegree.Allocate(NumVertices);

  viskores::cont::ArrayHandleIndex vertexIndexArray(NumVertices);
  Mesh2D_DEM_VertexOutdegreeStarter vertexOutdegreeStarter(nRows, nCols, ascending);
  viskores::worklet::DispatcherMapField<Mesh2D_DEM_VertexOutdegreeStarter>
    vertexOutdegreeStarterDispatcher(vertexOutdegreeStarter);

  vertexOutdegreeStarterDispatcher.Invoke(vertexIndexArray,    // input
                                          neighbourhoodMask,   // input
                                          mergeGraph.arcArray, // input (whole array)
                                          outdegree,           // output
                                          isCritical);         // output

  viskores::cont::Algorithm::ScanExclusive(isCritical, inverseIndex);

  // now we can compute how many critical points we carry forward
  viskores::Id nCriticalPoints = viskores::cont::ArrayGetValue(NumVertices - 1, inverseIndex) +
    viskores::cont::ArrayGetValue(NumVertices - 1, isCritical);

  // allocate space for the join graph vertex arrays
  mergeGraph.AllocateVertexArrays(nCriticalPoints);

  // compact the set of vertex indices to critical ones only
  viskores::cont::Algorithm::CopyIf(vertexIndexArray, isCritical, mergeGraph.valueIndex);

  // we initialise the prunesTo array to "NONE"
  viskores::cont::ArrayHandleConstant<viskores::Id> notAssigned(NO_VERTEX_ASSIGNED,
                                                                nCriticalPoints);
  viskores::cont::Algorithm::Copy(notAssigned, mergeGraph.prunesTo);

  // copy the outdegree from our temporary array
  // : mergeGraph.outdegree[vID] <= outdegree[mergeGraph.valueIndex[vID]]
  viskores::cont::Algorithm::CopyIf(outdegree, isCritical, mergeGraph.outdegree);

  // copy the chain maximum from arcArray
  // : mergeGraph.chainExtremum[vID] = inverseIndex[mergeGraph.arcArray[mergeGraph.valueIndex[vID]]]
  using IdArrayType = viskores::cont::ArrayHandle<viskores::Id>;
  using PermuteIndexType = viskores::cont::ArrayHandlePermutation<IdArrayType, IdArrayType>;

  viskores::cont::ArrayHandle<viskores::Id> tArray;
  tArray.Allocate(nCriticalPoints);
  viskores::cont::Algorithm::CopyIf(mergeGraph.arcArray, isCritical, tArray);
  viskores::cont::Algorithm::Copy(PermuteIndexType(tArray, inverseIndex), mergeGraph.chainExtremum);

  // and set up the active vertices - initially to identity
  viskores::cont::ArrayHandleIndex criticalVertsIndexArray(nCriticalPoints);
  viskores::cont::Algorithm::Copy(criticalVertsIndexArray, mergeGraph.activeVertices);

  // now we need to compute the firstEdge array from the outdegrees
  viskores::cont::Algorithm::ScanExclusive(mergeGraph.outdegree, mergeGraph.firstEdge);

  viskores::Id nCriticalEdges =
    viskores::cont::ArrayGetValue(nCriticalPoints - 1, mergeGraph.firstEdge) +
    viskores::cont::ArrayGetValue(nCriticalPoints - 1, mergeGraph.outdegree);

  // now we allocate the edge arrays
  mergeGraph.AllocateEdgeArrays(nCriticalEdges);

  // and we have to set them, so we go back to the vertices
  Mesh2D_DEM_SaddleStarter saddleStarter(nRows,      // input
                                         nCols,      // input
                                         ascending); // input
  viskores::worklet::DispatcherMapField<Mesh2D_DEM_SaddleStarter> saddleStarterDispatcher(
    saddleStarter);

  viskores::cont::ArrayHandleZip<viskores::cont::ArrayHandle<viskores::Id>,
                                 viskores::cont::ArrayHandle<viskores::Id>>
    outDegFirstEdge =
      viskores::cont::make_ArrayHandleZip(mergeGraph.outdegree, mergeGraph.firstEdge);

  saddleStarterDispatcher.Invoke(criticalVertsIndexArray, // input
                                 outDegFirstEdge,         // input (pair)
                                 mergeGraph.valueIndex,   // input
                                 neighbourhoodMask,       // input (whole array)
                                 mergeGraph.arcArray,     // input (whole array)
                                 inverseIndex,            // input (whole array)
                                 mergeGraph.edgeNear,     // output (whole array)
                                 mergeGraph.edgeFar,      // output (whole array)
                                 mergeGraph.activeEdges); // output (whole array)

  // finally, allocate and initialise the edgeSorter array
  viskores::cont::ArrayCopy(mergeGraph.activeEdges, mergeGraph.edgeSorter);
} // SetSaddleStarts()
}
}
}

#endif
