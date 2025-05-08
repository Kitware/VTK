//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#include <fides/DataModel.h>
#include <fides/Value.h>
#include <fides/xgc/XGCCommon.h>

#include <viskores/cont/UnknownArrayHandle.h>

#include <numeric>

namespace fides
{
namespace datamodel
{

class XGCCommon::XGCCommonImpl
{
  /// Functor created so that UnknownArrayHandle's CastAndCall() will handle making the
  /// appropriate cast to an ArrayHandle.
  /// This functor handles setting a scalar value
  struct SetScalarValueFunctor
  {
    template <typename T, typename S>
    VISKORES_CONT void operator()(const viskores::cont::ArrayHandle<T, S>& array,
                                  viskores::Id& value) const
    {
      value = static_cast<viskores::Id>(array.ReadPortal().Get(0));
    }
  };

  viskores::Id ReadScalar(std::string varName,
                          const std::unordered_map<std::string, std::string>& paths,
                          DataSourcesType& sources,
                          std::shared_ptr<Value> numPlanesValue)
  {
    // Since we're reading a scalar value from ADIOS, it should be immediately available,
    // regardless of using sync or deferred Gets.
    // It's also fine to just have an empty selections for reading this.
    fides::metadata::MetaData selections;
    auto valVec = numPlanesValue->Read(paths, sources, selections);
    if (valVec.empty())
    {
      throw std::runtime_error("ArrayXGC: No data read for " + varName);
    }

    auto& valAH = valVec[0];
    if (valAH.GetNumberOfValues() != 1)
    {
      throw std::runtime_error(varName + " should be a scalar value");
    }

    viskores::Id value;
    valAH.CastAndCallForTypes<viskores::TypeListScalarAll,
                              viskores::List<viskores::cont::StorageTagBasic>>(
      SetScalarValueFunctor(), value);
    return value;
  }

  void AddBlock(size_t blockId, viskores::Id startPlaneId, viskores::Id planeCount)
  {
    auto planeInfo = std::make_pair(startPlaneId, planeCount);
    this->PlaneMapping.insert(std::make_pair(blockId, std::move(planeInfo)));
  }

  // block id -> pair(plane id start, num planes in block)
  std::unordered_map<size_t, std::pair<viskores::Id, viskores::Id>> PlaneMapping;
  size_t NumberOfBlocks = 0;
  bool PlanesMapped = false;
  viskores::Id NumberOfPlanes = -1;

public:
  viskores::Id GetNumberOfPlanes(const std::unordered_map<std::string, std::string>& paths,
                                 DataSourcesType& sources,
                                 std::shared_ptr<Value> numPlanesValue)
  {
    this->NumberOfPlanes = this->ReadScalar("number_of_planes", paths, sources, numPlanesValue);
    return this->NumberOfPlanes;
  }

  // in addition to their normal plane assignment, every plane needs to also have the first plane
  // from the next block (or block 0 for the last block), in order to have the cells that are between
  // blocks
  void MapPlanesToBlocks(viskores::Id planesPerUserBlock)
  {
    if (this->NumberOfPlanes <= 0)
    {
      throw std::runtime_error("NumberOfPlanes needs to be set before mapping planes to blocks");
    }
    this->PlanesMapped = true;
    this->NumberOfBlocks = static_cast<size_t>(this->NumberOfPlanes / planesPerUserBlock);
    if (this->NumberOfBlocks == 0 || this->NumberOfBlocks == 1)
    {
      // In this case all planes belong to one block
      this->NumberOfBlocks = 1;
      this->AddBlock(0, 0, this->NumberOfPlanes);
      return;
    }

    // in this case, we need to make sure each block also gets the first plane from the next block
    size_t nPlanesRem = static_cast<size_t>(this->NumberOfPlanes) % this->NumberOfBlocks;
    // need to update planesPerUserBlock since it may have changed
    planesPerUserBlock = this->NumberOfPlanes / static_cast<viskores::Id>(this->NumberOfBlocks);
    viskores::Id startPlaneId = 0;
    for (size_t block = 0; block < this->NumberOfBlocks; ++block)
    {
      viskores::Id planeCount = planesPerUserBlock;
      if (block < nPlanesRem)
      {
        // add a plane to the first nPlanesRem Blocks
        planeCount++;
      }
      // to account for each block essentially needing a ghost plane,
      // increase the plane count, but it shouldn't affect the startPlaneId
      this->AddBlock(block, startPlaneId, planeCount + 1);
      startPlaneId += planeCount;
    }
  }

