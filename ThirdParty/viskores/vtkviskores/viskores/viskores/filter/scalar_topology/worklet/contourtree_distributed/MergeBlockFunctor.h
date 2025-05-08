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

#ifndef viskores_worklet_contourtree_distributed_mergeblockfunctor_h
#define viskores_worklet_contourtree_distributed_mergeblockfunctor_h

#include <viskores/Types.h>
#include <viskores/cont/ArrayHandleIndex.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/Types.h>

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

// Functor used by DIY reduce the merge data blocks in parallel
template <typename FieldType>
void MergeBlockFunctor(
  viskores::worklet::contourtree_distributed::ContourTreeBlockData<FieldType>*
    block,                                          // local Block.
  const viskoresdiy::ReduceProxy& rp,               // communication proxy
  const viskoresdiy::RegularMergePartners& partners // partners of the current block
)
{                 //MergeBlockFunctor
  (void)partners; // Avoid unused parameter warning

  const auto selfid = rp.gid();

  // TODO This should be changed so that we have the ContourTree itself as the block and then the
  //      ContourTreeMesh would still be used for exchange. In this case we would need to compute
  //      the ContourTreeMesh at the beginning of the function for the current block every time
  //      but then we would not need to compute those meshes when we initialize viskoresdiy
  //      and we don't need to have the special case for rank 0.

  // Here we do the deque first before the send due to the way the iteration is handled in DIY, i.e., in each iteration
  // A block needs to first collect the data from its neighours and then send the combined block to its neighbours
  // for the next iteration.
  // 1. dequeue the block and compute the new contour tree and contour tree mesh for the block if we have the hight GID
  std::vector<int> incoming;
  rp.incoming(incoming);
  for (const int ingid : incoming)
  {
    if (ingid != selfid)
    {
      viskores::worklet::contourtree_distributed::ContourTreeBlockData<FieldType> recvblock;
      rp.dequeue(ingid, recvblock);

      // Construct the two contour tree mesh by assignign the block data
      viskores::worklet::contourtree_augmented::ContourTreeMesh<FieldType> contourTreeMeshIn;
      contourTreeMeshIn.NumVertices = recvblock.NumVertices;
      contourTreeMeshIn.SortOrder = viskores::cont::ArrayHandleIndex(contourTreeMeshIn.NumVertices);
      contourTreeMeshIn.SortIndices =
        viskores::cont::ArrayHandleIndex(contourTreeMeshIn.NumVertices);
      contourTreeMeshIn.SortedValues = recvblock.SortedValue;
      contourTreeMeshIn.GlobalMeshIndex = recvblock.GlobalMeshIndex;
      contourTreeMeshIn.NeighborConnectivity = recvblock.NeighborConnectivity;
      contourTreeMeshIn.NeighborOffsets = recvblock.NeighborOffsets;
      contourTreeMeshIn.MaxNeighbors = recvblock.MaxNeighbors;

      viskores::worklet::contourtree_augmented::ContourTreeMesh<FieldType> contourTreeMeshOut;
      contourTreeMeshOut.NumVertices = block->NumVertices;
      contourTreeMeshOut.SortOrder =
        viskores::cont::ArrayHandleIndex(contourTreeMeshOut.NumVertices);
      contourTreeMeshOut.SortIndices =
        viskores::cont::ArrayHandleIndex(contourTreeMeshOut.NumVertices);
      contourTreeMeshOut.SortedValues = block->SortedValue;
      contourTreeMeshOut.GlobalMeshIndex = block->GlobalMeshIndex;
      contourTreeMeshOut.NeighborConnectivity = block->NeighborConnectivity;
      contourTreeMeshOut.NeighborOffsets = block->NeighborOffsets;
      contourTreeMeshOut.MaxNeighbors = block->MaxNeighbors;
      // Merge the two contour tree meshes
      contourTreeMeshOut.MergeWith(contourTreeMeshIn);

      // Compute the origin and size of the new block
      viskores::Id3 globalSize = block->GlobalSize;
      viskores::Id3 currBlockOrigin;
      currBlockOrigin[0] = std::min(recvblock.BlockOrigin[0], block->BlockOrigin[0]);
      currBlockOrigin[1] = std::min(recvblock.BlockOrigin[1], block->BlockOrigin[1]);
      currBlockOrigin[2] = std::min(recvblock.BlockOrigin[2], block->BlockOrigin[2]);
      viskores::Id3 currBlockMaxIndex; // Needed only to compute the block size
      currBlockMaxIndex[0] = std::max(recvblock.BlockOrigin[0] + recvblock.BlockSize[0],
                                      block->BlockOrigin[0] + block->BlockSize[0]);
      currBlockMaxIndex[1] = std::max(recvblock.BlockOrigin[1] + recvblock.BlockSize[1],
                                      block->BlockOrigin[1] + block->BlockSize[1]);
      currBlockMaxIndex[2] = std::max(recvblock.BlockOrigin[2] + recvblock.BlockSize[2],
                                      block->BlockOrigin[2] + block->BlockSize[2]);
      viskores::Id3 currBlockSize;
      currBlockSize[0] = currBlockMaxIndex[0] - currBlockOrigin[0];
      currBlockSize[1] = currBlockMaxIndex[1] - currBlockOrigin[1];
      currBlockSize[2] = currBlockMaxIndex[2] - currBlockOrigin[2];

      // On rank 0 we compute the contour tree at the end when the merge is done, so we don't need to do it here
      if (selfid == 0)
      {
        // Save the data from our block for the next iteration
        block->NumVertices = contourTreeMeshOut.NumVertices;
        block->SortedValue = contourTreeMeshOut.SortedValues;
        block->GlobalMeshIndex = contourTreeMeshOut.GlobalMeshIndex;
        block->NeighborConnectivity = contourTreeMeshOut.NeighborConnectivity;
        block->NeighborOffsets = contourTreeMeshOut.NeighborOffsets;
        block->MaxNeighbors = contourTreeMeshOut.MaxNeighbors;
        block->BlockOrigin = currBlockOrigin;
        block->BlockSize = currBlockSize;
        block->GlobalSize = globalSize;
      }
      else // If we are a block that will continue to be merged then we need compute the contour tree here
      {
        // Compute the contour tree from our merged mesh
        viskores::Id currNumIterations;
        viskores::worklet::contourtree_augmented::ContourTree currContourTree;
        viskores::worklet::contourtree_augmented::IdArrayType currSortOrder;
        viskores::worklet::ContourTreeAugmented worklet;
        viskores::cont::ArrayHandle<FieldType> currField;
        viskores::Id3 maxIdx(currBlockOrigin[0] + currBlockSize[0] - 1,
                             currBlockOrigin[1] + currBlockSize[1] - 1,
                             currBlockOrigin[2] + currBlockSize[2] - 1);
        auto meshBoundaryExecObj =
          contourTreeMeshOut.GetMeshBoundaryExecutionObject(globalSize, currBlockOrigin, maxIdx);
        worklet.Run(
          contourTreeMeshOut.SortedValues, // Unused param. Provide something to keep the API happy
          contourTreeMeshOut,
          currContourTree,
          currSortOrder,
          currNumIterations,
          block->ComputeRegularStructure,
          meshBoundaryExecObj);
        viskores::worklet::contourtree_augmented::ContourTreeMesh<FieldType>* newContourTreeMesh =
          0;
        if (block->ComputeRegularStructure == 1)
        {
          // If we have the fully augmented contour tree
          newContourTreeMesh =
            new viskores::worklet::contourtree_augmented::ContourTreeMesh<FieldType>(
              currContourTree.Arcs, contourTreeMeshOut);
        }
        else if (block->ComputeRegularStructure == 2)
        {
          // If we have the partially augmented (e.g., boundary augmented) contour tree
          newContourTreeMesh =
            new viskores::worklet::contourtree_augmented::ContourTreeMesh<FieldType>(
              currContourTree.Augmentnodes, currContourTree.Augmentarcs, contourTreeMeshOut);
        }
        else
        {
          // We should not be able to get here
          throw viskores::cont::ErrorFilterExecution(
            "Parallel contour tree requires at least parial boundary augmentation");
        }

        // Copy the data from newContourTreeMesh into  block
        block->NumVertices = newContourTreeMesh->NumVertices;
        block->SortedValue = newContourTreeMesh->SortedValues;
        block->GlobalMeshIndex = newContourTreeMesh->GlobalMeshIndex;
        block->NeighborConnectivity = newContourTreeMesh->NeighborConnectivity;
        block->NeighborOffsets = newContourTreeMesh->NeighborOffsets;
        block->MaxNeighbors = newContourTreeMesh->MaxNeighbors;
        block->BlockOrigin = currBlockOrigin;
        block->BlockSize = currBlockSize;
        block->GlobalSize = globalSize;

        // Viskores keeps track of the arrays for us, so we can savely delete the ContourTreeMesh
        // as all data has been transferred into our data block
        delete newContourTreeMesh;
      }
    }
  }
  // Send our current block (which is either our original block or the one we just combined from the ones we received) to our next neighbour.
  // Once a rank has send his block (either in its orignal or merged form) it is done with the reduce
  for (int cc = 0; cc < rp.out_link().size(); ++cc)
  {
    auto target = rp.out_link().target(cc);
    if (target.gid != selfid)
    {
      rp.enqueue(target, *block);
    }
  }
} //end MergeBlockFunctor

} // namespace contourtree_distributed
} // namespace worklet
} // namespace viskores

#endif
