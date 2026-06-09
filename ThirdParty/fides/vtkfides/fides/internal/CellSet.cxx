//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#include <fides/internal/CellSet.h>
#include <fides/internal/DataWrapHelper.h>

#if FIDES_USE_VISKORES
#include <viskores/CellShape.h>
#include <viskores/VectorAnalysis.h>
#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandleCast.h>
#include <viskores/cont/ArrayHandleXGCCoordinates.h>
#include <viskores/cont/CellSetExtrude.h>
#include <viskores/cont/UnknownArrayHandle.h>
#include <viskores/filter/clean_grid/CleanGrid.h>

#include <viskores/cont/Invoker.h>
#include <viskores/worklet/WorkletMapField.h>
#include <viskores/worklet/WorkletMapTopology.h>
#endif // FIDES_USE_VISKORES

#if FIDES_USE_VTK
#include <vtkPartitionedDataSet.h>
#include <vtkSmartPointer.h>
#endif

namespace fides
{
namespace datamodel
{

#if FIDES_USE_VISKORES
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

#endif // FIDES_USE_VISKORES

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
  else if (cellSetType == "polydata")
  {
    this->CellSetImpl.reset(new CellSetPolyData());
  }
#if FIDES_USE_VISKORES
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
#endif // FIDES_USE_VISKORES
  else
  {
    throw std::runtime_error(cellSetType + " is not a valid cell_set type.");
  }
  this->CellSetImpl->ProcessJSON(json, sources);
}

std::vector<size_t> CellSet::Read(const std::unordered_map<std::string, std::string>& paths,
                                  DataSourcesType& sources,
                                  const fides::metadata::MetaData& selections,
                                  OutputBuilder& builder)
{
  return this->CellSetImpl->Read(paths, sources, selections, builder);
}

void CellSet::CollectReadRequests(const std::unordered_map<std::string, std::string>& paths,
                                  DataSourcesType& sources,
                                  const fides::metadata::MetaData& selections,
                                  std::vector<ReadRequest>& out)
{
  this->CellSetImpl->CollectReadRequests(paths, sources, selections, out);
}

std::vector<size_t> CellSet::EmitTokens(const std::unordered_map<std::string, std::string>& paths,
                                        DataSourcesType& sources,
                                        const fides::metadata::MetaData& selections,
                                        const ReadResultMap& results,
                                        OutputBuilder& builder)
{
  return this->CellSetImpl->EmitTokens(paths, sources, selections, results, builder);
}

void CellSet::PostRead(fides::DataContainer& container, const fides::metadata::MetaData& selections)
{
  this->CellSetImpl->PostRead(container, selections);
}

void CellSetBase::PostRead(fides::DataContainer& container,
                           const fides::metadata::MetaData& selections)
{
#if FIDES_USE_VISKORES
  // Try unwrapping Viskores
  if (auto* vec = fides::GetDataAs<std::vector<viskores::cont::DataSet>>(container))
  {
    this->ProcessViskores(*vec, selections);
    return;
  }
#endif

#if FIDES_USE_VTK
  // Try unwrapping VTK
  if (auto* vtkPtr = fides::GetDataAs<fides::VTKCollection>(container))
  {
    this->ProcessVTK(*vtkPtr, selections);
    return;
  }
#endif

  // If we get here, the container held something we don't understand
  throw std::runtime_error("ArrayBase::PostRead - Unknown backend in DataContainer.");
}

void CellSetBase::CollectReadRequests(const std::unordered_map<std::string, std::string>&,
                                      DataSourcesType&,
                                      const fides::metadata::MetaData&,
                                      std::vector<ReadRequest>&)
{
  // Default: subclasses that haven't been converted to the two-pass
  // flow contribute no requests. The default EmitTokens then falls back
  // to the legacy Read.
}

std::vector<size_t> CellSetBase::EmitTokens(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources,
  const fides::metadata::MetaData& selections,
  const ReadResultMap& /*results*/,
  OutputBuilder& builder)
{
  return this->Read(paths, sources, selections, builder);
}

void CellSetSingleType::ProcessJSON(const rapidjson::Value& json, DataSourcesType& sources)
{
  this->CellSetBase::ProcessJSON(json, sources);

  if (!json.HasMember("cell_type"))
  {
    throw std::runtime_error(this->ObjectName + " must provide a cell_type.");
  }
  std::string cellType = json["cell_type"].GetString();

  fides::CellShape shape = fides::ConvertStringToCellShape(cellType);
  this->CellInformation = std::pair<unsigned char, int>(static_cast<unsigned char>(shape),
                                                        fides::GetCellShapeNumberOfPoints(shape));
}

namespace
{
ReadRequest MakeCellSetSingleTypeRequest(const std::string& dataSourceName,
                                         const std::string& variableName,
                                         const fides::metadata::MetaData& selections)
{
  ReadRequest req;
  req.DataSourceName = dataSourceName;
  req.VariableName = variableName;
  req.IsVector = fides::io::IsVector::Auto;
  req.MultiBlock = selections.Has(fides::keys::READ_AS_MULTIBLOCK()) &&
    selections.Get<fides::metadata::Bool>(fides::keys::READ_AS_MULTIBLOCK()).Value;
  req.Selection = SelectionKey::From(selections);
  return req;
}
}

std::vector<size_t> CellSetSingleType::Read(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources,
  const fides::metadata::MetaData& selections,
  OutputBuilder& builder)
{
  // Default DataObjectModel::Read drives CRR + Execute + Emit; this
  // override is no longer reached when the orchestrator uses the
  // two-phase API. Kept temporarily for any direct-Read call sites
  // (none in DataSetModel after step 7).
  std::vector<ReadRequest> reqs;
  this->CollectReadRequests(paths, sources, selections, reqs);
  ReadResultMap results = ReadPlan::Execute(reqs, paths, sources, selections);
  return this->EmitTokens(paths, sources, selections, results, builder);
}

void CellSetSingleType::CollectReadRequests(
  const std::unordered_map<std::string, std::string>& /*paths*/,
  DataSourcesType& /*sources*/,
  const fides::metadata::MetaData& selections,
  std::vector<ReadRequest>& out)
{
  // Static-cache hit: nothing to read this pass; EmitTokens will reissue
  // builder tokens from this->ConnectivityArrays.
  if (this->IsStatic && !this->ConnectivityArrays.empty())
  {
    return;
  }
  out.push_back(MakeCellSetSingleTypeRequest(this->DataSourceName, this->VariableName, selections));
}

std::vector<size_t> CellSetSingleType::EmitTokens(
  const std::unordered_map<std::string, std::string>& /*paths*/,
  DataSourcesType& /*sources*/,
  const fides::metadata::MetaData& selections,
  const ReadResultMap& results,
  OutputBuilder& builder)
{
  // Builder tokens are scoped to one builder instance, so reissue them every
  // step. For static cell sets, the raw connectivity arrays are reused from
  // this->ConnectivityArrays; otherwise pull fresh ones from `results`.
  if (!(this->IsStatic && !this->ConnectivityArrays.empty()))
  {
    auto req = MakeCellSetSingleTypeRequest(this->DataSourceName, this->VariableName, selections);
    auto it = results.find(req);
    if (it == results.end())
    {
      throw std::runtime_error("CellSetSingleType::EmitTokens: missing read result for variable '" +
                               this->VariableName + "'.");
    }
    this->ConnectivityArrays = it->second;
  }

  size_t nArrays = this->ConnectivityArrays.size();
  std::vector<size_t> cellSets(nArrays);

  fides::CellShape shape = static_cast<fides::CellShape>(this->CellInformation.first);
  int vertsPerCell = this->CellInformation.second;

  for (size_t i = 0; i < nArrays; i++)
  {
    size_t connToken = builder.CreateArray(this->ConnectivityArrays[i]);
    cellSets[i] = builder.CreateSingleTypeCellSet(shape, vertsPerCell, connToken);
  }

  return cellSets;
}

#if FIDES_USE_VISKORES
void CellSetSingleType::ProcessViskores(std::vector<viskores::cont::DataSet>& partitions,
                                        const fides::metadata::MetaData& fidesNotUsed(selections))
{
  size_t nParts = partitions.size();
  for (size_t i = 0; i < nParts; i++)
  {
    auto& pds = partitions[i];
    // Deep copy connectivity so the cell set owns its data (RawArrays may be cleared below)
    viskores::cont::ArrayHandle<viskores::Id> connCasted;
    viskores::cont::ArrayCopy(
      fides::internal::RawArrayToUnknownArrayHandle(this->ConnectivityArrays[i]), connCasted);

    viskores::UInt8 cellShapeViskores = fides::internal::ConvertCellShapeToViskores(
      static_cast<fides::CellShape>(this->CellInformation.first));

    viskores::cont::CellSetSingleType<> cellSet;
    if (pds.GetCellSet().IsValid())
    {
      cellSet = pds.GetCellSet().AsCellSet<viskores::cont::CellSetSingleType<>>();
    }

    cellSet.Fill(
      pds.GetNumberOfPoints(), cellShapeViskores, this->CellInformation.second, connCasted);
    pds.SetCellSet(cellSet);
  }
  if (!this->IsStatic)
  {
    this->ConnectivityArrays.clear();
  }
}
#endif // FIDES_USE_VISKORES

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
  // Also set DataSourceName/VariableName on the Array wrapper for direct data source access
  if (cellTypes.HasMember("data_source"))
    this->CellTypes->DataSourceName = cellTypes["data_source"].GetString();
  if (cellTypes.HasMember("variable"))
    this->CellTypes->VariableName = cellTypes["variable"].GetString();

