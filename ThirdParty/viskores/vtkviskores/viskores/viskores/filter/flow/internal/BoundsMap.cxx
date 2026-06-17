//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#include <viskores/filter/flow/internal/BoundsMap.h>

#include <viskores/CellShape.h>
#include <viskores/cont/AssignerPartitionedDataSet.h>
#include <viskores/cont/DataSetBuilderExplicit.h>
#include <viskores/cont/EnvironmentTracker.h>
#include <viskores/cont/ErrorFilterExecution.h>

#include <viskores/thirdparty/diy/diy.h>

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <numeric>
#include <set>

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
  if (globalMinId < 0)
  {
    throw viskores::cont::ErrorFilterExecution(
      "Invalid block ids. IDs must be non-negative and dense in [0, N).");
  }

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
  if (!globalUniqueBlockIds.empty())
  {
    // The synthetic locator cells built below use their cell id directly as the
    // global block id. That requires block ids to be dense in [0, N).
    const viskores::Id minId = *globalUniqueBlockIds.begin();
    const viskores::Id maxId = *globalUniqueBlockIds.rbegin();
    const viskores::Id uniqueCount = static_cast<viskores::Id>(globalUniqueBlockIds.size());
    const viskores::Id expectedDenseCount = maxId - minId + 1;
    if (minId != 0 || expectedDenseCount != uniqueCount)
    {
      throw viskores::cont::ErrorFilterExecution(
        "Invalid block ids. IDs must be non-negative and dense in [0, N).");
    }
  }
  this->TotalNumBlocks = static_cast<viskores::Id>(globalUniqueBlockIds.size());

  for (int rank = 0; rank < comm.size(); rank++)
  {
    for (const auto& bid : rankToBlockIds[rank])
    {
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

  this->BuildLocator();
}

viskores::Id3 BoundsMap::GetLocatorDims() const
{
  if (this->TotalNumBlocks <= 0)
    return viskores::Id3{ 1, 1, 1 };

  const viskores::Float64 ex = this->GlobalBounds.X.Length();
  const viskores::Float64 ey = this->GlobalBounds.Y.Length();
  const viskores::Float64 ez = this->GlobalBounds.Z.Length();

  // Keep the number of bins close to the number of block cells while respecting box aspect ratio.
  const viskores::Float64 volume = ex * ey * ez;
  viskores::Id3 dims{ 1, 1, 1 };
  if (volume > viskores::Float64{ 0 })
  {
    const viskores::Float64 targetBins =
      std::max(viskores::Float64{ 1 }, static_cast<viskores::Float64>(this->TotalNumBlocks));
    const viskores::Float64 scale = std::cbrt(targetBins / volume);
    dims[0] = std::max<viskores::Id>(1, static_cast<viskores::Id>(std::ceil(ex * scale)));
    dims[1] = std::max<viskores::Id>(1, static_cast<viskores::Id>(std::ceil(ey * scale)));
    dims[2] = std::max<viskores::Id>(1, static_cast<viskores::Id>(std::ceil(ez * scale)));
  }
  else
  {
    const viskores::Id n =
      std::max<viskores::Id>(1,
                             static_cast<viskores::Id>(std::ceil(
                               std::cbrt(static_cast<viskores::Float64>(this->TotalNumBlocks)))));
    dims = viskores::Id3{ n, n, n };
    if (ex == viskores::Float64{ 0 })
      dims[0] = 1;
    if (ey == viskores::Float64{ 0 })
      dims[1] = 1;
    if (ez == viskores::Float64{ 0 })
      dims[2] = 1;
  }

  return dims;
}

void BoundsMap::BuildLocator()
{
  if (this->TotalNumBlocks <= 0)
  {
    this->Locator = viskores::cont::CellLocatorUniformBins{};
    this->Locator.SetDims(viskores::Id3{ 1, 1, 1 });
    return;
  }

  std::vector<viskores::Vec3f> points;
  std::vector<viskores::UInt8> shapes;
  std::vector<viskores::IdComponent> numIndices;
  std::vector<viskores::Id> connectivity;

  points.reserve(static_cast<std::size_t>(this->TotalNumBlocks * 8));
  shapes.reserve(static_cast<std::size_t>(this->TotalNumBlocks));
  numIndices.reserve(static_cast<std::size_t>(this->TotalNumBlocks));
  connectivity.reserve(static_cast<std::size_t>(this->TotalNumBlocks * 8));

  // Each block bound is represented as one synthetic cell. Later flow worklets
  // can query the locator with CountAllCells/FindAllCellIds and use the returned
  // cell ids directly as global block ids.
  for (const auto& b : this->BlockBounds)
  {
    const viskores::Float64 xmin = b.X.Min;
    const viskores::Float64 xmax = b.X.Max;
    const viskores::Float64 ymin = b.Y.Min;
    const viskores::Float64 ymax = b.Y.Max;
    const viskores::Float64 zmin = b.Z.Min;
    const viskores::Float64 zmax = b.Z.Max;

    const bool varyX = (xmax - xmin) > viskores::Float64{ 0 };
    const bool varyY = (ymax - ymin) > viskores::Float64{ 0 };
    const bool varyZ = (zmax - zmin) > viskores::Float64{ 0 };

    auto pushPoint = [&points](viskores::Float64 x, viskores::Float64 y, viskores::Float64 z)
    {
      // CellLocatorUniformBins execution queries use Vec3f positions, so keep
      // the locator geometry in the same coordinate type.
      points.emplace_back(static_cast<viskores::FloatDefault>(x),
                          static_cast<viskores::FloatDefault>(y),
                          static_cast<viskores::FloatDefault>(z));
      return static_cast<viskores::Id>(points.size() - 1);
    };

    if (varyX && varyY && varyZ)
    {
      const viskores::Id p0 = pushPoint(xmin, ymin, zmin);
      const viskores::Id p1 = pushPoint(xmax, ymin, zmin);
      const viskores::Id p2 = pushPoint(xmax, ymax, zmin);
      const viskores::Id p3 = pushPoint(xmin, ymax, zmin);
      const viskores::Id p4 = pushPoint(xmin, ymin, zmax);
      const viskores::Id p5 = pushPoint(xmax, ymin, zmax);
      const viskores::Id p6 = pushPoint(xmax, ymax, zmax);
      const viskores::Id p7 = pushPoint(xmin, ymax, zmax);
      shapes.push_back(static_cast<viskores::UInt8>(viskores::CELL_SHAPE_HEXAHEDRON));
      numIndices.push_back(8);
      connectivity.insert(connectivity.end(), { p0, p1, p2, p3, p4, p5, p6, p7 });
    }
    else if (varyX && varyY)
    {
      const viskores::Id p0 = pushPoint(xmin, ymin, zmin);
      const viskores::Id p1 = pushPoint(xmax, ymin, zmin);
      const viskores::Id p2 = pushPoint(xmax, ymax, zmin);
      const viskores::Id p3 = pushPoint(xmin, ymax, zmin);
      shapes.push_back(static_cast<viskores::UInt8>(viskores::CELL_SHAPE_QUAD));
      numIndices.push_back(4);
      connectivity.insert(connectivity.end(), { p0, p1, p2, p3 });
    }
    else if (varyX && varyZ)
    {
      const viskores::Id p0 = pushPoint(xmin, ymin, zmin);
      const viskores::Id p1 = pushPoint(xmax, ymin, zmin);
      const viskores::Id p2 = pushPoint(xmax, ymin, zmax);
      const viskores::Id p3 = pushPoint(xmin, ymin, zmax);
      shapes.push_back(static_cast<viskores::UInt8>(viskores::CELL_SHAPE_QUAD));
      numIndices.push_back(4);
      connectivity.insert(connectivity.end(), { p0, p1, p2, p3 });
    }
    else if (varyY && varyZ)
    {
      const viskores::Id p0 = pushPoint(xmin, ymin, zmin);
      const viskores::Id p1 = pushPoint(xmin, ymax, zmin);
      const viskores::Id p2 = pushPoint(xmin, ymax, zmax);
      const viskores::Id p3 = pushPoint(xmin, ymin, zmax);
      shapes.push_back(static_cast<viskores::UInt8>(viskores::CELL_SHAPE_QUAD));
      numIndices.push_back(4);
      connectivity.insert(connectivity.end(), { p0, p1, p2, p3 });
    }
    else if (varyX)
    {
      const viskores::Id p0 = pushPoint(xmin, ymin, zmin);
      const viskores::Id p1 = pushPoint(xmax, ymin, zmin);
      shapes.push_back(static_cast<viskores::UInt8>(viskores::CELL_SHAPE_LINE));
      numIndices.push_back(2);
      connectivity.insert(connectivity.end(), { p0, p1 });
    }
    else if (varyY)
    {
      const viskores::Id p0 = pushPoint(xmin, ymin, zmin);
      const viskores::Id p1 = pushPoint(xmin, ymax, zmin);
      shapes.push_back(static_cast<viskores::UInt8>(viskores::CELL_SHAPE_LINE));
      numIndices.push_back(2);
      connectivity.insert(connectivity.end(), { p0, p1 });
    }
    else if (varyZ)
    {
      const viskores::Id p0 = pushPoint(xmin, ymin, zmin);
      const viskores::Id p1 = pushPoint(xmin, ymin, zmax);
      shapes.push_back(static_cast<viskores::UInt8>(viskores::CELL_SHAPE_LINE));
      numIndices.push_back(2);
      connectivity.insert(connectivity.end(), { p0, p1 });
    }
    else
    {
      const viskores::Id p0 = pushPoint(xmin, ymin, zmin);
      shapes.push_back(static_cast<viskores::UInt8>(viskores::CELL_SHAPE_VERTEX));
      numIndices.push_back(1);
      connectivity.push_back(p0);
    }
  }

  auto boundsDS =
    viskores::cont::DataSetBuilderExplicit::Create(points, shapes, numIndices, connectivity);

  this->Locator = viskores::cont::CellLocatorUniformBins{};
  this->Locator.SetDims(this->GetLocatorDims());
  this->Locator.SetCellSet(boundsDS.GetCellSet());
  this->Locator.SetCoordinates(boundsDS.GetCoordinateSystem());
  // The locator acceleration structure is built lazily when it is prepared for
  // the selected device. The current flow filters still use FindBlocks on the host,
  // so do not force device work for every BoundsMap construction.
}

}
}
}
} // namespace viskores::filter::flow::internal
