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
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
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

#ifndef viskores_worklet_contourtree_augmented_mesh_dem_contour_tree_mesh_h
#define viskores_worklet_contourtree_augmented_mesh_dem_contour_tree_mesh_h

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <viskores/Types.h>
#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayHandleConstant.h>
#include <viskores/cont/ArrayHandleGroupVecVariable.h>
#include <viskores/cont/ArrayHandleIndex.h>
#include <viskores/cont/ArrayHandlePermutation.h>
#include <viskores/cont/ArrayPortalToIterators.h>
#include <viskores/cont/ArrayRangeCompute.h>
#include <viskores/cont/ArrayRangeComputeTemplate.h>
#include <viskores/cont/ConvertNumComponentsToOffsets.h>
#include <viskores/cont/EnvironmentTracker.h>
#include <viskores/cont/Timer.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/ArrayTransforms.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/data_set_mesh/IdRelabeler.h> // This is needed only as an unused default argument.
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/meshtypes/MeshStructureContourTreeMesh.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/meshtypes/contourtreemesh/AddToArrayElementsWorklet.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/meshtypes/contourtreemesh/ApplyLookupTableDecorator.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/meshtypes/contourtreemesh/ArcComparator.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/meshtypes/contourtreemesh/ArcValidDecorator.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/meshtypes/contourtreemesh/CombinedSimulatedSimplicityIndexComparator.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/meshtypes/contourtreemesh/CombinedVectorDifferentFromNext.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/meshtypes/contourtreemesh/CopyIntoCombinedArrayWorklet.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/meshtypes/contourtreemesh/CopyIntoCombinedNeighborsWorklet.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/meshtypes/contourtreemesh/CopyNeighborsToPackedArray.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/meshtypes/contourtreemesh/FindDuplicateInOtherWorklet.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/meshtypes/contourtreemesh/GetArcFromDecorator.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/meshtypes/contourtreemesh/InitToCombinedSortOrderArraysWorklet.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/meshtypes/contourtreemesh/MergeSortedListsWithoutDuplicatesWorklet.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/meshtypes/contourtreemesh/ReplaceArcNumWithToVertexWorklet.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/meshtypes/mesh_boundary/ComputeMeshBoundaryContourTreeMesh.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/meshtypes/mesh_boundary/MeshBoundaryContourTreeMesh.h>
#include <viskores/io/ErrorIO.h>
#include <viskores/worklet/DispatcherMapField.h>

#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/PrintVectors.h> // TODO remove should not be needed

#include <viskores/cont/ExecutionObjectBase.h>

#include <viskores/cont/Invoker.h>

#include <algorithm>

VISKORES_THIRDPARTY_PRE_INCLUDE
#include <viskores/thirdparty/diy/Configure.h>
#include <viskores/thirdparty/diy/diy.h>
VISKORES_THIRDPARTY_POST_INCLUDE
// clang-format on

namespace contourtree_mesh_inc_ns =
  viskores::worklet::contourtree_augmented::mesh_dem_contourtree_mesh_inc;

// #define DEBUG_PRINT

namespace viskores
{
namespace worklet
{
namespace contourtree_augmented
{
template <typename FieldType>
class ContourTreeMesh : public viskores::cont::ExecutionObjectBase
{ // class ContourTreeMesh
public:
  //Mesh dependent helper functions
  void SetPrepareForExecutionBehavior(bool getMax);

  contourtree_mesh_inc_ns::MeshStructureContourTreeMesh PrepareForExecution(
    viskores::cont::DeviceAdapterId,
    viskores::cont::Token& token) const;

  ContourTreeMesh() {}

  // Constructor
  ContourTreeMesh(const IdArrayType& arcs,
                  const IdArrayType& inSortOrder,
                  const viskores::cont::ArrayHandle<FieldType>& values,
                  const IdArrayType& inGlobalMeshIndex);

  // Constructor
  ContourTreeMesh(const IdArrayType& nodes,
                  const IdArrayType& arcs,
                  const IdArrayType& inSortOrder,
                  const viskores::cont::ArrayHandle<FieldType>& values,
                  const IdArrayType& inGlobalMeshIndex);

  //  Construct a ContourTreeMesh from nodes/arcs and another ContourTreeMesh (instead of a DataSetMesh)
  //     nodes/arcs: From the contour tree
  //     ContourTreeMesh: the contour tree mesh used to compute the contour tree described by nodes/arcs
  ContourTreeMesh(const IdArrayType& nodes,
                  const IdArrayType& arcs,
                  const ContourTreeMesh<FieldType>& mesh);

  // Initalize contour tree mesh from mesh and arcs. For fully augmented contour tree with all
  // mesh vertices as nodes. Same as using { 0, 1, ..., nodes.size()-1 } as nodes for the
  // ContourTreeMeshh(nodes, arcsm mesh) constructor above
  ContourTreeMesh(const IdArrayType& arcs, const ContourTreeMesh<FieldType>& mesh);

  // Load contour tree mesh from file
  ContourTreeMesh(const char* filename)
  {
    Load(filename);
    this->NumVertices = this->SortedValues.GetNumberOfValues();
  }

  viskores::Id GetNumberOfVertices() const { return this->NumVertices; }

  // Combine two ContourTreeMeshes
  void MergeWith(ContourTreeMesh<FieldType>& other,
                 viskores::cont::LogLevel TreeLogLevel = viskores::cont::LogLevel::Perf,
                 std::string timingsMessage = "");

  // Save/Load the mesh helpers
  void Save(const char* filename) const;
  void Load(const char* filename);

  // Empty placeholder function to ensure compliance of this class with the interface
  // the other mesh classes. This is a no-op here since this class is initalized
  // from a known contour tree so sort is already done
  template <typename T, typename StorageType>
  void SortData(const viskores::cont::ArrayHandle<T, StorageType>& values) const
  {
    (void)values; // Do nothink but avoid unsused param warning
  }

  // Public fields
  static const int MAX_OUTDEGREE = 20;

  viskores::Id NumVertices;
  viskores::cont::ArrayHandleIndex SortOrder;
  viskores::cont::ArrayHandleIndex SortIndices;
  viskores::cont::ArrayHandle<FieldType> SortedValues;
  IdArrayType GlobalMeshIndex;
  // NeighborConnectivity stores for each vertex the indices of its neighbors. For each vertex
  // the indices are sorted by value, i.e, the first neighbour has the lowest and
  // the last neighbour the highest value for the vertex. In the array we just
  // concatinate the list of neighbors from all vertices, i.e., we first
  // have the list of neighbors of the first vertex, then the second vertex and so on, i.e.:
  // [ n_1_1, n_1_2, n_2_1, n_2_2, n_2_3, etc.]
  IdArrayType NeighborConnectivity;
  // NeighborOffsets gives us for each vertex an index into the neighours array indicating
  // the index where the list of neighbors for the vertex begins
  IdArrayType NeighborOffsets;
  // the maximum number of neighbors of a vertex
  viskores::Id MaxNeighbors;

