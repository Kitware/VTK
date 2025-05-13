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

#include <viskores/CellShape.h>
#include <viskores/VectorAnalysis.h>
#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayHandleCast.h>
#include <viskores/cont/ArrayHandleXGCCoordinates.h>
#include <viskores/cont/CellSetExtrude.h>
#include <viskores/cont/UnknownArrayHandle.h>
#include <viskores/filter/clean_grid/CleanGrid.h>

#include <viskores/cont/Invoker.h>
#include <viskores/worklet/WorkletMapField.h>
#include <viskores/worklet/WorkletMapTopology.h>

namespace fides
{
namespace datamodel
{
namespace fusionutil
{

//Calculate the radius for each point coordinate.
class CalcRadius : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn in, FieldOut out);
  using ExecutionSignature = void(_1 inArray, _2 outVal);
  using InputDomain = _1;

  template <typename T, typename S>
  VISKORES_EXEC void operator()(const T& pt, S& out) const
  {
    out = viskores::Sqrt(pt[0] * pt[0] + pt[1] * pt[1]);
  }
};

class CalcPhi : public viskores::worklet::WorkletMapField
{
public:
  CalcPhi(viskores::Id nPlanes, viskores::Id ptsPerPlane)
    : NumPlanes(nPlanes)
    , NumPtsPerPlane(ptsPerPlane)
  {
    this->Phi0 = 0;
    this->DeltaPhi = viskores::TwoPi() / static_cast<viskores::Float64>(this->NumPlanes);
  }

  using ControlSignature = void(FieldIn in, FieldOut out);
  using ExecutionSignature = void(InputIndex idx, _2 outVal);
  using InputDomain = _1;

  template <typename T>
  VISKORES_EXEC void operator()(const viskores::Id& idx, T& out) const
  {
    viskores::Id plane = idx / this->NumPtsPerPlane;
    viskores::Float64 planePhi = static_cast<viskores::Float64>(plane * this->DeltaPhi);
    out = static_cast<T>(this->Phi0 + planePhi);

    if (out < 0)
    {
      out += 360;
    }
  }

private:
  viskores::Id NumPlanes;
  viskores::Id NumPtsPerPlane;
  viskores::Float64 DeltaPhi;
  viskores::Float64 Phi0;
};

//Calculate the cell set connection IDs for GX Cellset
class CalcGXCellSetConnIds : public viskores::worklet::WorkletVisitCellsWithPoints
{
public:
  CalcGXCellSetConnIds(const viskores::Id& numPlanes,
                       const viskores::Id& numTheta,
                       const viskores::Id& srfMinIdx)
    : NumPlanes(numPlanes)
    , NumTheta(numTheta)
    , SurfaceMinIdx(srfMinIdx)
  {
    this->NumCellsPerSrf = (this->NumTheta - 1) * this->NumPlanes;
    this->NumPointsPerSrf = this->NumTheta * this->NumPlanes;
  }

  using ControlSignature = void(CellSetIn cellSet,
                                WholeArrayOut connectionIds,
                                FieldOutCell srfIndexField);
  using ExecutionSignature = void(InputIndex, _2, _3);
  using InputDomain = _1;

