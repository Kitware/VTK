//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#include <fides/Array.h>
#include <vtkm/cont/ArrayHandleSOA.h>
#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <vtkm/cont/ArrayHandleXGCCoordinates.h>

#include <vtkm/cont/Invoker.h>
#include <vtkm/worklet/WorkletMapField.h>

namespace fides
{
namespace datamodel
{
namespace fusionutil
{
class PlaneInserterField : public vtkm::worklet::WorkletMapField
{
public:
  PlaneInserterField(vtkm::Id nPlanes, vtkm::Id nPtsPerPlane, vtkm::Id numInsert)
    : NumPlanes(nPlanes)
    , PtsPerPlane(nPtsPerPlane)
    , NumInsert(numInsert)
  {
  }

  using ControlSignature = void(WholeArrayIn inField, WholeArrayOut outField);
  using ExecutionSignature = void(InputIndex, _1, _2);
  using InputDomain = _1;

  template <typename InFieldType, typename OutFieldType>
  VTKM_EXEC void operator()(const vtkm::Id& inIdx,
                            const InFieldType& inField,
                            OutFieldType& outField) const
  {
    vtkm::Id plane0PtIdx = inIdx;
    vtkm::Id inPlaneIdx = plane0PtIdx / this->PtsPerPlane;
    vtkm::Id pt0Offset = plane0PtIdx % this->PtsPerPlane;

    // This is correct:
    vtkm::Id plane1PtIdx = plane0PtIdx + this->PtsPerPlane;
    // Unless we're in the last plane:
    if (inPlaneIdx == this->NumPlanes - 1)
    {
      plane1PtIdx = plane0PtIdx % this->PtsPerPlane;
    }

    vtkm::Id firstOutputPlaneIndex = inPlaneIdx * (1 + NumInsert);

    const auto& y0 = inField.Get(plane0PtIdx);
    const auto& y1 = inField.Get(plane1PtIdx);
    outField.Set(firstOutputPlaneIndex * PtsPerPlane + pt0Offset, y0);

    vtkm::Id numOutCoords = outField.GetNumberOfValues();

    for (vtkm::Id i = 0; i < this->NumInsert; i++)
    {
      vtkm::Id outIdx = (firstOutputPlaneIndex + i + 1) * this->PtsPerPlane + pt0Offset;
      if (outIdx > numOutCoords)
      {
#if defined(VTKM_ENABLE_CUDA) || defined(VTKM_ENABLE_KOKKOS)
        // cuda doesn't like std::string::data
        this->RaiseError("Output index is outside the bounds of output array");
#else
        std::string err = "Output index is computed to be " + std::to_string(outIdx) +
          ", but the output array has size " + std::to_string(numOutCoords);
        this->RaiseError(err.data());
#endif
      }
      // To see this is correct, consider insertion of 1 plane.
      // Then y = (y0 + y1)/2.
      auto y = y0 + (i + 1) * (y1 - y0) / (this->NumInsert + 1);
      outField.Set(outIdx, y);
    }
  }

private:
  vtkm::Id NumPlanes;
  vtkm::Id PtsPerPlane;
  vtkm::Id NumInsert;
};

}

void ArrayPlaceholder::ProcessJSON(const rapidjson::Value& json, DataSourcesType&)
{
  if (!json.HasMember("array_type") || !json["array_type"].IsString())
  {
    throw std::runtime_error(this->ObjectName + " must provide a valid array_type.");
  }
  this->ArrayType = json["array_type"].GetString();

  if (!json.HasMember("data_source") || !json["data_source"].IsString())
  {
    throw std::runtime_error(this->ObjectName + " must provide a valid data_source.");
  }
  this->DataSourceName = json["data_source"].GetString();
}

std::vector<vtkm::cont::UnknownArrayHandle> ArrayPlaceholder::Read(
  const std::unordered_map<std::string, std::string>&,
  DataSourcesType&,
  const fides::metadata::MetaData&)
{
  throw std::runtime_error("ArrayPlaceholder::Read should not be called");
}

size_t ArrayPlaceholder::GetNumberOfBlocks(const std::unordered_map<std::string, std::string>&,
                                           DataSourcesType&)
{
  throw std::runtime_error("ArrayPlaceholder::GetNumberOfBlocks should not be called");
}

std::vector<vtkm::cont::UnknownArrayHandle> Array::Read(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources,
  const fides::metadata::MetaData& selections)
{
  return this->ArrayImpl->Read(paths, sources, selections);
}

void Array::PostRead(std::vector<vtkm::cont::DataSet>& partitions,
                     const fides::metadata::MetaData& selections)
{
  this->ArrayImpl->PostRead(partitions, selections);
}

size_t Array::GetNumberOfBlocks(const std::unordered_map<std::string, std::string>& paths,
                                DataSourcesType& sources)
{
  return this->ArrayImpl->GetNumberOfBlocks(paths, sources);
}

void Array::ProcessJSON(const rapidjson::Value& json, DataSourcesType& sources)
{
  if (!json.HasMember("array_type") || !json["array_type"].IsString())
  {
    throw std::runtime_error(this->ObjectName + " must provide a valid array_type.");
  }
  const std::string& arrayType = json["array_type"].GetString();
  if (arrayType == "basic")
  {
    this->ArrayImpl.reset(new ArrayBasic());
  }
  else if (arrayType == "uniform_point_coordinates")
  {
    this->ArrayImpl.reset(new ArrayUniformPointCoordinates());
  }
  else if (arrayType == "cartesian_product")
  {
    this->ArrayImpl.reset(new ArrayCartesianProduct());
  }
  else if (arrayType == "gtc_coordinates")
  {
    this->ArrayImpl.reset(new ArrayGTCCoordinates());
  }
  else if (arrayType == "xgc_coordinates")
  {
    this->ArrayImpl.reset(new ArrayXGCCoordinates());
  }
  else if (arrayType == "xgc_field")
  {
    this->ArrayImpl.reset(new ArrayXGCField());
  }
  else if (arrayType == "gtc_field")
  {
    this->ArrayImpl.reset(new ArrayGTCField());
  }
  else
  {
    throw std::runtime_error(arrayType + " is not a valid array type.");
  }
  this->ArrayImpl->ProcessJSON(json, sources);
}

void Array::CreatePlaceholder(const rapidjson::Value& json, DataSourcesType& sources)
{
  if (this->ArrayImpl)
  {
    throw std::runtime_error("ArrayPlaceholders should not have set ArrayImpl");
  }
  this->Placeholder.reset(new ArrayPlaceholder());
  this->Placeholder->ProcessJSON(json, sources);
}

void ArrayBasic::ProcessJSON(const rapidjson::Value& json, DataSourcesType& sources)
{
  this->ArrayBase::ProcessJSON(json, sources);

  if (json.HasMember("is_vector"))
  {
    const std::string& isVector = json["is_vector"].GetString();
    if (isVector == "true")
    {
      this->IsVector = fides::io::IsVector::Yes;
    }
    else if (isVector == "false")
    {
      this->IsVector = fides::io::IsVector::No;
    }
    else if (isVector == "auto")
    {
      this->IsVector = fides::io::IsVector::Auto;
    }
    else
    {
      throw std::runtime_error("Unrecognized value for is_vector: " + isVector);
    }
  }
}

std::vector<vtkm::cont::UnknownArrayHandle> ArrayBasic::Read(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources,
  const fides::metadata::MetaData& selections)
{
  return this->ReadSelf(paths, sources, selections, this->IsVector);
}

size_t ArrayBasic::GetNumberOfBlocks(const std::unordered_map<std::string, std::string>& paths,
                                     DataSourcesType& sources)
{
  auto itr = paths.find(this->DataSourceName);
  if (itr == paths.end())
  {
    throw std::runtime_error("Could not find data_source with name " + this->DataSourceName +
                             " among the input paths.");
  }
  const auto& ds = sources[this->DataSourceName];
  std::string path = itr->second + ds->FileName;
  ds->OpenSource(path);
  return ds->GetNumberOfBlocks(this->VariableName);
}

void ArrayUniformPointCoordinates::ProcessJSON(const rapidjson::Value& json,
                                               DataSourcesType& sources)
{
  if (!json.HasMember("dimensions") || !json["dimensions"].IsObject())
  {
    throw std::runtime_error(this->ObjectName + " must provide a dimensions object.");
  }
  this->Dimensions.reset(new Value());
  const auto& dimensions = json["dimensions"];
  this->Dimensions->ProcessJSON(dimensions, sources);

  if (json.HasMember("origin") && json["origin"].IsObject())
  {
    this->Origin.reset(new Value());
    const auto& origin = json["origin"];
    this->Origin->ProcessJSON(origin, sources);
  }

  if (json.HasMember("spacing") && json["spacing"].IsObject())
  {
    this->Spacing.reset(new Value());
    const auto& spacing = json["spacing"];
    this->Spacing->ProcessJSON(spacing, sources);
  }

  //See if we are using variable shape, or variables for dims/origin/spacing.
  this->DefinedFromVariableShape = true;
  if (dimensions.HasMember("source"))
  {
    std::string dimSrc, originSrc, spacingSrc;
    dimSrc = dimensions["source"].GetString();

    if (this->Spacing)
    {
      const auto& spacing = json["spacing"];
      if (spacing.HasMember("source"))
      {
        spacingSrc = spacing["source"].GetString();
      }
    }

    if (this->Origin)
    {
      const auto& origin = json["origin"];
      if (origin.HasMember("source"))
      {
        originSrc = origin["source"].GetString();
      }
    }

    if (dimSrc == "array_variable" && originSrc == "array_variable" &&
        spacingSrc == "array_variable")
    {
      this->DefinedFromVariableShape = false;
    }
  }
}

std::vector<vtkm::cont::UnknownArrayHandle> ArrayUniformPointCoordinates::Read(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources,
  const fides::metadata::MetaData& selections)
{
  std::vector<vtkm::cont::UnknownArrayHandle> ret;

  if (this->DefinedFromVariableShape)
  {
    // In this situation we can do everything now instead of waiting for
    // the PostRead
    std::vector<vtkm::cont::UnknownArrayHandle> dims =
      this->Dimensions->Read(paths, sources, selections);
    std::vector<vtkm::cont::UnknownArrayHandle> origins;
    if (this->Origin)
    {
      origins = this->Origin->Read(paths, sources, selections);
    }
    std::vector<vtkm::cont::UnknownArrayHandle> spacings;
    if (this->Spacing)
    {
      spacings = this->Spacing->Read(paths, sources, selections);
    }

    size_t nDims = dims.size();
    ret.reserve(nDims);

    for (const auto& array : dims)
    {
      auto dimsB = array.AsArrayHandle<vtkm::cont::ArrayHandle<size_t>>();
      auto dimsPortal = dimsB.ReadPortal();
      vtkm::Id3 dimValues(static_cast<vtkm::Id>(dimsPortal.Get(0)),
                          static_cast<vtkm::Id>(dimsPortal.Get(1)),
                          static_cast<vtkm::Id>(dimsPortal.Get(2)));
      vtkm::Vec3f originA(0.0, 0.0, 0.0);
      vtkm::Vec3f spacingA(1.0, 1.0, 1.0);
      if (this->Origin)
      {
        const auto& origin = origins[0];
        auto originB = origin.AsArrayHandle<vtkm::cont::ArrayHandle<double>>();
        auto originPortal = originB.ReadPortal();
        originA = vtkm::Vec3f(originPortal.Get(0), originPortal.Get(1), originPortal.Get(2));
      }
      if (this->Spacing)
      {
        const auto& spacing = spacings[0];
        auto spacingB = spacing.AsArrayHandle<vtkm::cont::ArrayHandle<double>>();
        auto spacingPortal = spacingB.ReadPortal();
        spacingA = vtkm::Vec3f(spacingPortal.Get(0), spacingPortal.Get(1), spacingPortal.Get(2));
      }
      // Shift origin to a local value. We have to do this because
      // VTK-m works with dimensions rather than extents and therefore
      // needs local origin.
      for (vtkm::IdComponent i = 0; i < 3; i++)
      {
        originA[i] = originA[i] + spacingA[i] * dimsPortal.Get(i + 3);
      }
      vtkm::cont::ArrayHandleUniformPointCoordinates ah(dimValues, originA, spacingA);
      ret.push_back(ah);
    }
  }
  else
  {
    // in this situation, we need to save the UnknownArrayHandles we read
    // and once we actually have the data in PostRead, then we can add the coordinates
    // to the dataset
    this->DimensionArrays = this->Dimensions->Read(paths, sources, selections);
    this->OriginArrays = this->Origin->Read(paths, sources, selections);
    this->SpacingArrays = this->Spacing->Read(paths, sources, selections);

    // In the case of CellSets that use data read from the ADIOS files,
    // we create empty UnknownCellSets for each partition and return a
    // vector of those. For CoordinateSystem, we'll actually create those
    // objects at PostRead and just return an empty vector here.
  }

  return ret;
}

void ArrayUniformPointCoordinates::PostRead(
  std::vector<vtkm::cont::DataSet>& partitions,
  const fides::metadata::MetaData& fidesNotUsed(selections))
{
  if (!this->DefinedFromVariableShape)
  {
    size_t nDims = this->DimensionArrays.size();
    for (std::size_t i = 0; i < nDims; i++)
    {
      auto d = this->DimensionArrays[i].AsArrayHandle<vtkm::cont::ArrayHandle<std::size_t>>();
      auto o = this->OriginArrays[i].AsArrayHandle<vtkm::cont::ArrayHandle<double>>();
      auto s = this->SpacingArrays[i].AsArrayHandle<vtkm::cont::ArrayHandle<double>>();
      auto dPortal = d.ReadPortal();
      auto oPortal = o.ReadPortal();
      auto sPortal = s.ReadPortal();

      vtkm::Id3 dValues(static_cast<vtkm::Id>(dPortal.Get(0)),
                        static_cast<vtkm::Id>(dPortal.Get(1)),
                        static_cast<vtkm::Id>(dPortal.Get(2)));
      vtkm::Vec3f oValues(oPortal.Get(0), oPortal.Get(1), oPortal.Get(2));
      vtkm::Vec3f sValues(sPortal.Get(0), sPortal.Get(1), sPortal.Get(2));
      vtkm::cont::ArrayHandleUniformPointCoordinates ah(dValues, oValues, sValues);
      vtkm::cont::CoordinateSystem coords("coordinates", ah);

      auto& ds = partitions[i];
      ds.AddCoordinateSystem(coords);
    }
  }
}

size_t ArrayUniformPointCoordinates::GetNumberOfBlocks(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources)
{
  return this->Dimensions->GetNumberOfBlocks(paths, sources);
}

void ArrayCartesianProduct::ProcessJSON(const rapidjson::Value& json, DataSourcesType& sources)
{
  if (!json.HasMember("x_array") || !json["x_array"].IsObject())
  {
    throw std::runtime_error(this->ObjectName + " must provide a x_array object.");
  }
  this->XArray.reset(new Array());
  const auto& xarray = json["x_array"];
  this->XArray->ProcessJSON(xarray, sources);

  if (!json.HasMember("y_array") || !json["y_array"].IsObject())
  {
    throw std::runtime_error(this->ObjectName + " must provide a y_array object.");
  }
  this->YArray.reset(new Array());
  const auto& yarray = json["y_array"];
  this->YArray->ProcessJSON(yarray, sources);

  if (!json.HasMember("z_array") || !json["z_array"].IsObject())
  {
    throw std::runtime_error(this->ObjectName + " must provide a z_array object.");
  }
  this->ZArray.reset(new Array());
  const auto& zarray = json["z_array"];
  this->ZArray->ProcessJSON(zarray, sources);
}

std::vector<vtkm::cont::UnknownArrayHandle> ArrayCartesianProduct::Read(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources,
  const fides::metadata::MetaData& selections)
{
  std::vector<vtkm::cont::UnknownArrayHandle> retVal;
  std::vector<vtkm::cont::UnknownArrayHandle> xarrays =
    this->XArray->Read(paths, sources, selections);
  std::vector<vtkm::cont::UnknownArrayHandle> yarrays =
    this->YArray->Read(paths, sources, selections);
  std::vector<vtkm::cont::UnknownArrayHandle> zarrays =
    this->ZArray->Read(paths, sources, selections);
  size_t nArrays = xarrays.size();
  for (size_t i = 0; i < nArrays; i++)
  {
    auto& xarray = xarrays[i];
    auto& yarray = yarrays[i];
    auto& zarray = zarrays[i];
    using floatType = vtkm::cont::ArrayHandle<float>;
    using doubleType = vtkm::cont::ArrayHandle<double>;
    if (xarray.IsType<floatType>() && yarray.IsType<floatType>() && zarray.IsType<floatType>())
    {
      auto xarrayF = xarray.AsArrayHandle<floatType>();
      auto yarrayF = yarray.AsArrayHandle<floatType>();
      auto zarrayF = zarray.AsArrayHandle<floatType>();
      retVal.push_back(vtkm::cont::make_ArrayHandleCartesianProduct(xarrayF, yarrayF, zarrayF));
    }
    else if (xarray.IsType<doubleType>() && yarray.IsType<doubleType>() &&
             zarray.IsType<doubleType>())
    {
      auto xarrayD = xarray.AsArrayHandle<doubleType>();
      auto yarrayD = yarray.AsArrayHandle<doubleType>();
      auto zarrayD = zarray.AsArrayHandle<doubleType>();
      retVal.push_back(vtkm::cont::make_ArrayHandleCartesianProduct(xarrayD, yarrayD, zarrayD));
    }
    else
    {
      throw std::runtime_error("Only float and double arrays are supported in cartesian products.");
    }
  }
  return retVal;
}

size_t ArrayCartesianProduct::GetNumberOfBlocks(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources)
{
  return this->XArray->GetNumberOfBlocks(paths, sources);
}

ArrayXGC::ArrayXGC()
  : CommonImpl(new XGCCommon())
{
}

size_t ArrayXGC::GetNumberOfBlocks(const std::unordered_map<std::string, std::string>& paths,
                                   DataSourcesType& sources)
{
  if (this->NumberOfPlanes < 0)
  {
    this->NumberOfPlanes = this->CommonImpl->GetNumberOfPlanes(paths, sources);
  }
  return this->CommonImpl->GetNumberOfBlocks();
}

void ArrayXGC::CheckEngineType(const std::unordered_map<std::string, std::string>& paths,
                               DataSourcesType& sources,
                               std::string& dataSourceName)
{
  auto itr = paths.find(dataSourceName);
  if (itr == paths.end())
  {
    throw std::runtime_error("Could not find data_source with name " + dataSourceName +
                             " among the input paths.");
  }
  const auto& ds = sources[dataSourceName];

  if (ds->GetEngineType() == fides::io::EngineType::Inline)
  {
    throw std::runtime_error("Inline engine not supported for XGC."
                             "Must use BP files and/or SST.");
  }
  this->EngineChecked = true;
}

std::vector<size_t> ArrayXGC::GetShape(const std::unordered_map<std::string, std::string>& paths,
                                       DataSourcesType& sources)
{
  auto itr = paths.find(this->DataSourceName);
  if (itr == paths.end())
  {
    throw std::runtime_error("Could not find data_source with name " + this->DataSourceName +
                             " among the input paths.");
  }
  const auto& ds = sources[this->DataSourceName];
  std::string path = itr->second + ds->FileName;
  ds->OpenSource(path);
  return ds->GetVariableShape(this->VariableName);
}

/// Functor created so that UnknownArrayHandle's CastAndCall() will handle making the
/// appropriate cast to an ArrayHandle.
/// The specialization is because it tries to use other StorageTags when compiling,
/// but we only want to support StorageTagBasic.
struct ArrayXGCCoordinates::AddToVectorFunctor
{
  template <typename T, typename S>
  VTKM_CONT void operator()(const vtkm::cont::ArrayHandle<T, S>&,
                            std::vector<vtkm::cont::UnknownArrayHandle>&,
                            vtkm::Id,
                            vtkm::Id,
                            vtkm::Id,
                            bool) const
  {
  }

