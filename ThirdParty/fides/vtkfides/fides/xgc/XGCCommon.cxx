//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#include <fides/internal/DataModel.h>
#include <fides/internal/Value.h>
#include <fides/xgc/XGCCommon.h>

#include <viskores/Math.h>

#include <cmath>
#include <numeric>

namespace fides
{
namespace datamodel
{

class XGCCommon::XGCConfig
{
public:
  std::shared_ptr<Value> NumberOfPlanes;
  viskores::Id PlanesPerUserBlock = 8;
  viskores::Id NumberOfCoordinatePlanes = -1;
  viskores::Float64 PlaneOffset = 0;
  std::string PlaneSpacingDataSource = "mesh";
  bool PlaneSpacingChecked = false;
  bool CoversFullToroidalDomain = true;
  bool IsPeriodic = true;
};

class XGCCommon::XGCCommonImpl
{
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

    auto& raw = valVec[0];
    if (raw.NumValues != 1)
    {
      throw std::runtime_error(varName + " should be a scalar value");
    }

    switch (raw.Type)
    {
      case fides::DataType::Float32:
        return static_cast<viskores::Id>(raw.GetValue<float>(0));
      case fides::DataType::Float64:
        return static_cast<viskores::Id>(raw.GetValue<double>(0));
      case fides::DataType::Int8:
        return static_cast<viskores::Id>(raw.GetValue<int8_t>(0));
      case fides::DataType::Int16:
        return static_cast<viskores::Id>(raw.GetValue<int16_t>(0));
      case fides::DataType::Int32:
        return static_cast<viskores::Id>(raw.GetValue<int32_t>(0));
      case fides::DataType::Int64:
        return static_cast<viskores::Id>(raw.GetValue<int64_t>(0));
      case fides::DataType::UInt8:
        return static_cast<viskores::Id>(raw.GetValue<uint8_t>(0));
      case fides::DataType::UInt16:
        return static_cast<viskores::Id>(raw.GetValue<uint16_t>(0));
      case fides::DataType::UInt32:
        return static_cast<viskores::Id>(raw.GetValue<uint32_t>(0));
      case fides::DataType::UInt64:
        return static_cast<viskores::Id>(raw.GetValue<uint64_t>(0));
      default:
        throw std::runtime_error("Unsupported data type for " + varName);
    }
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
  std::pair<std::vector<XGCBlockInfo>, fides::metadata::Set<size_t>>
  GetXGCBlockInfo(const std::vector<size_t>& userBlocks, bool getPlaneSelection, bool periodic)
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
        if (!periodic && this->NumberOfBlocks > 1 && b == this->NumberOfBlocks - 1)
        {
          // The last block only needs plane 0 as a ghost when the global
          // XGC mesh is periodic.
          --blockInfo.NumberOfPlanesOwned;
        }
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

