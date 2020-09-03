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
#include <fides/xgc/ArrayHandleXGCCoords.h>
#include <fides/xgc/ArrayHandleXGCField.h>

#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>

namespace fides
{
namespace datamodel
{

void ArrayPlaceholder::ProcessJSON(const rapidjson::Value& json,
                 DataSourcesType&)
{
  if (!json.HasMember("array_type") || !json["array_type"].IsString())
  {
    throw std::runtime_error(
        this->ObjectName + " must provide a valid array_type.");
  }
  this->ArrayType = json["array_type"].GetString();

  if (!json.HasMember("data_source") || !json["data_source"].IsString())
  {
    throw std::runtime_error(
        this->ObjectName + " must provide a valid data_source.");
  }
  this->DataSourceName = json["data_source"].GetString();
}

std::vector<vtkm::cont::VariantArrayHandle> ArrayPlaceholder::Read(
  const std::unordered_map<std::string, std::string>&,
  DataSourcesType&,
  const fides::metadata::MetaData&)
{
  throw std::runtime_error("ArrayPlaceholder::Read should not be called");
}

size_t ArrayPlaceholder::GetNumberOfBlocks(
  const std::unordered_map<std::string, std::string>&,
  DataSourcesType&)
{
  throw std::runtime_error("ArrayPlaceholder::GetNumberOfBlocks should not be called");
}

std::vector<vtkm::cont::VariantArrayHandle> Array::Read(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources,
  const fides::metadata::MetaData& selections)
{
  return this->ArrayImpl->Read(paths, sources, selections);
}

size_t Array::GetNumberOfBlocks(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources)
{
  return this->ArrayImpl->GetNumberOfBlocks(paths, sources);
}

void Array::ProcessJSON(const rapidjson::Value& json,
                        DataSourcesType& sources)
{
  if (!json.HasMember("array_type") || !json["array_type"].IsString())
  {
    throw std::runtime_error(
      this->ObjectName  + " must provide a valid array_type.");
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
  else if (arrayType == "xgc_coordinates")
  {
    this->ArrayImpl.reset(new ArrayXGCCoordinates());
  }
  else if (arrayType == "xgc_field")
  {
    this->ArrayImpl.reset(new ArrayXGCField());
  }
  else
  {
    throw std::runtime_error(arrayType + " is not a valid array type.");
  }
  this->ArrayImpl->ProcessJSON(json, sources);
}

void Array::CreatePlaceholder(const rapidjson::Value& json,
    DataSourcesType& sources)
{
  if (this->ArrayImpl)
  {
    throw std::runtime_error("ArrayPlaceholders should not have set ArrayImpl");
  }
  this->Placeholder.reset(new ArrayPlaceholder());
  this->Placeholder->ProcessJSON(json, sources);
}

void ArrayBasic::ProcessJSON(const rapidjson::Value& json,
                             DataSourcesType& sources)
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
      throw std::runtime_error(
        "Unrecognized value for is_vector: " + isVector);
    }
  }
}

std::vector<vtkm::cont::VariantArrayHandle> ArrayBasic::Read(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources,
  const fides::metadata::MetaData& selections)
{
  return this->ReadSelf(paths, sources, selections, this->IsVector);
}

