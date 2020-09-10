//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#include <fides/CellSet.h>

#include <vtkm/cont/Algorithm.h>
#include <vtkm/CellShape.h>
#include <vtkm/cont/CellSetExtrude.h>

namespace fides
{
namespace datamodel
{

void CellSet::ProcessJSON(const rapidjson::Value& json,
                          DataSourcesType& sources)
{
  if (!json.HasMember("cell_set_type") || !json["cell_set_type"].IsString())
  {
    throw std::runtime_error(
      this->ObjectName  + " must provide a valid cell_set_type.");
  }
  const std::string& cellSetType = json["cell_set_type"].GetString();
  if (cellSetType == "structured")
  {
    this->CellSetImpl.reset(new CellSetStructured());
  }
  else if (cellSetType == "single_type")
  {
    this->CellSetImpl.reset(new CellSetSingleType());
  }
  else if (cellSetType == "explicit")
  {
    this->CellSetImpl.reset(new CellSetExplicit());
  }
  else if (cellSetType == "xgc")
  {
    this->CellSetImpl.reset(new CellSetXGC());
  }
  else
  {
    throw std::runtime_error(cellSetType + " is not a valid cell_set type.");
  }
  this->CellSetImpl->ProcessJSON(json, sources);
}

std::vector<vtkm::cont::DynamicCellSet> CellSet::Read(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources,
  const fides::metadata::MetaData& selections)
{
  return this->CellSetImpl->Read(paths, sources, selections);
}

void CellSet::PostRead(
  vtkm::cont::PartitionedDataSet& partitions,
  const fides::metadata::MetaData& selections)
{
  this->CellSetImpl->PostRead(partitions, selections);
}

void CellSetSingleType::ProcessJSON(const rapidjson::Value& json,
                                    DataSourcesType& sources)
{
  this->CellSetBase::ProcessJSON(json, sources);

  if (!json.HasMember("cell_type"))
  {
    throw std::runtime_error(
      this->ObjectName  + " must provide a cell_type.");
  }
  std::string cellType = json["cell_type"].GetString();

  if (cellType == "vertex")
  {
    this->CellInformation = std::pair<unsigned char, int>(
      vtkm::CELL_SHAPE_VERTEX, 1);
  }
  else if (cellType == "line")
  {
    this->CellInformation = std::pair<unsigned char, int>(
      vtkm::CELL_SHAPE_LINE, 2);
  }
  else if (cellType == "triangle")
  {
    this->CellInformation = std::pair<unsigned char, int>(
      vtkm::CELL_SHAPE_TRIANGLE, 3);
  }
  else if (cellType == "quad")
  {
    this->CellInformation = std::pair<unsigned char, int>(
      vtkm::CELL_SHAPE_QUAD, 4);
  }
  else if (cellType == "tetrahedron")
  {
    this->CellInformation = std::pair<unsigned char, int>(
      vtkm::CELL_SHAPE_TETRA, 4);
  }
  else if (cellType == "hexahedron")
  {
    this->CellInformation = std::pair<unsigned char, int>(
      vtkm::CELL_SHAPE_HEXAHEDRON, 8);
  }
  else if (cellType == "wedge")
  {
    this->CellInformation = std::pair<unsigned char, int>(
      vtkm::CELL_SHAPE_WEDGE, 6);
  }
  else if (cellType == "pyramid")
  {
    this->CellInformation = std::pair<unsigned char, int>(
      vtkm::CELL_SHAPE_PYRAMID, 5);
  }
  else
  {
    throw std::runtime_error(
      "Unrecognized cell type " + cellType);
  }
}

std::vector<vtkm::cont::DynamicCellSet> CellSetSingleType::Read(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources,
  const fides::metadata::MetaData& selections)
{
  if(this->IsStatic && !this->CellSetCache.empty())
  {
    return this->CellSetCache;
  }

  // Temporarily setting IsStatic to false to avoid
  // caching the array also.
  bool isStatic = this->IsStatic;
  this->IsStatic = false;
  this->ConnectivityArrays = this->ReadSelf(paths, sources, selections);
  this->IsStatic = isStatic;
  std::vector<vtkm::cont::DynamicCellSet> cellSets;
  size_t nArrays = this->ConnectivityArrays.size();
  cellSets.reserve(nArrays);
  for(size_t i=0; i<nArrays; i++)
  {
    vtkm::cont::CellSetSingleType<> cellSet;
    cellSets.push_back(cellSet);
  }
  if (this->IsStatic)
  {
    this->CellSetCache = cellSets;
  }
  return cellSets;
}

void CellSetSingleType::PostRead(
  vtkm::cont::PartitionedDataSet& partitions,
  const fides::metadata::MetaData& fidesNotUsed(selections))
{
  size_t nParts = static_cast<size_t>(partitions.GetNumberOfPartitions());
  for(size_t i=0; i<nParts; i++)
  {
    auto& pds = partitions.GetPartition(static_cast<vtkm::Id>(i));
    vtkm::cont::ArrayHandle<vtkm::Id> connCasted =
      this->ConnectivityArrays[i].Cast<vtkm::cont::ArrayHandle<vtkm::Id> >();
    auto& cellSet = pds.GetCellSet().Cast<vtkm::cont::CellSetSingleType<>>();
    cellSet.Fill(pds.GetNumberOfPoints(),
                 this->CellInformation.first,
                 this->CellInformation.second,
                 connCasted);
  }
  if (!this->IsStatic)
  {
    this->ConnectivityArrays.clear();
  }
}

void CellSetExplicit::ProcessJSON(const rapidjson::Value& json,
                                    DataSourcesType& sources)
{
  if (!json.HasMember("cell_types") || !json["cell_types"].IsObject())
  {
    throw std::runtime_error(
      this->ObjectName  + " must provide a cell_types object.");
  }
  this->CellTypes.reset(new Array());
  const auto& cellTypes = json["cell_types"];
  this->CellTypes->ProcessJSON(cellTypes, sources);

  if (!json.HasMember("number_of_vertices") || !json["number_of_vertices"].IsObject())
  {
    throw std::runtime_error(
      this->ObjectName  + " must provide a number_of_vertices object.");
  }
  this->NumberOfVertices.reset(new Array());
  const auto& numVertices = json["number_of_vertices"];
  this->NumberOfVertices->ProcessJSON(numVertices, sources);

  if (!json.HasMember("connectivity") || !json["connectivity"].IsObject())
  {
    throw std::runtime_error(
      this->ObjectName  + " must provide a connectivity object.");
  }
  this->Connectivity.reset(new Array());
  const auto& conn = json["connectivity"];
  this->Connectivity->ProcessJSON(conn, sources);
}

std::vector<vtkm::cont::DynamicCellSet> CellSetExplicit::Read(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources,
  const fides::metadata::MetaData& selections)
{
  if(this->IsStatic && !this->CellSetCache.empty())
  {
    return this->CellSetCache;
  }

  this->ConnectivityArrays =
    this->Connectivity->Read(paths, sources, selections);
  this->NumberOfVerticesArrays =
    this->NumberOfVertices->Read(paths, sources, selections);
  this->CellTypesArrays =
    this->CellTypes->Read(paths, sources, selections);

  std::vector<vtkm::cont::DynamicCellSet> cellSets;
  size_t nArrays = this->ConnectivityArrays.size();
  cellSets.reserve(nArrays);
  for(size_t i=0; i<nArrays; i++)
  {
    vtkm::cont::CellSetExplicit<> cellSet;
    cellSets.push_back(cellSet);
  }
  if (this->IsStatic)
  {
    this->CellSetCache = cellSets;
  }
  return cellSets;
}

void CellSetExplicit::PostRead(
  vtkm::cont::PartitionedDataSet& partitions,
  const fides::metadata::MetaData& fidesNotUsed(selections))
{
  size_t nParts = static_cast<size_t>(partitions.GetNumberOfPartitions());
  for(size_t i=0; i<nParts; i++)
  {
    const auto& pds = partitions.GetPartition(static_cast<vtkm::Id>(i));
    vtkm::cont::ArrayHandle<vtkm::IdComponent> nVertsCasted =
      this->NumberOfVerticesArrays[i].Cast<vtkm::cont::ArrayHandle<vtkm::IdComponent> >();
    vtkm::cont::ArrayHandle<vtkm::Id> offsets;
    vtkm::cont::Algorithm::ScanExtended(
      vtkm::cont::make_ArrayHandleCast<vtkm::Id, vtkm::cont::ArrayHandle<vtkm::IdComponent>>(nVertsCasted), offsets);
    vtkm::cont::ArrayHandle<vtkm::Id> connCasted =
      this->ConnectivityArrays[i].Cast<vtkm::cont::ArrayHandle<vtkm::Id> >();
    vtkm::cont::ArrayHandle<vtkm::UInt8> typesCasted =
      this->CellTypesArrays[i].Cast<vtkm::cont::ArrayHandle<vtkm::UInt8> >();
    auto& cellSet = pds.GetCellSet().Cast<vtkm::cont::CellSetExplicit<>>();
    cellSet.Fill(pds.GetNumberOfPoints(), typesCasted, connCasted, offsets);
  }
  if (!this->IsStatic)
  {
    this->ConnectivityArrays.clear();
    this->NumberOfVerticesArrays.clear();
    this->CellTypesArrays.clear();
  }
}

void CellSetStructured::ProcessJSON(const rapidjson::Value& json,
                                    DataSourcesType& sources)
{
  if (!json.HasMember("dimensions") || !json["dimensions"].IsObject())
  {
    throw std::runtime_error(
      this->ObjectName  + " must provide a dimensions object.");
  }
  this->Dimensions.reset(new Value());
  const auto& dimensions = json["dimensions"];
  this->Dimensions->ProcessJSON(dimensions, sources);
}

std::vector<vtkm::cont::DynamicCellSet> CellSetStructured::Read(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources,
  const fides::metadata::MetaData& selections)
{
  std::vector<vtkm::cont::VariantArrayHandle> dims =
    this->Dimensions->Read(paths, sources, selections);
  std::vector<vtkm::cont::DynamicCellSet> ret;
  ret.reserve(dims.size());
  for(const auto& array : dims)
  {
    auto dimsB = array.Cast<vtkm::cont::ArrayHandle<size_t> >();
    auto dimsPortal = dimsB.ReadPortal();
    vtkm::Id3 dimValues(static_cast<vtkm::Id>(dimsPortal.Get(0)),
                        static_cast<vtkm::Id>(dimsPortal.Get(1)),
                        static_cast<vtkm::Id>(dimsPortal.Get(2)));
    vtkm::cont::CellSetStructured<3> cellSet;
    cellSet.SetPointDimensions(dimValues);
    vtkm::Id3 start(static_cast<vtkm::Id>(dimsPortal.Get(3)),
                    static_cast<vtkm::Id>(dimsPortal.Get(4)),
                    static_cast<vtkm::Id>(dimsPortal.Get(5)));
    cellSet.SetGlobalPointIndexStart(start);
    ret.push_back(cellSet);
  }
  return ret;
}


void CellSetXGC::ProcessJSON(const rapidjson::Value& json,
                                    DataSourcesType& sources)
{
    if (!json.HasMember("cells") || !json["cells"].IsObject())
    {
      throw std::runtime_error("must provide a cells object for XGC CellSet.");
    }
    this->CellConnectivity.reset(new Array());
    this->CellConnectivity->ProcessJSON(json["cells"], sources);

    if (!json.HasMember("plane_connectivity") || !json["plane_connectivity"].IsObject())
    {
      throw std::runtime_error("must provide a plane_connectivity object for XGC CellSet.");
    }
    this->PlaneConnectivity.reset(new Array());
    this->PlaneConnectivity->ProcessJSON(json["plane_connectivity"], sources);

    if (json.HasMember("periodic") && json["periodic"].IsBool())
    {
      this->IsPeriodic = json["periodic"].GetBool();
    }
}

std::vector<vtkm::cont::DynamicCellSet> CellSetXGC::Read(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources,
  const fides::metadata::MetaData& selections)
{
  if(this->IsStatic && !this->CellSetCache.empty())
  {
    return this->CellSetCache;
  }

  if (this->NumberOfPlanes < 0)
  {
    this->NumberOfPlanes = this->CommonImpl->GetNumberOfPlanes(paths, sources);
  }

  fides::metadata::MetaData newSelections = selections;
  newSelections.Remove(fides::keys::BLOCK_SELECTION());

  std::vector<vtkm::cont::DynamicCellSet> cellSets;

  //load the connect_list
  std::vector<vtkm::cont::VariantArrayHandle> connectivityVec =
    this->CellConnectivity->Read(paths, sources, newSelections);
  if (connectivityVec.size() != 1)
  {
    throw std::runtime_error("XGC CellConnectivity should have one Array");
  }

  using intType = vtkm::cont::ArrayHandle<vtkm::Int32>;
  intType connectivityAH;
  if (connectivityVec[0].IsType<intType>())
  {
    connectivityVec[0].CopyTo(connectivityAH);
  }
  else
  {
    throw std::runtime_error(
      "Only int arrays are supported for XGC cell connectivity.");
  }

  std::vector<vtkm::cont::VariantArrayHandle> planeConnectivityVec =
      this->PlaneConnectivity->Read(paths, sources, newSelections);

  if (planeConnectivityVec.size() > 1)
  {
    throw std::runtime_error("xgc nextNode is supposed to be included in one array.");
  }
  intType planeConnectivityAH;
  if (planeConnectivityVec[0].IsType<intType>())
  {
    planeConnectivityVec[0].CopyTo(planeConnectivityAH);
  }
  else
  {
    throw std::runtime_error(
      "Only int arrays are supported for XGC plane connectivity.");
  }

  auto numPointsPerPlane = planeConnectivityVec[0].GetNumberOfValues();
  // blocks info doesn't need to be added to the selection for CellSet, since
  // it's not needed for reading the data
  std::vector<XGCBlockInfo> blocksInfo;
  if (selections.Has(fides::keys::BLOCK_SELECTION()))
  {
    blocksInfo = this->CommonImpl->GetXGCBlockInfo(
        selections.Get<fides::metadata::Vector<size_t> >(fides::keys::BLOCK_SELECTION()).Data);
  }
  else
  {
    blocksInfo = this->CommonImpl->GetXGCBlockInfo(std::vector<size_t>());
  }
  if (blocksInfo.empty())
  {
    throw std::runtime_error("No XGC block info returned. May want to double check block selection.");
  }

  for (size_t i = 0; i < blocksInfo.size(); ++i)
  {
    const auto& block = blocksInfo[i];
    auto xgcCell = vtkm::cont::CellSetExtrude(connectivityAH,
      static_cast<vtkm::Int32>(numPointsPerPlane),
      static_cast<vtkm::Int32>(block.NumberOfPlanesOwned),
      planeConnectivityAH, this->IsPeriodic);

    cellSets.push_back(xgcCell);
  }

  if (this->IsStatic)
  {
    this->CellSetCache = cellSets;
  }
  return cellSets;
}

}
}