  // Print Contents
  void PrintContent(std::ostream& outStream = std::cout) const;

  // Debug print routine
  void DebugPrint(const char* message, const char* fileName, long lineNum) const;

  // Get boundary execution object
  MeshBoundaryContourTreeMeshExec GetMeshBoundaryExecutionObject(viskores::Id3 globalSize,
                                                                 viskores::Id3 minIdx,
                                                                 viskores::Id3 maxIdx) const;

  void GetBoundaryVertices(IdArrayType& boundaryVertexArray,                    // output
                           IdArrayType& boundarySortIndexArray,                 // output
                           MeshBoundaryContourTreeMeshExec* meshBoundaryExecObj //input
  ) const;

  /// copies the global IDs for a set of sort IDs
  /// notice that the sort ID is the same as the mesh ID for the ContourTreeMesh class.
  /// To reduce memory usage we here use a fancy array handle rather than copy data
  /// as is needed for the DataSetMesh types.
  /// We here return a fancy array handle to convert values on-the-fly without requiring additional memory
  /// @param[in] sortIds Array with sort Ids to be converted from local to global Ids
  /// @param[in] localToGlobalIdRelabeler This parameter is here only for
  ///            consistency with the DataSetMesh types but is not
  ///            used here and as such can simply be set to nullptr
  inline viskores::cont::ArrayHandlePermutation<IdArrayType, IdArrayType>
  GetGlobalIdsFromSortIndices(const IdArrayType& sortIds,
                              const viskores::worklet::contourtree_augmented::mesh_dem::IdRelabeler*
                                localToGlobalIdRelabeler = nullptr) const
  {                                 // GetGlobalIDsFromSortIndices()
    (void)localToGlobalIdRelabeler; // avoid compiler warning
    return viskores::cont::make_ArrayHandlePermutation(sortIds, this->GlobalMeshIndex);
  } // GetGlobalIDsFromSortIndices()

  /// copies the global IDs for a set of mesh IDs
  /// notice that the sort ID is the same as the mesh ID for the ContourTreeMesh class.
  /// To reduce memory usage we here use a fancy array handle rather than copy data
  /// as is needed for the DataSetMesh types.
  /// MeshIdArrayType must be an array if Ids. Usually this is a viskores::worklet::contourtree_augmented::IdArrayType
  /// but in some cases it may also be a fancy array to avoid memory allocation
  /// We here return a fancy array handle to convert values on-the-fly without requiring additional memory
  /// @param[in] meshIds Array with mesh Ids to be converted from local to global Ids
  /// @param[in] localToGlobalIdRelabeler This parameter is here only for
  ///            consistency with the DataSetMesh types but is not
  ///            used here and as such can simply be set to nullptr
  template <typename MeshIdArrayType>
  inline viskores::cont::ArrayHandlePermutation<MeshIdArrayType, IdArrayType>
  GetGlobalIdsFromMeshIndices(const MeshIdArrayType& meshIds,
                              const viskores::worklet::contourtree_augmented::mesh_dem::IdRelabeler*
                                localToGlobalIdRelabeler = nullptr) const
  {                                 // GetGlobalIDsFromMeshIndices()
    (void)localToGlobalIdRelabeler; // avoid compiler warning
    return viskores::cont::make_ArrayHandlePermutation(meshIds, this->GlobalMeshIndex);
  } // GetGlobalIDsFromMeshIndices()

private:
  viskores::cont::Invoker Invoke;

  bool mGetMax; // Define the behavior for the PrepareForExecution function

  // Private init and helper functions
  void InitializeNeighborConnectivityFromArcs(const IdArrayType& arcs);
  void ComputeMaxNeighbors();

  // Private helper functions for saving data vectors
  // Internal helper function to save 1D index array to file
  template <typename ValueType>
  void SaveVector(std::ostream& os, const viskores::cont::ArrayHandle<ValueType>& vec) const;