  /// This version is for creating the coordinates AHs.
  template <typename T>
  VTKM_CONT void operator()(const vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagBasic>& array,
                            std::vector<vtkm::cont::UnknownArrayHandle>& retVal,
                            vtkm::Id numberOfPlanes,
                            vtkm::Id numberOfPlanesOwned,
                            vtkm::Id planeStartId,
                            bool isCylindrical) const
  {
    retVal.push_back(vtkm::cont::make_ArrayHandleXGCCoordinates(
      array, numberOfPlanesOwned, isCylindrical, numberOfPlanes, planeStartId));
  }
};

void ArrayXGCCoordinates::ProcessJSON(const rapidjson::Value& json, DataSourcesType& sources)
{
  this->ArrayXGC::ProcessJSON(json, sources);
  if (!json.HasMember("is_cylindrical") || !json["is_cylindrical"].IsBool())
  {
    throw std::runtime_error(this->ObjectName + " must provide a coordinates_type.");
  }
  this->IsCylindrical = json["is_cylindrical"].GetBool();
}

std::vector<vtkm::cont::UnknownArrayHandle> ArrayXGCCoordinates::Read(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources,
  const fides::metadata::MetaData& selections)
{
  if (!this->EngineChecked)
  {
    this->CheckEngineType(paths, sources, this->DataSourceName);
  }

  if (this->NumberOfPlanes < 0)
  {
    this->NumberOfPlanes = this->CommonImpl->GetNumberOfPlanes(paths, sources);
  }

  fides::metadata::MetaData newSelections = selections;
  // Removing because for XGC Fides blocks are not the same as ADIOS blocks
  newSelections.Remove(fides::keys::BLOCK_SELECTION());

  std::vector<vtkm::cont::UnknownArrayHandle> retVal;

  auto coordArrays = this->ReadSelf(paths, sources, newSelections, fides::io::IsVector::No);
  if (coordArrays.size() != 1)
  {
    throw std::runtime_error("ArrayXGCCoordinates supports only one coordinates array");
  }

  auto& coordsAH = coordArrays[0];
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

  size_t numInsertPlanes = 0;
  if (selections.Has(fides::keys::fusion::PLANE_INSERTION()))
  {
    numInsertPlanes =
      selections.Get<fides::metadata::Size>(fides::keys::fusion::PLANE_INSERTION()).NumberOfItems;
  }

  for (size_t i = 0; i < blocksInfo.size(); ++i)
  {
    const auto& block = blocksInfo[i];
    coordsAH
      .CastAndCallForTypes<vtkm::TypeListFieldScalar, vtkm::List<vtkm::cont::StorageTagBasic>>(
        AddToVectorFunctor(),
        retVal,
        NumberOfPlanes * (1 + numInsertPlanes),
        block.NumberOfPlanesOwned * (1 + numInsertPlanes),
        block.PlaneStartId * (1 + numInsertPlanes),
        this->IsCylindrical);
  }
  return retVal;
}

vtkm::cont::UnknownArrayHandle ArrayXGCField::Read3DVariable(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources,
  const fides::metadata::MetaData& selections)
{
  auto itr = paths.find(this->DataSourceName);
  if (itr == paths.end())
  {
    throw std::runtime_error("Could not find data_source with name " + this->DataSourceName +
                             " among the input paths.");
  }
  const auto& ds = sources[this->DataSourceName];
  std::string path = itr->second + ds->FileName;
  ds->OpenSource(path);
  auto arrays = ds->ReadMultiBlockVariable(this->VariableName, selections);
  if (arrays.size() != 1)
  {
    throw std::runtime_error("3d field should be read into a single ArrayHandle");
  }
  return arrays[0];
}

std::vector<vtkm::cont::UnknownArrayHandle> ArrayXGCField::Read(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources,
  const fides::metadata::MetaData& selections)
{
  if (!this->EngineChecked)
  {
    this->CheckEngineType(paths, sources, this->DataSourceName);
  }

  if (this->NumberOfPlanes < 0)
  {
    this->NumberOfPlanes = this->CommonImpl->GetNumberOfPlanes(paths, sources);
  }

  if (!this->FieldDimsChecked)
  {
    auto shape = this->GetShape(paths, sources);
    if (shape.size() == 1 || shape.size() == 2)
    {
      // shape.size() is 2 for 3d variables
      // and size is 1 for 2d variables
      for (auto& s : shape)
      {
        if (s == static_cast<size_t>(this->NumberOfPlanes))
        {
          // In this case, we know the variable has a dimension for the plane,
          // so it's a 3D variable
          this->Is2DField = false;
        }
      }
    }

    this->FieldDimsChecked = true;
  }

  std::vector<vtkm::cont::UnknownArrayHandle> retVal;

  metadata::MetaData newSelections = selections;
  // Removing because for XGC Fides blocks are not the same as ADIOS blocks
  newSelections.Remove(fides::keys::BLOCK_SELECTION());
  std::pair<std::vector<XGCBlockInfo>, fides::metadata::Set<size_t>> info;
  if (selections.Has(fides::keys::BLOCK_SELECTION()))
  {
    info = this->CommonImpl->GetXGCBlockInfoWithPlaneSelection(
      selections.Get<fides::metadata::Vector<size_t>>(fides::keys::BLOCK_SELECTION()).Data);
  }
  else
  {
    info = this->CommonImpl->GetXGCBlockInfoWithPlaneSelection(std::vector<size_t>());
  }
  auto& blocksInfo = info.first;
  if (blocksInfo.empty())
  {
    throw std::runtime_error(
      "No XGC block info returned. May want to double check block selection.");
  }

  if (this->Is2DField)
  {
    auto fieldData = this->ReadSelf(paths, sources, newSelections, fides::io::IsVector::No);
    assert(fieldData.size() == 1);
    retVal.push_back(fieldData[0]);
  }
  else
  {
    // read all planes (if its in a requested blocks) once only
    for (const auto& block : blocksInfo)
    {
      fides::metadata::Vector<size_t> planesToRead;

      for (vtkm::Id i = block.PlaneStartId; i < block.PlaneStartId + block.NumberOfPlanesOwned; ++i)
      {
        auto planeId = i;
        if (planeId == this->NumberOfPlanes)
        {
          // to handle last plane on n-1 block
          planeId = 0;
        }
        planesToRead.Data.push_back(static_cast<size_t>(planeId));
      }
      newSelections.Remove(fides::keys::BLOCK_SELECTION());
      newSelections.Set(fides::keys::BLOCK_SELECTION(), planesToRead);
      auto planeData = this->Read3DVariable(paths, sources, newSelections);
      retVal.push_back(planeData);
    }
  }

  return retVal;
}

void ArrayXGCField::PostRead(std::vector<vtkm::cont::DataSet>& dataSets,
                             const fides::metadata::MetaData& metaData)
{
  size_t numInsertPlanes = 0;
  if (!this->Is2DField && metaData.Has(fides::keys::fusion::PLANE_INSERTION()))
  {
    numInsertPlanes =
      metaData.Get<fides::metadata::Size>(fides::keys::fusion::PLANE_INSERTION()).NumberOfItems;
  }

  if (numInsertPlanes == 0)
  {
    return;
  }

  if (dataSets.size() > 1)
    throw std::runtime_error("Plane insert for XGC not supported for multiple datasets.");

  const auto& cs = dataSets[0].GetCellSet();
  if (!cs.IsType<vtkm::cont::CellSetExtrude>())
    throw std::runtime_error("Wrong type of cell set for XGC dataset.");

  auto cellSet = cs.AsCellSet<vtkm::cont::CellSetExtrude>();
  vtkm::Id ptsPerPlane = cellSet.GetNumberOfPointsPerPlane();
  vtkm::Id numPlanes = this->NumberOfPlanes;
  vtkm::Id totNumPlanes = numPlanes + (numPlanes * numInsertPlanes);

  if (!dataSets[0].HasPointField(this->VariableName))
  {
    return;
  }

  auto fieldArray = dataSets[0].GetField(this->VariableName).GetData();

  vtkm::cont::Invoker invoke;

  using floatType = vtkm::cont::ArrayHandle<float>;
  using doubleType = vtkm::cont::ArrayHandle<double>;
  fusionutil::PlaneInserterField planeInserter(numPlanes, ptsPerPlane, numInsertPlanes);
  if (fieldArray.IsType<floatType>())
  {
    auto inArr = fieldArray.AsArrayHandle<floatType>();
    vtkm::cont::ArrayHandle<float> outArr;
    outArr.Allocate(totNumPlanes * ptsPerPlane);
    invoke(planeInserter, inArr, outArr);
    dataSets[0].AddPointField(this->VariableName, outArr);
  }
  else if (fieldArray.IsType<doubleType>())
  {
    auto inArr = fieldArray.AsArrayHandle<doubleType>();
    vtkm::cont::ArrayHandle<double> outArr;
    outArr.Allocate(totNumPlanes * ptsPerPlane);
    invoke(planeInserter, inArr, outArr);
    dataSets[0].AddPointField(this->VariableName, outArr);
  }
  else
    throw std::runtime_error("Unsupported field type for XGC.");
}

void ArrayGTCCoordinates::ProcessJSON(const rapidjson::Value& json, DataSourcesType& sources)
{
  if (!json.HasMember("x_array") || !json["x_array"].IsObject())
  {
    throw std::runtime_error(this->ObjectName + " must provide a x_array object.");
  }
  this->XArray.reset(new ArrayBasic());
  const auto& xarray = json["x_array"];
  this->XArray->ProcessJSON(xarray, sources);

  if (!json.HasMember("y_array") || !json["y_array"].IsObject())
  {
    throw std::runtime_error(this->ObjectName + " must provide a y_array object.");
  }
  this->YArray.reset(new ArrayBasic());
  const auto& yarray = json["y_array"];
  this->YArray->ProcessJSON(yarray, sources);

  if (!json.HasMember("z_array") || !json["z_array"].IsObject())
  {
    throw std::runtime_error(this->ObjectName + " must provide a z_array object.");
  }
  this->ZArray.reset(new ArrayBasic());
  const auto& zarray = json["z_array"];
  this->ZArray->ProcessJSON(zarray, sources);
}

std::vector<vtkm::cont::UnknownArrayHandle> ArrayGTCCoordinates::Read(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources,
  const fides::metadata::MetaData& selections)
{
  std::vector<vtkm::cont::UnknownArrayHandle> retVal;

  //First time, so read and set cache.
  if (!this->IsCached)
  {
    auto newSelections = selections;
    fides::metadata::Bool flag(true);
    newSelections.Set(fides::keys::READ_AS_MULTIBLOCK(), flag);

    //Add some checks.
    auto xarrays = this->XArray->Read(paths, sources, newSelections);
    auto yarrays = this->YArray->Read(paths, sources, newSelections);
    auto zarrays = this->ZArray->Read(paths, sources, newSelections);
    if (!(xarrays.size() == 1 && yarrays.size() == 1 && zarrays.size() == 1))
    {
      throw std::runtime_error("Wrong number arrays for GTC coords.");
    }

    auto& xarray = xarrays[0];
    auto& yarray = yarrays[0];
    auto& zarray = zarrays[0];

    using floatType = vtkm::cont::ArrayHandle<float>;
    using doubleType = vtkm::cont::ArrayHandle<double>;
    if (xarray.IsType<floatType>() && yarray.IsType<floatType>() && zarray.IsType<floatType>())
    {
      auto xarrayF = xarray.AsArrayHandle<floatType>();
      auto yarrayF = yarray.AsArrayHandle<floatType>();
      auto zarrayF = zarray.AsArrayHandle<floatType>();
      this->CachedCoords =
        vtkm::cont::make_ArrayHandleSOA<vtkm::Vec3f_32>({ xarrayF, yarrayF, zarrayF });
    }
    else if (xarray.IsType<doubleType>() && yarray.IsType<doubleType>() &&
             zarray.IsType<doubleType>())
    {
      auto xarrayD = xarray.AsArrayHandle<doubleType>();
      auto yarrayD = yarray.AsArrayHandle<doubleType>();
      auto zarrayD = zarray.AsArrayHandle<doubleType>();
      this->CachedCoords =
        vtkm::cont::make_ArrayHandleSOA<vtkm::Vec3f_64>({ xarrayD, yarrayD, zarrayD });
    }
    else
    {
      throw std::runtime_error("Only float and double arrays are supported in ArrayGTC products.");
    }
  }

  retVal.push_back(this->CachedCoords);
  return retVal;
}

size_t ArrayGTCCoordinates::GetNumberOfBlocks(const std::unordered_map<std::string, std::string>&,
                                              DataSourcesType&)
{
  return 1;
}

class ArrayGTCCoordinates::PlaneInserter : public vtkm::worklet::WorkletMapField
{
public:
  PlaneInserter(vtkm::Id nPlanes, vtkm::Id nPtsPerPlane, vtkm::Id numInsert)
    : NumPlanes(nPlanes)
    , PtsPerPlane(nPtsPerPlane)
    , NumInsert(numInsert)
  {
    this->dT = 1 / static_cast<vtkm::Float64>(this->NumInsert + 1);
    this->dPhi = 1 / static_cast<vtkm::Float64>(this->NumPlanes * (1 + this->NumInsert));
  }

