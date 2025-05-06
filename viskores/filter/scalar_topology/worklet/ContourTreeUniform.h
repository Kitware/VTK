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

#ifndef viskores_worklet_ContourTreeUniform_h
#define viskores_worklet_ContourTreeUniform_h

#include <viskores/Math.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleCounting.h>
#include <viskores/cont/Field.h>
#include <viskores/worklet/DispatcherMapField.h>
#include <viskores/worklet/WorkletMapField.h>

// For numerous functions inside contourTree GCC is able to determine if i is
// always greater than j ( or vice-versa ) and optimizes those call sites.
// But when it does these optimizations is presumes that i and j will not
// overflow and emits a Wstrict-overflow warning
#ifdef VISKORES_GCC
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-overflow"
#endif

#include <viskores/filter/scalar_topology/worklet/contourtree/ChainGraph.h>
#include <viskores/filter/scalar_topology/worklet/contourtree/ContourTree.h>
#include <viskores/filter/scalar_topology/worklet/contourtree/MergeTree.h>
#include <viskores/filter/scalar_topology/worklet/contourtree/Mesh2D_DEM_Triangulation.h>
#include <viskores/filter/scalar_topology/worklet/contourtree/Mesh3D_DEM_Triangulation.h>

const bool JOIN = true;
const bool SPLIT = false;
const bool JOIN_3D = true;
const bool SPLIT_3D = false;

namespace viskores
{
namespace worklet
{

class ContourTreeMesh2D
{
public:
  template <typename FieldType, typename StorageType>
  void Run(const viskores::cont::ArrayHandle<FieldType, StorageType> fieldArray,
           const viskores::Id nRows,
           const viskores::Id nCols,
           viskores::cont::ArrayHandle<viskores::Pair<viskores::Id, viskores::Id>>& saddlePeak)
  {
    viskores::Id nSlices = 1;

    // Build the mesh and fill in the values
    contourtree::Mesh2D_DEM_Triangulation<FieldType, StorageType> mesh(fieldArray, nRows, nCols);

    // Initialize the join tree so that all arcs point to maxima
    contourtree::MergeTree<FieldType, StorageType> joinTree(
      fieldArray, nRows, nCols, nSlices, JOIN);
    mesh.SetStarts(joinTree.extrema, JOIN);
    joinTree.BuildRegularChains();

    // Create the active topology graph from the regular graph
    contourtree::ChainGraph<FieldType, StorageType> joinGraph(fieldArray, joinTree.extrema, JOIN);
    mesh.SetSaddleStarts(joinGraph, JOIN);

    // Call join graph to finish computation
    joinGraph.Compute(joinTree.saddles);

    // Initialize the split tree so that all arcs point to maxima
    contourtree::MergeTree<FieldType, StorageType> splitTree(
      fieldArray, nRows, nCols, nSlices, SPLIT);
    mesh.SetStarts(splitTree.extrema, SPLIT);
    splitTree.BuildRegularChains();

    // Create the active topology graph from the regular graph
    contourtree::ChainGraph<FieldType, StorageType> splitGraph(
      fieldArray, splitTree.extrema, SPLIT);
    mesh.SetSaddleStarts(splitGraph, SPLIT);

    // Call split graph to finish computation
    splitGraph.Compute(splitTree.saddles);

    // Now compute the contour tree
    contourtree::ContourTree<FieldType, StorageType> contourTree(
      fieldArray, joinTree, splitTree, joinGraph, splitGraph);

    contourTree.CollectSaddlePeak(saddlePeak);
  }
};

class ContourTreeMesh3D
{
public:
  template <typename FieldType, typename StorageType>
  void Run(const viskores::cont::ArrayHandle<FieldType, StorageType> fieldArray,
           const viskores::Id nRows,
           const viskores::Id nCols,
           const viskores::Id nSlices,
           viskores::cont::ArrayHandle<viskores::Pair<viskores::Id, viskores::Id>>& saddlePeak)
  {
    // Build the mesh and fill in the values
    contourtree::Mesh3D_DEM_Triangulation<FieldType, StorageType> mesh(
      fieldArray, nRows, nCols, nSlices);

    // Initialize the join tree so that all arcs point to maxima
    contourtree::MergeTree<FieldType, StorageType> joinTree(
      fieldArray, nRows, nCols, nSlices, JOIN_3D);
    mesh.SetStarts(joinTree.extrema, JOIN_3D);
    joinTree.BuildRegularChains();

    // Create the active topology graph from the regular graph
    contourtree::ChainGraph<FieldType, StorageType> joinGraph(
      fieldArray, joinTree.extrema, JOIN_3D);
    mesh.SetSaddleStarts(joinGraph, JOIN_3D);

    // Call join graph to finish computation
    joinGraph.Compute(joinTree.saddles);

    // Initialize the split tree so that all arcs point to maxima
    contourtree::MergeTree<FieldType, StorageType> splitTree(
      fieldArray, nRows, nCols, nSlices, SPLIT_3D);
    mesh.SetStarts(splitTree.extrema, SPLIT_3D);
    splitTree.BuildRegularChains();

    // Create the active topology graph from the regular graph
    contourtree::ChainGraph<FieldType, StorageType> splitGraph(
      fieldArray, splitTree.extrema, SPLIT_3D);
    mesh.SetSaddleStarts(splitGraph, SPLIT_3D);

    // Call split graph to finish computation
    splitGraph.Compute(splitTree.saddles);

    // Now compute the contour tree
    contourtree::ContourTree<FieldType, StorageType> contourTree(
      fieldArray, joinTree, splitTree, joinGraph, splitGraph);

    contourTree.CollectSaddlePeak(saddlePeak);
  }
};
}
} // namespace viskores::worklet

#ifdef VISKORES_GCC
#pragma GCC diagnostic pop
#endif

#endif // viskores_worklet_ContourTreeUniform_h