  if (!json.HasMember("number_of_vertices") || !json["number_of_vertices"].IsObject())
  {
    std::string err = std::string(__FILE__) + ":" + std::to_string(__LINE__) +
      ": Must provide a number_of_vertices object for object " + this->ObjectName;
    throw std::runtime_error(err);
  }
  this->NumberOfVertices.reset(new Array());
  const auto& numVertices = json["number_of_vertices"];
  this->NumberOfVertices->ProcessJSON(numVertices, sources);
  if (numVertices.HasMember("data_source"))
    this->NumberOfVertices->DataSourceName = numVertices["data_source"].GetString();
  if (numVertices.HasMember("variable"))
    this->NumberOfVertices->VariableName = numVertices["variable"].GetString();

  if (!json.HasMember("connectivity") || !json["connectivity"].IsObject())
  {
    std::string err = std::string(__FILE__) + ":" + std::to_string(__LINE__) +
      ": Must provide a connectivity object for object " + this->ObjectName;
    throw std::runtime_error(err);
  }
  this->Connectivity.reset(new Array());
  const auto& conn = json["connectivity"];
  this->Connectivity->ProcessJSON(conn, sources);
  if (conn.HasMember("data_source"))
    this->Connectivity->DataSourceName = conn["data_source"].GetString();
  if (conn.HasMember("variable"))
    this->Connectivity->VariableName = conn["variable"].GetString();
}

