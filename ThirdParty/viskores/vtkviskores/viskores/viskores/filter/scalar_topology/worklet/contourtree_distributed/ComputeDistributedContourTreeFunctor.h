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

#ifndef viskores_worklet_contourtree_distributed_computedistributedcontourtreefunctor_h
#define viskores_worklet_contourtree_distributed_computedistributedcontourtreefunctor_h

#include <viskores/Types.h>
#include <viskores/cont/Error.h>
#include <viskores/filter/scalar_topology/worklet/ContourTreeUniformAugmented.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/Types.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/DistributedContourTreeBlockData.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/PrintGraph.h>

// clang-format off
VISKORES_THIRDPARTY_PRE_INCLUDE
#include <viskores/thirdparty/diy/diy.h>
VISKORES_THIRDPARTY_POST_INCLUDE
// clang-format on


namespace viskores
{
namespace worklet
{
namespace contourtree_distributed
{

/// Functor used by DIY reduce the merge data blocks in parallel
template <typename FieldType>
class ComputeDistributedContourTreeFunctor
{
public:
  /// Create the functor
  /// @param[in] globalSize  Global extents of the input mesh (i.e., number of mesh points in each dimension).
  /// @param[in] useBoundaryExtremaOnly Use boundary extrema only (instead of the full boundary) during the fan in.
  /// @param[in] timingsLogLevel Set the viskores::cont:LogLevel to be used to record timings information
  ///                            specific to the computation of the hierachical contour tree.
  /// @param[in] treeLogLevel Set the viskores::cont:LogLevel to be used to record metadata information
  ///                         about the various trees computed as part of the hierarchical contour tree compute.
  ComputeDistributedContourTreeFunctor(
    viskores::Id3 globalSize,
    bool useBoundaryExtremaOnly,
    viskores::cont::LogLevel timingsLogLevel = viskores::cont::LogLevel::Perf,
    viskores::cont::LogLevel treeLogLevel = viskores::cont::LogLevel::Info)
    : GlobalSize(globalSize)
    , UseBoundaryExtremaOnly(useBoundaryExtremaOnly)
    , TimingsLogLevel(timingsLogLevel)
    , TreeLogLevel(treeLogLevel)
  {
  }