  using ControlSignature = void(WholeArrayIn inCoords, WholeArrayOut outCoords);
  using ExecutionSignature = void(InputIndex, _1, _2);
  using InputDomain = _1;

  //Need to template this later.
  using PointType = vtkm::Vec<vtkm::Float32, 3>;

  template <typename InCoordsType, typename OutCoordsType>
  VTKM_EXEC void operator()(const vtkm::Id& inIdx,
                            const InCoordsType& inCoords,
                            OutCoordsType& outCoords) const
  {
    vtkm::Id plane0PtIdx = inIdx;
    vtkm::Id inPlaneIdx = plane0PtIdx / this->PtsPerPlane;
    vtkm::Id pt0Offset = plane0PtIdx % this->PtsPerPlane;
    // This is correct:
    vtkm::Id plane1PtIdx = plane0PtIdx + this->PtsPerPlane;
    // Unless we're in the last plane:
    if (inPlaneIdx == this->NumPlanes - 1)
    {
      plane1PtIdx = plane0PtIdx % this->PtsPerPlane;
    }

    vtkm::Id firstOutputPlaneIndex = inPlaneIdx * (1 + this->NumInsert);

    const auto& plane0Pt = inCoords.Get(plane0PtIdx);
    const auto& plane1Pt = inCoords.Get(plane1PtIdx);
    outCoords.Set(firstOutputPlaneIndex * this->PtsPerPlane + pt0Offset, plane0Pt);

    //Now add NumInsert interpolated points.
    const auto rad = vtkm::Sqrt((plane0Pt[0] * plane0Pt[0] + plane0Pt[1] * plane0Pt[1]));
    const auto Z = plane0Pt[2];

    //optimize this later...
    auto phi0 = vtkm::ATan2(plane0Pt[1], plane0Pt[0]);
    auto phi1 = vtkm::ATan2(plane1Pt[1], plane1Pt[0]);

    if (phi0 < phi1)
    {
      phi0 += vtkm::TwoPi();
    }

    vtkm::Float64 t = dT;
    vtkm::Id numOutCoords = outCoords.GetNumberOfValues();

    for (vtkm::Id i = 0; i < this->NumInsert; i++)
    {
      //calculate the index for the inbetween plane points.
      vtkm::Id outIdx = (firstOutputPlaneIndex + i + 1) * this->PtsPerPlane + pt0Offset;
      if (outIdx > numOutCoords)
      {
#if defined(VTKM_ENABLE_CUDA) || defined(VTKM_ENABLE_KOKKOS)
        // cuda doesn't like std::string::data
        this->RaiseError("Output index is outside the bounds of output array");
#else
        std::string err = "Output index is computed to be " + std::to_string(outIdx) +
          ", but the output array has size " + std::to_string(numOutCoords);
        this->RaiseError(err.data());
#endif
      }
      //interpolate phi, convert to cartesian.
      auto phi = phi0 + t * (phi1 - phi0);
      PointType outPt(rad * vtkm::Cos(phi), rad * vtkm::Sin(phi), Z);
      outCoords.Set(outIdx, outPt);
      t += dT;
    }
  }

private:
  vtkm::Id NumPlanes;
  vtkm::Id PtsPerPlane;
  vtkm::Id NumInsert;
  vtkm::Float64 dT;
  vtkm::Float64 dPhi;
};

void ArrayGTCCoordinates::PostRead(std::vector<vtkm::cont::DataSet>& dataSets,
                                   const fides::metadata::MetaData& metaData)
{
  if (dataSets.size() != 1)
  {
    throw std::runtime_error("Wrong number of partitions for GTC DataSets.");
  }

  auto& dataSet = dataSets[0];
  auto& cs = dataSet.GetCoordinateSystem();

  size_t numInsertPlanes = 0;
  if (metaData.Has(fides::keys::fusion::PLANE_INSERTION()))
  {
    numInsertPlanes =
      metaData.Get<fides::metadata::Size>(fides::keys::fusion::PLANE_INSERTION()).NumberOfItems;
  }

  if (numInsertPlanes == 0)
  {
    this->CachedCoords = cs.GetData();
    this->IsCached = true;
  }
  else if (!this->IsCached)
  {
    //Make sure fields are there.
    if (!dataSet.HasField("num_planes") || !dataSet.HasField("num_pts_per_plane"))
    {
      throw std::runtime_error("num_planes and/or num_pts_per_plane not found.");
    }

    using intType = vtkm::cont::ArrayHandle<int>;
    auto numPlanes = dataSet.GetField("num_planes").GetData().AsArrayHandle<intType>();
    auto numPtsPerPlane = dataSet.GetField("num_pts_per_plane").GetData().AsArrayHandle<intType>();

    vtkm::Id numberOfPlanes = static_cast<vtkm::Id>(numPlanes.ReadPortal().Get(0));
    vtkm::Id numberOfPointsPerPlane = static_cast<vtkm::Id>(numPtsPerPlane.ReadPortal().Get(0));

    ArrayGTCCoordinates::PlaneInserter plnIns(
      numberOfPlanes, numberOfPointsPerPlane, numInsertPlanes);
    if (cs.GetData().IsType<GTCCoordsType32>())
    {
      auto inCoords = cs.GetData().AsArrayHandle<GTCCoordsType32>();
      GTCCoordsType32 newCoords;
      vtkm::Id numTotalPlanes = numberOfPlanes * (1 + numInsertPlanes);
      vtkm::cont::Invoker invoke;
      newCoords.Allocate(numTotalPlanes * numberOfPointsPerPlane);
      invoke(plnIns, inCoords, newCoords);

      this->CachedCoords = newCoords;
    }
    else if (cs.GetData().IsType<GTCCoordsType64>())
    {
      auto inCoords = cs.GetData().AsArrayHandle<GTCCoordsType64>();
      GTCCoordsType64 newCoords;
      vtkm::Id numTotalPlanes = numberOfPlanes * (1 + numInsertPlanes);
      vtkm::cont::Invoker invoke;
      newCoords.Allocate(numTotalPlanes * numberOfPointsPerPlane);
      invoke(plnIns, inCoords, newCoords);

      this->CachedCoords = newCoords;
    }
    else
    {
      throw std::runtime_error("unsupported coordinates type for GTC.");
    }

    this->IsCached = true;
  }

  //Set coords to cached coordinates.
  cs = vtkm::cont::CoordinateSystem("coords", this->CachedCoords);
}

/// Reads and returns array handles.
std::vector<vtkm::cont::UnknownArrayHandle> ArrayGTCField::Read(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources,
  const fides::metadata::MetaData& selections)
{
  auto newSelections = selections;
  fides::metadata::Bool flag(true);
  newSelections.Set(fides::keys::READ_AS_MULTIBLOCK(), flag);

  return this->ReadSelf(paths, sources, newSelections);
}

void ArrayGTCField::PostRead(std::vector<vtkm::cont::DataSet>& dataSets,
                             const fides::metadata::MetaData& metaData)
{
  if (dataSets.size() != 1)
  {
    throw std::runtime_error("Wrong number of partitions for GTC DataSets.");
  }

  size_t numInsertPlanes = 0;
  if (metaData.Has(fides::keys::fusion::PLANE_INSERTION()))
  {
    numInsertPlanes =
      metaData.Get<fides::metadata::Size>(fides::keys::fusion::PLANE_INSERTION()).NumberOfItems;
  }
  //No extra planes, we're done.
  if (numInsertPlanes == 0)
  {
    return;
  }

  //Add additional planes.
  auto& dataSet = dataSets[0];

  //Grab metadata on the first read.
  if (!this->IsCached)
  {
    if (!dataSet.HasField("num_planes") || !dataSet.HasField("num_pts_per_plane"))
    {
      throw std::runtime_error("num_planes and/or num_pts_per_plane not found.");
    }
    using intType = vtkm::cont::ArrayHandle<int>;
    auto numPlanes = dataSet.GetField("num_planes").GetData().AsArrayHandle<intType>();
    auto numPtsPerPlane = dataSet.GetField("num_pts_per_plane").GetData().AsArrayHandle<intType>();

    this->NumberOfPointsPerPlane = static_cast<vtkm::Id>(numPtsPerPlane.ReadPortal().Get(0));
    this->NumberOfPlanes = static_cast<vtkm::Id>(numPlanes.ReadPortal().Get(0));

    this->IsCached = true;
  }

  if (dataSet.HasPointField(this->VariableName))
  {
    const auto& field = dataSet.GetField(this->VariableName);
    const auto& arr = field.GetData();

    vtkm::Id numTotalPlanes = this->NumberOfPlanes * (1 + numInsertPlanes);
    fusionutil::PlaneInserterField planeInserter(
      this->NumberOfPlanes, this->NumberOfPointsPerPlane, numInsertPlanes);
    vtkm::cont::Invoker invoke;

    if (arr.IsType<vtkm::cont::ArrayHandle<vtkm::Float32>>())
    {
      auto inArr = arr.AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Float32>>();
      vtkm::cont::ArrayHandle<vtkm::Float32> outArr;
      outArr.Allocate(numTotalPlanes * this->NumberOfPointsPerPlane);
      invoke(planeInserter, inArr, outArr);

      dataSet.AddPointField(this->VariableName, outArr);
    }
    else if (arr.IsType<vtkm::cont::ArrayHandle<vtkm::Float64>>())
    {
      auto inArr = arr.AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Float64>>();
      vtkm::cont::ArrayHandle<vtkm::Float64> outArr;
      outArr.Allocate(numTotalPlanes * this->NumberOfPointsPerPlane);
      invoke(planeInserter, inArr, outArr);

      dataSet.AddPointField(this->VariableName, outArr);
    }
    else
    {
      throw std::runtime_error("Unsupported type for GTC field.");
    }
  }
}

}
}
