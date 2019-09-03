/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDIYExplicitAssigner.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDIYExplicitAssigner.h"

#include "vtkMath.h"
#include "vtkObjectFactory.h"

#include <algorithm>
#include <cassert>
#include <iterator>
#include <numeric>

//----------------------------------------------------------------------------
vtkDIYExplicitAssigner::vtkDIYExplicitAssigner(
  diy::mpi::communicator comm, int local_blocks, bool force_power_of_two /*=false*/)
  : diy::StaticAssigner(comm.size(), local_blocks)
{
  std::vector<int> block_counts;
  if (comm.size() > 1)
  {
    block_counts.resize(comm.size());
    diy::mpi::all_gather(comm, local_blocks, block_counts);
  }
  else
  {
    block_counts.push_back(local_blocks);
  }
  assert(block_counts.size() >= 1);

  if (force_power_of_two)
  {
    const int global_num_blocks = std::accumulate(block_counts.begin(), block_counts.end(), 0);
    const int global_block_counts_pow_2 = vtkMath::NearestPowerOfTwo(global_num_blocks);

    // we pad each rank with extra blocks
    auto extra_blocks = global_block_counts_pow_2 - global_num_blocks;
    const auto extra_blocks_per_rank =
      static_cast<int>(std::ceil(extra_blocks / static_cast<double>(block_counts.size())));
    for (auto& count : block_counts)
    {
      if (extra_blocks > 0)
      {
        const auto padding = std::min(extra_blocks_per_rank, extra_blocks);
        count += padding;
        extra_blocks -= padding;
      }
    }
    assert(
      std::accumulate(block_counts.begin(), block_counts.end(), 0) == global_block_counts_pow_2);
  }

  // convert to inclusive-scan
  this->IScanBlockCounts = std::move(block_counts);
  for (size_t cc = 1; cc < this->IScanBlockCounts.size(); ++cc)
  {
    this->IScanBlockCounts[cc] += this->IScanBlockCounts[cc - 1];
  }
  this->set_nblocks(this->IScanBlockCounts.back());

  assert(
    force_power_of_two == false || vtkMath::NearestPowerOfTwo(this->nblocks()) == this->nblocks());
}

//----------------------------------------------------------------------------
int vtkDIYExplicitAssigner::rank(int gid) const
{
  auto iter =
    std::lower_bound(this->IScanBlockCounts.begin(), this->IScanBlockCounts.end(), gid + 1);
  assert(iter != this->IScanBlockCounts.end());
  return static_cast<int>(std::distance(this->IScanBlockCounts.begin(), iter));
}

//----------------------------------------------------------------------------
void vtkDIYExplicitAssigner::local_gids(int rank, std::vector<int>& gids) const
{
  const auto min = rank == 0 ? 0 : this->IScanBlockCounts[rank - 1];
  const auto max = this->IScanBlockCounts[rank];
  gids.resize(max - min);
  std::iota(gids.begin(), gids.end(), min);
}