namespace
{
ReadRequest MakeCellSetExplicitRequest(const std::string& dataSourceName,
                                       const std::string& variableName,
                                       const fides::metadata::MetaData& selections)
{
  ReadRequest req;
  req.DataSourceName = dataSourceName;
  req.VariableName = variableName;
  req.IsVector = fides::io::IsVector::Auto;
  req.MultiBlock = selections.Has(fides::keys::READ_AS_MULTIBLOCK()) &&
    selections.Get<fides::metadata::Bool>(fides::keys::READ_AS_MULTIBLOCK()).Value;
  req.Selection = SelectionKey::From(selections);
  return req;
}
}

std::vector<size_t> CellSetExplicit::Read(const std::unordered_map<std::string, std::string>& paths,
                                          DataSourcesType& sources,
                                          const fides::metadata::MetaData& selections,
                                          OutputBuilder& builder)
{
  std::vector<ReadRequest> reqs;
  this->CollectReadRequests(paths, sources, selections, reqs);
  ReadResultMap results = ReadPlan::Execute(reqs, paths, sources, selections);
  return this->EmitTokens(paths, sources, selections, results, builder);
}

void CellSetExplicit::CollectReadRequests(
  const std::unordered_map<std::string, std::string>& /*paths*/,
  DataSourcesType& /*sources*/,
  const fides::metadata::MetaData& selections,
  std::vector<ReadRequest>& out)
{
  // Static-cache hit: nothing to read this pass; EmitTokens will reissue
  // builder tokens from the per-role raw-array members.
  if (this->IsStatic && !this->ConnectivityArrays.empty())
  {
    return;
  }
  out.push_back(MakeCellSetExplicitRequest(
    this->Connectivity->DataSourceName, this->Connectivity->VariableName, selections));
  out.push_back(MakeCellSetExplicitRequest(
    this->NumberOfVertices->DataSourceName, this->NumberOfVertices->VariableName, selections));
  out.push_back(MakeCellSetExplicitRequest(
    this->CellTypes->DataSourceName, this->CellTypes->VariableName, selections));
}

