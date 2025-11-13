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

#ifndef viskores_filter_flow_internal_BoundsMap_h
#define viskores_filter_flow_internal_BoundsMap_h

#include <viskores/Bounds.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/Field.h>
#include <viskores/cont/PartitionedDataSet.h>

#include <algorithm>
#include <set>
#include <vector>

namespace viskores
{
namespace filter
{
namespace flow
{
namespace internal
{

class VISKORES_ALWAYS_EXPORT BoundsMap
{
public:
  VISKORES_CONT BoundsMap() {}

  VISKORES_CONT BoundsMap(const viskores::cont::DataSet& dataSet) { this->Init({ dataSet }); }

  VISKORES_CONT BoundsMap(const viskores::cont::DataSet& dataSet, const viskores::Id& blockId)
  {
    this->Init({ dataSet }, { blockId });
  }

  VISKORES_CONT BoundsMap(const std::vector<viskores::cont::DataSet>& dataSets)
  {
    this->Init(dataSets);
  }

  VISKORES_CONT BoundsMap(const viskores::cont::PartitionedDataSet& pds)
  {
    this->Init(pds.GetPartitions());
  }

  VISKORES_CONT BoundsMap(const viskores::cont::PartitionedDataSet& pds,
                          const std::vector<viskores::Id>& blockIds)
  {
    this->Init(pds.GetPartitions(), blockIds);
  }

  VISKORES_CONT viskores::Bounds GetGlobalBounds() const { return this->GlobalBounds; }

  VISKORES_CONT viskores::Bounds GetBlockBounds(viskores::Id idx) const
  {
    VISKORES_ASSERT(idx >= 0 && static_cast<std::size_t>(idx) < this->BlockBounds.size());

    return this->BlockBounds[static_cast<std::size_t>(idx)];
  }

  VISKORES_CONT viskores::Id GetLocalBlockId(viskores::Id idx) const
  {
    VISKORES_ASSERT(idx >= 0 && idx < this->LocalNumBlocks);
    return this->LocalIDs[static_cast<std::size_t>(idx)];
  }

  VISKORES_CONT std::vector<int> FindRank(viskores::Id blockId) const
  {
    auto it = this->BlockToRankMap.find(blockId);
    if (it == this->BlockToRankMap.end())
      return {};

    return it->second;
  }

  VISKORES_CONT std::vector<viskores::Id> FindBlocks(const viskores::Vec3f& p) const
  {
    return this->FindBlocks(p, -1);
  }

  VISKORES_CONT std::vector<viskores::Id> FindBlocks(
    const viskores::Vec3f& p,
    const std::vector<viskores::Id>& ignoreBlocks) const
  {
    viskores::Id ignoreID = (ignoreBlocks.empty() ? -1 : ignoreBlocks[0]);
    return FindBlocks(p, ignoreID);
  }

  VISKORES_CONT std::vector<viskores::Id> FindBlocks(const viskores::Vec3f& p,
                                                     viskores::Id ignoreBlock) const
  {
    std::vector<viskores::Id> blockIDs;
    if (this->GlobalBounds.Contains(p))
    {
      viskores::Id blockId = 0;
      for (auto& it : this->BlockBounds)
      {
        if (blockId != ignoreBlock && it.Contains(p))
          blockIDs.emplace_back(blockId);
        blockId++;
      }
    }

    return blockIDs;
  }

  VISKORES_CONT viskores::Id GetTotalNumBlocks() const { return this->TotalNumBlocks; }
  VISKORES_CONT viskores::Id GetLocalNumBlocks() const { return this->LocalNumBlocks; }

private:
  VISKORES_CONT void Init(const std::vector<viskores::cont::DataSet>& dataSets,
                          const std::vector<viskores::Id>& blockIds);

  VISKORES_CONT void Init(const std::vector<viskores::cont::DataSet>& dataSets);

  VISKORES_CONT void Build(const std::vector<viskores::cont::DataSet>& dataSets);

  viskores::Id LocalNumBlocks = 0;
  std::vector<viskores::Id> LocalIDs;
  std::map<viskores::Id, std::vector<viskores::Int32>> BlockToRankMap;
  viskores::Id TotalNumBlocks = 0;
  std::vector<viskores::Bounds> BlockBounds;
  viskores::Bounds GlobalBounds;
};

}
}
}
} // namespace viskores::filter::flow::internal

#endif //viskores_filter_flow_internal_BoundsMap_h