  /// Operator used by DIY to compute a step in the fan in
  /// @param[in] block The local data block to be processed in this step. Instance of DistributedContourTreeBlockData.
  /// @param[in] rp DIY communication proxy
  // @param[in] unused partners of the current block (unused)
  void operator()(
    viskores::worklet::contourtree_distributed::DistributedContourTreeBlockData<FieldType>* block,
    const viskoresdiy::ReduceProxy& rp,
    const viskoresdiy::RegularSwapPartners&) const
  {
    // Track timing of main steps
    viskores::cont::Timer totalTimer; // Total time for each call
    totalTimer.Start();
    viskores::cont::Timer timer; // Time individual steps
    timer.Start();
    std::stringstream timingsStream;

    // Get our rank and DIY id
    const viskores::Id rank = viskores::cont::EnvironmentTracker::GetCommunicator().rank();
    const auto selfid = rp.gid();

    // Here we do the deque first before the send due to the way the iteration is handled in DIY, i.e., in each iteration
    // A block needs to first collect the data from its neighours and then send the combined block to its neighbours
    // for the next iteration.
    // 1. dequeue the block and compute the new contour tree and contour tree mesh for the block if we have the hight GID
    std::vector<int> incoming;
    rp.incoming(incoming);
    // log the time for getting the data from DIY
    timingsStream << "    " << std::setw(38) << std::left << "DIY Incoming Data"
                  << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
    timer.Start();

    // Compute the joint contour tree
    for (const int ingid : incoming)
    {
      // NOTE/IMPORTANT: In each round we should have only one swap partner (despite for-loop here).
      // If that assumption does not hold, it will break things.
      // NOTE/IMPORTANT: This assumption only holds if the number of blocks is a power of two.
      // Otherwise, we may need to process more than one incoming block
      if (ingid != selfid)
      {
        viskores::cont::Timer loopTimer; // time the steps of this loop
        loopTimer.Start();

        viskores::Id3 otherBlockOrigin;
        rp.dequeue(ingid, otherBlockOrigin);
        viskores::Id3 otherBlockSize;
        rp.dequeue(ingid, otherBlockSize);
        viskores::worklet::contourtree_augmented::ContourTreeMesh<FieldType> otherContourTreeMesh;
        rp.dequeue(ingid, otherContourTreeMesh);

        timingsStream << "      Subphase of Merge Block" << std::endl;
        timingsStream << "        |-->" << std::setw(38) << std::left << "DIY Deque Data"
                      << ": " << loopTimer.GetElapsedTime() << " seconds" << std::endl;
        loopTimer.Start();

#ifdef DEBUG_PRINT_CTUD
        VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                       "Local block has extents: " << block->BlockOrigin << " " << block->BlockSize
                                                   << std::endl
                                                   << "Combining with block received from ID "
                                                   << ingid << " with extents: " << otherBlockOrigin
                                                   << " " << otherBlockSize << std::endl);
#endif

        // Merge the two contour tree meshes
        std::stringstream mergeMessageStream;
        mergeMessageStream << "    Rank    : " << rank << std::endl
                           << "    DIY Id  : " << selfid << std::endl
                           << "    Other Id: " << ingid << std::endl
                           << "    Round   : " << rp.round() << std::endl;
        block->ContourTreeMeshes.back().MergeWith(
          otherContourTreeMesh, this->TimingsLogLevel, mergeMessageStream.str());

        timingsStream << "        |-->" << std::setw(38) << std::left << "Merge Contour Tree Mesh"
                      << ": " << loopTimer.GetElapsedTime() << " seconds" << std::endl;
        loopTimer.Start();

#ifdef DEBUG_PRINT_CTUD
        // save the corresponding .gv file for the contour tree mesh
        std::string contourTreeMeshFileName = std::string("Rank_") +
          std::to_string(static_cast<int>(rank)) + std::string("_Block_") +
          std::to_string(static_cast<int>(block->LocalBlockNo)) + "_Round_" +
          std::to_string(rp.round()) + "_Partner_" + std::to_string(ingid) +
          std::string("_Step_0_Combined_Mesh.gv");
        std::string contourTreeMeshLabel = std::string("Block ") +
          std::to_string(static_cast<int>(block->LocalBlockNo)) + " Round " +
          std::to_string(rp.round()) + " Partner " + std::to_string(ingid) +
          std::string(" Step 0 Combined Mesh");
        std::string contourTreeMeshString =
          viskores::worklet::contourtree_distributed::ContourTreeMeshDotGraphPrint<FieldType>(
            contourTreeMeshLabel,
            block->ContourTreeMeshes.back(),
            worklet::contourtree_distributed::SHOW_CONTOUR_TREE_MESH_ALL);
        std::ofstream contourTreeMeshFile(contourTreeMeshFileName);
        contourTreeMeshFile << contourTreeMeshString;
        timingsStream << "        |-->" << std::setw(38) << std::left
                      << "Save Contour Tree Mesh Dot"
                      << ": " << loopTimer.GetElapsedTime() << " seconds" << std::endl;
        loopTimer.Start();
#endif

        // Compute the origin and size of the new block
        viskores::Id3 currBlockOrigin{
          std::min(otherBlockOrigin[0], block->BlockOrigin[0]),
          std::min(otherBlockOrigin[1], block->BlockOrigin[1]),
          std::min(otherBlockOrigin[2], block->BlockOrigin[2]),
        };
        viskores::Id3 currBlockMaxIndex{ // Needed only to compute the block size
                                         std::max(otherBlockOrigin[0] + otherBlockSize[0],
                                                  block->BlockOrigin[0] + block->BlockSize[0]),
                                         std::max(otherBlockOrigin[1] + otherBlockSize[1],
                                                  block->BlockOrigin[1] + block->BlockSize[1]),
                                         std::max(otherBlockOrigin[2] + otherBlockSize[2],
                                                  block->BlockOrigin[2] + block->BlockSize[2])
        };
        viskores::Id3 currBlockSize{ currBlockMaxIndex[0] - currBlockOrigin[0],
                                     currBlockMaxIndex[1] - currBlockOrigin[1],
                                     currBlockMaxIndex[2] - currBlockOrigin[2] };

        // Compute the contour tree from our merged mesh
        viskores::Id currNumIterations;
        block->ContourTrees.emplace_back(); // Create new empty contour tree object
        viskores::worklet::contourtree_augmented::IdArrayType currSortOrder;
        viskores::worklet::ContourTreeAugmented worklet;
        worklet.TimingsLogLevel =
          viskores::cont::LogLevel::Off; // disable the print logging, we'll print this later
        viskores::Id3 maxIdx{ currBlockOrigin[0] + currBlockSize[0] - 1,
                              currBlockOrigin[1] + currBlockSize[1] - 1,
                              currBlockOrigin[2] + currBlockSize[2] - 1 };
        auto meshBoundaryExecObj = block->ContourTreeMeshes.back().GetMeshBoundaryExecutionObject(
          this->GlobalSize, currBlockOrigin, maxIdx);
        try
        {
          worklet.Run(block->ContourTreeMeshes.back()
                        .SortedValues, // Unused param. Provide something to keep the API happy
                      block->ContourTreeMeshes.back(),
                      block->ContourTrees.back(),
                      currSortOrder,
                      currNumIterations,
                      1, // Fully augmented
                      meshBoundaryExecObj);
        }
        // In case the contour tree got stuck, expand the debug information from
        // the message to check whether we combined bad blocks
        catch (const viskores::cont::ErrorInternal& ex)
        {
          std::stringstream ex_message;
          ex_message << ex.what();
          ex_message << " Self/In DIY Id=(" << selfid << ", " << ingid << ")";
          ex_message << " Rank=" << rank << " Round=" << rp.round();
          ex_message << " Origin Self=(" << block->BlockOrigin[0] << ", " << block->BlockOrigin[1]
                     << ", " << block->BlockOrigin[2] << ")";
          ex_message << " Origin In=(" << otherBlockOrigin[0] << ", " << otherBlockOrigin[1] << ", "
                     << otherBlockOrigin[2] << ")";
          ex_message << " Origin Comb=(" << currBlockOrigin[0] << ", " << currBlockOrigin[1] << ", "
                     << currBlockOrigin[2] << ")";
          ex_message << " Size Self=(" << block->BlockSize[0] << ", " << block->BlockSize[1] << ", "
                     << block->BlockSize[2] << ")";
          ex_message << " Size In=(" << otherBlockSize[0] << ", " << otherBlockSize[1] << ", "
                     << otherBlockSize[2] << ")";
          ex_message << " Size Comb=(" << currBlockSize[0] << ", " << currBlockSize[1] << ", "
                     << currBlockSize[2] << ")";
          std::throw_with_nested(viskores::cont::ErrorInternal(ex_message.str()));
        }

        // Update block extents
        block->BlockOrigin = currBlockOrigin;
        block->BlockSize = currBlockSize;

        timingsStream << "        |-->" << std::setw(38) << std::left
                      << "Compute Joint Contour Tree"
                      << ": " << loopTimer.GetElapsedTime() << " seconds" << std::endl;
        loopTimer.Start();

#ifdef DEBUG_PRINT_CTUD
        /*
        // TODO: GET THIS COMPILING. NEED TO LIKELY PUT THIS IN A SEPARATE FUNCTION TO GET THE STORAGE TYPE TEMPLATE PARAMETER
        // TODO/FIXME: At this time we should only be dealing with contour tree meshes and not possibly other mesh types,
        // and block does not have a Meshes member. Shouldn't this all be ContourTreeMesh instead?
        // and the ones for the contour tree regular and superstructures
        std::string regularStructureFileName = std::string("Rank_") +
          std::to_string(static_cast<int>(rank)) + std::string("_Block_") +
          std::to_string(static_cast<int>(block->LocalBlockNo)) + "_Round_" +
          std::to_string(rp.round()) + " Partner " + std::to_string(ingid) +
          std::string("_Step_1_Contour_Tree_Regular_Structure.gv");
        std::string regularStructureLabel = std::string("Block ") +
          std::to_string(static_cast<int>(block->LocalBlockNo)) + " Round " +
          std::to_string(rp.round()) + " Partner " + std::to_string(ingid) +
          std::string(" Step 1 Contour Tree Regular Structure");
        std::string regularStructureString =
                      worklet::contourtree_distributed::ContourTreeDotGraphPrint < FieldType,
                    MeshType,
                    viskores::worklet::contourtree_augmented::IdArrayType()(
                      regularStructureLabel,
                      block->Meshes.back(),
                      block->ContourTrees.back(),
                      worklet::contourtree_distributed::SHOW_REGULAR_STRUCTURE |
                        worklet::contourtree_distributed::SHOW_ALL_IDS);
        std::ofstream regularStructureFile(regularStructureFileName);
        regularStructureFile << regularStructureString;

        std::string superStructureFileName = std::string("Rank_") +
          std::to_string(static_cast<int>(rank)) + std::string("_Block_") +
          std::to_string(static_cast<int>(block->LocalBlockNo)) + "_Round_" +
          std::to_string(rp.round()) + " Partner " + std::to_string(ingid) +
          std::string("_Step_2_Contour_Tree_Super_Structure.gv");
        std::ofstream superStructureFile(superStructureFileName);
        superStructureFile << worklet::contourtree_distributed::ContourTreeDotGraphPrint < T,
          MeshType,
          viskores::worklet::contourtree_augmented::IdArrayType()(
            std::string("Block ") + std::to_string(static_cast<int>(block->LocalBlockNo)) +
              " Round " + std::to_string(rp.round()) + " Partner " + std::to_string(ingid) +
              std::string(" Step 2 Contour Tree Super Structure"),
            block->Meshes.back(),
            block->ContourTrees.back(),
            worklet::contourtree_distributed::SHOW_SUPER_STRUCTURE |
              worklet::contourtree_distributed::SHOW_HYPER_STRUCTURE |
              worklet::contourtree_distributed::SHOW_ALL_IDS |
              worklet::contourtree_distributed::SHOW_ALL_SUPERIDS |
              worklet::contourtree_distributed::SHOW_ALL_HYPERIDS);
        */
#endif

        // Log the contour tree timiing stats
        (void)rank; // Suppress unused variable warning if logging is disabled.
        VISKORES_LOG_S(this->TimingsLogLevel,
                       std::endl
                         << "    ---------------- Contour Tree Worklet Timings ------------------"
                         << std::endl
                         << "    Rank    : " << rank << std::endl
                         << "    DIY Id  : " << selfid << std::endl
                         << "    In Id   : " << ingid << std::endl
                         << "    Round   : " << rp.round() << std::endl
                         << worklet.TimingsLogString);
        // Log the contour tree size stats
        VISKORES_LOG_S(this->TreeLogLevel,
                       std::endl
                         << "    ---------------- Contour Tree Array Sizes ---------------------"
                         << std::endl
                         << "    Rank    : " << rank << std::endl
                         << "    DIY Id  : " << selfid << std::endl
                         << "    In Id   : " << ingid << std::endl
                         << "    Round   : " << rp.round() << std::endl
                         << block->ContourTrees.back().PrintArraySizes());

      } // end if (ingid != selfid)
    }   // end for