  // need to determine a plane to block mapping, then when the user does a block
  // selection, we use this mapping to determine which planes (adios blocks) to read
  std::pair<std::vector<XGCBlockInfo>, fides::metadata::Set<size_t>> GetXGCBlockInfo(
    const std::vector<size_t>& userBlocks,
    bool getPlaneSelection)
  {
    std::vector<XGCBlockInfo> allBlocks;
    fides::metadata::Set<size_t> planesToRead;
    for (const auto& b : userBlocks)
    {
      if (b < this->NumberOfBlocks)
      {
        auto& block = this->PlaneMapping[b];
        XGCBlockInfo blockInfo;
        blockInfo.BlockId = b;
        blockInfo.NumberOfPlanesOwned = block.second;
        blockInfo.PlaneStartId = block.first;
        if (getPlaneSelection)
        {
          for (viskores::Id i = blockInfo.PlaneStartId;
               i < blockInfo.PlaneStartId + blockInfo.NumberOfPlanesOwned;
               ++i)
          {
            viskores::Id planeId = i;
            if (planeId == this->NumberOfPlanes)
            {
              // to handle last plane on n-1 block
              planeId = 0;
            }
            planesToRead.Data.insert(static_cast<size_t>(planeId));
          }
        }
        allBlocks.push_back(std::move(blockInfo));
      }
    }
    return std::make_pair(allBlocks, planesToRead);
  }

  size_t GetNumberOfBlocks()
  {
    if (!this->PlanesMapped)
    {
      throw std::runtime_error("Requesting number of blocks when XGC planes haven't"
                               " been mapped to blocks yet.");
    }
    return this->NumberOfBlocks;
  }
};

XGCCommon::XGCCommon()
  : Impl(new XGCCommonImpl())
{
}

XGCCommon::~XGCCommon() = default;

std::shared_ptr<Value> XGCCommon::NumberOfPlanes = nullptr;
viskores::Id XGCCommon::PlanesPerUserBlock = 8;

void XGCCommon::ProcessNumberOfPlanes(const rapidjson::Value& nPlanes, DataSourcesType& sources)
{
  if (!nPlanes.IsObject())
  {
    throw std::runtime_error("number_of_planes should be an object.");
  }
  if (!NumberOfPlanes)
  {
    NumberOfPlanes.reset(new Value());
    NumberOfPlanes->ProcessJSON(nPlanes, sources);
  }
  if (nPlanes.HasMember("planes_per_block") && nPlanes["planes_per_block"].IsInt())
  {
    PlanesPerUserBlock = nPlanes["planes_per_block"].GetInt();
  }
}

viskores::Id XGCCommon::GetNumberOfPlanes(const std::unordered_map<std::string, std::string>& paths,
                                          DataSourcesType& sources)
{
  viskores::Id numberOfPlanes = this->Impl->GetNumberOfPlanes(paths, sources, NumberOfPlanes);
  this->Impl->MapPlanesToBlocks(PlanesPerUserBlock);
  return numberOfPlanes;
}

size_t XGCCommon::GetNumberOfBlocks()
{
  return this->Impl->GetNumberOfBlocks();
}

std::vector<XGCBlockInfo> XGCCommon::GetXGCBlockInfo(const std::vector<size_t>& userBlocks)
{
  if (userBlocks.empty())
  {
    // In this case, we assume the user wants all blocks
    std::vector<size_t> blocks;
    blocks.resize(this->Impl->GetNumberOfBlocks());
    std::iota(blocks.begin(), blocks.end(), 0);
    return this->Impl->GetXGCBlockInfo(blocks, false).first;
  }
  return this->Impl->GetXGCBlockInfo(userBlocks, false).first;
}

std::pair<std::vector<XGCBlockInfo>, fides::metadata::Set<size_t>>
XGCCommon::GetXGCBlockInfoWithPlaneSelection(const std::vector<size_t>& userBlocks)
{
  if (userBlocks.empty())
  {
    // In this case, we assume the user wants all blocks
    std::vector<size_t> blocks;
    blocks.resize(this->Impl->GetNumberOfBlocks());
    std::iota(blocks.begin(), blocks.end(), 0);
    return this->Impl->GetXGCBlockInfo(blocks, true);
  }
  return this->Impl->GetXGCBlockInfo(userBlocks, true);
}

}
}