  template <typename ConnectionArrayType, typename SrfIndexType>
  VISKORES_EXEC void operator()(const viskores::Id& cellId,
                                ConnectionArrayType& resultIds,
                                SrfIndexType& srfIndexField) const
  {
    viskores::Id srfIdx = cellId / this->NumCellsPerSrf;
    viskores::Id plnIdx = cellId / (this->NumTheta - 1) % this->NumPlanes;
    viskores::Id cellIdx = cellId % (this->NumTheta - 1);
    viskores::Id srfOffset = srfIdx * this->NumPointsPerSrf;

    // Offset for points on the first and second plane.
    viskores::Id offset0 = srfOffset + plnIdx * this->NumTheta;
    viskores::Id offset1 = srfOffset + (plnIdx + 1) * this->NumTheta;

    // if last plane, wrap around to first plane.
    if (plnIdx == this->NumPlanes - 1)
      offset1 = srfOffset;

    //connection ids for the 4 points of the quad.
    // note: quad connection order is: p0, p1, p3, p4
    auto p0 = offset0 + cellIdx + 0;
    auto p1 = p0 + 1;
    auto p2 = offset1 + cellIdx + 0;
    auto p3 = p2 + 1;

    viskores::Id index = cellId * 4;
    resultIds.Set(index + 0, p0);
    resultIds.Set(index + 1, p1);
    resultIds.Set(index + 2, p3);
    resultIds.Set(index + 3, p2);

    srfIndexField = this->SurfaceMinIdx + srfIdx;
  }

private:
  viskores::Id NumCellsPerSrf;
  viskores::Id NumPlanes;
  viskores::Id NumPointsPerSrf;
  viskores::Id NumTheta;
  viskores::Id SurfaceMinIdx;
};
} //namespace fides::datamodel::fusionutil

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
  else if (cellSetType == "gx")
  {
    this->CellSetImpl.reset(new CellSetGX());
  }
  else
  {
    throw std::runtime_error(cellSetType + " is not a valid cell_set type.");
  }
  this->CellSetImpl->ProcessJSON(json, sources);
}

std::vector<viskores::cont::UnknownCellSet> CellSet::Read(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources,
  const fides::metadata::MetaData& selections)
{
  return this->CellSetImpl->Read(paths, sources, selections);
}

void CellSet::PostRead(std::vector<viskores::cont::DataSet>& partitions,
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
    this->CellInformation = std::pair<unsigned char, int>(viskores::CELL_SHAPE_VERTEX, 1);
  }
  else if (cellType == "line")
  {
    this->CellInformation = std::pair<unsigned char, int>(viskores::CELL_SHAPE_LINE, 2);
  }
  else if (cellType == "triangle")
  {
    this->CellInformation = std::pair<unsigned char, int>(viskores::CELL_SHAPE_TRIANGLE, 3);
  }
  else if (cellType == "quad")
  {
    this->CellInformation = std::pair<unsigned char, int>(viskores::CELL_SHAPE_QUAD, 4);
  }
  else if (cellType == "tetrahedron")
  {
    this->CellInformation = std::pair<unsigned char, int>(viskores::CELL_SHAPE_TETRA, 4);
  }
  else if (cellType == "hexahedron")
  {
    this->CellInformation = std::pair<unsigned char, int>(viskores::CELL_SHAPE_HEXAHEDRON, 8);
  }
  else if (cellType == "wedge")
  {
    this->CellInformation = std::pair<unsigned char, int>(viskores::CELL_SHAPE_WEDGE, 6);
  }
  else if (cellType == "pyramid")
  {
    this->CellInformation = std::pair<unsigned char, int>(viskores::CELL_SHAPE_PYRAMID, 5);
  }
  else
  {
    throw std::runtime_error("Unrecognized cell type " + cellType);
  }
}

std::vector<viskores::cont::UnknownCellSet> CellSetSingleType::Read(
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

  size_t nArrays = this->ConnectivityArrays.size();
  std::vector<viskores::cont::UnknownCellSet> cellSets(nArrays);

  if (this->IsStatic)
  {
    this->CellSetCache = cellSets;
  }
  return cellSets;
}