    // log the time needed to compute the local contour tree
    timingsStream << "    " << std::setw(38) << std::left << "Merge Block (Compute Joint Tree)"
                  << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
    timer.Start();

    // If we are not in the first round (contour tree mesh for that round was pre-computed
    // in filter outside functor) and if we are sending to someone else (i.e., not in
    // last round) then compute contour tree mesh to send and save it.
    if (rp.round() != 0 && rp.out_link().size() != 0)
    {
      viskores::Id3 maxIdx{ block->BlockOrigin[0] + block->BlockSize[0] - 1,
                            block->BlockOrigin[1] + block->BlockSize[1] - 1,
                            block->BlockOrigin[2] + block->BlockSize[2] - 1 };

      // Compute BRACT
      viskores::worklet::contourtree_distributed::BoundaryTree boundaryTree;
      // ... Get the mesh boundary object
      auto meshBoundaryExecObj = block->ContourTreeMeshes.back().GetMeshBoundaryExecutionObject(
        this->GlobalSize, block->BlockOrigin, maxIdx);
      // Make the BRACT and InteriorForest (i.e., residue)
      block->InteriorForests.emplace_back();
      auto boundaryTreeMaker = viskores::worklet::contourtree_distributed::BoundaryTreeMaker<
        viskores::worklet::contourtree_augmented::ContourTreeMesh<FieldType>,
        viskores::worklet::contourtree_augmented::MeshBoundaryContourTreeMeshExec>(
        &(block->ContourTreeMeshes.back()),
        meshBoundaryExecObj,
        block->ContourTrees.back(),
        &boundaryTree,
        &(block->InteriorForests.back()));
      // Construct the BRACT and InteriorForest. Since we are working on a ContourTreeMesh we do
      // not need to provide and IdRelabeler here in order to compute the InteriorForest
      boundaryTreeMaker.Construct(nullptr, this->UseBoundaryExtremaOnly);
      // Construct contour tree mesh from BRACT
      block->ContourTreeMeshes.emplace_back(
        boundaryTree.VertexIndex, boundaryTree.Superarcs, block->ContourTreeMeshes.back());

#ifdef DEBUG_PRINT_CTUD
      /*
      // TODO: GET THIS COMPILING.
      // TODO/FIXME: Need to get inggid here somehow
      // save the Boundary Tree as a dot file
      std::string boundaryTreeFileName = std::string("Rank_") +
        std::to_string(static_cast<int>(rank)) + std::string("_Block_") +
        std::to_string(static_cast<int>(block->LocalBlockNo)) + "_Round_" +
        std::to_string(rp.round()) + "_Partner_" + std::to_string(ingid) +
        std::string("_Step_3_Boundary_Tree.gv");
      std::ofstream boundaryTreeFile(boundaryTreeFileName);
      boundaryTreeFile << viskores::worklet::contourtree_distributed::BoundaryTreeDotGraphPrint
        (std::string("Block ") + std::to_string(static_cast<int>(block->LocalBlockNo)) + " Round " +
         std::to_string(rp.round()) + " Partner " + std::to_string(ingid) +
         std::string(" Step 3 Boundary Tree"),
         block->Meshes.back()],
         block->BoundaryTrees.back());

      // and save the Interior Forest as another dot file
      std::string interiorForestFileName = std::string("Rank_") +
        std::to_string(static_cast<int>(rank)) + std::string("_Block_") +
        std::to_string(static_cast<int>(block->LocalBlockNo)) + "_Round_" +
        std::to_string(rp.round()) + "_Partner_" + std::to_string(ingid) +
        std::string("_Step_4_Interior_Forest.gv");
      std::ofstream interiorForestFile(interiorForestFileName);
      interiorForestFileName << InteriorForestDotGraphPrintFile<MeshType>(
        std::string("Block ") + std::to_string(static_cast<int>(block->LocalBlockNo)) + " Round " +
          std::to_string(rp.round()) + " Partner " + std::to_string(ingid) +
          std::string(" Step 4 Interior Forest"),
        block->InteriorForests.back(),
        block->ContourTrees.back(),
        block->BoundaryTrees.back(),
        block->Meshes.back());

      // save the corresponding .gv file
      std::string boundaryTreeMeshFileName = std::string("Rank_") +
        std::to_string(static_cast<int>(rank)) + std::string("_Block_") +
        std::to_string(static_cast<int>(block->LocalBlockNo)) + "_Round_" +
        std::to_string(rp.round()) + "_Partner_" + std::to_string(ingid) +
        std::string("_Step_5_Boundary_Tree_Mesh.gv");
      std::ofstream boundaryTreeMeshFile(boundaryTreeMeshFileName);
      boundaryTreeMeshFile
        << viskores::worklet::contourtree_distributed::ContourTreeMeshDotGraphPrint<FieldType>(
             std::string("Block ") + std::to_string(static_cast<int>(block->LocalBlockNo)) +
               " Round " + std::to_string(rp.round()) + " Partner " + std::to_string(ingid) +
               std::string(" Step 5 Boundary Tree Mesh"),
             block->ContourTreeMeshes.back(),
             worklet::contourtree_distributed::SHOW_CONTOUR_TREE_MESH_ALL);
      */
#endif

      // Log the boundary tree size statistics
      VISKORES_LOG_S(this->TreeLogLevel,
                     std::endl
                       << "    ---------------- Boundary Tree Array Sizes ---------------------"
                       << std::endl
                       << "    Rank    : " << rank << std::endl
                       << "    DIY Id  : " << selfid << std::endl
                       << "    Round   : " << rp.round() << std::endl
                       << boundaryTree.PrintArraySizes());
      // Log the interior forest statistics
      VISKORES_LOG_S(this->TreeLogLevel,
                     std::endl
                       << "    ---------------- Interior Forest Array Sizes ---------------------"
                       << std::endl
                       << "    Rank    : " << rank << std::endl
                       << "    DIY Id  : " << selfid << std::endl
                       << "    Round   : " << rp.round() << std::endl
                       << block->InteriorForests.back().PrintArraySizes());
    } // end if (rp.round() != 0 && rp.out_link().size() != 0)

