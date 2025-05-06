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

#ifndef viskores_worklet_ContourTreeUniformAugmented_h
#define viskores_worklet_ContourTreeUniformAugmented_h


#include <sstream>
#include <utility>

// VISKORES includes
#include <viskores/Math.h>
#include <viskores/Types.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleCounting.h>
#include <viskores/cont/Field.h>
#include <viskores/cont/Timer.h>
#include <viskores/worklet/DispatcherMapField.h>
#include <viskores/worklet/WorkletMapField.h>

// Contour tree worklet includes
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/ActiveGraph.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/ContourTree.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/ContourTreeMaker.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/DataSetMesh.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/MergeTree.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/MeshExtrema.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/Types.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/meshtypes/ContourTreeMesh.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/meshtypes/mesh_boundary/MeshBoundary2D.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/meshtypes/mesh_boundary/MeshBoundary3D.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/meshtypes/mesh_boundary/MeshBoundaryContourTreeMesh.h>

namespace viskores
{
namespace worklet
{

/// Compute the contour tree for 2d and 3d uniform grids and arbitrary topology graphs
class ContourTreeAugmented
{
public:
  /*!
  * Log level to be used for outputting timing information. Default is viskores::cont::LogLevel::Perf
  * Use viskores::cont::LogLevel::Off to disable outputing the results via viskores logging here. The
  * results are saved in the TimingsLogString variable so we can use it to do our own logging
  */
  viskores::cont::LogLevel TimingsLogLevel = viskores::cont::LogLevel::Perf;

  /// Remember the results from our time-keeping so we can customize our logging
  std::string TimingsLogString;


  /*!
  * Run the contour tree to merge an existing set of contour trees
  *
  *  fieldArray   : Needed only as a pass-through value but not used in this case
  *  mesh : The ContourTreeMesh for which the contour tree should be computed
  *  contourTree  : The output contour tree to be computed (output)
  *  sortOrder    : The sort order for the mesh vertices (output)
  *  nIterations  : The number of iterations used to compute the contour tree (output)
  *  computeRegularStructure : 0=Off, 1=full augmentation with all vertices
  *                            2=boundary augmentation using meshBoundary
  *  meshBoundary : This parameter is generated by calling mesh.GetMeshBoundaryExecutionObject
  *                 For regular 2D/3D meshes this required no extra parameters, however, for a
  *                 ContourTreeMesh additional information about the block must be given. Rather
  *                 than generating the MeshBoundary descriptor here, we therefore, require it
  *                 as an input. The MeshBoundary is used to augment the contour tree with the
  *                 mesh boundary vertices. It is needed only if we want to augement by the
  *                 mesh boundary and computeRegularStructure is False (i.e., if we compute
  *                 the full regular strucuture this is not needed because all vertices
  *                 (including the boundary) will be addded to the tree anyways.
  */
  template <typename FieldType,
            typename StorageType,
            typename MeshType,
            typename MeshBoundaryMeshExecType>
  void Run(const viskores::cont::ArrayHandle<FieldType, StorageType> fieldArray,
           MeshType& mesh,
           contourtree_augmented::ContourTree& contourTree,
           contourtree_augmented::IdArrayType& sortOrder,
           viskores::Id& nIterations,
           unsigned int computeRegularStructure,
           const MeshBoundaryMeshExecType& meshBoundary)
  {
    RunContourTree(
      fieldArray, // Just a place-holder to fill the required field. Used when calling SortData on the contour tree which is a no-op
      contourTree,
      sortOrder,
      nIterations,
      mesh,
      computeRegularStructure,
      meshBoundary);
    return;
  }