size_t ArrayBasic::GetNumberOfBlocks(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources)
{
  auto itr = paths.find(this->DataSourceName);
  if (itr == paths.end())
  {
    throw std::runtime_error("Could not find data_source with name "
      + this->DataSourceName + " among the input paths.");
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
    throw std::runtime_error(
      this->ObjectName  + " must provide a dimensions object.");
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
}

std::vector<vtkm::cont::VariantArrayHandle>
  ArrayUniformPointCoordinates::Read(
    const std::unordered_map<std::string, std::string>& paths,
    DataSourcesType& sources,
    const fides::metadata::MetaData& selections)
{
  std::vector<vtkm::cont::VariantArrayHandle> dims =
    this->Dimensions->Read(paths, sources, selections);
  std::vector<vtkm::cont::VariantArrayHandle> origins;
  if (this->Origin)
  {
    origins = this->Origin->Read(paths, sources, selections);
  }
  std::vector<vtkm::cont::VariantArrayHandle> spacings;
  if (this->Spacing)
  {
    spacings = this->Spacing->Read(paths, sources, selections);
  }
  std::vector<vtkm::cont::VariantArrayHandle> ret;
  size_t nDims = dims.size();
  ret.reserve(nDims);
  for(const auto& array : dims)
  {
    // const auto& array = dims[i];
    auto dimsB = array.Cast<vtkm::cont::ArrayHandle<size_t> >();
    auto dimsPortal = dimsB.ReadPortal();
    vtkm::Id3 dimValues(static_cast<vtkm::Id>(dimsPortal.Get(0)),
                        static_cast<vtkm::Id>(dimsPortal.Get(1)),
                        static_cast<vtkm::Id>(dimsPortal.Get(2)));
    vtkm::Vec<vtkm::FloatDefault, 3> originA;
    originA = vtkm::Vec<vtkm::FloatDefault, 3>(0.0, 0.0, 0.0);
    vtkm::Vec<vtkm::FloatDefault, 3> spacingA;
    spacingA = vtkm::Vec<vtkm::FloatDefault, 3>(1.0, 1.0, 1.0);
    if (this->Origin)
    {
      const auto& origin = origins[0];
      auto originB = origin.Cast<vtkm::cont::ArrayHandle<double> >();
      auto originPortal = originB.ReadPortal();
      originA = vtkm::Vec<vtkm::FloatDefault, 3>(originPortal.Get(0),
                                                 originPortal.Get(1),
                                                 originPortal.Get(2));
    }
    if (this->Spacing)
    {
      const auto& spacing = spacings[0];
      auto spacingB = spacing.Cast<vtkm::cont::ArrayHandle<double> >();
      auto spacingPortal = spacingB.ReadPortal();
      spacingA = vtkm::Vec<vtkm::FloatDefault, 3>(spacingPortal.Get(0),
                                                  spacingPortal.Get(1),
                                                  spacingPortal.Get(2));
    }
    // Shift origin to a local value. We have to do this because
    // VTK-m works with dimensions rather than extents and therefore
    // needs local origin.
    for(vtkm::IdComponent i=0; i<3; i++)
    {
      originA[i] = originA[i] + spacingA[i] * dimsPortal.Get(i+3);
    }
    vtkm::cont::ArrayHandleUniformPointCoordinates ah(
      dimValues, originA, spacingA);
    ret.push_back(ah);
  }
  return ret;
}

size_t ArrayUniformPointCoordinates::GetNumberOfBlocks(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources)
{
  return this->Dimensions->GetNumberOfBlocks(paths, sources);
}

void ArrayCartesianProduct::ProcessJSON(const rapidjson::Value& json,
                                        DataSourcesType& sources)
{
  if (!json.HasMember("x_array") || !json["x_array"].IsObject())
  {
    throw std::runtime_error(
      this->ObjectName  + " must provide a x_array object.");
  }
  this->XArray.reset(new Array());
  const auto& xarray = json["x_array"];
  this->XArray->ProcessJSON(xarray, sources);

  if (!json.HasMember("y_array") || !json["y_array"].IsObject())
  {
    throw std::runtime_error(
      this->ObjectName  + " must provide a y_array object.");
  }
  this->YArray.reset(new Array());
  const auto& yarray = json["y_array"];
  this->YArray->ProcessJSON(yarray, sources);

  if (!json.HasMember("z_array") || !json["z_array"].IsObject())
  {
    throw std::runtime_error(
      this->ObjectName  + " must provide a z_array object.");
  }
  this->ZArray.reset(new Array());
  const auto& zarray = json["z_array"];
  this->ZArray->ProcessJSON(zarray, sources);

}

std::vector<vtkm::cont::VariantArrayHandle>
  ArrayCartesianProduct::Read(
    const std::unordered_map<std::string, std::string>& paths,
    DataSourcesType& sources,
    const fides::metadata::MetaData& selections)
{
  std::vector<vtkm::cont::VariantArrayHandle> retVal;
  std::vector<vtkm::cont::VariantArrayHandle> xarrays =
    this->XArray->Read(paths, sources, selections);
  std::vector<vtkm::cont::VariantArrayHandle> yarrays =
    this->YArray->Read(paths, sources, selections);
  std::vector<vtkm::cont::VariantArrayHandle> zarrays =
    this->ZArray->Read(paths, sources, selections);
  size_t nArrays = xarrays.size();
  for(size_t i=0; i<nArrays; i++)
  {
    auto& xarray = xarrays[i];
    auto& yarray = yarrays[i];
    auto& zarray = zarrays[i];
    using floatType = vtkm::cont::ArrayHandle<float>;
    using doubleType = vtkm::cont::ArrayHandle<double>;
    if (xarray.IsType<floatType>() &&
        yarray.IsType<floatType>() &&
        zarray.IsType<floatType>())
    {
      auto xarrayF = xarray.Cast<floatType>();
      auto yarrayF = yarray.Cast<floatType>();
      auto zarrayF = zarray.Cast<floatType>();
      retVal.push_back(
        vtkm::cont::make_ArrayHandleCartesianProduct(
          xarrayF, yarrayF, zarrayF));
    }
    else if (xarray.IsType<doubleType>() &&
             yarray.IsType<doubleType>() &&
             zarray.IsType<doubleType>())
    {
      auto xarrayD = xarray.Cast<doubleType>();
      auto yarrayD = yarray.Cast<doubleType>();
      auto zarrayD = zarray.Cast<doubleType>();
      retVal.push_back(
        vtkm::cont::make_ArrayHandleCartesianProduct(
          xarrayD, yarrayD, zarrayD));
    }
    else
    {
      throw std::runtime_error(
        "Only float and double arrays are supported in cartesian products.");
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
  : CommonImpl(new XGCCommon()) {}

size_t ArrayXGC::GetNumberOfBlocks(
    const std::unordered_map<std::string, std::string>& paths,
    DataSourcesType &sources)
{
  if (this->NumberOfPlanes < 0)
  {
    this->NumberOfPlanes =
      this->CommonImpl->GetNumberOfPlanes(paths, sources);
  }
  return this->CommonImpl->GetNumberOfBlocks();
}

void ArrayXGC::CheckEngineType(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources, std::string& dataSourceName)
{
  auto itr = paths.find(dataSourceName);
  if (itr == paths.end())
  {
    throw std::runtime_error("Could not find data_source with name " +
      dataSourceName + " among the input paths.");
  }
  const auto &ds = sources[dataSourceName];

  if (ds->GetEngineType() == fides::io::EngineType::Inline)
  {
    throw std::runtime_error("Inline engine not supported for XGC."
      "Must use BP files and/or SST.");
  }
  this->EngineChecked = true;
}

std::vector<size_t> ArrayXGC::GetShape(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources)
{
  auto itr = paths.find(this->DataSourceName);
  if (itr == paths.end())
  {
    throw std::runtime_error("Could not find data_source with name " +
      this->DataSourceName + " among the input paths.");
  }
  const auto &ds = sources[this->DataSourceName];
  std::string path = itr->second + ds->FileName;
  ds->OpenSource(path);
  return ds->GetVariableShape(this->VariableName);
}

/// Functor created so that VariantArrayHandle's CastAndCall() will handle making the
/// appropriate cast to an ArrayHandle.
/// The specialization is because it tries to use other StorageTags when compiling,
/// but we only want to support StorageTagBasic.
struct ArrayXGCCoordinates::AddToVectorFunctor
{
  template<typename T, typename S>
  VTKM_CONT void operator()(const vtkm::cont::ArrayHandle<T, S>&,
    std::vector<vtkm::cont::VariantArrayHandle>&,
    vtkm::Id, vtkm::Id, vtkm::Id, bool) const {}

  /// This version is for creating the coordinates AHs.
  template<typename T>
  VTKM_CONT void operator()(const vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagBasic>& array,
    std::vector<vtkm::cont::VariantArrayHandle>& retVal,
    vtkm::Id numberOfPlanes, vtkm::Id numberOfPlanesOwned,
    vtkm::Id planeStartId, bool isCylindrical) const
  {
    retVal.push_back(
      vtkm::cont::make_ArrayHandleXGCCoords(
        array, numberOfPlanes, numberOfPlanesOwned,
        planeStartId, isCylindrical));
  }
};

void ArrayXGCCoordinates::ProcessJSON(const rapidjson::Value& json,
  DataSourcesType& sources)
{
  this->ArrayXGC::ProcessJSON(json, sources);
  if (!json.HasMember("is_cylindrical") || !json["is_cylindrical"].IsBool())
  {
    throw std::runtime_error(
      this->ObjectName + " must provide a coordinates_type.");
  }
  this->IsCylindrical = json["is_cylindrical"].GetBool();
}

std::vector<vtkm::cont::VariantArrayHandle> ArrayXGCCoordinates::Read(
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
    this->NumberOfPlanes =
      this->CommonImpl->GetNumberOfPlanes(paths, sources);
  }

  fides::metadata::MetaData newSelections = selections;
  // Removing because for XGC Fides blocks are not the same as ADIOS blocks
  newSelections.Remove(fides::keys::BLOCK_SELECTION());

  std::vector<vtkm::cont::VariantArrayHandle> retVal;

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
    coordsAH.ResetTypes(vtkm::TypeListScalarAll()).CastAndCall(
        AddToVectorFunctor(), retVal, NumberOfPlanes, block.NumberOfPlanesOwned,
        block.PlaneStartId, this->IsCylindrical);
  }
  return retVal;
}

/// Functor created so that VariantArrayHandle's CastAndCall() will handle making the
/// appropriate cast to an ArrayHandle.
/// The specialization is because it tries to use other StorageTags when compiling,
/// but we only want to support StorageTagBasic.
struct ArrayXGCField::AddToVectorFunctor
{
  template<typename T, typename S>
  VTKM_CONT void operator()(const vtkm::cont::ArrayHandle<T, S>&,
    std::vector<vtkm::cont::VariantArrayHandle>&,
    std::vector<vtkm::cont::VariantArrayHandle>&,
    vtkm::Id,
    bool) const {}

  /// This version is for creating the field AHs
  template<typename T>
  VTKM_CONT void operator()(const vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagBasic>& array,
    std::vector<vtkm::cont::VariantArrayHandle>& allPlanes,
    std::vector<vtkm::cont::VariantArrayHandle>& retVal,
    vtkm::Id numberOfPlanesOwned,
    bool is2dField) const
  {
    std::vector<vtkm::cont::ArrayHandle<T> > planesCasted;
    planesCasted.push_back(array);
    for (size_t i = 1; i < allPlanes.size(); i++)
    {
      planesCasted.push_back(allPlanes[i].Cast<vtkm::cont::ArrayHandle<T> >());
    }

    retVal.push_back(
      vtkm::cont::make_ArrayHandleXGCField(
        planesCasted, numberOfPlanesOwned, is2dField));
  }
};

std::unordered_map<size_t, vtkm::cont::VariantArrayHandle> ArrayXGCField::Read3DVariable(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources,
  const fides::metadata::MetaData& selections)
{
  auto itr = paths.find(this->DataSourceName);
  if (itr == paths.end())
  {
    throw std::runtime_error("Could not find data_source with name "
      + this->DataSourceName + " among the input paths.");
  }
  const auto& ds = sources[this->DataSourceName];
  std::string path = itr->second + ds->FileName;
  ds->OpenSource(path);
  return ds->ReadXGC3DVariable(this->VariableName, selections);
}

std::vector<vtkm::cont::VariantArrayHandle> ArrayXGCField::Read(
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
    this->NumberOfPlanes =
      this->CommonImpl->GetNumberOfPlanes(paths, sources);
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

  std::vector<vtkm::cont::VariantArrayHandle> retVal;

  metadata::MetaData newSelections = selections;
  // Removing because for XGC Fides blocks are not the same as ADIOS blocks
  newSelections.Remove(fides::keys::BLOCK_SELECTION());
  std::pair<std::vector<XGCBlockInfo>, fides::metadata::Set<size_t> > info;
  if (selections.Has(fides::keys::BLOCK_SELECTION()))
  {
    info = this->CommonImpl->GetXGCBlockInfoWithPlaneSelection(
        selections.Get<fides::metadata::Vector<size_t> >(fides::keys::BLOCK_SELECTION()).Data);
  }
  else
  {
    info = this->CommonImpl->GetXGCBlockInfoWithPlaneSelection(std::vector<size_t>());
  }
  auto& blocksInfo = info.first;
  auto& planesToRead = info.second;
  if (blocksInfo.empty())
  {
    throw std::runtime_error("No XGC block info returned. May want to double check block selection.");
  }

  if (this->Is2DField)
  {
    auto fieldData = this->ReadSelf(paths, sources, newSelections, fides::io::IsVector::No);
    assert(fieldData.size() == 1);
    auto& fieldAH = fieldData[0];
    for (size_t i = 0; i < blocksInfo.size(); ++i)
    {
      const auto& block = blocksInfo[i];
      fieldAH.ResetTypes(vtkm::TypeListScalarAll()).CastAndCall(
        AddToVectorFunctor(), fieldData, retVal, block.NumberOfPlanesOwned, this->Is2DField);
    }
  }
  else
  {
    newSelections.Set(fides::keys::PLANE_SELECTION(), planesToRead);
    // read all planes (if its in a requested blocks) once only
    auto planeData = this->Read3DVariable(paths, sources, newSelections);
    for (const auto& block : blocksInfo)
    {
      std::vector<vtkm::cont::VariantArrayHandle> planes;
      for (vtkm::Id i = block.PlaneStartId; i < block.PlaneStartId + block.NumberOfPlanesOwned; ++i)
      {
        vtkm::Id planeId = i;
        if (planeId == this->NumberOfPlanes)
        {
          // to handle last plane on n-1 block
          planeId = 0;
        }
        auto& plane = planeData[static_cast<size_t>(planeId)];
        planes.push_back(plane);
      }
      auto& fieldAH = planes[0];
      fieldAH.ResetTypes(vtkm::TypeListScalarAll()).CastAndCall(
        AddToVectorFunctor(), planes, retVal, block.NumberOfPlanesOwned, this->Is2DField);
    }
  }

  return retVal;
}

}
}