void CellSetSingleType::PostRead(std::vector<viskores::cont::DataSet>& partitions,
                                 const fides::metadata::MetaData& fidesNotUsed(selections))
{
  size_t nParts = partitions.size();
  for (size_t i = 0; i < nParts; i++)
  {
    auto& pds = partitions[i];
    // if the array isn't stored as a signed int, we'll have to do a deep copy
    // into another UnknownArrayHandle
    viskores::cont::UnknownArrayHandle connUnknown = viskores::cont::ArrayHandle<viskores::Id>{};
    connUnknown.CopyShallowIfPossible(this->ConnectivityArrays[i]);
    auto connCasted = connUnknown.AsArrayHandle<viskores::cont::ArrayHandle<viskores::Id>>();

    viskores::cont::CellSetSingleType<> cellSet;
    if (pds.GetCellSet().IsValid())
    {
      cellSet = pds.GetCellSet().AsCellSet<viskores::cont::CellSetSingleType<>>();
    }

    cellSet.Fill(pds.GetNumberOfPoints(),
                 this->CellInformation.first,
                 this->CellInformation.second,
                 connCasted);
    pds.SetCellSet(cellSet);
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

std::vector<viskores::cont::UnknownCellSet> CellSetExplicit::Read(
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

  size_t nArrays = this->ConnectivityArrays.size();
  std::vector<viskores::cont::UnknownCellSet> cellSets(nArrays);

  if (this->IsStatic)
  {
    this->CellSetCache = cellSets;
  }

  return cellSets;
}

void CellSetExplicit::PostRead(std::vector<viskores::cont::DataSet>& partitions,
                               const fides::metadata::MetaData& fidesNotUsed(selections))
{
  size_t nParts = partitions.size();
  for (size_t i = 0; i < nParts; i++)
  {
    auto& pds = partitions[i];
    viskores::cont::ArrayHandle<viskores::IdComponent> nVertsCasted =
      this->NumberOfVerticesArrays[i]
        .AsArrayHandle<viskores::cont::ArrayHandle<viskores::IdComponent>>();
    viskores::cont::ArrayHandle<viskores::Id> offsets;
    viskores::cont::Algorithm::ScanExtended(
      viskores::cont::make_ArrayHandleCast<viskores::Id,
                                           viskores::cont::ArrayHandle<viskores::IdComponent>>(
        nVertsCasted),
      offsets);

    viskores::cont::UnknownArrayHandle connUnknown = viskores::cont::ArrayHandle<viskores::Id>{};
    connUnknown.CopyShallowIfPossible(this->ConnectivityArrays[i]);
    viskores::cont::ArrayHandle<viskores::Id> connCasted =
      connUnknown.AsArrayHandle<viskores::cont::ArrayHandle<viskores::Id>>();

    viskores::cont::UnknownArrayHandle typesUnknown =
      viskores::cont::ArrayHandle<viskores::UInt8>{};
    typesUnknown.CopyShallowIfPossible(this->CellTypesArrays[i]);
    viskores::cont::ArrayHandle<viskores::UInt8> typesCasted =
      typesUnknown.AsArrayHandle<viskores::cont::ArrayHandle<viskores::UInt8>>();

    viskores::cont::CellSetExplicit<> cellSet;
    if (pds.GetCellSet().IsValid())
    {
      cellSet = pds.GetCellSet().AsCellSet<viskores::cont::CellSetExplicit<>>();
    }

    cellSet.Fill(pds.GetNumberOfPoints(), typesCasted, connCasted, offsets);
    pds.SetCellSet(cellSet);
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

std::vector<viskores::cont::UnknownCellSet> CellSetStructured::Read(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources,
  const fides::metadata::MetaData& selections)
{
  this->DimensionArrays = this->Dimensions->Read(paths, sources, selections);

  std::size_t nArrays = this->DimensionArrays.size();
  std::vector<viskores::cont::UnknownCellSet> ret(nArrays);

  return ret;
}

void CellSetStructured::PostRead(std::vector<viskores::cont::DataSet>& partitions,
                                 const fides::metadata::MetaData& fidesNotUsed(selections))
{
  std::size_t nParts = partitions.size();

  for (std::size_t i = 0; i < nParts; i++)
  {
    auto& ds = partitions[i];

    viskores::cont::CellSetStructured<3> cellSet;
    if (ds.GetCellSet().IsValid())
    {
      cellSet = ds.GetCellSet().AsCellSet<viskores::cont::CellSetStructured<3>>();
    }
    viskores::cont::UnknownArrayHandle dimUnknown = viskores::cont::ArrayHandle<std::size_t>{};
    dimUnknown.CopyShallowIfPossible(this->DimensionArrays[i]);
    const auto& dimArray = dimUnknown.AsArrayHandle<viskores::cont::ArrayHandle<std::size_t>>();
    auto dimPortal = dimArray.ReadPortal();

    viskores::Id3 dims(static_cast<viskores::Id>(dimPortal.Get(0)),
                       static_cast<viskores::Id>(dimPortal.Get(1)),
                       static_cast<viskores::Id>(dimPortal.Get(2)));
    cellSet.SetPointDimensions(dims);

    if (dimArray.GetNumberOfValues() > 3)
    {
      viskores::Id3 start(static_cast<viskores::Id>(dimPortal.Get(3)),
                          static_cast<viskores::Id>(dimPortal.Get(4)),
                          static_cast<viskores::Id>(dimPortal.Get(5)));
      cellSet.SetGlobalPointIndexStart(start);
    }
    ds.SetCellSet(cellSet);
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

std::vector<viskores::cont::UnknownCellSet> CellSetXGC::Read(
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

  size_t numInsertPlanes = 0;
  if (selections.Has(fides::keys::fusion::PLANE_INSERTION()))
  {
    numInsertPlanes =
      selections.Get<fides::metadata::Size>(fides::keys::fusion::PLANE_INSERTION()).NumberOfItems;
  }

  fides::metadata::MetaData newSelections = selections;
  newSelections.Remove(fides::keys::BLOCK_SELECTION());

  std::vector<viskores::cont::UnknownCellSet> cellSets;

  //load the connect_list
  std::vector<viskores::cont::UnknownArrayHandle> connectivityVec =
    this->CellConnectivity->Read(paths, sources, newSelections);
  if (connectivityVec.size() != 1)
  {
    throw std::runtime_error("XGC CellConnectivity should have one Array");
  }

  using intType = viskores::cont::ArrayHandle<viskores::Int32>;
  intType connectivityAH;
  if (connectivityVec[0].IsType<intType>())
  {
    connectivityVec[0].AsArrayHandle(connectivityAH);
  }
  else
  {
    throw std::runtime_error("Only int arrays are supported for XGC cell connectivity.");
  }

  std::vector<viskores::cont::UnknownArrayHandle> planeConnectivityVec =
    this->PlaneConnectivity->Read(paths, sources, newSelections);

  if (planeConnectivityVec.size() > 1)
  {
    throw std::runtime_error("xgc nextNode is supposed to be included in one array.");
  }
  intType planeConnectivityAH;
  if (planeConnectivityVec[0].IsType<intType>())
  {
    planeConnectivityVec[0].AsArrayHandle(planeConnectivityAH);
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
    if (numInsertPlanes > 0)
    {
      std::cerr << "WARNING: PLANE_INSERTION not supported when using BLOCK_SELECTION. Ignoring."
                << std::endl;
      numInsertPlanes = 0;
    }
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
    viskores::Int32 numPlanes = block.NumberOfPlanesOwned * (1 + numInsertPlanes);
    auto xgcCell = viskores::cont::CellSetExtrude(connectivityAH,
                                                  static_cast<viskores::Int32>(numPointsPerPlane),
                                                  static_cast<viskores::Int32>(numPlanes),
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

class CellSetXGC::CalcPsi : public viskores::worklet::WorkletMapField
{
public:
  CalcPsi(double psix, viskores::Id ptsPerPlane)
    : PsiX(psix)
    , PointsPerPlane(ptsPerPlane)
  {
  }

  using ControlSignature = void(WholeArrayIn in, FieldOut out);
  using ExecutionSignature = void(_1 inArray, OutputIndex idx, _2 outVal);
  using InputDomain = _2;

  template <typename T, typename S>
  VISKORES_EXEC void operator()(const T& in, const viskores::Id& idx, S& out) const
  {
    out = in.Get(idx % this->PointsPerPlane) / this->PsiX;
  }

private:
  double PsiX;
  viskores::Id PointsPerPlane;
};

void CellSetXGC::PostRead(std::vector<viskores::cont::DataSet>& partitions,
                          const fides::metadata::MetaData& selections)
{
  //This is a hack until we decide with XGC how to cellset connectivity.
  for (auto& ds : partitions)
  {
    auto cs = ds.GetCellSet().AsCellSet<viskores::cont::CellSetExtrude>();
    viskores::cont::ArrayHandle<int> nextNode;
    viskores::Id n = cs.GetNumberOfPointsPerPlane() * cs.GetNumberOfPlanes();
    nextNode.Allocate(n);
    auto portal = nextNode.WritePortal();
    for (viskores::Id i = 0; i < n; i++)
      portal.Set(i, i);
    viskores::cont::CellSetExtrude newCS(cs.GetConnectivityArray(),
                                         cs.GetNumberOfPointsPerPlane(),
                                         cs.GetNumberOfPlanes(),
                                         nextNode,
                                         cs.GetIsPeriodic());

    ds.SetCellSet(newCS);
  }

  bool addR = false, addPhi = false, addPsi = false;

  if (selections.Has(fides::keys::fusion::ADD_R_FIELD()))
    addR = selections.Get<fides::metadata::Bool>(fides::keys::fusion::ADD_R_FIELD()).Value;
  if (selections.Has(fides::keys::fusion::ADD_PHI_FIELD()))
    addPhi = selections.Get<fides::metadata::Bool>(fides::keys::fusion::ADD_PHI_FIELD()).Value;
  if (selections.Has(fides::keys::fusion::ADD_PSI_FIELD()))
    addPsi = selections.Get<fides::metadata::Bool>(fides::keys::fusion::ADD_PSI_FIELD()).Value;

  if (addR || addPhi || addPsi)
  {
    for (auto& ds : partitions)
    {
      using CellSetType = viskores::cont::CellSetExtrude;
      using CoordsType = viskores::cont::ArrayHandleXGCCoordinates<double>;

      const auto& cs = ds.GetCellSet().AsCellSet<CellSetType>();
      const auto& coords = ds.GetCoordinateSystem().GetData().AsArrayHandle<CoordsType>();

      viskores::cont::Invoker invoke;
      if (addR)
      {
        viskores::cont::ArrayHandle<viskores::Float64> var;
        invoke(fusionutil::CalcRadius{}, coords, var);
        ds.AddPointField("R", var);
      }
      if (addPhi)
      {
        fusionutil::CalcPhi calcPhi(cs.GetNumberOfPlanes(), cs.GetNumberOfPointsPerPlane());
        viskores::cont::ArrayHandle<viskores::Float64> var;
        invoke(calcPhi, coords, var);
        ds.AddPointField("Phi", var);
      }
      if (addPsi)
      {
        viskores::Float64 psi_x = ds.GetField("psi_x")
                                    .GetData()
                                    .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Float64>>()
                                    .ReadPortal()
                                    .Get(0);
        auto psi = ds.GetField("PSI")
                     .GetData()
                     .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Float64>>();

        viskores::cont::ArrayHandle<viskores::Float64> var;
        var.Allocate(coords.GetNumberOfValues());
        CellSetXGC::CalcPsi calcPsi(psi_x, cs.GetNumberOfPointsPerPlane());
        invoke(calcPsi, psi, var);
        ds.AddPointField("Psi", var);
      }
    }
  }
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
}

std::vector<viskores::cont::UnknownCellSet> CellSetGTC::Read(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources,
  const fides::metadata::MetaData& selections)
{
  // The size of the cellSetsGTC is always one
  std::vector<viskores::cont::UnknownCellSet> cellSets(1);

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

    //Create the cell sets. We will fill them in PostRead
    viskores::cont::CellSetSingleType<> cellSet;
    this->CachedCellSet = cellSet;
  }

  return cellSets;
}

void CellSetGTC::PostRead(std::vector<viskores::cont::DataSet>& partitions,
                          const fides::metadata::MetaData& selections)
{
  if (partitions.size() != 1)
  {
    throw std::runtime_error("Wrong type for partitions for GTC DataSets.");
  }

  //Add additional fields if requested.
  bool addR = false, addPhi = false;
  if (selections.Has(fides::keys::fusion::ADD_R_FIELD()))
    addR = selections.Get<fides::metadata::Bool>(fides::keys::fusion::ADD_R_FIELD()).Value;
  if (selections.Has(fides::keys::fusion::ADD_PHI_FIELD()))
    addPhi = selections.Get<fides::metadata::Bool>(fides::keys::fusion::ADD_PHI_FIELD()).Value;
  if (selections.Has(fides::keys::fusion::FUSION_PERIODIC_CELLSET()))
    this->PeriodicCellSet =
      selections.Get<fides::metadata::Bool>(fides::keys::fusion::FUSION_PERIODIC_CELLSET()).Value;

  auto& dataSet = partitions[0];
  if (this->IsCached)
  {
    dataSet.SetCellSet(this->CachedCellSet);

    if (addR)
    {
      if (!this->RArrayCached)
      {
        throw std::runtime_error("R Array not cached.");
      }
      dataSet.AddPointField("R", this->RArray);
    }
    if (addPhi)
    {
      if (!this->PhiArrayCached)
      {
        throw std::runtime_error("Phi Array not cached.");
      }
      dataSet.AddPointField("Phi", this->PhiArray);
    }
    return;
  }

  if (!dataSet.HasField("num_planes") || !dataSet.HasField("num_pts_per_plane"))
  {
    throw std::runtime_error("num_planes and/or num_pts_per_plane not found.");
  }

  using intType = viskores::cont::ArrayHandle<int>;
  auto numPlanes = dataSet.GetField("num_planes").GetData().AsArrayHandle<intType>();
  auto numPtsPerPlane = dataSet.GetField("num_pts_per_plane").GetData().AsArrayHandle<intType>();

  this->NumberOfPointsPerPlane = static_cast<viskores::Id>(numPtsPerPlane.ReadPortal().Get(0));
  this->NumberOfPlanes = static_cast<viskores::Id>(numPlanes.ReadPortal().Get(0));

  if (selections.Has(fides::keys::fusion::PLANE_INSERTION()))
  {
    auto numInsertPlanes =
      selections.Get<fides::metadata::Size>(fides::keys::fusion::PLANE_INSERTION()).NumberOfItems;
    this->NumberOfPlanes = this->NumberOfPlanes * (1 + numInsertPlanes);
  }

  //Calculate the cellset.
  this->ComputeCellSet(dataSet);

  if (addR)
  {
    viskores::cont::Invoker invoke;

    const auto& coords = dataSet.GetCoordinateSystem().GetData();
    invoke(fusionutil::CalcRadius{}, coords, this->RArray);
    this->RArrayCached = true;
    dataSet.AddPointField("R", this->RArray);
  }
  if (addPhi)
  {
    viskores::cont::Invoker invoke;
    const auto& coords = dataSet.GetCoordinateSystem().GetData();
    fusionutil::CalcPhi calcPhi(this->NumberOfPlanes, this->NumberOfPointsPerPlane);
    invoke(calcPhi, coords, this->PhiArray);
    this->PhiArrayCached = true;
    dataSet.AddPointField("Phi", this->PhiArray);
  }
}

template <typename T, typename C>
std::vector<viskores::Id> CellSetGTC::ComputeConnectivity(
  const viskores::cont::ArrayHandle<viskores::Vec<T, 3>, C>& coords,
  const viskores::cont::ArrayHandle<int>& igrid,
  const viskores::cont::ArrayHandle<int>& indexShift)
{
  std::vector<viskores::Id> nPoloidalNodes; // Number of nodes in each poloidal contour
  std::vector<viskores::Id> poloidalIndex;  // Starting node index of each poloidal contour
  viskores::Id nNodes = this->NumberOfPointsPerPlane;

  auto igridPortal = igrid.ReadPortal();
  auto coordsPortal = coords.ReadPortal();

  for (viskores::Id i = 0; i < igrid.GetNumberOfValues() - 1; i++)
  {
    poloidalIndex.push_back(igridPortal.Get(i));
    viskores::Id nPts = static_cast<viskores::Id>(igridPortal.Get(i + 1) - igridPortal.Get(i));
    nPoloidalNodes.push_back(nPts);
  }

  // For each point get the nearest neighbor on the flux surface.
  // Work from the outside to the inside because there are more nodes
  // on the outside. As such, neighbors will get used multiple times
  // thus allowing for degenerate connections to be found.
  std::vector<viskores::Id> neighborIndex(nNodes);
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
      T minDist = viskores::Infinity<T>();
      for (size_t i = 0; i < static_cast<size_t>(nPoloidalNodes[k - 1] - 1); i++)
      {
        // Index of the test node.
        viskores::Id m = poloidalIndex[k - 1] + i;
        auto tmpPt = coordsPortal.Get(m);

        auto dist = viskores::MagnitudeSquared(basePt - tmpPt);
        if (dist < minDist)
        {
          neighborIndex[l] = m;
          minDist = dist;
        }
      }
    }
  }

  std::vector<viskores::Id> vtxList;
  viskores::Id nElements = 0;

  // Work from the outside to the inside because there are more nodes
  // on the outside. As such, neighbors will get used multiple times
  // thus allowing for degenerate connections to be found.
  for (size_t k = nPoloidalNodes.size() - 1; k > 0; k--)
  {
    for (size_t j = 0; j < static_cast<size_t>(nPoloidalNodes[k] - 1); j++)
    {
      viskores::Id l = poloidalIndex[k] + static_cast<viskores::Id>(j);
      viskores::Id l1 = (l + 1);

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

  std::vector<viskores::Id> connIds;
  connIds.reserve(nElements * 6 * (this->NumberOfPlanes - 1));
  for (viskores::Id i = 0; i < this->NumberOfPlanes - 1; i++)
  {
    viskores::Id off = i * this->NumberOfPointsPerPlane;
    viskores::Id off2 = (i + 1) * this->NumberOfPointsPerPlane;
    for (viskores::Id j = 0; j < nElements; j++)
    {
      connIds.push_back(vtxList[j * 3 + 0] + off);
      connIds.push_back(vtxList[j * 3 + 1] + off);
      connIds.push_back(vtxList[j * 3 + 2] + off);

      connIds.push_back(vtxList[j * 3 + 0] + off2);
      connIds.push_back(vtxList[j * 3 + 1] + off2);
      connIds.push_back(vtxList[j * 3 + 2] + off2);
    }
  }

  if (this->PeriodicCellSet)
  {
    //Connect the first/last plane.
    //Uses index-shift to map between flux surfaces.
    auto indexShiftPortal = indexShift.ReadPortal();
    std::vector<viskores::Id> pn(this->NumberOfPointsPerPlane, -1);
    viskores::Id n = igridPortal.GetNumberOfValues();
    for (viskores::Id gi = 0; gi < n - 1; gi++)
    {
      viskores::Id n0 = igridPortal.Get(gi);
      viskores::Id nn = igridPortal.Get(gi + 1) - 1;
      viskores::Id shift = static_cast<viskores::Id>(indexShiftPortal.Get(gi));

      for (viskores::Id i = 0; i < (nn - n0); i++)
      {
        viskores::Id i0 = i;
        viskores::Id i1 = i - shift;
        if (i1 < 0)
          i1 = i1 + (nn - n0);
        pn[n0 + i0] = n0 + i1;
      }
    }

    viskores::Id offset = nNodes * (this->NumberOfPlanes - 1);
    for (viskores::Id i = 0; i < nElements; i++)
    {
      viskores::Id ids[3] = { connIds[i * 6 + 0], connIds[i * 6 + 1], connIds[i * 6 + 2] };
      if (!(ids[0] < nNodes && ids[1] < nNodes && ids[2] < nNodes))
        throw std::runtime_error("Invalid connectivity for GTC Cellset.");

      //plane N-1
      connIds.push_back(pn[ids[0]] + offset);
      connIds.push_back(pn[ids[1]] + offset);
      connIds.push_back(pn[ids[2]] + offset);

      //plane 0
      connIds.push_back(ids[0]);
      connIds.push_back(ids[1]);
      connIds.push_back(ids[2]);
    }
  }

  return connIds;
}

void CellSetGTC::ComputeCellSet(viskores::cont::DataSet& dataSet)
{
  viskores::cont::ArrayHandle<int> igrid =
    this->IGridArrays[0].AsArrayHandle<viskores::cont::ArrayHandle<int>>();
  viskores::cont::ArrayHandle<int> indexShift =
    this->IndexShiftArrays[0].AsArrayHandle<viskores::cont::ArrayHandle<int>>();

  //These are fortran indices, so need to make it 0 based.
  //Use Viskores to do this.
  auto portal = igrid.WritePortal();
  for (int i = 0; i < portal.GetNumberOfValues(); i++)
    portal.Set(i, portal.Get(i) - 1);

  auto cs = dataSet.GetCoordinateSystem().GetData();
  std::vector<viskores::Id> connIds;
  viskores::Id numCoords = 0;
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
    throw std::runtime_error("Unsupported type for GTC coordinates system.");
  }

  viskores::cont::CellSetSingleType<> cellSet;
  if (dataSet.GetCellSet().IsValid())
  {
    cellSet = dataSet.GetCellSet().AsCellSet<viskores::cont::CellSetSingleType<>>();
  }

  auto connIdsAH = viskores::cont::make_ArrayHandle(connIds, viskores::CopyFlag::On);
  cellSet.Fill(numCoords, viskores::CELL_SHAPE_WEDGE, 6, connIdsAH);
  dataSet.SetCellSet(cellSet);

  this->CachedCellSet = cellSet;
  this->IsCached = true;
}

std::vector<viskores::cont::UnknownCellSet> CellSetGX::Read(
  const std::unordered_map<std::string, std::string>& fidesNotUsed(paths),
  DataSourcesType& fidesNotUsed(sources),
  const fides::metadata::MetaData& fidesNotUsed(selections))
{
  std::vector<viskores::cont::UnknownCellSet> cellSets;
  viskores::cont::CellSetSingleType<> cellSet;
  cellSets.push_back(cellSet);
  return cellSets;
}

viskores::Id CellSetGX::GetMetaDataValue(const viskores::cont::DataSet& ds,
                                         const std::string& fieldNm) const
{
  if (!ds.HasField(fieldNm, viskores::cont::Field::Association::WholeDataSet))
    throw std::runtime_error("Error: CellSetGX missing field " + fieldNm);

  const auto& field =
    ds.GetField(fieldNm, viskores::cont::Field::Association::WholeDataSet).GetData();
  if (field.GetNumberOfValues() != 1)
    throw std::runtime_error("Error: Wrong number of values in field " + fieldNm);
  if (!field.IsType<viskores::cont::ArrayHandle<viskores::Id>>())
    throw std::runtime_error("Error: Wrong type in field " + fieldNm);

  return field.AsArrayHandle<viskores::cont::ArrayHandle<viskores::Id>>().ReadPortal().Get(0);
}

void CellSetGX::PostRead(std::vector<viskores::cont::DataSet>& partitions,
                         const fides::metadata::MetaData& fidesNotUsed(selections))
{
  if (partitions.size() != 1)
    throw std::runtime_error("Wrong number of datasets.");

  auto& ds = partitions[0];

  viskores::Id numTheta = this->GetMetaDataValue(ds, "num_theta");
  viskores::Id numZeta = this->GetMetaDataValue(ds, "num_zeta");
  viskores::Id nfp = this->GetMetaDataValue(ds, "nfp");
  viskores::Id numSurfaces = this->GetMetaDataValue(ds, "num_surfaces");
  viskores::Id srfIdxMin = this->GetMetaDataValue(ds, "surface_min_index");

  viskores::Id ptsPerPlane = numTheta;
  viskores::Id numPlanes = numZeta * nfp;
  viskores::Id numCellsPerSurface = (ptsPerPlane - 1) * (numPlanes - 1);
  viskores::Id totNumCells = numSurfaces * numCellsPerSurface;

  //create cell with with empty connection ids.
  auto cellSet = viskores::cont::CellSetSingleType<>();
  viskores::cont::ArrayHandle<viskores::Id> connIds, surfaceIndices;
  connIds.Allocate(totNumCells * 4);
  cellSet.Fill(ds.GetNumberOfPoints(), viskores::CELL_SHAPE_QUAD, 4, connIds);

  //call worklet to set the point ids for the cellset.
  // create a cell centered variable with the surface index.
  viskores::cont::Invoker invoke;
  fides::datamodel::fusionutil::CalcGXCellSetConnIds worklet(numPlanes, numTheta, srfIdxMin);
  invoke(worklet, cellSet, connIds, surfaceIndices);

  ds.SetCellSet(cellSet);
  ds.AddCellField("SurfaceIndex", surfaceIndices);

  //Call CleanGrid filter to remove duplicates and merge points.
  viskores::filter::clean_grid::CleanGrid cleaner;
  cleaner.SetMergePoints(true);
  cleaner.SetCompactPointFields(false);
  cleaner.SetRemoveDegenerateCells(true);
  cleaner.SetTolerance(1e-6);
  ds = cleaner.Execute(ds);
}

}
}
