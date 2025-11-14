//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#include <viskores/filter/flow/internal/BoundsMap.h>

#include <viskores/cont/AssignerPartitionedDataSet.h>
#include <viskores/cont/EnvironmentTracker.h>
#include <viskores/cont/ErrorFilterExecution.h>

#include <viskores/thirdparty/diy/diy.h>

#ifdef VISKORES_ENABLE_MPI
#include <mpi.h>
#include <viskores/thirdparty/diy/mpi-cast.h>
#endif

namespace viskores
{
namespace filter
{
namespace flow
{
namespace internal
{

void BoundsMap::Init(const std::vector<viskores::cont::DataSet>& dataSets,
                     const std::vector<viskores::Id>& blockIds)
{
  if (dataSets.size() != blockIds.size())
    throw viskores::cont::ErrorFilterExecution("Number of datasets and block ids must match");

  this->LocalIDs = blockIds;
  this->LocalNumBlocks = dataSets.size();

  viskoresdiy::mpi::communicator comm = viskores::cont::EnvironmentTracker::GetCommunicator();

  //1. Get the min/max blockId
  viskores::Id locMinId = 0, locMaxId = 0;
  if (!this->LocalIDs.empty())
  {
    locMinId = *std::min_element(this->LocalIDs.begin(), this->LocalIDs.end());
    locMaxId = *std::max_element(this->LocalIDs.begin(), this->LocalIDs.end());
  }

  viskores::Id globalMinId = 0, globalMaxId = 0;

  viskoresdiy::mpi::all_reduce(
    comm, locMinId, globalMinId, viskoresdiy::mpi::minimum<viskores::Id>{});
  viskoresdiy::mpi::all_reduce(
    comm, locMaxId, globalMaxId, viskoresdiy::mpi::maximum<viskores::Id>{});
  if (globalMinId != 0 || (globalMaxId - globalMinId) < 1)
    throw viskores::cont::ErrorFilterExecution("Invalid block ids");

  //2. Find out how many blocks everyone has.
  std::vector<viskores::Id> locBlockCounts(comm.size(), 0), globalBlockCounts(comm.size(), 0);
  locBlockCounts[comm.rank()] = this->LocalIDs.size();
  viskoresdiy::mpi::all_reduce(comm, locBlockCounts, globalBlockCounts, std::plus<viskores::Id>{});

  //note: there might be duplicates...
  viskores::Id globalNumBlocks =
    std::accumulate(globalBlockCounts.begin(), globalBlockCounts.end(), viskores::Id{ 0 });

  //3. given the counts per rank, calc offset for this rank.
  viskores::Id offset = 0;
  for (int i = 0; i < comm.rank(); i++)
    offset += globalBlockCounts[i];

  //4. calc the blocks on each rank.
  std::vector<viskores::Id> localBlockIds(globalNumBlocks, 0);
  viskores::Id idx = 0;
  for (const auto& bid : this->LocalIDs)
    localBlockIds[offset + idx++] = bid;

  //use an MPI_Alltoallv instead.
  std::vector<viskores::Id> globalBlockIds(globalNumBlocks, 0);
  viskoresdiy::mpi::all_reduce(comm, localBlockIds, globalBlockIds, std::plus<viskores::Id>{});


  //5. create a rank -> blockId map.
  //  rankToBlockIds[rank] = {this->LocalIDs on rank}.
  std::vector<std::vector<viskores::Id>> rankToBlockIds(comm.size());

  offset = 0;
  for (int rank = 0; rank < comm.size(); rank++)
  {
    viskores::Id numBIds = globalBlockCounts[rank];
    rankToBlockIds[rank].resize(numBIds);
    for (viskores::Id i = 0; i < numBIds; i++)
      rankToBlockIds[rank][i] = globalBlockIds[offset + i];

    offset += numBIds;
  }

  //6. there might be duplicates, so count number of UNIQUE blocks.
  std::set<viskores::Id> globalUniqueBlockIds;
  globalUniqueBlockIds.insert(globalBlockIds.begin(), globalBlockIds.end());
  this->TotalNumBlocks = globalUniqueBlockIds.size();

  //Build a vector of :  blockIdsToRank[blockId] = {ranks that have this blockId}
  std::vector<std::vector<viskores::Id>> blockIdsToRank(this->TotalNumBlocks);
  for (int rank = 0; rank < comm.size(); rank++)
  {
    for (const auto& bid : rankToBlockIds[rank])
    {
      blockIdsToRank[bid].push_back(rank);
      this->BlockToRankMap[bid].push_back(rank);
    }
  }

  this->Build(dataSets);
}

void BoundsMap::Init(const std::vector<viskores::cont::DataSet>& dataSets)
{
  this->LocalNumBlocks = dataSets.size();

  viskores::cont::AssignerPartitionedDataSet assigner(this->LocalNumBlocks);
  this->TotalNumBlocks = assigner.nblocks();
  std::vector<int> ids;

  viskoresdiy::mpi::communicator Comm = viskores::cont::EnvironmentTracker::GetCommunicator();
  assigner.local_gids(Comm.rank(), ids);
  for (const auto& i : ids)
    this->LocalIDs.emplace_back(static_cast<viskores::Id>(i));

  for (viskores::Id id = 0; id < this->TotalNumBlocks; id++)
    this->BlockToRankMap[id] = { assigner.rank(static_cast<int>(id)) };
  this->Build(dataSets);
}

void BoundsMap::Build(const std::vector<viskores::cont::DataSet>& dataSets)
{
  std::vector<viskores::Float64> vals(static_cast<std::size_t>(this->TotalNumBlocks * 6), 0);
  std::vector<viskores::Float64> vals2(vals.size());

  std::vector<viskores::Float64> localMins((this->TotalNumBlocks * 3),
                                           std::numeric_limits<viskores::Float64>::max());
  std::vector<viskores::Float64> localMaxs((this->TotalNumBlocks * 3),
                                           -std::numeric_limits<viskores::Float64>::max());

  for (std::size_t i = 0; i < this->LocalIDs.size(); i++)
  {
    const viskores::cont::DataSet& ds = dataSets[static_cast<std::size_t>(i)];
    viskores::Bounds bounds = ds.GetCoordinateSystem().GetBounds();

    viskores::Id localID = this->LocalIDs[i];
    localMins[localID * 3 + 0] = bounds.X.Min;
    localMins[localID * 3 + 1] = bounds.Y.Min;
    localMins[localID * 3 + 2] = bounds.Z.Min;
    localMaxs[localID * 3 + 0] = bounds.X.Max;
    localMaxs[localID * 3 + 1] = bounds.Y.Max;
    localMaxs[localID * 3 + 2] = bounds.Z.Max;
  }

  std::vector<viskores::Float64> globalMins, globalMaxs;

#ifdef VISKORES_ENABLE_MPI
  globalMins.resize(this->TotalNumBlocks * 3);
  globalMaxs.resize(this->TotalNumBlocks * 3);

  viskoresdiy::mpi::communicator comm = viskores::cont::EnvironmentTracker::GetCommunicator();

  viskoresdiy::mpi::all_reduce(
    comm, localMins, globalMins, viskoresdiy::mpi::minimum<viskores::Float64>{});
  viskoresdiy::mpi::all_reduce(
    comm, localMaxs, globalMaxs, viskoresdiy::mpi::maximum<viskores::Float64>{});
#else
  globalMins = localMins;
  globalMaxs = localMaxs;
#endif

  this->BlockBounds.resize(static_cast<std::size_t>(this->TotalNumBlocks));
  this->GlobalBounds = viskores::Bounds();

  std::size_t idx = 0;
  for (auto& block : this->BlockBounds)
  {
    block = viskores::Bounds(globalMins[idx + 0],
                             globalMaxs[idx + 0],
                             globalMins[idx + 1],
                             globalMaxs[idx + 1],
                             globalMins[idx + 2],
                             globalMaxs[idx + 2]);
    this->GlobalBounds.Include(block);
    idx += 3;
  }
}

}
}
}
} // namespace viskores::filter::flow::internal
