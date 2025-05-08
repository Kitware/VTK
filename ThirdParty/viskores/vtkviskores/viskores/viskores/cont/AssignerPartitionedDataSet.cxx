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
#include <viskores/cont/AssignerPartitionedDataSet.h>

#include <viskores/cont/EnvironmentTracker.h>
#include <viskores/cont/PartitionedDataSet.h>


#include <viskores/thirdparty/diy/diy.h>

#include <algorithm> // std::lower_bound
#include <numeric>   // std::iota

namespace viskores
{
namespace cont
{

VISKORES_CONT
AssignerPartitionedDataSet::AssignerPartitionedDataSet(
  const viskores::cont::PartitionedDataSet& pds)
  : AssignerPartitionedDataSet(pds.GetNumberOfPartitions())
{
}

VISKORES_CONT
AssignerPartitionedDataSet::AssignerPartitionedDataSet(viskores::Id num_partitions)
  : viskoresdiy::StaticAssigner(viskores::cont::EnvironmentTracker::GetCommunicator().size(), 1)
  , IScanPartitionCounts()
{
  auto comm = viskores::cont::EnvironmentTracker::GetCommunicator();
  if (comm.size() > 1)
  {
    viskores::Id iscan;
    viskoresdiy::mpi::scan(comm, num_partitions, iscan, std::plus<viskores::Id>());
    viskoresdiy::mpi::all_gather(comm, iscan, this->IScanPartitionCounts);
  }
  else
  {
    this->IScanPartitionCounts.push_back(num_partitions);
  }

  this->set_nblocks(static_cast<int>(this->IScanPartitionCounts.back()));
}

VISKORES_CONT
AssignerPartitionedDataSet::~AssignerPartitionedDataSet() {}

VISKORES_CONT
void AssignerPartitionedDataSet::local_gids(int my_rank, std::vector<int>& gids) const
{
  const size_t s_rank = static_cast<size_t>(my_rank);
  if (my_rank == 0)
  {
    assert(this->IScanPartitionCounts.size() > 0);
    gids.resize(static_cast<size_t>(this->IScanPartitionCounts[s_rank]));
    std::iota(gids.begin(), gids.end(), 0);
  }
  else if (my_rank > 0 && s_rank < this->IScanPartitionCounts.size())
  {
    gids.resize(static_cast<size_t>(this->IScanPartitionCounts[s_rank] -
                                    this->IScanPartitionCounts[s_rank - 1]));
    std::iota(gids.begin(), gids.end(), static_cast<int>(this->IScanPartitionCounts[s_rank - 1]));
  }
}

VISKORES_CONT
int AssignerPartitionedDataSet::rank(int gid) const
{
  return static_cast<int>(std::lower_bound(this->IScanPartitionCounts.begin(),
                                           this->IScanPartitionCounts.end(),
                                           gid + 1) -
                          this->IScanPartitionCounts.begin());
}


} // viskores::cont
} // viskores