  // Internal helper function to Load 1D index array from file
  template <typename ValueType>
  void LoadVector(std::istream& is, viskores::cont::ArrayHandle<ValueType>& vec);

}; // ContourTreeMesh

// print content
template <typename FieldType>
inline void ContourTreeMesh<FieldType>::PrintContent(std::ostream& outStream /*= std::cout*/) const
{ // PrintContent()
  PrintHeader(this->NumVertices, outStream);
  //PrintIndices("SortOrder", SortOrder, outStream);
  PrintValues("SortedValues", this->SortedValues, -1, outStream);
  PrintIndices("GlobalMeshIndex", this->GlobalMeshIndex, -1, outStream);
  PrintIndices("NeighborConnectivity", this->NeighborConnectivity, -1, outStream);
  PrintIndices("NeighborOffsets", this->NeighborOffsets, -1, outStream);
  outStream << "MaxNeighbors=" << this->MaxNeighbors << std::endl;
  outStream << "mGetMax=" << this->mGetMax << std::endl;
} // PrintContent()

// debug routine
template <typename FieldType>
inline void ContourTreeMesh<FieldType>::DebugPrint(const char* message,
                                                   const char* fileName,
                                                   long lineNum) const
{ // DebugPrint()
  std::cout << "---------------------------" << std::endl;
  std::cout << std::setw(30) << std::left << fileName << ":" << std::right << std::setw(4)
            << lineNum << std::endl;
  std::cout << std::left << std::string(message) << std::endl;
  std::cout << "Contour Tree Mesh Contains:     " << std::endl;
  std::cout << "---------------------------" << std::endl;
  std::cout << std::endl;

  PrintContent(std::cout);
} // DebugPrint()

// create the contour tree mesh from contour tree data
template <typename FieldType>
ContourTreeMesh<FieldType>::ContourTreeMesh(const IdArrayType& arcs,
                                            const IdArrayType& inSortOrder,
                                            const viskores::cont::ArrayHandle<FieldType>& values,
                                            const IdArrayType& inGlobalMeshIndex)
  : SortOrder()
  , SortedValues()
  , GlobalMeshIndex(inGlobalMeshIndex)
  , NeighborConnectivity()
  , NeighborOffsets()
{
  this->NumVertices = inSortOrder.GetNumberOfValues();
  // Initalize the SortedIndices as a smart array handle
  this->SortIndices = viskores::cont::ArrayHandleIndex(this->NumVertices);
  this->SortOrder = viskores::cont::ArrayHandleIndex(this->NumVertices);
  // values permuted by SortOrder to sort the values
  auto permutedValues = viskores::cont::make_ArrayHandlePermutation(inSortOrder, values);
  // TODO check if we actually need to make this copy here. we could just store the permutedValues array to save memory
  viskores::cont::Algorithm::Copy(permutedValues, this->SortedValues);
  this->InitializeNeighborConnectivityFromArcs(arcs);
#ifdef DEBUG_PRINT
  // Print the contents fo this for debugging
  DebugPrint("ContourTreeMesh Initialized", __FILE__, __LINE__);
#endif
}


template <typename FieldType>
inline ContourTreeMesh<FieldType>::ContourTreeMesh(
  const IdArrayType& nodes,
  const IdArrayType& arcs,
  const IdArrayType& inSortOrder,
  const viskores::cont::ArrayHandle<FieldType>& values,
  const IdArrayType& inGlobalMeshIndex)
  : GlobalMeshIndex(inGlobalMeshIndex)
  , NeighborConnectivity()
  , NeighborOffsets()
{
  // Initialize the SortedValues array with values permuted by the SortOrder permuted by the nodes, i.e.,
  // this->SortedValues[v] = values[inSortOrder[nodes[v]]];
  viskores::cont::ArrayHandlePermutation<IdArrayType, IdArrayType> permutedSortOrder(nodes,
                                                                                     inSortOrder);
  auto permutedValues = viskores::cont::make_ArrayHandlePermutation(permutedSortOrder, values);
  viskores::cont::Algorithm::Copy(permutedValues, this->SortedValues);
  // Initalize the SortedIndices as a smart array handle
  this->NumVertices = this->SortedValues.GetNumberOfValues();
  this->SortIndices = viskores::cont::ArrayHandleIndex(this->NumVertices);
  this->SortOrder = viskores::cont::ArrayHandleIndex(this->NumVertices);
  this->InitializeNeighborConnectivityFromArcs(arcs);
#ifdef DEBUG_PRINT
  // Print the contents fo this for debugging
  DebugPrint("ContourTreeMesh Initialized", __FILE__, __LINE__);
#endif
}

template <typename FieldType>
inline ContourTreeMesh<FieldType>::ContourTreeMesh(const IdArrayType& arcs,
                                                   const ContourTreeMesh<FieldType>& mesh)
  : SortedValues(mesh.SortedValues)
  , GlobalMeshIndex(mesh.GlobalMeshIndex)
  , NeighborConnectivity()
  , NeighborOffsets()
{
  // Initalize the SortedIndices as a smart array handle
  this->NumVertices = this->SortedValues.GetNumberOfValues();
  this->SortIndices = viskores::cont::ArrayHandleIndex(this->NumVertices);
  this->SortOrder = viskores::cont::ArrayHandleIndex(this->NumVertices);
  this->InitializeNeighborConnectivityFromArcs(arcs);
#ifdef DEBUG_PRINT
  // Print the contents fo this for debugging
  DebugPrint("ContourTreeMesh Initialized", __FILE__, __LINE__);
#endif
}


template <typename FieldType>
inline ContourTreeMesh<FieldType>::ContourTreeMesh(const IdArrayType& nodes,
                                                   const IdArrayType& arcs,
                                                   const ContourTreeMesh<FieldType>& mesh)
  : NeighborConnectivity()
  , NeighborOffsets()
{
  // Initatlize the global mesh index with the GlobalMeshIndex permutted by the nodes
  viskores::cont::ArrayHandlePermutation<IdArrayType, IdArrayType> permutedGlobalMeshIndex(
    nodes, mesh.GlobalMeshIndex);
  viskores::cont::Algorithm::Copy(permutedGlobalMeshIndex, this->GlobalMeshIndex);
  // Initialize the SortedValues array with the SortedValues permutted by the nodes
  auto permutedSortedValues = viskores::cont::make_ArrayHandlePermutation(nodes, mesh.SortedValues);
  viskores::cont::Algorithm::Copy(permutedSortedValues, this->SortedValues);
  // Initialize the neighbors from the arcs
  this->NumVertices = this->SortedValues.GetNumberOfValues();
  this->SortIndices = viskores::cont::ArrayHandleIndex(this->NumVertices);
  this->SortOrder = viskores::cont::ArrayHandleIndex(this->NumVertices);
  this->InitializeNeighborConnectivityFromArcs(arcs);
#ifdef DEBUG_PRINT
  // Print the contents fo this for debugging
  DebugPrint("ContourTreeMesh Initialized", __FILE__, __LINE__);
#endif
}

// Helper function to copy select set of indices of source array into
// select set of inidces of destination array. Important: srcIndices
// and  dstIndices must contain the same number of values.
template <typename PT1, typename PT2, typename PT3, typename PT4>
inline void CopyArrayByIndices(const PT1& srcArray,
                               const PT2& srcIndices,
                               PT3& dstArray,
                               const PT4& dstIndices)
{
  VISKORES_ASSERT(srcIndices.GetNumberOfValues() == dstIndices.GetNumberOfValues());
  auto srcPermutation = make_ArrayHandlePermutation(srcIndices, srcArray);
  auto dstPermuation = make_ArrayHandlePermutation(dstIndices, dstArray);
  viskores::cont::Algorithm::Copy(srcPermutation, dstPermuation);
}

// Helper function doing the same as previous function, but for
// arrays of vector. This is necessary since we use an array
// created with ArrayHandleGroupVecVariable as destination,
// which breaks some conventions of ArrayHandle and does not work
// with viskores::cont::Algorithm::Copy.
template <typename PT1, typename PT2, typename PT3, typename PT4>
inline void CopyVecArrayByIndices(const PT1& srcArray,
                                  const PT2& srcIndices,
                                  PT3& dstArray,
                                  const PT4& dstIndices)
{
  VISKORES_ASSERT(srcIndices.GetNumberOfValues() == dstIndices.GetNumberOfValues());
  auto srcPermutation = make_ArrayHandlePermutation(srcIndices, srcArray);
  auto dstPermuation = make_ArrayHandlePermutation(dstIndices, dstArray);
  // Use a worklet for copying data since ArrayHandleGroupVecVariable does
  // not work as destination for viskores::cont::Algorithm::Copy.
  viskores::cont::Invoker invoke;
  invoke(
    contourtree_mesh_inc_ns::CopyIntoCombinedNeighborsWorklet{}, srcPermutation, dstPermuation);
}

// Initalize the contour tree from the arcs array
template <typename FieldType>
inline void ContourTreeMesh<FieldType>::InitializeNeighborConnectivityFromArcs(
  const IdArrayType& arcs)
{
  // This function computes the neighbor connecitvity (NeighborConnectivity, NeighborOffsets) from
  // an arc array. An arc array consists of undirected arcs. arc[i] connects contour tree nodes
  // i and arc[i]. For the neighbor connectiviy in the contour tree mesh, we first convert these
  // into two directed arcs that are then used to compute a list of neighbors in the mesh for
  // each node.
  //
  // Take this simple graph for example:
  //
  /* 4
      \
       \> 3 -> 1 <- 0
       /
      /
     2
    (Use this comment style to avoid warnings about multi-line comments triggered by '\' at
     the end of the line).
  */
  // (This is a graph with nodes 0 through 4 and edges 0 -> 1, 2 -> 3, 3 -> 1, 4 -> 3).
  // The way the graph is structured, each nodes has at most one outgoing edge.
  // The contour tree algorithm stores this in an arcs array:
  //
  // idx:  0 1 2 3 4
  // arcs: 1 - 3 1 3 (- = NO_SUCH_ELEMENT, meaning no arc originating from this node)
  //
  // This function translates this into the internal contour tree mesh represnetation,
  // which is "regular" viskores connectivity format, i.e., the connectity array is a
  // flat list of neighbor vertices and offsets give the start index of the
  // neighbor list for each vertex:
  //
  // connectivity: 1 0 3 3 1 2 4 3
  // counts: 1 2 1 3 1
  // offset: 0 1 3 4 7 8

  // Step 1: Implictely view arc array as directed arcs and add arcs in the opposite
  // direction. In the resulting arc list, arc 2*idx is the arc idx->arcs[idx] and arc
  // 2*idx+1 is the arc arcs[idx]->idx, i.e., in our example,
  // idx:  0 1 2 3 4 5 6 7 8 9
  // from: 0 1 1 - 2 3 3 1 4 3
  // to:   1 0 - 1 3 2 1 3 3 4
  viskores::Id nArcsTotal = 2 * arcs.GetNumberOfValues();
  viskores::cont::ArrayHandleIndex indexArray(nArcsTotal);
  auto arcIsValidArray = make_ArrayHandleDecorator(
    nArcsTotal, mesh_dem_contourtree_mesh_inc::ArcValidDecoratorImpl{}, arcs);
  // We first generate a list of "valid" arcs in this->NeighborConnectivity, in our
  // example:
  // connectivity: 0 1 4 5 6 7 8 9
  viskores::cont::Algorithm::CopyIf(indexArray, arcIsValidArray, this->NeighborConnectivity);
  viskores::Id nValidArcs = this->NeighborConnectivity.GetNumberOfValues();

  // Step 2: Sort arcs---by permuting their indices in the connectiviy array---so
  // that all arcs originating at the same vertex (same `from`) are adjacent.
  // All arcs are in neighbors array based on  sort index of their 'from' vertex
  // (and then within a run sorted by sort index of their 'to' vertex).
  // In our example this results in:
  // connectivity: 0 1 7 4 6 5 9 8
  // corresponding to an arc order of
  // from: 0 1 1 2 3 3 3 4
  // to:   1 0 3 3 1 2 4 3
  viskores::cont::Algorithm::Sort(this->NeighborConnectivity,
                                  contourtree_mesh_inc_ns::ArcComparator(arcs));

  // We can now obtain counts of the connectivity array by counting the number
  // of consecutive `from` entries with the same value. In our example:
  // counts: 1 2 1 3 1
  auto arcFrom = make_ArrayHandleDecorator(nValidArcs,
                                           mesh_dem_contourtree_mesh_inc::GetArcFromDecoratorImpl{},
                                           this->NeighborConnectivity,
                                           arcs);
  auto constOne = viskores::cont::make_ArrayHandleConstant(viskores::Id{ 1 }, nValidArcs);
  viskores::cont::ArrayHandle<viskores::Id> uniqueKeys;
  viskores::cont::ArrayHandle<viskores::Id> counts;
  viskores::cont::Algorithm::ReduceByKey(arcFrom, constOne, uniqueKeys, counts, viskores::Add());
  VISKORES_ASSERT(uniqueKeys.GetNumberOfValues() == this->NumVertices);

  // Convert counts into offsts for the connectivity array
  viskores::Id neighborOffsetsSize;
  viskores::cont::ConvertNumComponentsToOffsets(counts, this->NeighborOffsets, neighborOffsetsSize);

  // Finally, the correct connectivity array correspons to the `to` array,
  // so replace arc indices with its `to`vertex. In our example, this results in:
  // connectivity: 1 0 3 3 1 2 4 3
  // which is exactly the array we needed to compute
  contourtree_mesh_inc_ns::ReplaceArcNumWithToVertexWorklet replaceArcNumWithToVertexWorklet;
  this->Invoke(replaceArcNumWithToVertexWorklet,
               this->NeighborConnectivity, // input/output
               arcs                        // input
  );

  // Compute maximum number of neighbors
  this->ComputeMaxNeighbors();

#ifdef DEBUG_PRINT
  std::cout << std::setw(30) << std::left << __FILE__ << ":" << std::right << std::setw(4)
            << __LINE__ << std::endl;
  auto neighborOffsetPortal = this->NeighborOffsets.ReadPortal();
  auto neighborConnectivityPortal = this->NeighborConnectivity.ReadPortal();
  for (viskores::Id vtx = 0; vtx < NeighborOffsets.GetNumberOfValues(); ++vtx)
  {
    std::cout << vtx << ": ";
    viskores::Id neighboursBeginIndex = neighborOffsetPortal.Get(vtx);
    viskores::Id neighboursEndIndex = (vtx < this->NumVertices - 1)
      ? neighborOffsetPortal.Get(vtx + 1)
      : NeighborConnectivity.GetNumberOfValues();

    for (viskores::Id ni = neighboursBeginIndex; ni < neighboursEndIndex; ++ni)
    {
      std::cout << neighborConnectivityPortal.Get(ni) << " ";
    }
    std::cout << std::endl;
  }
  std::cout << "Max neighbours: " << this->MaxNeighbors << std::endl;
#endif
}

template <typename FieldType>
inline void ContourTreeMesh<FieldType>::ComputeMaxNeighbors()
{
  auto neighborCounts = make_ArrayHandleOffsetsToNumComponents(this->NeighborOffsets);

  viskores::cont::ArrayHandle<viskores::Range> rangeArray =
    viskores::cont::ArrayRangeComputeTemplate(neighborCounts);
  this->MaxNeighbors = static_cast<viskores::Id>(rangeArray.ReadPortal().Get(0).Max);
}

// Define the behavior for the execution object generate by the PrepareForExecution function
template <typename FieldType>
inline void ContourTreeMesh<FieldType>::SetPrepareForExecutionBehavior(bool getMax)
{
  this->mGetMax = getMax;
}

// Get VISKORES execution object that represents the structure of the mesh and provides the mesh helper functions on the device
template <typename FieldType>
contourtree_mesh_inc_ns::MeshStructureContourTreeMesh inline ContourTreeMesh<
  FieldType>::PrepareForExecution(viskores::cont::DeviceAdapterId device,
                                  viskores::cont::Token& token) const
{
  return contourtree_mesh_inc_ns::MeshStructureContourTreeMesh(this->NeighborConnectivity,
                                                               this->NeighborOffsets,
                                                               this->MaxNeighbors,
                                                               this->mGetMax,
                                                               device,
                                                               token);
}

// Helper functor, basically negates criterion for CopyIf
struct IsUnique
{
  VISKORES_EXEC_CONT bool operator()(viskores::IdComponent isInOther) const
  {
    return isInOther == 0;
  }
};

// Combine two ContourTreeMeshes
template <typename FieldType>
inline void ContourTreeMesh<FieldType>::MergeWith(ContourTreeMesh<FieldType>& other,
                                                  viskores::cont::LogLevel timingsLogLevel,
                                                  std::string timingsMessage)
{ // Merge With
#ifdef DEBUG_PRINT
  this->DebugPrint("THIS ContourTreeMesh", __FILE__, __LINE__);
  other.DebugPrint("OTHER ContourTreeMesh", __FILE__, __LINE__);
#endif
  // Track timing of main steps
  viskores::cont::Timer totalTimer; // Total time for each call
  totalTimer.Start();
  viskores::cont::Timer timer; // Time individual steps
  timer.Start();
  std::stringstream timingsStream;

  // Create combined sort order
  // TODO This vector could potentially be implemented purely as a smart array handle to reduce memory usage
  IdArrayType overallSortOrder;
  overallSortOrder.Allocate(this->NumVertices + other.NumVertices);

  { // Create a new scope so that the following two vectors get deleted when leaving the scope
    auto thisIndices = viskores::cont::ArrayHandleIndex(this->NumVertices); // A regular index array
    MarkOther markOtherFunctor;
    auto otherIndices = viskores::cont::make_ArrayHandleTransform(
      viskores::cont::ArrayHandleIndex(other.NumVertices), markOtherFunctor);
    contourtree_mesh_inc_ns::CombinedSimulatedSimplicityIndexComparator<FieldType>
      cssicFunctorExecObj(
        this->GlobalMeshIndex, other.GlobalMeshIndex, this->SortedValues, other.SortedValues);
    contourtree_mesh_inc_ns::CopyIntoCombinedArrayWorklet<true>
      copyIntoCombinedArrayWorkletLowerBound;
    this->Invoke(copyIntoCombinedArrayWorkletLowerBound,
                 thisIndices,
                 otherIndices,
                 cssicFunctorExecObj,
                 overallSortOrder);
    contourtree_mesh_inc_ns::CopyIntoCombinedArrayWorklet<false>
      copyIntoCombinedArrayWorkletUpperBound;
    this->Invoke(copyIntoCombinedArrayWorkletUpperBound,
                 otherIndices,
                 thisIndices,
                 cssicFunctorExecObj,
                 overallSortOrder);
  }
  timingsStream << "    " << std::setw(38) << std::left << "Create OverallSortOrder"
                << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
  timer.Start();

#ifdef DEBUG_PRINT
  std::cout << "OverallSortOrder.size  " << overallSortOrder.GetNumberOfValues() << std::endl;
  PrintIndices("overallSortOrder", overallSortOrder);
  std::cout << std::endl;
#endif

  IdArrayType overallSortIndex;
  overallSortIndex.Allocate(overallSortOrder.GetNumberOfValues());
  {
    // Array decorator with functor returning 0, 1 for each element depending
    // on whethern the current value is different from the next.
    auto differentFromNextArr = viskores::cont::make_ArrayHandleDecorator(
      overallSortOrder.GetNumberOfValues() - 1,
      mesh_dem_contourtree_mesh_inc::CombinedVectorDifferentFromNextDecoratorImpl{},
      overallSortOrder,
      this->GlobalMeshIndex,
      other.GlobalMeshIndex);

    // Compute the extended scan of our transformed combined vector
    viskores::cont::Algorithm::ScanExtended(differentFromNextArr, overallSortIndex);
  }
  viskores::Id numVerticesCombined =
    ArrayGetValue(overallSortIndex.GetNumberOfValues() - 1, overallSortIndex) + 1;

#ifdef DEBUG_PRINT
  std::cout << "OverallSortIndex.size  " << overallSortIndex.GetNumberOfValues() << std::endl;
  PrintIndices("overallSortIndex", overallSortIndex);
  std::cout << "numVerticesCombined: " << numVerticesCombined << std::endl;
  std::cout << std::endl;
#endif
  timingsStream << "    " << std::setw(38) << std::left << "Create OverallSortIndex"
                << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
  timer.Start();

  // thisToCombinedSortOrder and otherToCombinedSortOrder
  IdArrayType thisToCombinedSortOrder;
  thisToCombinedSortOrder.Allocate(this->NumVertices);
  IdArrayType otherToCombinedSortOrder;
  otherToCombinedSortOrder.Allocate(other.NumVertices);
  contourtree_mesh_inc_ns::InitToCombinedSortOrderArraysWorklet
    initToCombinedSortOrderArraysWorklet;
  this->Invoke(initToCombinedSortOrderArraysWorklet,
               overallSortIndex,
               overallSortOrder,
               thisToCombinedSortOrder,
               otherToCombinedSortOrder);

#ifdef DEBUG_PRINT
  PrintIndices("thisToCombinedSortOrder", thisToCombinedSortOrder);
  PrintIndices("otherToCombinedSortOrder", otherToCombinedSortOrder);
#endif
  timingsStream << "    " << std::setw(38) << std::left << "Create This/OtherCombinedSortOrder"
                << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
  timer.Start();

  // Map neighbor IDs to global ID (ID in the combined) and group them
  auto neighborConnectivityGlobalThis =
    make_ArrayHandleDecorator(this->NeighborConnectivity.GetNumberOfValues(),
                              mesh_dem_contourtree_mesh_inc::ApplyLookupTableDecoratorImpl{},
                              this->NeighborConnectivity,
                              thisToCombinedSortOrder);
  auto neighborConnectivityGlobalGroupsThis = viskores::cont::make_ArrayHandleGroupVecVariable(
    neighborConnectivityGlobalThis, this->NeighborOffsets);

  auto neighborConnectivityGlobalOther =
    make_ArrayHandleDecorator(other.NeighborConnectivity.GetNumberOfValues(),
                              mesh_dem_contourtree_mesh_inc::ApplyLookupTableDecoratorImpl{},
                              other.NeighborConnectivity,
                              otherToCombinedSortOrder);
  auto neighborConnectivityGlobalGroupsOther = viskores::cont::make_ArrayHandleGroupVecVariable(
    neighborConnectivityGlobalOther, other.NeighborOffsets);

  // Merge the two neighborhood connecitivy lists. First, we split neighbor connectivity
  // into three groups (i) vertices only in this, (ii) vertices only in other, (iii)
  // vertices in both meshes. We then compute cobmined neighbor connectivity for vertices
  // in both meshes. Finally, we copy them into the combined array.

  // Split vertices into groups (i) uniuqe this, (ii) unique other, (iii) in both
  // ... compute arrays containing 1 if the element is in the other respective array
  viskores::cont::ArrayHandle<viskores::IdComponent> thisToCombinedSortOrderIsDuplicate;
  thisToCombinedSortOrderIsDuplicate.Allocate(thisToCombinedSortOrder.GetNumberOfValues());
  viskores::cont::ArrayHandle<viskores::IdComponent> otherToCombinedSortOrderIsDuplicate;
  otherToCombinedSortOrderIsDuplicate.AllocateAndFill(otherToCombinedSortOrder.GetNumberOfValues(),
                                                      viskores::IdComponent{ 0 });
  this->Invoke(contourtree_mesh_inc_ns::FindDuplicateInOtherWorklet{},
               thisToCombinedSortOrder,
               otherToCombinedSortOrder,
               thisToCombinedSortOrderIsDuplicate,
               otherToCombinedSortOrderIsDuplicate);

#ifdef DEBUG_PRINT
  PrintIndices("thisToCombinedSortOrderIsDuplicate", thisToCombinedSortOrderIsDuplicate);
  PrintIndices("otherToCombinedSortOrderIsDuplicate", otherToCombinedSortOrderIsDuplicate);
#endif
  // ... create lists for all groups to be used to restrict operations to them
  viskores::cont::ArrayHandleIndex indicesThis(thisToCombinedSortOrder.GetNumberOfValues());
  viskores::cont::ArrayHandleIndex indicesOther(otherToCombinedSortOrder.GetNumberOfValues());

  IdArrayType indicesThisUnique, indicesThisDuplicate;
  viskores::cont::Algorithm::CopyIf(
    indicesThis, thisToCombinedSortOrderIsDuplicate, indicesThisUnique, IsUnique{});
  viskores::cont::Algorithm::CopyIf(
    indicesThis, thisToCombinedSortOrderIsDuplicate, indicesThisDuplicate);

#ifdef DEBUG_PRINT
  PrintIndices("indicesThisUnique", indicesThisUnique);
  PrintIndices("indicesThisDuplicate", indicesThisDuplicate);
#endif

  IdArrayType indicesOtherUnique, indicesOtherDuplicate;
  viskores::cont::Algorithm::CopyIf(
    indicesOther, otherToCombinedSortOrderIsDuplicate, indicesOtherUnique, IsUnique{});
  viskores::cont::Algorithm::CopyIf(
    indicesOther, otherToCombinedSortOrderIsDuplicate, indicesOtherDuplicate);

#ifdef DEBUG_PRINT
  PrintIndices("indicesOtherUnique", indicesOtherUnique);
  PrintIndices("indicesOtherDuplicate", indicesOtherDuplicate);
#endif

  VISKORES_ASSERT(indicesThisDuplicate.GetNumberOfValues() ==
                  indicesOtherDuplicate.GetNumberOfValues());

  // Merge the neighbor groups for vertices that occur in both meshes
  // ... compute combined counts (with duplicates)
  auto neighborCountsThis = make_ArrayHandleOffsetsToNumComponents(this->NeighborOffsets);
  auto permutedNeighborCountsThis =
    viskores::cont::make_ArrayHandlePermutation(indicesThisDuplicate, neighborCountsThis);
  auto neighborCountsOther = make_ArrayHandleOffsetsToNumComponents(other.NeighborOffsets);
  auto permutedNeighborCountsOther =
    viskores::cont::make_ArrayHandlePermutation(indicesOtherDuplicate, neighborCountsOther);
  viskores::cont::ArrayHandle<viskores::IdComponent> combinedCommonNeighborCountSums;
  viskores::cont::Algorithm::Transform(permutedNeighborCountsThis,
                                       permutedNeighborCountsOther,
                                       combinedCommonNeighborCountSums,
                                       viskores::Sum());

  // ... merge sorted lists
  // ...... create output arrays/groups
  viskores::Id unpackedCombinedCommonNeighborConnectivitySize;
  IdArrayType unpackedCombinedCommonNeighborOffsets;
  viskores::cont::ConvertNumComponentsToOffsets(combinedCommonNeighborCountSums,
                                                unpackedCombinedCommonNeighborOffsets,
                                                unpackedCombinedCommonNeighborConnectivitySize);
  IdArrayType unpackedCombinedCommonNeighborConnectivity;
  unpackedCombinedCommonNeighborConnectivity.Allocate(
    unpackedCombinedCommonNeighborConnectivitySize);
  auto unpackedCombinedCommonNeighborConnectivityGroups = make_ArrayHandleGroupVecVariable(
    unpackedCombinedCommonNeighborConnectivity, unpackedCombinedCommonNeighborOffsets);

  // ....... create permuted input arrays/groups
  auto permutedNeighborConnectivityGlobalGroupsThis =
    make_ArrayHandlePermutation(indicesThisDuplicate, neighborConnectivityGlobalGroupsThis);
  auto permutedNeighborConnectivityGlobalGroupsOther =
    make_ArrayHandlePermutation(indicesOtherDuplicate, neighborConnectivityGlobalGroupsOther);

  // ........ create array for actual counts of unique neighbors
  viskores::cont::ArrayHandle<viskores::IdComponent> packedCombinedCommonNeighborCounts;
  packedCombinedCommonNeighborCounts.Allocate(combinedCommonNeighborCountSums.GetNumberOfValues());

  // ........ perform merge
  this->Invoke(contourtree_mesh_inc_ns::MergeSortedListsWithoutDuplicatesWorklet{},
               permutedNeighborConnectivityGlobalGroupsThis,
               permutedNeighborConnectivityGlobalGroupsOther,
               unpackedCombinedCommonNeighborConnectivityGroups,
               packedCombinedCommonNeighborCounts);

  // ... pack sorted lists
  // ...... create the new offsets array for the merged groups (without duplicates).
  viskores::Id packedCombinedCommonNeighborConnectivitySize;
  viskores::cont::ArrayHandle<viskores::Id> packedCombinedCommonNeighborOffsets;
  viskores::cont::ConvertNumComponentsToOffsets(packedCombinedCommonNeighborCounts,
                                                packedCombinedCommonNeighborOffsets,
                                                packedCombinedCommonNeighborConnectivitySize);

  // ...... create a new grouped array for the packed connectivity
  viskores::cont::ArrayHandle<viskores::Id> packedCombinedCommonNeighborConnectivity;
  packedCombinedCommonNeighborConnectivity.Allocate(packedCombinedCommonNeighborConnectivitySize);
  auto packedCommonNeighborConnectivityGroups = viskores::cont::make_ArrayHandleGroupVecVariable(
    packedCombinedCommonNeighborConnectivity, packedCombinedCommonNeighborOffsets);

  // ...... copy data to the packed array.
  this->Invoke(contourtree_mesh_inc_ns::CopyNeighborsToPackedArray{},
               unpackedCombinedCommonNeighborConnectivityGroups,
               packedCommonNeighborConnectivityGroups);

  // Create array for all three groups
  // ... create combined counts array
  IdArrayType combinedNeighborCounts;
  combinedNeighborCounts.Allocate(numVerticesCombined);

  auto thisOnlyToCombinedSortOrder =
    make_ArrayHandlePermutation(indicesThisUnique, thisToCombinedSortOrder);
  auto otherOnlyToCombinedSortOrder =
    make_ArrayHandlePermutation(indicesOtherUnique, otherToCombinedSortOrder);
  auto commonToCombinedSortOrder =
    make_ArrayHandlePermutation(indicesThisDuplicate, thisToCombinedSortOrder);

  CopyArrayByIndices(
    neighborCountsThis, indicesThisUnique, combinedNeighborCounts, thisOnlyToCombinedSortOrder);
  CopyArrayByIndices(
    neighborCountsOther, indicesOtherUnique, combinedNeighborCounts, otherOnlyToCombinedSortOrder);
  auto commonCombinedNeighborCounts =
    make_ArrayHandlePermutation(commonToCombinedSortOrder, combinedNeighborCounts);
  viskores::cont::Algorithm::Copy(packedCombinedCommonNeighborCounts, commonCombinedNeighborCounts);

  // ... create offsets and allocate combinedNeighborConnectivity array
  viskores::Id combinedNeighborConnectivitySize;
  viskores::cont::ArrayHandle<viskores::Id> combinedNeighborOffsets;
  viskores::cont::ConvertNumComponentsToOffsets(
    combinedNeighborCounts, combinedNeighborOffsets, combinedNeighborConnectivitySize);
  IdArrayType combinedNeighborConnectivity;
  combinedNeighborConnectivity.Allocate(combinedNeighborConnectivitySize);
  auto combinedNeighborConnectivityGroups =
    make_ArrayHandleGroupVecVariable(combinedNeighborConnectivity, combinedNeighborOffsets);

  // ... copy the connectivity data including  previously merged lists
  CopyVecArrayByIndices(neighborConnectivityGlobalGroupsThis,
                        indicesThisUnique,
                        combinedNeighborConnectivityGroups,
                        thisOnlyToCombinedSortOrder);
  CopyVecArrayByIndices(neighborConnectivityGlobalGroupsOther,
                        indicesOtherUnique,
                        combinedNeighborConnectivityGroups,
                        otherOnlyToCombinedSortOrder);
  auto commonCombinedNeighborConnectivityGroups =
    make_ArrayHandlePermutation(commonToCombinedSortOrder, combinedNeighborConnectivityGroups);
  this->Invoke(contourtree_mesh_inc_ns::CopyIntoCombinedNeighborsWorklet{},
               packedCommonNeighborConnectivityGroups,
               commonCombinedNeighborConnectivityGroups);
  // Why doesn't the following copy work instead?
  // viskores::cont::Algorithm::Copy(packedCommonNeighborConnectivityGroups, commonCombinedNeighborConnectivityGroups);

  timingsStream << "    " << std::setw(38) << std::left << "Compute CombinedNeighborConnectivity"
                << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
  timer.Start();

  // Compute combined global mesh index arrays
  IdArrayType combinedGlobalMeshIndex;
  combinedGlobalMeshIndex.Allocate(numVerticesCombined);
  { // make sure arrays used for copy go out of scope
    auto permutedCombinedGlobalMeshIndex =
      viskores::cont::make_ArrayHandlePermutation(thisToCombinedSortOrder, combinedGlobalMeshIndex);
    viskores::cont::Algorithm::Copy(GlobalMeshIndex, permutedCombinedGlobalMeshIndex);
  }
  { // make sure arrays used for copy go out of scope
    auto permutedCombinedGlobalMeshIndex = viskores::cont::make_ArrayHandlePermutation(
      otherToCombinedSortOrder, combinedGlobalMeshIndex);
    viskores::cont::Algorithm::Copy(other.GlobalMeshIndex, permutedCombinedGlobalMeshIndex);
  }

  timingsStream << "    " << std::setw(38) << std::left << "Create CombinedGlobalMeshIndex"
                << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
  timer.Start();

  // Compute combined sorted values
  viskores::cont::ArrayHandle<FieldType> combinedSortedValues;
  combinedSortedValues.Allocate(numVerticesCombined);
  { // make sure arrays used for copy go out of scope
    auto permutedCombinedSortedValues =
      viskores::cont::make_ArrayHandlePermutation(thisToCombinedSortOrder, combinedSortedValues);
    viskores::cont::Algorithm::Copy(SortedValues, permutedCombinedSortedValues);
  }
  { // make sure arrays used for copy go out of scope
    auto permutedCombinedSortedValues =
      viskores::cont::make_ArrayHandlePermutation(otherToCombinedSortOrder, combinedSortedValues);
    viskores::cont::Algorithm::Copy(other.SortedValues, permutedCombinedSortedValues);
  }

  timingsStream << "    " << std::setw(38) << std::left << "Create CombinedSortedValues"
                << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
  timer.Start();

  // Swap in combined version. VISKORES ArrayHandles are smart so we can just swap in the new for the old
  this->SortedValues = combinedSortedValues;
  this->GlobalMeshIndex = combinedGlobalMeshIndex;
  this->NeighborConnectivity = combinedNeighborConnectivity;
  this->NeighborOffsets = combinedNeighborOffsets;
  this->NumVertices = SortedValues.GetNumberOfValues();
  this->SortIndices = viskores::cont::ArrayHandleIndex(this->NumVertices);
  this->SortOrder = viskores::cont::ArrayHandleIndex(this->NumVertices);

  timingsStream << "    " << std::setw(38) << std::left << "Swap in new arrays"
                << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
  timer.Start();

  // Re-compute maximum number of neigbours
  ComputeMaxNeighbors();

  timingsStream << "    " << std::setw(38) << std::left << "Compute MaxNeighbors"
                << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
  timer.Start();
  timingsStream << "    " << std::setw(38) << std::left << "Total time MergeWith"
                << ": " << totalTimer.GetElapsedTime() << " seconds" << std::endl;
  timer.Start();
  // Record the times we logged
  VISKORES_LOG_S(timingsLogLevel,
                 std::endl
                   << "    ---------------- ContourTreeMesh MergeWith ---------------------"
                   << std::endl
                   << timingsMessage << timingsStream.str());
  // Prevent unused parameter warning when compiled without logging
  (void)timingsLogLevel;
  (void)timingsMessage;

#ifdef DEBUG_PRINT
  // Print the contents fo this for debugging
  DebugPrint("ContourTreeMeshes merged", __FILE__, __LINE__);
#endif
} // Merge With


template <typename FieldType>
inline void ContourTreeMesh<FieldType>::Save(const char* filename) const
{
  std::ofstream os(filename);
  SaveVector(os, this->SortedValues);
  SaveVector(os, this->GlobalMeshIndex);
  SaveVector(os, this->NeighborConnectivity);
  SaveVector(os, this->NeighborOffsets);
}

template <typename FieldType>
inline void ContourTreeMesh<FieldType>::Load(const char* filename)
{
  std::ifstream is(filename);
  if (!is.is_open())
  {
    throw viskores::io::ErrorIO(std::string("Unable to open file: ") + std::string(filename));
  }
  LoadVector(is, this->SortedValues);
  LoadVector(is, this->GlobalMeshIndex);
  LoadVector(is, this->NeighborConnectivity);
  LoadVector(is, this->NeighborOffsets);
  this->ComputeMaxNeighbors();
  this->NumVertices = this->SortedValues.GetNumberOfValues();
  this->SortOrder = viskores::cont::ArrayHandleIndex(this->NumVertices);
  this->SortIndices = viskores::cont::ArrayHandleIndex(this->NumVertices);
}

template <typename FieldType>
template <typename ValueType>
inline void ContourTreeMesh<FieldType>::SaveVector(
  std::ostream& os,
  const viskores::cont::ArrayHandle<ValueType>& vec) const
{
  viskores::Id numVals = vec.GetNumberOfValues();
  //os.write(rXeinterpret_cast<const char*>(&numVals), sizeof(ValueType));
  os << numVals << ": ";
  auto vecPortal = vec.ReadPortal();
  for (viskores::Id i = 0; i < numVals; ++i)
    os << vecPortal.Get(i) << " ";
  //os.write(reinterpret_cast<const char*>(vecPortal.Get(i)), sizeof(ValueType));
  os << std::endl;
}

template <typename FieldType>
template <typename ValueType>
inline void ContourTreeMesh<FieldType>::LoadVector(std::istream& is,
                                                   viskores::cont::ArrayHandle<ValueType>& vec)
{
  viskores::Id numVals;
  is >> numVals;
  char colon = is.get();
  if (colon != ':')
  {
    throw viskores::io::ErrorIO("Error parsing file");
  }

  vec.Allocate(numVals);
  auto vecPortal = vec.WritePortal();
  ValueType val;
  for (viskores::Id i = 0; i < numVals; ++i)
  {
    is >> val;
    vecPortal.Set(i, val);
  }
}

template <typename FieldType>
inline MeshBoundaryContourTreeMeshExec ContourTreeMesh<FieldType>::GetMeshBoundaryExecutionObject(
  viskores::Id3 globalSize,
  viskores::Id3 minIdx,
  viskores::Id3 maxIdx) const
{
  return MeshBoundaryContourTreeMeshExec(this->GlobalMeshIndex, globalSize, minIdx, maxIdx);
}

template <typename FieldType>
inline void ContourTreeMesh<FieldType>::GetBoundaryVertices(
  IdArrayType& boundaryVertexArray,                    // output
  IdArrayType& boundarySortIndexArray,                 // output
  MeshBoundaryContourTreeMeshExec* meshBoundaryExecObj //input
) const
{
  // start by generating a temporary array of indices
  auto indexArray = viskores::cont::ArrayHandleIndex(this->GlobalMeshIndex.GetNumberOfValues());
  // compute the boolean array indicating which values lie on the boundary
  viskores::cont::ArrayHandle<bool> isOnBoundary;
  ComputeMeshBoundaryContourTreeMesh computeMeshBoundaryContourTreeMeshWorklet;
  this->Invoke(computeMeshBoundaryContourTreeMeshWorklet,
               indexArray,           // input
               *meshBoundaryExecObj, // input
               isOnBoundary          // outut
  );

  // we will conditionally copy the boundary vertices' indices, capturing the end iterator to compute the # of boundary vertices
  viskores::cont::Algorithm::CopyIf(indexArray, isOnBoundary, boundaryVertexArray);
  // duplicate these into the index array, since the BRACT uses indices into the underlying mesh anyway
  viskores::cont::Algorithm::Copy(boundaryVertexArray, boundarySortIndexArray);
}

} // namespace contourtree_augmented
} // worklet
} // viskores

//#undef DEBUG_PRINT

#endif