  /*!
   * Run the contour tree analysis. This helper function is used to
   * allow one to run the contour tree in a consistent fashion independent
   * of whether the data is 2D, 3D, or 3D_MC. This function initalizes
   * the approbritate mesh class from the contourtree_augmented worklet
   * and constructs ths mesh boundary exectuion object to be used. It the
   * subsequently calls RunContourTree method to compute the actual contour tree.
   *
   *  fieldArray   : Needed only as a pass-through value but not used in this case
   *  mesh : The ContourTreeMesh for which the contour tree should be computed
   *  contourTree  : The output contour tree to be computed (output)
   *  sortOrder    : The sort order for the mesh vertices (output)
   *  nIterations  : The number of iterations used to compute the contour tree (output)
   *  nRows        : Number of rows (i.e, x values) in the input mesh
   *  nCols        : Number of columns (i.e, y values) in the input mesh
   *  nSlices      : Number of slicex (i.e, z values) in the input mesh. Default is 1
   *                 to avoid having to set the nSlices for 2D input meshes
   *  useMarchingCubes : Boolean indicating whether marching cubes (true) or freudenthal (false)
   *                     connectivity should be used. Valid only for 3D input data. Default is false.
   *  computeRegularStructure : 0=Off, 1=full augmentation with all vertices
   *                            2=boundary augmentation using meshBoundary.
   */
  template <typename FieldType, typename StorageType>
  void Run(const viskores::cont::ArrayHandle<FieldType, StorageType> fieldArray,
           contourtree_augmented::ContourTree& contourTree,
           contourtree_augmented::IdArrayType& sortOrder,
           viskores::Id& nIterations,
           const viskores::Id3 meshSize,
           bool useMarchingCubes = false,
           unsigned int computeRegularStructure = 1)
  {
    using namespace viskores::worklet::contourtree_augmented;
    // 2D Contour Tree
    if (meshSize[2] == 1)
    {
      // Build the mesh and fill in the values
      DataSetMeshTriangulation2DFreudenthal mesh(viskores::Id2{ meshSize[0], meshSize[1] });
      // Run the contour tree on the mesh
      RunContourTree(fieldArray,
                     contourTree,
                     sortOrder,
                     nIterations,
                     mesh,
                     computeRegularStructure,
                     mesh.GetMeshBoundaryExecutionObject());
      return;
    }
    // 3D Contour Tree using marching cubes
    else if (useMarchingCubes)
    {
      // Build the mesh and fill in the values
      DataSetMeshTriangulation3DMarchingCubes mesh(meshSize);
      // Run the contour tree on the mesh
      RunContourTree(fieldArray,
                     contourTree,
                     sortOrder,
                     nIterations,
                     mesh,
                     computeRegularStructure,
                     mesh.GetMeshBoundaryExecutionObject());
      return;
    }
    // 3D Contour Tree with Freudenthal
    else
    {
      // Build the mesh and fill in the values
      DataSetMeshTriangulation3DFreudenthal mesh(meshSize);
      // Run the contour tree on the mesh
      RunContourTree(fieldArray,
                     contourTree,
                     sortOrder,
                     nIterations,
                     mesh,
                     computeRegularStructure,
                     mesh.GetMeshBoundaryExecutionObject());
      return;
    }
  }


private:
  /*!
  *  Run the contour tree for the given mesh. This function implements the main steps for
  *  computing the contour tree after the mesh has been constructed using the approbrite
  *  contour tree mesh class.
  *
  *  fieldArray   : The values of the mesh
  *  contourTree  : The output contour tree to be computed (output)
  *  sortOrder    : The sort order for the mesh vertices (output)
  *  nIterations  : The number of iterations used to compute the contour tree (output)
  *  mesh : The specific mesh (see viskores/worklet/contourtree_augmented/mesh_dem_meshtypes
  *  computeRegularStructure : 0=Off, 1=full augmentation with all vertices
  *                            2=boundary augmentation using meshBoundary
  *  meshBoundary : This parameter is generated by calling mesh.GetMeshBoundaryExecutionObject
  *                 For regular 2D/3D meshes this required no extra parameters, however, for a
  *                 ContourTreeMesh additional information about the block must be given. Rather
  *                 than generating the MeshBoundary descriptor here, we therefore, require it
  *                 as an input. The MeshBoundary is used to augment the contour tree with the
  *                 mesh boundary vertices. It is needed only if we want to augement by the
  *                 mesh boundary and computeRegularStructure is False (i.e., if we compute
  *                 the full regular strucuture this is not needed because all vertices
  *                 (including the boundary) will be addded to the tree anyways.
  */
  template <typename FieldType,
            typename StorageType,
            typename MeshClass,
            typename MeshBoundaryClass>
  void RunContourTree(const viskores::cont::ArrayHandle<FieldType, StorageType> fieldArray,
                      contourtree_augmented::ContourTree& contourTree,
                      contourtree_augmented::IdArrayType& sortOrder,
                      viskores::Id& nIterations,
                      MeshClass& mesh,
                      unsigned int computeRegularStructure,
                      const MeshBoundaryClass& meshBoundary)
  {
    using namespace viskores::worklet::contourtree_augmented;
    // Stage 1: Load the data into the mesh. This is done in the Run() method above and accessible
    //          here via the mesh parameter. The actual data load is performed outside of the
    //          worklet in the example contour tree app (or whoever uses the worklet)

    // Stage 2 : Sort the data on the mesh to initialize sortIndex & indexReverse on the mesh
    // Start the timer for the mesh sort
    viskores::cont::Timer timer;
    timer.Start();
    std::stringstream timingsStream; // Use a string stream to log in one message

    // Sort the mesh data
    mesh.SortData(fieldArray);
    timingsStream << "    " << std::setw(38) << std::left << "Sort Data"
                  << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
    timer.Start();

    // Stage 3: Assign every mesh vertex to a peak
    MeshExtrema extrema(mesh.NumVertices);
    extrema.SetStarts(mesh, true);
    extrema.BuildRegularChains(true);
    timingsStream << "    " << std::setw(38) << std::left << "Join Tree Regular Chains"
                  << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
    timer.Start();

    // Stage 4: Identify join saddles & construct Active Join Graph
    MergeTree joinTree(mesh.NumVertices, true);
    ActiveGraph joinGraph(true);
    joinGraph.Initialise(mesh, extrema);
    timingsStream << "    " << std::setw(38) << std::left << "Join Tree Initialize Active Graph"
                  << ": " << timer.GetElapsedTime() << " seconds" << std::endl;

#ifdef DEBUG_PRINT
    joinGraph.DebugPrint("Active Graph Instantiated", __FILE__, __LINE__);
#endif
    timer.Start();

    // Stage 5: Compute Join Tree Hyperarcs from Active Join Graph
    joinGraph.MakeMergeTree(joinTree, extrema);
    timingsStream << "    " << std::setw(38) << std::left << "Join Tree Compute"
                  << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
#ifdef DEBUG_PRINT
    joinTree.DebugPrint("Join tree Computed", __FILE__, __LINE__);
    joinTree.DebugPrintTree("Join tree", __FILE__, __LINE__, mesh);
#endif
    timer.Start();

    // Stage 6: Assign every mesh vertex to a pit
    extrema.SetStarts(mesh, false);
    extrema.BuildRegularChains(false);
    timingsStream << "    " << std::setw(38) << std::left << "Split Tree Regular Chains"
                  << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
    timer.Start();

    // Stage 7:     Identify split saddles & construct Active Split Graph
    MergeTree splitTree(mesh.NumVertices, false);
    ActiveGraph splitGraph(false);
    splitGraph.Initialise(mesh, extrema);
    timingsStream << "    " << std::setw(38) << std::left << "Split Tree Initialize Active Graph"
                  << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
#ifdef DEBUG_PRINT
    splitGraph.DebugPrint("Active Graph Instantiated", __FILE__, __LINE__);
#endif
    timer.Start();

    // Stage 8: Compute Split Tree Hyperarcs from Active Split Graph
    splitGraph.MakeMergeTree(splitTree, extrema);
    timingsStream << "    " << std::setw(38) << std::left << "Split Tree Compute"
                  << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
#ifdef DEBUG_PRINT
    splitTree.DebugPrint("Split tree Computed", __FILE__, __LINE__);
    // Debug split and join tree
    joinTree.DebugPrintTree("Join tree", __FILE__, __LINE__, mesh);
    splitTree.DebugPrintTree("Split tree", __FILE__, __LINE__, mesh);
#endif
    timer.Start();

    // Stage 9: Join & Split Tree are Augmented, then combined to construct Contour Tree
    contourTree.Init(mesh.NumVertices);
    ContourTreeMaker treeMaker(contourTree, joinTree, splitTree);
    // 9.1 First we compute the hyper- and super- structure
    treeMaker.ComputeHyperAndSuperStructure();
    timingsStream << "    " << std::setw(38) << std::left
                  << "Contour Tree Hyper and Super Structure"
                  << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
    timer.Start();

    // 9.2 Then we compute the regular structure
    if (computeRegularStructure == 1) // augment with all vertices
    {
      treeMaker.ComputeRegularStructure(extrema);
      timingsStream << "    " << std::setw(38) << std::left << "Contour Tree Regular Structure"
                    << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
    }
    else if (computeRegularStructure == 2) // augment by the mesh boundary
    {
      treeMaker.ComputeBoundaryRegularStructure(extrema, mesh, meshBoundary);
      timingsStream << "    " << std::setw(38) << std::left
                    << "Contour Tree Boundary Regular Structure"
                    << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
    }
    timer.Start();

    // Collect the output data
    nIterations = treeMaker.ContourTreeResult.NumIterations;
    //  Need to make a copy of sortOrder since ContourTreeMesh uses a smart array handle
    // TODO: Check if we can just make sortOrder a return array with variable type or if we can make the SortOrder return optional
    // TODO/FIXME: According to Ken Moreland the short answer is no. We may need to go back and refactor this when we
    // improve the contour tree API. https://gitlab.kitware.com/vtk/viskores/-/merge_requests/2263#note_831128 for more details.
    viskores::cont::Algorithm::Copy(mesh.SortOrder, sortOrder);
    // ProcessContourTree::CollectSortedSuperarcs<DeviceAdapter>(contourTree, mesh.SortOrder, saddlePeak);
    // contourTree.SortedArcPrint(mesh.SortOrder);
    // contourTree.PrintDotSuperStructure();

    // Log the collected timing results in one coherent log entry
    this->TimingsLogString = timingsStream.str();
    if (this->TimingsLogLevel != viskores::cont::LogLevel::Off)
    {
      VISKORES_LOG_S(
        this->TimingsLogLevel,
        std::endl
          << "    ------------------- Contour Tree Worklet Timings ----------------------"
          << std::endl
          << this->TimingsLogString);
    }
  }
};

} // namespace viskores
} // namespace viskores::worklet

#endif // viskores_worklet_ContourTreeUniformAugmented_h