    // log the time to compute the boundary tree, interior forest, and contour tree mesh, i.e, the data we need to send
    timingsStream << "    " << std::setw(38) << std::left << "Compute Trees To Send"
                  << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
    timer.Start();


    // Send our current block (which is either our original block or the one we just combined from the ones we received) to our next neighbour.
    // Once a rank has send his block (either in its orignal or merged form) it is done with the reduce
    for (int cc = 0; cc < rp.out_link().size(); ++cc)
    {
      auto target = rp.out_link().target(cc);
      if (target.gid != selfid)
      {
        rp.enqueue(target, block->BlockOrigin);
        rp.enqueue(target, block->BlockSize);
        rp.enqueue(target, block->ContourTreeMeshes.back());
        VISKORES_LOG_S(this->TreeLogLevel,
                       std::endl
                         << "FanInEnqueue: Rank=" << rank << "; Round=" << rp.round()
                         << "; DIY Send Id=" << selfid << "; DIY Target ID=" << target.gid
                         << std::endl);
      }
    } // end for

    // Log the time for enqueue the data for sending via DIY
    timingsStream << "    " << std::setw(38) << std::left << "DIY Enqueue Data"
                  << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
    // Log the total this functor call step took
    timingsStream << "    " << std::setw(38) << std::left << "Total Time Functor Step"
                  << ": " << totalTimer.GetElapsedTime() << " seconds" << std::endl;
    // Record the times we logged
    VISKORES_LOG_S(this->TimingsLogLevel,
                   std::endl
                     << "    ---------------- Fan In Functor Step ---------------------"
                     << std::endl
                     << "    Rank    : " << rank << std::endl
                     << "    DIY Id  : " << selfid << std::endl
                     << "    Round   : " << rp.round() << std::endl
                     << timingsStream.str());

  } //end ComputeDistributedContourTreeFunctor


private:
  /// Extends of the global mesh
  viskores::Id3 GlobalSize;

  /// Use boundary extrema only (instead of the full boundary) during the fan in
  bool UseBoundaryExtremaOnly;

  /// Log level to be used for outputting timing information. Default is viskores::cont::LogLevel::Perf
  viskores::cont::LogLevel TimingsLogLevel = viskores::cont::LogLevel::Perf;

  /// Log level to be used for outputting metadata about the trees. Default is viskores::cont::LogLevel::Info
  viskores::cont::LogLevel TreeLogLevel = viskores::cont::LogLevel::Info;
};


} // namespace contourtree_distributed
} // namespace worklet
} // namespace viskores

#endif