std::vector<size_t> CellSetExplicit::EmitTokens(
  const std::unordered_map<std::string, std::string>& /*paths*/,
  DataSourcesType& /*sources*/,
  const fides::metadata::MetaData& selections,
  const ReadResultMap& results,
  OutputBuilder& builder)
{
  // Builder tokens are scoped to one builder instance, so reissue them every
  // step. For static cell sets, the raw arrays are reused from the per-role
  // members; otherwise pull fresh ones from `results`.
  if (!(this->IsStatic && !this->ConnectivityArrays.empty()))
  {
    auto lookup = [&](const std::string& src, const std::string& var) {
      auto req = MakeCellSetExplicitRequest(src, var, selections);
      auto it = results.find(req);
      if (it == results.end())
      {
        throw std::runtime_error("CellSetExplicit::EmitTokens: missing read result for variable '" +
                                 var + "'.");
      }
      return it->second;
    };

    this->ConnectivityArrays =
      lookup(this->Connectivity->DataSourceName, this->Connectivity->VariableName);
    this->NumberOfVerticesArrays =
      lookup(this->NumberOfVertices->DataSourceName, this->NumberOfVertices->VariableName);
    this->CellTypesArrays = lookup(this->CellTypes->DataSourceName, this->CellTypes->VariableName);
  }

  size_t nArrays = this->ConnectivityArrays.size();
  std::vector<size_t> cellSets(nArrays);

  for (size_t i = 0; i < nArrays; i++)
  {
    size_t connToken = builder.CreateArray(this->ConnectivityArrays[i]);
    size_t numVertsToken = builder.CreateArray(this->NumberOfVerticesArrays[i]);
    size_t cellTypesToken = builder.CreateArray(this->CellTypesArrays[i]);
    cellSets[i] = builder.CreateExplicitCellSet(cellTypesToken, numVertsToken, connToken);
  }

  return cellSets;
}

