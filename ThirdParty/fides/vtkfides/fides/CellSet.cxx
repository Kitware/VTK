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

#include <vtkm/CellShape.h>
#include <vtkm/VectorAnalysis.h>
#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayHandleCast.h>
#include <vtkm/cont/CellSetExtrude.h>
#include <vtkm/cont/UnknownArrayHandle.h>

namespace fides
{
namespace datamodel
{

void CellSet::ProcessJSON(const rapidjson::Value& json, DataSourcesType& sources)
{
  if (!json.HasMember("cell_set_type") || !json["cell_set_type"].IsString())
  {
    throw std::runtime_error(this->ObjectName + " must provide a valid cell_set_type.");
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
  else if (cellSetType == "gtc")
  {
    this->CellSetImpl.reset(new CellSetGTC());
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

void CellSet::PostRead(std::vector<vtkm::cont::DataSet>& partitions,
                       const fides::metadata::MetaData& selections)
{
  this->CellSetImpl->PostRead(partitions, selections);
}

void CellSetSingleType::ProcessJSON(const rapidjson::Value& json, DataSourcesType& sources)
{
  this->CellSetBase::ProcessJSON(json, sources);

  if (!json.HasMember("cell_type"))
  {
    throw std::runtime_error(this->ObjectName + " must provide a cell_type.");
  }
  std::string cellType = json["cell_type"].GetString();

  if (cellType == "vertex")
  {
    this->CellInformation = std::pair<unsigned char, int>(vtkm::CELL_SHAPE_VERTEX, 1);
  }
  else if (cellType == "line")
  {
    this->CellInformation = std::pair<unsigned char, int>(vtkm::CELL_SHAPE_LINE, 2);
  }
  else if (cellType == "triangle")
  {
    this->CellInformation = std::pair<unsigned char, int>(vtkm::CELL_SHAPE_TRIANGLE, 3);
  }
  else if (cellType == "quad")
  {
    this->CellInformation = std::pair<unsigned char, int>(vtkm::CELL_SHAPE_QUAD, 4);
  }
  else if (cellType == "tetrahedron")
  {
    this->CellInformation = std::pair<unsigned char, int>(vtkm::CELL_SHAPE_TETRA, 4);
  }
  else if (cellType == "hexahedron")
  {
    this->CellInformation = std::pair<unsigned char, int>(vtkm::CELL_SHAPE_HEXAHEDRON, 8);
  }
  else if (cellType == "wedge")
  {
    this->CellInformation = std::pair<unsigned char, int>(vtkm::CELL_SHAPE_WEDGE, 6);
  }
  else if (cellType == "pyramid")
  {
    this->CellInformation = std::pair<unsigned char, int>(vtkm::CELL_SHAPE_PYRAMID, 5);
  }
  else
  {
    throw std::runtime_error("Unrecognized cell type " + cellType);
  }
}

std::vector<vtkm::cont::DynamicCellSet> CellSetSingleType::Read(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources,
  const fides::metadata::MetaData& selections)
{
  if (this->IsStatic && !this->CellSetCache.empty())
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
  for (size_t i = 0; i < nArrays; i++)
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

void CellSetSingleType::PostRead(std::vector<vtkm::cont::DataSet>& partitions,
                                 const fides::metadata::MetaData& fidesNotUsed(selections))
{
  size_t nParts = partitions.size();
  for (size_t i = 0; i < nParts; i++)
  {
    auto& pds = partitions[i];
    vtkm::cont::ArrayHandle<vtkm::Id> connCasted =
      this->ConnectivityArrays[i].Cast<vtkm::cont::ArrayHandle<vtkm::Id>>();
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

void CellSetExplicit::ProcessJSON(const rapidjson::Value& json, DataSourcesType& sources)
{
  if (!json.HasMember("cell_types") || !json["cell_types"].IsObject())
  {
    std::string err = std::string(__FILE__) + ":" + std::to_string(__LINE__) +
      ": Must provide a cell_types object for object " + this->ObjectName;
    throw std::runtime_error(err);
  }
  this->CellTypes.reset(new Array());
  const auto& cellTypes = json["cell_types"];
  this->CellTypes->ProcessJSON(cellTypes, sources);

  if (!json.HasMember("number_of_vertices") || !json["number_of_vertices"].IsObject())
  {
    std::string err = std::string(__FILE__) + ":" + std::to_string(__LINE__) +
      ": Must provide a number_of_vertices object for object " + this->ObjectName;
    throw std::runtime_error(err);
  }
  this->NumberOfVertices.reset(new Array());
  const auto& numVertices = json["number_of_vertices"];
  this->NumberOfVertices->ProcessJSON(numVertices, sources);

  if (!json.HasMember("connectivity") || !json["connectivity"].IsObject())
  {
    std::string err = std::string(__FILE__) + ":" + std::to_string(__LINE__) +
      ": Must provide a connectivity object for object " + this->ObjectName;
    throw std::runtime_error(err);
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
  if (this->IsStatic && !this->CellSetCache.empty())
  {
    return this->CellSetCache;
  }

  this->ConnectivityArrays = this->Connectivity->Read(paths, sources, selections);
  this->NumberOfVerticesArrays = this->NumberOfVertices->Read(paths, sources, selections);
  this->CellTypesArrays = this->CellTypes->Read(paths, sources, selections);

  std::vector<vtkm::cont::DynamicCellSet> cellSets;
  size_t nArrays = this->ConnectivityArrays.size();
  cellSets.reserve(nArrays);
  for (size_t i = 0; i < nArrays; i++)
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

void CellSetExplicit::PostRead(std::vector<vtkm::cont::DataSet>& partitions,
                               const fides::metadata::MetaData& fidesNotUsed(selections))
{
  size_t nParts = partitions.size();
  for (size_t i = 0; i < nParts; i++)
  {
    const auto& pds = partitions[i];
    vtkm::cont::ArrayHandle<vtkm::IdComponent> nVertsCasted =
      this->NumberOfVerticesArrays[i].Cast<vtkm::cont::ArrayHandle<vtkm::IdComponent>>();
    vtkm::cont::ArrayHandle<vtkm::Id> offsets;
    vtkm::cont::Algorithm::ScanExtended(
      vtkm::cont::make_ArrayHandleCast<vtkm::Id, vtkm::cont::ArrayHandle<vtkm::IdComponent>>(
        nVertsCasted),
      offsets);
    vtkm::cont::ArrayHandle<vtkm::Id> connCasted =
      this->ConnectivityArrays[i].Cast<vtkm::cont::ArrayHandle<vtkm::Id>>();
    vtkm::cont::ArrayHandle<vtkm::UInt8> typesCasted =
      this->CellTypesArrays[i].Cast<vtkm::cont::ArrayHandle<vtkm::UInt8>>();
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

void CellSetStructured::ProcessJSON(const rapidjson::Value& json, DataSourcesType& sources)
{
  if (!json.HasMember("dimensions") || !json["dimensions"].IsObject())
  {
    throw std::runtime_error(this->ObjectName + " must provide a dimensions object.");
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
  this->DimensionArrays = this->Dimensions->Read(paths, sources, selections);

  std::size_t nArrays = this->DimensionArrays.size();
  std::vector<vtkm::cont::DynamicCellSet> ret;
  ret.reserve(nArrays);

  for (std::size_t i = 0; i < nArrays; i++)
  {
    vtkm::cont::CellSetStructured<3> cellSet;
    ret.push_back(cellSet);
  }
  return ret;
}

void CellSetStructured::PostRead(std::vector<vtkm::cont::DataSet>& partitions,
                                 const fides::metadata::MetaData& fidesNotUsed(selections))
{
  std::size_t nParts = partitions.size();

  for (std::size_t i = 0; i < nParts; i++)
  {
    const auto& ds = partitions[i];
    auto& cellSet = ds.GetCellSet().Cast<vtkm::cont::CellSetStructured<3>>();
    const auto& dimArray = this->DimensionArrays[i].Cast<vtkm::cont::ArrayHandle<std::size_t>>();
    auto dimPortal = dimArray.ReadPortal();

    vtkm::Id3 dims(static_cast<vtkm::Id>(dimPortal.Get(0)),
                   static_cast<vtkm::Id>(dimPortal.Get(1)),
                   static_cast<vtkm::Id>(dimPortal.Get(2)));
    cellSet.SetPointDimensions(dims);

    if (dimArray.GetNumberOfValues() > 3)
    {
      vtkm::Id3 start(static_cast<vtkm::Id>(dimPortal.Get(3)),
                      static_cast<vtkm::Id>(dimPortal.Get(4)),
                      static_cast<vtkm::Id>(dimPortal.Get(5)));
      cellSet.SetGlobalPointIndexStart(start);
    }
  }
}


void CellSetXGC::ProcessJSON(const rapidjson::Value& json, DataSourcesType& sources)
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
  if (this->IsStatic && !this->CellSetCache.empty())
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
    throw std::runtime_error("Only int arrays are supported for XGC cell connectivity.");
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
    throw std::runtime_error("Only int arrays are supported for XGC plane connectivity.");
  }

  auto numPointsPerPlane = planeConnectivityVec[0].GetNumberOfValues();
  // blocks info doesn't need to be added to the selection for CellSet, since
  // it's not needed for reading the data
  std::vector<XGCBlockInfo> blocksInfo;
  if (selections.Has(fides::keys::BLOCK_SELECTION()))
  {
    blocksInfo = this->CommonImpl->GetXGCBlockInfo(
      selections.Get<fides::metadata::Vector<size_t>>(fides::keys::BLOCK_SELECTION()).Data);
  }
  else
  {
    blocksInfo = this->CommonImpl->GetXGCBlockInfo(std::vector<size_t>());
  }
  if (blocksInfo.empty())
  {
    throw std::runtime_error(
      "No XGC block info returned. May want to double check block selection.");
  }

  for (size_t i = 0; i < blocksInfo.size(); ++i)
  {
    const auto& block = blocksInfo[i];
    auto xgcCell = vtkm::cont::CellSetExtrude(connectivityAH,
                                              static_cast<vtkm::Int32>(numPointsPerPlane),
                                              static_cast<vtkm::Int32>(block.NumberOfPlanesOwned),
                                              planeConnectivityAH,
                                              this->IsPeriodic);

    cellSets.push_back(xgcCell);
  }

  if (this->IsStatic)
  {
    this->CellSetCache = cellSets;
  }
  return cellSets;
}


void CellSetGTC::ProcessJSON(const rapidjson::Value& json, DataSourcesType& sources)
{
  if (!json.HasMember("igrid") || !json["igrid"].IsObject())
  {
    throw std::runtime_error("must provide a igrid object for GTC CellSet.");
  }
  this->IGrid.reset(new Array());
  this->IGrid->ProcessJSON(json["igrid"], sources);

  if (!json.HasMember("index_shift") || !json["index_shift"].IsObject())
  {
    throw std::runtime_error("must provide a index_shift object for GTC CellSet.");
  }
  this->IndexShift.reset(new Array());
  this->IndexShift->ProcessJSON(json["index_shift"], sources);

  if (!json.HasMember("num_planes") || !json["num_planes"].IsObject())
  {
    throw std::runtime_error("must provide a num_planes object for GTC CellSet.");
  }
  this->NumPlanes.reset(new Array());
  this->NumPlanes->ProcessJSON(json["num_planes"], sources);

  if (!json.HasMember("num_pts_per_plane") || !json["num_pts_per_plane"].IsObject())
  {
    throw std::runtime_error("must provide a num_pts_per_plane object for GTC CellSet.");
  }
  this->NumPtsPerPlane.reset(new Array());
  this->NumPtsPerPlane->ProcessJSON(json["num_pts_per_plane"], sources);
}

std::vector<vtkm::cont::DynamicCellSet> CellSetGTC::Read(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources,
  const fides::metadata::MetaData& selections)
{
  std::vector<vtkm::cont::DynamicCellSet> cellSets;

  if (!this->IsCached)
  {
    this->IGridArrays = this->IGrid->Read(paths, sources, selections);
    if (this->IGridArrays.size() != 1)
    {
      throw std::runtime_error("igrid object not found for GTC CellSet.");
    }

    this->IndexShiftArrays = this->IndexShift->Read(paths, sources, selections);
    if (this->IndexShiftArrays.size() != 1)
    {
      throw std::runtime_error("index_shift object not found for GTC CellSet.");
    }

    this->NumPlanesArrays = this->NumPlanes->Read(paths, sources, selections);
    if (this->NumPlanesArrays.size() != 1 || this->NumPlanesArrays[0].GetNumberOfValues() != 1)
    {
      throw std::runtime_error("num_planes object not found for GTC CellSet.");
    }

    this->NumPtsPerPlaneArrays = this->NumPtsPerPlane->Read(paths, sources, selections);
    if (this->NumPtsPerPlaneArrays.size() != 1 ||
        this->NumPtsPerPlaneArrays[0].GetNumberOfValues() != 1)
    {
      throw std::runtime_error("num_pts_per_plane object not found for GTC CellSet.");
    }

    //Create the cell sets. We will fill them in PostRead
    vtkm::cont::CellSetSingleType<> cellSet;
    this->CachedCellSet = cellSet;
  }

  cellSets.push_back(this->CachedCellSet);
  return cellSets;
}

void CellSetGTC::PostRead(std::vector<vtkm::cont::DataSet>& partitions,
                          const fides::metadata::MetaData& fidesNotUsed(selections))
{
  if (partitions.size() != 1)
  {
    throw std::runtime_error("Wrong type for partitions for GTC DataSets.");
  }

  if (this->IsCached)
  {
    return;
  }

  auto nPlns = this->NumPlanesArrays[0];
  auto nPtsPerPln = this->NumPtsPerPlaneArrays[0];

  using intType = vtkm::cont::ArrayHandle<int>;
  if (!(nPlns.IsType<intType>() && nPtsPerPln.IsType<intType>()))
  {
    throw std::runtime_error("Wrong type for num_planes and/or num_pts_per_plane in GTC cell.");
  }

  auto numPlanesI = nPlns.AsArrayHandle<intType>();
  auto numPtsPerPlaneI = nPtsPerPln.AsArrayHandle<intType>();

  this->NumberOfPointsPerPlane = static_cast<vtkm::Id>(numPtsPerPlaneI.ReadPortal().Get(0));
  this->NumberOfPlanes = static_cast<vtkm::Id>(numPlanesI.ReadPortal().Get(0));

  auto ds = partitions[0];
  this->ComputeCellSet(ds);

  //Cached cellset has been computed now.
  this->IsCached = true;
}

template <typename T, typename C>
std::vector<vtkm::Id> CellSetGTC::ComputeConnectivity(
  const vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, C>& coords,
  const vtkm::cont::ArrayHandle<int>& igrid,
  const vtkm::cont::ArrayHandle<int>& indexShift)
{
  std::vector<vtkm::Id> nPoloidalNodes; // Number of nodes in each poloidal contour
  std::vector<vtkm::Id> poloidalIndex;  // Starting node index of each poloidal contour
  vtkm::Id nNodes = this->NumberOfPointsPerPlane;

  auto igridPortal = igrid.ReadPortal();
  auto coordsPortal = coords.ReadPortal();

  for (vtkm::Id i = 0; i < igrid.GetNumberOfValues() - 1; i++)
  {
    poloidalIndex.push_back(igridPortal.Get(i));
    vtkm::Id nPts = static_cast<vtkm::Id>(igridPortal.Get(i + 1) - igridPortal.Get(i));
    nPoloidalNodes.push_back(nPts);
  }

  // For each point get the nearest neighbor on the flux surface.
  // Work from the outside to the inside because there are more nodes
  // on the outside. As such, neighbors will get used multiple times
  // thus allowing for degenerate connections to be found.
  std::vector<vtkm::Id> neighborIndex(nNodes);
  for (size_t k = nPoloidalNodes.size() - 1; k > 0; --k)
  {
    for (size_t j = 0; j < static_cast<size_t>(nPoloidalNodes[k] - 1); ++j)
    {
      // Index of the working node.
      int l = poloidalIndex[k] + j;
      auto basePt = coordsPortal.Get(l);

      // Find the node on the adjacent contour that is the closest to
      // the working node. Brute force search.
      // Never search the last node because it is the same as the
      // first node.
      T minDist = vtkm::Infinity<T>();
      for (size_t i = 0; i < static_cast<size_t>(nPoloidalNodes[k - 1] - 1); i++)
      {
        // Index of the test node.
        vtkm::Id m = poloidalIndex[k - 1] + i;
        auto tmpPt = coordsPortal.Get(m);

        auto dist = vtkm::MagnitudeSquared(basePt - tmpPt);
        if (dist < minDist)
        {
          neighborIndex[l] = m;
          minDist = dist;
        }
      }
    }
  }

  std::vector<vtkm::Id> vtxList;
  vtkm::Id nElements = 0;

  // Work from the outside to the inside because there are more nodes
  // on the outside. As such, neighbors will get used multiple times
  // thus allowing for degenerate connections to be found.
  for (size_t k = nPoloidalNodes.size() - 1; k > 0; k--)
  {
    for (size_t j = 0; j < static_cast<size_t>(nPoloidalNodes[k] - 1); j++)
    {
      vtkm::Id l = poloidalIndex[k] + static_cast<vtkm::Id>(j);
      vtkm::Id l1 = (l + 1);

      // Never use the last node cause it is the same as the first node.
      if (l1 == poloidalIndex[k] + nPoloidalNodes[k] - 1)
        l1 = poloidalIndex[k];

      // Check for a degenerate triangle
      if (neighborIndex[l] == neighborIndex[l1])
      {
        vtxList.push_back(l);
        vtxList.push_back(l1);
        vtxList.push_back(neighborIndex[l]);
        nElements++;
      }
      else
      {
        vtxList.push_back(l);
        vtxList.push_back(l1);
        vtxList.push_back(neighborIndex[l]);
        nElements++;

        vtxList.push_back(l1);
        vtxList.push_back(neighborIndex[l1]);
        vtxList.push_back(neighborIndex[l]);
        nElements++;
      }
    }
  }

  std::vector<vtkm::Id> connIds;
  connIds.reserve(nElements * 6 * (this->NumberOfPlanes - 1));
  for (vtkm::Id i = 0; i < this->NumberOfPlanes - 1; i++)
  {
    vtkm::Id off = i * this->NumberOfPointsPerPlane;
    vtkm::Id off2 = (i + 1) * this->NumberOfPointsPerPlane;
    for (vtkm::Id j = 0; j < nElements; j++)
    {
      connIds.push_back(vtxList[j * 3 + 0] + off);
      connIds.push_back(vtxList[j * 3 + 1] + off);
      connIds.push_back(vtxList[j * 3 + 2] + off);

      connIds.push_back(vtxList[j * 3 + 0] + off2);
      connIds.push_back(vtxList[j * 3 + 1] + off2);
      connIds.push_back(vtxList[j * 3 + 2] + off2);
    }
  }

  //Connect the first/last plane.
  //Uses index-shift to map between flux surfaces.
  auto indexShiftPortal = indexShift.ReadPortal();
  std::vector<vtkm::Id> pn(this->NumberOfPointsPerPlane, -1);
  vtkm::Id n = igridPortal.GetNumberOfValues();
  for (vtkm::Id gi = 0; gi < n - 1; gi++)
  {
    vtkm::Id n0 = igridPortal.Get(gi);
    vtkm::Id nn = igridPortal.Get(gi + 1) - 1;
    vtkm::Id shift = static_cast<vtkm::Id>(indexShiftPortal.Get(gi));

    for (vtkm::Id i = 0; i < (nn - n0); i++)
    {
      vtkm::Id i0 = i;
      vtkm::Id i1 = i - shift;
      if (i1 < 0)
        i1 = i1 + (nn - n0);
      pn[n0 + i0] = n0 + i1;
    }
  }

  vtkm::Id offset = nNodes * (this->NumberOfPlanes - 1);
  for (vtkm::Id i = 0; i < nElements; i++)
  {
    vtkm::Id ids[3] = { connIds[i * 6 + 0], connIds[i * 6 + 1], connIds[i * 6 + 2] };
    if (!(ids[0] < nNodes && ids[1] < nNodes && ids[2] < nNodes))
      throw std::runtime_error("Invalid connectivity for GTC Cellset.");

    //plane 0
    connIds.push_back(ids[0]);
    connIds.push_back(ids[1]);
    connIds.push_back(ids[2]);

    //plane N-1
    connIds.push_back(pn[ids[0]] + offset);
    connIds.push_back(pn[ids[1]] + offset);
    connIds.push_back(pn[ids[2]] + offset);
  }

  return connIds;
}

void CellSetGTC::ComputeCellSet(vtkm::cont::DataSet& dataSet)
{
  vtkm::cont::ArrayHandle<int> igrid =
    this->IGridArrays[0].AsArrayHandle<vtkm::cont::ArrayHandle<int>>();
  vtkm::cont::ArrayHandle<int> indexShift =
    this->IndexShiftArrays[0].AsArrayHandle<vtkm::cont::ArrayHandle<int>>();

  //These are fortran indices, so need to make it 0 based.
  //Use VTKm to do this.
  auto portal = igrid.WritePortal();
  for (int i = 0; i < portal.GetNumberOfValues(); i++)
    portal.Set(i, portal.Get(i) - 1);

  auto cs = dataSet.GetCoordinateSystem().GetData();
  std::vector<vtkm::Id> connIds;
  vtkm::Id numCoords = 0;
  if (cs.IsType<GTCCoordsType32>())
  {
    auto coords = cs.AsArrayHandle<GTCCoordsType32>();
    connIds = this->ComputeConnectivity(coords, igrid, indexShift);
    numCoords = coords.GetNumberOfValues();
  }
  else if (cs.IsType<GTCCoordsType64>())
  {
    auto coords = cs.AsArrayHandle<GTCCoordsType64>();
    connIds = this->ComputeConnectivity(coords, igrid, indexShift);
    numCoords = coords.GetNumberOfValues();
  }
  else
  {
    throw std::runtime_error("Unsuported type for GTC coordinates system.");
  }

  if (!dataSet.GetCellSet().IsType<vtkm::cont::CellSetSingleType<>>())
  {
    throw std::runtime_error("Unsuported cellset type for GTC.");
  }

  auto& cellSet = dataSet.GetCellSet().Cast<vtkm::cont::CellSetSingleType<>>();
  auto connIdsAH = vtkm::cont::make_ArrayHandle(connIds, vtkm::CopyFlag::On);
  cellSet.Fill(numCoords, vtkm::CELL_SHAPE_WEDGE, 6, connIdsAH);
}

}
}