  size_t GetNumberOfBlocks() const
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
  , Config(CurrentConfig)
{
  if (!this->Config)
    this->Config = std::make_shared<XGCConfig>();
}

XGCCommon::~XGCCommon() = default;

thread_local std::shared_ptr<XGCCommon::XGCConfig> XGCCommon::CurrentConfig;

void XGCCommon::ProcessNumberOfPlanes(const rapidjson::Value& nPlanes, DataSourcesType& sources)
{
  if (!nPlanes.IsObject())
  {
    throw std::runtime_error("number_of_planes should be an object.");
  }
  auto config = std::make_shared<XGCConfig>();
  config->NumberOfPlanes = std::make_shared<Value>();
  config->NumberOfPlanes->ProcessJSON(nPlanes, sources);
  if (nPlanes.HasMember("planes_per_block") && nPlanes["planes_per_block"].IsInt())
  {
    config->PlanesPerUserBlock = nPlanes["planes_per_block"].GetInt();
  }
  CurrentConfig = std::move(config);
}

viskores::Id XGCCommon::GetNumberOfPlanes(const std::unordered_map<std::string, std::string>& paths,
                                          DataSourcesType& sources)
{
  if (!this->Config->NumberOfPlanes)
  {
    throw std::runtime_error("XGC number_of_planes has not been configured.");
  }
  viskores::Id numberOfPlanes =
    this->Impl->GetNumberOfPlanes(paths, sources, this->Config->NumberOfPlanes);
  if (!this->Config->PlaneSpacingChecked)
  {
    this->Config->NumberOfCoordinatePlanes = numberOfPlanes;
    const auto& planeSpacingDataSource = this->Config->PlaneSpacingDataSource;
    auto meshSource = sources.find(planeSpacingDataSource);
    if (meshSource != sources.end())
    {
      meshSource->second->OpenSource(paths, planeSpacingDataSource);
      std::string planeSpacingVariable = "delta_phi";
      bool hasPlaneSpacing = true;
      try
      {
        meshSource->second->GetVariableShape(planeSpacingVariable);
      }
      catch (const std::runtime_error&)
      {
        hasPlaneSpacing = false;
      }
      if (hasPlaneSpacing)
      {
        fides::metadata::MetaData selections;
        auto values = meshSource->second->GetScalarVariable(planeSpacingVariable, selections);
        if (values.size() != 1 || values[0].NumValues != 1)
          throw std::runtime_error("XGC delta_phi should be a scalar value.");
        const auto planeSpacing = GetRawArrayValueAs<viskores::Float64>(values[0], 0);
        if (!(planeSpacing > 0))
          throw std::runtime_error("XGC delta_phi must be positive.");
        const auto coordinatePlanes = static_cast<viskores::Id>(
          std::llround(static_cast<double>(viskores::TwoPi() / planeSpacing)));
        const auto spacingError = std::abs(coordinatePlanes * planeSpacing - viskores::TwoPi());
        if (coordinatePlanes < numberOfPlanes || spacingError > 1.e-6)
        {
          throw std::runtime_error(
            "XGC delta_phi does not describe an integral number of planes in a full turn.");
        }
        this->Config->NumberOfCoordinatePlanes = coordinatePlanes;
        this->Config->CoversFullToroidalDomain = coordinatePlanes == numberOfPlanes;
      }
    }
    this->Config->PlaneSpacingChecked = true;
  }
  this->Impl->MapPlanesToBlocks(this->Config->PlanesPerUserBlock);
  return numberOfPlanes;
}

viskores::Id XGCCommon::GetNumberOfCoordinatePlanes() const
{
  if (this->Config->NumberOfCoordinatePlanes <= 0)
    throw std::runtime_error("XGC number of coordinate planes has not been read.");
  return this->Config->NumberOfCoordinatePlanes;
}

size_t XGCCommon::GetNumberOfBlocks()
{
  return this->Impl->GetNumberOfBlocks();
}

std::vector<XGCBlockInfo> XGCCommon::GetXGCBlockInfo(const std::vector<size_t>& userBlocks,
                                                     bool periodic)
{
  if (userBlocks.empty())
  {
    // In this case, we assume the user wants all blocks
    std::vector<size_t> blocks;
    blocks.resize(this->Impl->GetNumberOfBlocks());
    std::iota(blocks.begin(), blocks.end(), 0);
    return this->Impl->GetXGCBlockInfo(blocks, false, periodic).first;
  }
  return this->Impl->GetXGCBlockInfo(userBlocks, false, periodic).first;
}

std::pair<std::vector<XGCBlockInfo>, fides::metadata::Set<size_t>>
XGCCommon::GetXGCBlockInfoWithPlaneSelection(const std::vector<size_t>& userBlocks, bool periodic)
{
  if (userBlocks.empty())
  {
    // In this case, we assume the user wants all blocks
    std::vector<size_t> blocks;
    blocks.resize(this->Impl->GetNumberOfBlocks());
    std::iota(blocks.begin(), blocks.end(), 0);
    return this->Impl->GetXGCBlockInfo(blocks, true, periodic);
  }
  return this->Impl->GetXGCBlockInfo(userBlocks, true, periodic);
}

void XGCCommon::SetIsPeriodic(bool periodic)
{
  this->Config->IsPeriodic = periodic;
}

void XGCCommon::SetPlaneSpacingDataSource(const std::string& dataSourceName)
{
  this->Config->PlaneSpacingDataSource = dataSourceName;
}

bool XGCCommon::GetIsPeriodic(const fides::metadata::MetaData& selections) const
{
  if (selections.Has(fides::keys::fusion::FUSION_PERIODIC_CELLSET()))
  {
    return selections.Get<fides::metadata::Bool>(fides::keys::fusion::FUSION_PERIODIC_CELLSET())
      .Value;
  }
  return this->Config->IsPeriodic && this->Config->CoversFullToroidalDomain;
}

void XGCCommon::SetPlaneOffset(viskores::Float64 planeOffset)
{
  this->Config->PlaneOffset = planeOffset;
}

viskores::Float64 XGCCommon::GetPlaneOffset() const
{
  return this->Config->PlaneOffset;
}
}
}