#if FIDES_USE_VISKORES
void CellSetExplicit::ProcessViskores(std::vector<viskores::cont::DataSet>& partitions,
                                      const fides::metadata::MetaData& fidesNotUsed(selections))
{
  size_t nParts = partitions.size();
  for (size_t i = 0; i < nParts; i++)
  {
    auto& pds = partitions[i];

    // Deep copy nVerts (only used for ScanExtended, so CopyShallowIfPossible is
    // fine here since offsets is a freshly allocated array)
    viskores::cont::UnknownArrayHandle nVertsUnknown =
      viskores::cont::ArrayHandle<viskores::IdComponent>{};
    nVertsUnknown.CopyShallowIfPossible(
      fides::internal::RawArrayToUnknownArrayHandle(this->NumberOfVerticesArrays[i]));
    viskores::cont::ArrayHandle<viskores::IdComponent> nVertsCasted =
      nVertsUnknown.AsArrayHandle<viskores::cont::ArrayHandle<viskores::IdComponent>>();
    viskores::cont::ArrayHandle<viskores::Id> offsets;
    viskores::cont::Algorithm::ScanExtended(
      viskores::cont::make_ArrayHandleCast<viskores::Id,
                                           viskores::cont::ArrayHandle<viskores::IdComponent>>(
        nVertsCasted),
      offsets);

    // Deep copy connectivity and types so the cell set owns its data
    // (RawArrays may be cleared below)
    viskores::cont::ArrayHandle<viskores::Id> connCasted;
    viskores::cont::ArrayCopy(
      fides::internal::RawArrayToUnknownArrayHandle(this->ConnectivityArrays[i]), connCasted);

    viskores::cont::ArrayHandle<viskores::UInt8> typesCasted;
    viskores::cont::ArrayCopy(
      fides::internal::RawArrayToUnknownArrayHandle(this->CellTypesArrays[i]), typesCasted);

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
#endif // FIDES_USE_VISKORES

namespace
{
// Parse one optional polydata role (e.g. "polys": {"offsets": {...},
// "connectivity": {...}}). When the role key is absent, leave the role
// empty; the writer reads no requests and emits InvalidToken pairs for
// it. When the role key is present, both "offsets" and "connectivity"
// must be provided.
void ProcessPolyDataRole(CellSetPolyData::Role& role,
                         const rapidjson::Value& parent,
                         const char* roleName,
                         DataSourcesType& sources)
{
  if (!parent.HasMember(roleName))
  {
    return;
  }
  const auto& roleObj = parent[roleName];
  if (!roleObj.IsObject())
  {
    throw std::runtime_error(std::string("polydata cell_set: '") + roleName +
                             "' must be an object");
  }
  auto parseSub = [&](const char* key) {
    if (!roleObj.HasMember(key) || !roleObj[key].IsObject())
    {
      throw std::runtime_error(std::string("polydata cell_set: '") + roleName +
                               "' must provide a '" + key + "' object");
    }
    const auto& sub = roleObj[key];
    auto arr = std::make_unique<Array>();
    arr->ProcessJSON(sub, sources);
    if (sub.HasMember("data_source"))
    {
      arr->DataSourceName = sub["data_source"].GetString();
    }
    if (sub.HasMember("variable"))
    {
      arr->VariableName = sub["variable"].GetString();
    }
    return arr;
  };
  role.Offsets = parseSub("offsets");
  role.Connectivity = parseSub("connectivity");
}

ReadRequest MakeCellSetPolyDataRequest(const std::string& dataSourceName,
                                       const std::string& variableName,
                                       const fides::metadata::MetaData& selections)
{
  ReadRequest req;
  req.DataSourceName = dataSourceName;
  req.VariableName = variableName;
  req.IsVector = fides::io::IsVector::Auto;
  req.MultiBlock = selections.Has(fides::keys::READ_AS_MULTIBLOCK()) &&
    selections.Get<fides::metadata::Bool>(fides::keys::READ_AS_MULTIBLOCK()).Value;
  req.Selection = SelectionKey::From(selections);
  return req;
}
} // namespace

void CellSetPolyData::ProcessJSON(const rapidjson::Value& json, DataSourcesType& sources)
{
  ProcessPolyDataRole(this->Verts, json, "verts", sources);
  ProcessPolyDataRole(this->Lines, json, "lines", sources);
  ProcessPolyDataRole(this->Polys, json, "polys", sources);
  ProcessPolyDataRole(this->Strips, json, "strips", sources);
}

std::vector<size_t> CellSetPolyData::Read(const std::unordered_map<std::string, std::string>& paths,
                                          DataSourcesType& sources,
                                          const fides::metadata::MetaData& selections,
                                          OutputBuilder& builder)
{
  std::vector<ReadRequest> reqs;
  this->CollectReadRequests(paths, sources, selections, reqs);
  ReadResultMap results = ReadPlan::Execute(reqs, paths, sources, selections);
  return this->EmitTokens(paths, sources, selections, results, builder);
}

void CellSetPolyData::CollectReadRequests(
  const std::unordered_map<std::string, std::string>& /*paths*/,
  DataSourcesType& /*sources*/,
  const fides::metadata::MetaData& selections,
  std::vector<ReadRequest>& out)
{
  auto addRole = [&](const Role& role) {
    if (!role.Present())
    {
      return;
    }
    out.push_back(MakeCellSetPolyDataRequest(
      role.Offsets->DataSourceName, role.Offsets->VariableName, selections));
    out.push_back(MakeCellSetPolyDataRequest(
      role.Connectivity->DataSourceName, role.Connectivity->VariableName, selections));
  };
  addRole(this->Verts);
  addRole(this->Lines);
  addRole(this->Polys);
  addRole(this->Strips);
}

std::vector<size_t> CellSetPolyData::EmitTokens(
  const std::unordered_map<std::string, std::string>& /*paths*/,
  DataSourcesType& /*sources*/,
  const fides::metadata::MetaData& selections,
  const ReadResultMap& results,
  OutputBuilder& builder)
{
  auto lookup = [&](const std::string& src,
                    const std::string& var) -> std::vector<fides::RawArray> {
    auto req = MakeCellSetPolyDataRequest(src, var, selections);
    auto it = results.find(req);
    if (it == results.end())
    {
      throw std::runtime_error("CellSetPolyData::EmitTokens: missing read result for variable '" +
                               var + "'.");
    }
    return it->second;
  };

  auto materializeRole = [&](Role& role) {
    if (!role.Present())
    {
      role.OffsetsArrays.clear();
      role.ConnArrays.clear();
      return;
    }
    role.OffsetsArrays = lookup(role.Offsets->DataSourceName, role.Offsets->VariableName);
    role.ConnArrays = lookup(role.Connectivity->DataSourceName, role.Connectivity->VariableName);
  };
  materializeRole(this->Verts);
  materializeRole(this->Lines);
  materializeRole(this->Polys);
  materializeRole(this->Strips);

  // Determine partition count from whichever role is present. Empty
  // polydata (no roles populated) still produces one partition per
  // block — but we have nothing to read in that case, so the caller
  // would have to fall back to the coord-system block count. Use the
  // first role with data; if none, emit zero partitions.
  size_t nParts = 0;
  for (Role* r : { &this->Verts, &this->Lines, &this->Polys, &this->Strips })
  {
    if (!r->OffsetsArrays.empty())
    {
      nParts = r->OffsetsArrays.size();
      break;
    }
  }

  std::vector<size_t> cellSets(nParts);
  for (size_t i = 0; i < nParts; i++)
  {
    auto roleTokens = [&](const Role& role) -> std::pair<size_t, size_t> {
      if (!role.Present() || i >= role.OffsetsArrays.size() || i >= role.ConnArrays.size())
      {
        return { OutputBuilder::InvalidToken, OutputBuilder::InvalidToken };
      }
      return { builder.CreateArray(role.OffsetsArrays[i]),
               builder.CreateArray(role.ConnArrays[i]) };
    };
    auto vOC = roleTokens(this->Verts);
    auto lOC = roleTokens(this->Lines);
    auto pOC = roleTokens(this->Polys);
    auto sOC = roleTokens(this->Strips);
    cellSets[i] = builder.CreatePolyDataCellSet(
      vOC.first, vOC.second, lOC.first, lOC.second, pOC.first, pOC.second, sOC.first, sOC.second);
  }
  return cellSets;
}

#if FIDES_USE_VISKORES
void CellSetPolyData::ProcessViskores(std::vector<viskores::cont::DataSet>& /*partitions*/,
                                      const fides::metadata::MetaData& /*selections*/)
{
  // Viskores has no native polydata; flattening the four roles into a
  // single CellSetExplicit (with strip expansion) is left as a follow-up
  // until a Viskores polydata consumer materializes. Polydata reads via
  // the VTK backend work; the Viskores backend currently throws.
  throw std::runtime_error("CellSetPolyData::ProcessViskores: polydata is not yet supported "
                           "by the Viskores backend; use the VTK backend.");
}
#endif // FIDES_USE_VISKORES

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

std::vector<size_t> CellSetStructured::Read(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources,
  const fides::metadata::MetaData& selections,
  OutputBuilder& builder)
{
  this->DimensionArrays = this->Dimensions->Read(paths, sources, selections);

  std::size_t nArrays = this->DimensionArrays.size();
  std::vector<size_t> ret(nArrays);

  for (std::size_t i = 0; i < nArrays; i++)
  {
    const auto& raw = this->DimensionArrays[i];
    int64_t dims[3] = { static_cast<int64_t>(GetRawArrayValueAs<std::size_t>(raw, 0)),
                        static_cast<int64_t>(GetRawArrayValueAs<std::size_t>(raw, 1)),
                        static_cast<int64_t>(GetRawArrayValueAs<std::size_t>(raw, 2)) };
    ret[i] = builder.CreateStructuredCellSet(dims);
  }

  return ret;
}

#if FIDES_USE_VISKORES
void CellSetStructured::ProcessViskores(std::vector<viskores::cont::DataSet>& partitions,
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

    const auto& raw = this->DimensionArrays[i];
    viskores::Id3 dims(static_cast<viskores::Id>(GetRawArrayValueAs<std::size_t>(raw, 0)),
                       static_cast<viskores::Id>(GetRawArrayValueAs<std::size_t>(raw, 1)),
                       static_cast<viskores::Id>(GetRawArrayValueAs<std::size_t>(raw, 2)));
    cellSet.SetPointDimensions(dims);

    if (raw.NumValues > 3)
    {
      viskores::Id3 start(static_cast<viskores::Id>(GetRawArrayValueAs<std::size_t>(raw, 3)),
                          static_cast<viskores::Id>(GetRawArrayValueAs<std::size_t>(raw, 4)),
                          static_cast<viskores::Id>(GetRawArrayValueAs<std::size_t>(raw, 5)));
      cellSet.SetGlobalPointIndexStart(start);
    }
    ds.SetCellSet(cellSet);
  }
}
#endif // FIDES_USE_VISKORES


#if FIDES_USE_VISKORES

void CellSetXGC::ProcessJSON(const rapidjson::Value& json, DataSourcesType& sources)
{
  if (!json.HasMember("cells") || !json["cells"].IsObject())
  {
    throw std::runtime_error("must provide a cells object for XGC CellSet.");
  }
  this->CellConnectivity.reset(new Array());
  this->CellConnectivity->ProcessJSON(json["cells"], sources);
  // Also set DataSourceName/VariableName on the Array wrapper for direct data source access
  // (Array::ProcessJSON delegates to internal ArrayImpl and does not set these on itself)
  if (json["cells"].HasMember("data_source"))
    this->CellConnectivity->DataSourceName = json["cells"]["data_source"].GetString();
  if (json["cells"].HasMember("variable"))
    this->CellConnectivity->VariableName = json["cells"]["variable"].GetString();

  if (!json.HasMember("plane_connectivity") || !json["plane_connectivity"].IsObject())
  {
    throw std::runtime_error("must provide a plane_connectivity object for XGC CellSet.");
  }
  this->PlaneConnectivity.reset(new Array());
  this->PlaneConnectivity->ProcessJSON(json["plane_connectivity"], sources);
  if (json["plane_connectivity"].HasMember("data_source"))
    this->PlaneConnectivity->DataSourceName = json["plane_connectivity"]["data_source"].GetString();
  if (json["plane_connectivity"].HasMember("variable"))
    this->PlaneConnectivity->VariableName = json["plane_connectivity"]["variable"].GetString();

  if (json.HasMember("periodic") && json["periodic"].IsBool())
  {
    this->IsPeriodic = json["periodic"].GetBool();
  }
}

std::vector<size_t> CellSetXGC::Read(const std::unordered_map<std::string, std::string>& paths,
                                     DataSourcesType& sources,
                                     const fides::metadata::MetaData& selections,
                                     OutputBuilder& fidesNotUsed(builder))
{
  if (this->IsStatic && !this->CellSetCache.empty())
  {
    return std::vector<size_t>(this->CellSetCache.size(), 0);
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
  // Read raw connectivity data via the data source directly
  {
    const auto& ds = sources[this->CellConnectivity->DataSourceName];
    ds->OpenSource(paths, this->CellConnectivity->DataSourceName);
  }

  // Pass IsVector::No so the connectivity is read as a flat 1D array
  std::vector<fides::RawArray> connectivityRaw =
    sources[this->CellConnectivity->DataSourceName]->ReadVariable(
      this->CellConnectivity->VariableName, newSelections, fides::io::IsVector::No);

  if (connectivityRaw.size() != 1)
  {
    throw std::runtime_error("XGC CellConnectivity should have one Array");
  }

  using intType = viskores::cont::ArrayHandle<viskores::Int32>;

  // Ask the helper to deep-copy the data so it survives scope
  auto connUnknown =
    fides::internal::RawArrayToUnknownArrayHandle(connectivityRaw[0], viskores::CopyFlag::On);

  if (!connUnknown.IsType<intType>())
  {
    throw std::runtime_error("Only int arrays are supported for XGC cell connectivity.");
  }

  intType connectivityAH = connUnknown.AsArrayHandle<intType>();

  {
    const auto& ds = sources[this->PlaneConnectivity->DataSourceName];
    ds->OpenSource(paths, this->PlaneConnectivity->DataSourceName);
  }

  // Pass IsVector::No here as well
  std::vector<fides::RawArray> planeConnectivityRaw =
    sources[this->PlaneConnectivity->DataSourceName]->ReadVariable(
      this->PlaneConnectivity->VariableName, newSelections, fides::io::IsVector::No);

  if (planeConnectivityRaw.size() > 1)
  {
    throw std::runtime_error("xgc nextNode is supposed to be included in one array.");
  }

  // Ask the helper to deep-copy the data
  auto planeConnUnknown =
    fides::internal::RawArrayToUnknownArrayHandle(planeConnectivityRaw[0], viskores::CopyFlag::On);

  if (!planeConnUnknown.IsType<intType>())
  {
    throw std::runtime_error("Only int arrays are supported for XGC plane connectivity.");
  }

  intType planeConnectivityAH = planeConnUnknown.AsArrayHandle<intType>();

  auto numPointsPerPlane = planeConnectivityRaw[0].NumValues;
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

  // Always store cell sets for PostRead (not just when IsStatic).
  // Since Read returns InvalidToken (0) for XGC, the builder does not
  // set cell sets on DataSets. PostRead applies them.
  this->CellSetCache = cellSets;
  // Return placeholder tokens (one per block). The real cell sets are
  // stored internally and applied during PostRead.
  return std::vector<size_t>(cellSets.size(), 0);
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

void CellSetXGC::ProcessViskores(std::vector<viskores::cont::DataSet>& partitions,
                                 const fides::metadata::MetaData& selections)
{
  // Apply cached cell sets to DataSets (they were created in Read but
  // not set by the builder since Read returns InvalidToken)
  for (size_t i = 0; i < partitions.size() && i < this->CellSetCache.size(); i++)
  {
    partitions[i].SetCellSet(this->CellSetCache[i]);
  }

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
  // Also set DataSourceName/VariableName on the Array wrapper for direct data source access
  if (json["igrid"].HasMember("data_source"))
    this->IGrid->DataSourceName = json["igrid"]["data_source"].GetString();
  if (json["igrid"].HasMember("variable"))
    this->IGrid->VariableName = json["igrid"]["variable"].GetString();

  if (!json.HasMember("index_shift") || !json["index_shift"].IsObject())
  {
    throw std::runtime_error("must provide a index_shift object for GTC CellSet.");
  }
  this->IndexShift.reset(new Array());
  this->IndexShift->ProcessJSON(json["index_shift"], sources);
  if (json["index_shift"].HasMember("data_source"))
    this->IndexShift->DataSourceName = json["index_shift"]["data_source"].GetString();
  if (json["index_shift"].HasMember("variable"))
    this->IndexShift->VariableName = json["index_shift"]["variable"].GetString();
}

std::vector<size_t> CellSetGTC::Read(const std::unordered_map<std::string, std::string>& paths,
                                     DataSourcesType& sources,
                                     const fides::metadata::MetaData& selections,
                                     OutputBuilder& fidesNotUsed(builder))
{
  // The size of the cellSetsGTC is always one
  std::vector<size_t> cellSets(1, 0);

  if (!this->IsCached)
  {
    // Read raw arrays via the data source directly and convert to Viskores
    // UnknownArrayHandle for internal use
    {
      const auto& ds = sources[this->IGrid->DataSourceName];
      ds->OpenSource(paths, this->IGrid->DataSourceName);
    }
    std::vector<fides::RawArray> igridRaw =
      sources[this->IGrid->DataSourceName]->ReadVariable(this->IGrid->VariableName, selections);
    if (igridRaw.size() != 1)
    {
      throw std::runtime_error("igrid object not found for GTC CellSet.");
    }
    // Deep copy into owned ArrayHandles so data survives after RawArrays go out of scope
    this->IGridArrays.clear();
    {
      viskores::cont::ArrayHandle<int> owned;
      viskores::cont::ArrayCopy(fides::internal::RawArrayToUnknownArrayHandle(igridRaw[0]), owned);
      this->IGridArrays.push_back(owned);
    }

    {
      const auto& ds = sources[this->IndexShift->DataSourceName];
      ds->OpenSource(paths, this->IndexShift->DataSourceName);
    }
    std::vector<fides::RawArray> indexShiftRaw =
      sources[this->IndexShift->DataSourceName]->ReadVariable(this->IndexShift->VariableName,
                                                              selections);
    if (indexShiftRaw.size() != 1)
    {
      throw std::runtime_error("index_shift object not found for GTC CellSet.");
    }
    this->IndexShiftArrays.clear();
    {
      viskores::cont::ArrayHandle<int> owned;
      viskores::cont::ArrayCopy(fides::internal::RawArrayToUnknownArrayHandle(indexShiftRaw[0]),
                                owned);
      this->IndexShiftArrays.push_back(owned);
    }

    //Create the cell sets. We will fill them in PostRead
    viskores::cont::CellSetSingleType<> cellSet;
    this->CachedCellSet = cellSet;
  }

  return cellSets;
}

void CellSetGTC::ProcessViskores(std::vector<viskores::cont::DataSet>& partitions,
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

std::vector<size_t> CellSetGX::Read(
  const std::unordered_map<std::string, std::string>& fidesNotUsed(paths),
  DataSourcesType& fidesNotUsed(sources),
  const fides::metadata::MetaData& fidesNotUsed(selections),
  OutputBuilder& fidesNotUsed(builder))
{
  // Return a single placeholder token. The real cell set
  // is constructed in PostRead.
  std::vector<size_t> cellSets(1, 0);
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

void CellSetGX::ProcessViskores(std::vector<viskores::cont::DataSet>& partitions,
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

#endif // FIDES_USE_VISKORES

}
}
