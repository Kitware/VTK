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

#include <algorithm>
#include <viskores/cont/CellSetStructured.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/EnvironmentTracker.h>
#include <viskores/filter/scalar_topology/internal/ComputeBlockIndices.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/meshtypes/contourtreemesh/CopyIntoCombinedArrayWorklet.h>

namespace // anonymous namespace for local helper classes
{

struct OriginsBlock
{
  std::vector<int> Origins;
  OriginsBlock(const std::vector<int>& origins)
    : Origins(origins)
  {
  }
  static void Destroy(void* b) { delete static_cast<OriginsBlock*>(b); }
};

struct MergeOriginsFunctor
{
  void operator()(OriginsBlock* b,
                  const viskoresdiy::ReduceProxy& rp,     // communication proxy
                  const viskoresdiy::RegularSwapPartners& // partners of the current block (unused)
  ) const
  {
    const auto selfid = rp.gid();

    std::vector<int> incoming;
    rp.incoming(incoming);
    for (const int ingid : incoming)
    {
      if (ingid != selfid)
      {
        std::vector<int> incoming_origins;
        rp.dequeue(ingid, incoming_origins);

        std::vector<int> merged_origins;
        std::merge(incoming_origins.begin(),
                   incoming_origins.end(),
                   b->Origins.begin(),
                   b->Origins.end(),
                   std::inserter(merged_origins, merged_origins.begin()));
        auto last = std::unique(merged_origins.begin(), merged_origins.end());
        merged_origins.erase(last, merged_origins.end());
        std::swap(merged_origins, b->Origins);
      }
    }
    for (int cc = 0; cc < rp.out_link().size(); ++cc)
    {
      auto target = rp.out_link().target(cc);
      if (target.gid != selfid)
      {
        rp.enqueue(target, b->Origins);
      }
    }
  }
};

}
namespace viskores
{
namespace filter
{
namespace scalar_topology
{
namespace internal
{

VISKORES_CONT viskoresdiy::DiscreteBounds ComputeBlockIndices(
  const viskores::cont::PartitionedDataSet& input,
  DiscreteBoundsDivisionVector& diyDivisions,
  std::vector<int>& diyLocalBlockGids)
{
  auto firstDS = input.GetPartition(0);
  viskores::Id3 dummy1, firstGlobalPointDimensions, dummy2;
  firstDS.GetCellSet().CastAndCallForTypes<VISKORES_DEFAULT_CELL_SET_LIST_STRUCTURED>(
    viskores::worklet::contourtree_augmented::GetLocalAndGlobalPointDimensions(),
    dummy1,
    firstGlobalPointDimensions,
    dummy2);
  int numDims = firstGlobalPointDimensions[2] > 1 ? 3 : 2;

  diyDivisions.clear();
  viskoresdiy::DiscreteBounds diyBounds(numDims);
  std::vector<DiscreteBoundsDivisionVector> diyBlockCoords(input.GetNumberOfPartitions());

  for (viskores::IdComponent d = 0; d < numDims; ++d)
  {
    // Set bounds for this dimension
    diyBounds.min[d] = 0;
    diyBounds.max[d] = static_cast<int>(firstGlobalPointDimensions[d]);

    // Get the list of origins along current coordinate axis
    std::vector<int> local_origins;

    for (viskores::Id ds_no = 0; ds_no < input.GetNumberOfPartitions(); ++ds_no)
    {
      viskores::Id3 globalPointIndexStart;
      input.GetPartition(ds_no)
        .GetCellSet()
        .CastAndCallForTypes<VISKORES_DEFAULT_CELL_SET_LIST_STRUCTURED>(
          viskores::worklet::contourtree_augmented::GetLocalAndGlobalPointDimensions(),
          dummy1,
          dummy2,
          globalPointIndexStart);

      local_origins.push_back(static_cast<int>(globalPointIndexStart[d]));
    }

    // Sort and remove duplicates
    OriginsBlock* origins_block = new OriginsBlock(local_origins);
    std::sort(origins_block->Origins.begin(), origins_block->Origins.end());
    auto last = std::unique(origins_block->Origins.begin(), origins_block->Origins.end());
    origins_block->Origins.erase(last, origins_block->Origins.end());

    // Create global list of origins across all ranks
    auto comm = viskores::cont::EnvironmentTracker::GetCommunicator();
    auto rank = comm.rank();
    auto size = comm.size();
    viskoresdiy::Master master(comm, 1, -1, 0, OriginsBlock::Destroy);
    master.add(rank, origins_block, new viskoresdiy::Link);
    viskoresdiy::ContiguousAssigner assigner(size, size);
    viskoresdiy::DiscreteBounds bounds(1);
    bounds.min[0] = 0;
    bounds.max[0] = size - 1;
    viskoresdiy::RegularDecomposer<viskoresdiy::DiscreteBounds> decomposer(1, bounds, size);
    viskoresdiy::RegularSwapPartners partners(decomposer, 2, true);
    viskoresdiy::reduce(master, assigner, partners, MergeOriginsFunctor{});

    // Number of blocks/divisions along axis is number of unique origins along this axis
    diyDivisions.push_back(static_cast<int>(origins_block->Origins.size()));

    // Block index aling this axis is the index of the origin in that list
    for (viskores::Id ds_no = 0; ds_no < input.GetNumberOfPartitions(); ++ds_no)
    {
      diyBlockCoords[ds_no].push_back(static_cast<int>(std::find(origins_block->Origins.begin(),
                                                                 origins_block->Origins.end(),
                                                                 local_origins[ds_no]) -
                                                       origins_block->Origins.begin()));
    }
  }

  // Compute global block IDs
  diyLocalBlockGids.clear();
  for (viskores::Id ds_no = 0; ds_no < input.GetNumberOfPartitions(); ++ds_no)
  {
    diyLocalBlockGids.push_back(
      viskoresdiy::RegularDecomposer<viskoresdiy::DiscreteBounds>::coords_to_gid(
        diyBlockCoords[ds_no], diyDivisions));
  }

  return diyBounds;
}

VISKORES_CONT viskoresdiy::DiscreteBounds ComputeBlockIndices(
  const viskores::cont::PartitionedDataSet& input,
  viskores::Id3 blocksPerDim,
  const viskores::cont::ArrayHandle<viskores::Id3>& blockIndices,
  DiscreteBoundsDivisionVector& diyDivisions,
  std::vector<int>& diyLocalBlockGids)
{
  auto firstDS = input.GetPartition(0);
  viskores::Id3 dummy1, firstGlobalPointDimensions, dummy2;
  firstDS.GetCellSet().CastAndCallForTypes<VISKORES_DEFAULT_CELL_SET_LIST_STRUCTURED>(
    viskores::worklet::contourtree_augmented::GetLocalAndGlobalPointDimensions(),
    dummy1,
    firstGlobalPointDimensions,
    dummy2);
  int numDims = firstGlobalPointDimensions[2] > 1 ? 3 : 2;

  diyDivisions.clear();
  viskoresdiy::DiscreteBounds diyBounds(numDims);
  for (viskores::IdComponent d = 0; d < numDims; ++d)
  {
    // Set bounds for this dimension
    diyBounds.min[d] = 0;
    diyBounds.max[d] = static_cast<int>(firstGlobalPointDimensions[d]);
    diyDivisions.push_back(static_cast<int>(blocksPerDim[d]));
  }

  // Compute global block IDs
  diyLocalBlockGids.clear();
  auto blockIndicesPortal = blockIndices.ReadPortal();
  for (viskores::Id ds_no = 0; ds_no < input.GetNumberOfPartitions(); ++ds_no)
  {
    auto currBlockIndices = blockIndicesPortal.Get(ds_no);
    DiscreteBoundsDivisionVector diyBlockCoords(numDims);
    for (viskores::IdComponent d = 0; d < numDims; ++d)
    {
      diyBlockCoords[d] = static_cast<int>(currBlockIndices[d]);
    }

    diyLocalBlockGids.push_back(
      viskoresdiy::RegularDecomposer<viskoresdiy::DiscreteBounds>::coords_to_gid(diyBlockCoords,
                                                                                 diyDivisions));
  }

  return diyBounds;
}

} // namespace internal
} // namespace scalar_topology
} // namespace filter
} // namespace viskores
