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
#include <viskores/Math.h>
#include <viskores/cont/ArrayHandleCounting.h>
#include <viskores/cont/ArrayHandleSOA.h>
#include <viskores/cont/ArrayHandleUniformPointCoordinates.h>
#include <viskores/cont/ArrayHandleView.h>
#include <viskores/cont/ArrayHandleXGCCoordinates.h>

#include <viskores/Version.h>
#include <viskores/cont/Invoker.h>
#include <viskores/worklet/WorkletMapField.h>

namespace fides
{
namespace datamodel
{

VISKORES_EXEC static void Index1d_3d(const viskores::Id& idx,
                                     const viskores::Id& fidesNotUsed(nx),
                                     const viskores::Id& ny,
                                     const viskores::Id& nz,
                                     viskores::Id& i,
                                     viskores::Id& j,
                                     viskores::Id& k)
{
  i = idx / (ny * nz);
  j = (idx / nz) % ny;
  k = idx % nz;
}

namespace fusionutil
{

class CalcCosSin : public viskores::worklet::WorkletMapField
{
public:
  CalcCosSin(const viskores::Id numZeta,
             const viskores::Id numTheta,
             const viskores::Id& numAmplitudes)
    : NumTheta(numTheta)
    , NumZeta(numZeta)
    , NumAmplitudes(numAmplitudes)
  {
  }

  using ControlSignature = void(WholeArrayIn taxField,
                                WholeArrayIn zaxField,
                                WholeArrayIn xmField,
                                WholeArrayIn xnField,
                                FieldOut cosVal,
                                FieldOut sinVal,
                                FieldOut xVal);
  using ExecutionSignature = void(InputIndex,
                                  _1 zaxField,
                                  _2 taxField,
                                  _3 xmField,
                                  _4 xnField,
                                  _5 cosVal,
                                  _6 sinVal,
                                  _7 xVal);
  using InputDomain = _5;

  template <typename ZTArrayType, typename XMNArrayType, typename OutputArrayType>
  VISKORES_EXEC void operator()(const viskores::Id& idx,
                                const ZTArrayType& zaxField,
                                const ZTArrayType& taxField,
                                const XMNArrayType& xmField,
                                const XMNArrayType& xnField,
                                OutputArrayType& cosVal,
                                OutputArrayType& sinVal,
                                OutputArrayType& xVal) const
  {
    viskores::Id index = idx * this->NumAmplitudes;
    viskores::Id zi, ti, xmi;
    Index1d_3d(index, this->NumZeta, this->NumTheta, this->NumAmplitudes, zi, ti, xmi);

    const auto& zeta = zaxField.Get(zi);
    const auto& theta = taxField.Get(ti);
    for (viskores::IdComponent i = 0; i < this->NumAmplitudes; i++)
    {
      auto xm = xmField.Get(i);
      auto xn = xnField.Get(i);
      auto xx = xm * theta - xn * zeta;
      xVal[i] = xx;
      cosVal[i] = viskores::Cos(xx);
      sinVal[i] = viskores::Sin(xx);
    }
  }

private:
  viskores::Id NumTheta = 0;
  viskores::Id NumZeta = 0;
  viskores::Id NumAmplitudes = 0;
};

class CalcRZL : public viskores::worklet::WorkletMapField
{
public:
  CalcRZL(const viskores::Id& numAmplitudes, const viskores::Id& srfIndex)
    : NumAmplitudes(numAmplitudes)
    , SurfaceIndex(srfIndex)
  {
  }

  using ControlSignature = void(FieldOut RZL,
                                WholeArrayIn rmnc,
                                WholeArrayIn zmns,
                                WholeArrayIn lmns,
                                FieldIn cosValues,
                                FieldIn sinValues);
  using ExecutionSignature = void(_1 RZL, _2 rmnc, _3 zmns, _4 lmns, _5 cosValues, _6 sinValues);
  using InputDomain = _1;

  template <typename OutputArrayType, typename InputArrayType, typename InputArrayType2>
  VISKORES_EXEC void operator()(OutputArrayType& RZL,
                                const InputArrayType& rmnc,
                                const InputArrayType& zmns,
                                const InputArrayType& lmns,
                                const InputArrayType2& cosValues,
                                const InputArrayType2& sinValues) const
  {
    RZL[0] = 0;
    RZL[1] = 0;
    RZL[2] = 0;
    for (viskores::Id i = 0; i < this->NumAmplitudes; i++)
    {
      RZL[0] += rmnc.Get(this->SurfaceIndex)[i] * cosValues[i][this->SurfaceIndex];
      RZL[1] += zmns.Get(this->SurfaceIndex)[i] * sinValues[i][this->SurfaceIndex];
      RZL[2] += lmns.Get(this->SurfaceIndex)[i] * sinValues[i][this->SurfaceIndex];
    }
  }

private:
  viskores::Id NumAmplitudes = 0;
  viskores::Id SurfaceIndex = 0;
};

class CalcNFP : public viskores::worklet::WorkletMapField
{
public:
  CalcNFP(const viskores::Id& numNFP, const viskores::Id& numZeta, const viskores::Id& numTheta)
    : NumNFP(numNFP)
    , NumZeta(numZeta)
    , NumTheta(numTheta)
  {
  }

  using ControlSignature =
    void(WholeArrayIn RZL, WholeArrayIn Zn, WholeArrayIn Zeta, FieldOut Phi_n, FieldOut RZL_n);
  using ExecutionSignature = void(InputIndex, _1 RZL, _2 Zn, _3 Zeta, _4 Phi_n, _5 RZL_n);
  using InputDomain = _5;

  template <typename RZLArrayType,
            typename ZetaArrayType,
            typename PhiOutputType,
            typename RZLOutputType>
  VISKORES_EXEC void operator()(const viskores::Id& index,
                                const RZLArrayType& RZL,
                                const ZetaArrayType& Zn,
                                const ZetaArrayType& Zeta,
                                PhiOutputType& Phi_n,
                                RZLOutputType& RZL_n) const
  {
    viskores::Id nfp_i, zi, ti;
    Index1d_3d(index, this->NumNFP, this->NumZeta, this->NumTheta, nfp_i, zi, ti);
    viskores::Id idx0 = zi * this->NumTheta + ti;

    auto z = Zn.Get(nfp_i);
    Phi_n = Zeta.Get(zi) + z;
    RZL_n = RZL.Get(idx0);
  }

private:
  viskores::Id NumNFP = 0;
  viskores::Id NumZeta = 0;
  viskores::Id NumTheta = 0;
};

class ConvertRZPhiToXYZ : public viskores::worklet::WorkletMapField
{
public:
  ConvertRZPhiToXYZ(const viskores::Id& numNFP,
                    const viskores::Id& numZeta,
                    const viskores::Id& numTheta)
    : NumNFP(numNFP)
    , NumZeta(numZeta)
    , NumTheta(numTheta)
  {
  }
  using ControlSignature = void(FieldOut XYZ, FieldOut Lambda, WholeArrayIn Phi_n, FieldIn RZL);
  using ExecutionSignature = void(InputIndex, _1, _2, _3, _4);
  using InputDomain = _1;

  template <typename XYZType, typename LambdaType, typename PhiType, typename RZLType>
  VISKORES_EXEC void operator()(const viskores::Id& index,
                                XYZType& xyz,
                                LambdaType& lambda,
                                const PhiType& Phi_n,
                                const RZLType& rzl) const
  {
    //Phi_n is a of size: (nfp*numZeta, nTheta)
    viskores::Id xmi, zi, ti;
    Index1d_3d(index, this->NumNFP, this->NumZeta, this->NumTheta, xmi, zi, ti);

    // X = R*cos(phi), Y= R*sin(phi)
    xyz[0] = rzl[0] * viskores::Cos(Phi_n.Get(index));
    xyz[1] = rzl[0] * viskores::Sin(Phi_n.Get(index));
    xyz[2] = rzl[1];

    lambda = rzl[2];
  }

private:
  viskores::Id NumNFP = 0;
  viskores::Id NumZeta = 0;
  viskores::Id NumTheta = 0;
};

class PlaneInserterField : public viskores::worklet::WorkletMapField
{
public:
  PlaneInserterField(viskores::Id nPlanes, viskores::Id nPtsPerPlane, viskores::Id numInsert)
    : NumPlanes(nPlanes)
    , PtsPerPlane(nPtsPerPlane)
    , NumInsert(numInsert)
  {
  }

  using ControlSignature = void(WholeArrayIn inField, WholeArrayOut outField);
  using ExecutionSignature = void(InputIndex, _1, _2);
  using InputDomain = _1;

  template <typename InFieldType, typename OutFieldType>
  VISKORES_EXEC void operator()(const viskores::Id& inIdx,
                                const InFieldType& inField,
                                OutFieldType& outField) const
  {
    viskores::Id plane0PtIdx = inIdx;
    viskores::Id inPlaneIdx = plane0PtIdx / this->PtsPerPlane;
    viskores::Id pt0Offset = plane0PtIdx % this->PtsPerPlane;

    // This is correct:
    viskores::Id plane1PtIdx = plane0PtIdx + this->PtsPerPlane;
    // Unless we're in the last plane:
    if (inPlaneIdx == this->NumPlanes - 1)
    {
      plane1PtIdx = plane0PtIdx % this->PtsPerPlane;
    }

    viskores::Id firstOutputPlaneIndex = inPlaneIdx * (1 + NumInsert);

    const auto& y0 = inField.Get(plane0PtIdx);
    const auto& y1 = inField.Get(plane1PtIdx);
    outField.Set(firstOutputPlaneIndex * PtsPerPlane + pt0Offset, y0);

    viskores::Id numOutCoords = outField.GetNumberOfValues();

    for (viskores::Id i = 0; i < this->NumInsert; i++)
    {
      viskores::Id outIdx = (firstOutputPlaneIndex + i + 1) * this->PtsPerPlane + pt0Offset;
      if (outIdx > numOutCoords)
      {
#if defined(VISKORES_ENABLE_CUDA) || defined(VISKORES_ENABLE_KOKKOS)
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
  viskores::Id NumPlanes;
  viskores::Id PtsPerPlane;
  viskores::Id NumInsert;
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

std::vector<viskores::cont::UnknownArrayHandle> ArrayPlaceholder::Read(
  const std::unordered_map<std::string, std::string>&,
  DataSourcesType&,
  const fides::metadata::MetaData&)
{
  throw std::runtime_error("ArrayPlaceholder::Read should not be called");
}

size_t ArrayPlaceholder::GetNumberOfBlocks(const std::unordered_map<std::string, std::string>&,
                                           DataSourcesType&,
                                           const std::string&)
{
  throw std::runtime_error("ArrayPlaceholder::GetNumberOfBlocks should not be called");
}

std::set<std::string> ArrayPlaceholder::GetGroupNames(
  const std::unordered_map<std::string, std::string>&,
  DataSourcesType&)
{
  throw std::runtime_error("ArrayPlaceholder::GetGroupNames should not be called");
}

std::vector<viskores::cont::UnknownArrayHandle> Array::Read(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources,
  const fides::metadata::MetaData& selections)
{
  return this->ArrayImpl->Read(paths, sources, selections);
}

void Array::PostRead(std::vector<viskores::cont::DataSet>& partitions,
                     const fides::metadata::MetaData& selections)
{
  this->ArrayImpl->PostRead(partitions, selections);
}

size_t Array::GetNumberOfBlocks(const std::unordered_map<std::string, std::string>& paths,
                                DataSourcesType& sources,
                                const std::string& groupName /*=""*/)
{
  return this->ArrayImpl->GetNumberOfBlocks(paths, sources, groupName);
}

std::set<std::string> Array::GetGroupNames(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources)
{
  return this->ArrayImpl->GetGroupNames(paths, sources);
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
  else if (arrayType == "composite")
  {
    this->ArrayImpl.reset(new ArrayComposite());
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
  else if (arrayType == "gx_coordinates")
  {
    this->ArrayImpl.reset(new ArrayGXCoordinates());
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

std::vector<viskores::cont::UnknownArrayHandle> ArrayBasic::Read(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources,
  const fides::metadata::MetaData& selections)
{
  return this->ReadSelf(paths, sources, selections, this->IsVector);
}

size_t ArrayBasic::GetNumberOfBlocks(const std::unordered_map<std::string, std::string>& paths,
                                     DataSourcesType& sources,
                                     const std::string& groupName /*=""*/)
{
  const auto& ds = sources[this->DataSourceName];
  ds->OpenSource(paths, this->DataSourceName);
  return ds->GetNumberOfBlocks(this->VariableName, groupName);
}

std::set<std::string> ArrayBasic::GetGroupNames(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources)
{
  const auto& ds = sources[this->DataSourceName];
  ds->OpenSource(paths, this->DataSourceName);
  return ds->GetGroupNames(this->VariableName);
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

std::vector<viskores::cont::UnknownArrayHandle> ArrayUniformPointCoordinates::Read(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources,
  const fides::metadata::MetaData& selections)
{
  std::vector<viskores::cont::UnknownArrayHandle> ret;

  if (this->DefinedFromVariableShape)
  {
    // In this situation we can do everything now instead of waiting for
    // the PostRead
    std::vector<viskores::cont::UnknownArrayHandle> dims =
      this->Dimensions->Read(paths, sources, selections);
    std::vector<viskores::cont::UnknownArrayHandle> origins;
    if (this->Origin)
    {
      origins = this->Origin->Read(paths, sources, selections);
    }
    std::vector<viskores::cont::UnknownArrayHandle> spacings;
    if (this->Spacing)
    {
      spacings = this->Spacing->Read(paths, sources, selections);
    }

    size_t nDims = dims.size();
    ret.reserve(nDims);

    for (const auto& array : dims)
    {
      auto dimsB = array.AsArrayHandle<viskores::cont::ArrayHandle<size_t>>();
      auto dimsPortal = dimsB.ReadPortal();
      viskores::Id3 dimValues(static_cast<viskores::Id>(dimsPortal.Get(0)),
                              static_cast<viskores::Id>(dimsPortal.Get(1)),
                              static_cast<viskores::Id>(dimsPortal.Get(2)));
      viskores::Vec3f originA(0.0, 0.0, 0.0);
      viskores::Vec3f spacingA(1.0, 1.0, 1.0);
      if (this->Origin)
      {
        const auto& origin = origins[0];
        auto originB = origin.AsArrayHandle<viskores::cont::ArrayHandle<double>>();
        auto originPortal = originB.ReadPortal();
        originA = viskores::Vec3f(originPortal.Get(0), originPortal.Get(1), originPortal.Get(2));
      }
      if (this->Spacing)
      {
        const auto& spacing = spacings[0];
        auto spacingB = spacing.AsArrayHandle<viskores::cont::ArrayHandle<double>>();
        auto spacingPortal = spacingB.ReadPortal();
        spacingA =
          viskores::Vec3f(spacingPortal.Get(0), spacingPortal.Get(1), spacingPortal.Get(2));
      }
      // Shift origin to a local value. We have to do this because
      // Viskores works with dimensions rather than extents and therefore
      // needs local origin.
      for (viskores::IdComponent i = 0; i < 3; i++)
      {
        originA[i] = originA[i] + spacingA[i] * dimsPortal.Get(i + 3);
      }
      viskores::cont::ArrayHandleUniformPointCoordinates ah(dimValues, originA, spacingA);
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
  std::vector<viskores::cont::DataSet>& partitions,
  const fides::metadata::MetaData& fidesNotUsed(selections))
{
  if (!this->DefinedFromVariableShape)
  {
    size_t nDims = this->DimensionArrays.size();
    for (std::size_t i = 0; i < nDims; i++)
    {
      viskores::cont::UnknownArrayHandle dimUnknown = viskores::cont::ArrayHandle<std::size_t>{};
      dimUnknown.CopyShallowIfPossible(this->DimensionArrays[i]);
      auto d = dimUnknown.AsArrayHandle<viskores::cont::ArrayHandle<std::size_t>>();
      auto o = this->OriginArrays[i].AsArrayHandle<viskores::cont::ArrayHandle<double>>();
      auto s = this->SpacingArrays[i].AsArrayHandle<viskores::cont::ArrayHandle<double>>();
      auto dPortal = d.ReadPortal();
      auto oPortal = o.ReadPortal();
      auto sPortal = s.ReadPortal();

      viskores::Id3 dValues(static_cast<viskores::Id>(dPortal.Get(0)),
                            static_cast<viskores::Id>(dPortal.Get(1)),
                            static_cast<viskores::Id>(dPortal.Get(2)));
      viskores::Vec3f oValues(oPortal.Get(0), oPortal.Get(1), oPortal.Get(2));
      viskores::Vec3f sValues(sPortal.Get(0), sPortal.Get(1), sPortal.Get(2));
      viskores::cont::ArrayHandleUniformPointCoordinates ah(dValues, oValues, sValues);
      viskores::cont::CoordinateSystem coords("coordinates", ah);

      auto& ds = partitions[i];
      ds.AddCoordinateSystem(coords);
    }
  }
}

size_t ArrayUniformPointCoordinates::GetNumberOfBlocks(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources,
  const std::string& groupName /*=""*/)
{
  return this->Dimensions->GetNumberOfBlocks(paths, sources, groupName);
}

std::set<std::string> ArrayUniformPointCoordinates::GetGroupNames(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources)
{
  return this->Dimensions->GetGroupNames(paths, sources);
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

std::vector<viskores::cont::UnknownArrayHandle> ArrayCartesianProduct::Read(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources,
  const fides::metadata::MetaData& selections)
{
  std::vector<viskores::cont::UnknownArrayHandle> retVal;
  std::vector<viskores::cont::UnknownArrayHandle> xarrays =
    this->XArray->Read(paths, sources, selections);
  std::vector<viskores::cont::UnknownArrayHandle> yarrays =
    this->YArray->Read(paths, sources, selections);
  std::vector<viskores::cont::UnknownArrayHandle> zarrays =
    this->ZArray->Read(paths, sources, selections);
  size_t nArrays = xarrays.size();
  for (size_t i = 0; i < nArrays; i++)
  {
    auto& xarray = xarrays[i];
    auto& yarray = yarrays[i];
    auto& zarray = zarrays[i];
    using floatType = viskores::cont::ArrayHandle<float>;
    using doubleType = viskores::cont::ArrayHandle<double>;
    if (xarray.IsType<floatType>() && yarray.IsType<floatType>() && zarray.IsType<floatType>())
    {
      auto xarrayF = xarray.AsArrayHandle<floatType>();
      auto yarrayF = yarray.AsArrayHandle<floatType>();
      auto zarrayF = zarray.AsArrayHandle<floatType>();
      retVal.push_back(viskores::cont::make_ArrayHandleCartesianProduct(xarrayF, yarrayF, zarrayF));
    }
    else if (xarray.IsType<doubleType>() && yarray.IsType<doubleType>() &&
             zarray.IsType<doubleType>())
    {
      auto xarrayD = xarray.AsArrayHandle<doubleType>();
      auto yarrayD = yarray.AsArrayHandle<doubleType>();
      auto zarrayD = zarray.AsArrayHandle<doubleType>();
      retVal.push_back(viskores::cont::make_ArrayHandleCartesianProduct(xarrayD, yarrayD, zarrayD));
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
  DataSourcesType& sources,
  const std::string& groupName /*=""*/)
{
  return this->XArray->GetNumberOfBlocks(paths, sources, groupName);
}

std::set<std::string> ArrayCartesianProduct::GetGroupNames(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources)
{
  return this->XArray->GetGroupNames(paths, sources);
}

std::vector<viskores::cont::UnknownArrayHandle> ArrayComposite::Read(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources,
  const fides::metadata::MetaData& selections)
{
  std::vector<viskores::cont::UnknownArrayHandle> retVal;
  std::vector<viskores::cont::UnknownArrayHandle> xarrays =
    this->XArray->Read(paths, sources, selections);
  std::vector<viskores::cont::UnknownArrayHandle> yarrays =
    this->YArray->Read(paths, sources, selections);
  std::vector<viskores::cont::UnknownArrayHandle> zarrays =
    this->ZArray->Read(paths, sources, selections);
  size_t nArrays = xarrays.size();
  for (size_t i = 0; i < nArrays; i++)
  {
    auto& xarray = xarrays[i];
    auto& yarray = yarrays[i];
    auto& zarray = zarrays[i];
    using floatType = viskores::cont::ArrayHandle<float>;
    using doubleType = viskores::cont::ArrayHandle<double>;
    if (xarray.IsType<floatType>() && yarray.IsType<floatType>() && zarray.IsType<floatType>())
    {
      auto xarrayF = xarray.AsArrayHandle<floatType>();
      auto yarrayF = yarray.AsArrayHandle<floatType>();
      auto zarrayF = zarray.AsArrayHandle<floatType>();
      retVal.push_back(
        viskores::cont::make_ArrayHandleSOA<viskores::Vec3f_32>({ xarrayF, yarrayF, zarrayF }));
    }
    else if (xarray.IsType<doubleType>() && yarray.IsType<doubleType>() &&
             zarray.IsType<doubleType>())
    {
      auto xarrayD = xarray.AsArrayHandle<doubleType>();
      auto yarrayD = yarray.AsArrayHandle<doubleType>();
      auto zarrayD = zarray.AsArrayHandle<doubleType>();
      retVal.push_back(
        viskores::cont::make_ArrayHandleSOA<viskores::Vec3f_64>({ xarrayD, yarrayD, zarrayD }));
    }
    else
    {
      throw std::runtime_error("Only float and double arrays are supported in cartesian products.");
    }
  }
  return retVal;
}

ArrayXGC::ArrayXGC()
  : CommonImpl(new XGCCommon())
{
}

size_t ArrayXGC::GetNumberOfBlocks(const std::unordered_map<std::string, std::string>& paths,
                                   DataSourcesType& sources,
                                   const std::string& /*groupName*/ /*=""*/)
{
  if (this->NumberOfPlanes < 0)
  {
    this->NumberOfPlanes = this->CommonImpl->GetNumberOfPlanes(paths, sources);
  }
  return this->CommonImpl->GetNumberOfBlocks();
}

void ArrayXGC::CheckEngineType(const std::unordered_map<std::string, std::string>&,
                               DataSourcesType& sources,
                               std::string& dataSourceName)
{
  const auto& ds = sources[dataSourceName];

  if (ds->GetEngineType() == fides::io::EngineType::Inline)
  {
    throw std::runtime_error("Inline engine not supported for XGC."
                             "Must use BP files and/or SST.");
  }
  this->EngineChecked = true;
}

std::vector<size_t> ArrayXGC::GetShape(const std::unordered_map<std::string, std::string>& paths,
                                       DataSourcesType& sources,
                                       const std::string& groupName /*=""*/)
{
  const auto& ds = sources[this->DataSourceName];
  ds->OpenSource(paths, this->DataSourceName);
  return ds->GetVariableShape(this->VariableName, groupName);
}

/// Functor created so that UnknownArrayHandle's CastAndCall() will handle making the
/// appropriate cast to an ArrayHandle.
/// The specialization is because it tries to use other StorageTags when compiling,
/// but we only want to support StorageTagBasic.
struct ArrayXGCCoordinates::AddToVectorFunctor
{
  template <typename T, typename S>
  VISKORES_CONT void operator()(const viskores::cont::ArrayHandle<T, S>&,
                                std::vector<viskores::cont::UnknownArrayHandle>&,
                                viskores::Id,
                                viskores::Id,
                                viskores::Id,
                                bool) const
  {
  }

  /// This version is for creating the coordinates AHs.
  template <typename T>
  VISKORES_CONT void operator()(
    const viskores::cont::ArrayHandle<T, viskores::cont::StorageTagBasic>& array,
    std::vector<viskores::cont::UnknownArrayHandle>& retVal,
    viskores::Id numberOfPlanes,
    viskores::Id numberOfPlanesOwned,
    viskores::Id planeStartId,
    bool isCylindrical) const
  {
    retVal.push_back(viskores::cont::make_ArrayHandleXGCCoordinates(
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

std::vector<viskores::cont::UnknownArrayHandle> ArrayXGCCoordinates::Read(
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

  std::vector<viskores::cont::UnknownArrayHandle> retVal;

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
    coordsAH.CastAndCallForTypes<viskores::TypeListFieldScalar,
                                 viskores::List<viskores::cont::StorageTagBasic>>(
      AddToVectorFunctor(),
      retVal,
      NumberOfPlanes * (1 + numInsertPlanes),
      block.NumberOfPlanesOwned * (1 + numInsertPlanes),
      block.PlaneStartId * (1 + numInsertPlanes),
      this->IsCylindrical);
  }
  return retVal;
}

viskores::cont::UnknownArrayHandle ArrayXGCField::Read3DVariable(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources,
  const fides::metadata::MetaData& selections)
{
  const auto& ds = sources[this->DataSourceName];
  ds->OpenSource(paths, this->DataSourceName);
  auto arrays = ds->ReadMultiBlockVariable(this->VariableName, selections);
  if (arrays.size() != 1)
  {
    throw std::runtime_error("3d field should be read into a single ArrayHandle");
  }
  return arrays[0];
}

std::vector<viskores::cont::UnknownArrayHandle> ArrayXGCField::Read(
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
    std::string groupName;
    if (selections.Has(fides::keys::GROUP_SELECTION()))
    {
      groupName = selections.Get<fides::metadata::String>(fides::keys::GROUP_SELECTION()).Data;
    }
    auto shape = this->GetShape(paths, sources, groupName);
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

  std::vector<viskores::cont::UnknownArrayHandle> retVal;

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

      for (viskores::Id i = block.PlaneStartId; i < block.PlaneStartId + block.NumberOfPlanesOwned;
           ++i)
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

void ArrayXGCField::PostRead(std::vector<viskores::cont::DataSet>& dataSets,
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
  if (!cs.IsType<viskores::cont::CellSetExtrude>())
    throw std::runtime_error("Wrong type of cell set for XGC dataset.");

  auto cellSet = cs.AsCellSet<viskores::cont::CellSetExtrude>();
  viskores::Id ptsPerPlane = cellSet.GetNumberOfPointsPerPlane();
  viskores::Id numPlanes = this->NumberOfPlanes;
  viskores::Id totNumPlanes = numPlanes + (numPlanes * numInsertPlanes);

  if (!dataSets[0].HasPointField(this->VariableName))
  {
    return;
  }

  auto fieldArray = dataSets[0].GetField(this->VariableName).GetData();

  viskores::cont::Invoker invoke;

  using floatType = viskores::cont::ArrayHandle<float>;
  using doubleType = viskores::cont::ArrayHandle<double>;
  fusionutil::PlaneInserterField planeInserter(numPlanes, ptsPerPlane, numInsertPlanes);
  if (fieldArray.IsType<floatType>())
  {
    auto inArr = fieldArray.AsArrayHandle<floatType>();
    viskores::cont::ArrayHandle<float> outArr;
    outArr.Allocate(totNumPlanes * ptsPerPlane);
    invoke(planeInserter, inArr, outArr);
    dataSets[0].AddPointField(this->VariableName, outArr);
  }
  else if (fieldArray.IsType<doubleType>())
  {
    auto inArr = fieldArray.AsArrayHandle<doubleType>();
    viskores::cont::ArrayHandle<double> outArr;
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

std::vector<viskores::cont::UnknownArrayHandle> ArrayGTCCoordinates::Read(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources,
  const fides::metadata::MetaData& selections)
{
  std::vector<viskores::cont::UnknownArrayHandle> retVal;

  //First time, so read and set cache.
  if (!this->IsCached)
  {
    auto newSelections = selections;
    fides::metadata::Bool flag(true);
    newSelections.Set(fides::keys::READ_AS_MULTIBLOCK(), flag);
    // Removing because for XGC Fides blocks are not the same as ADIOS blocks
    newSelections.Remove(fides::keys::BLOCK_SELECTION());

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

    using floatType = viskores::cont::ArrayHandle<float>;
    using doubleType = viskores::cont::ArrayHandle<double>;
    if (xarray.IsType<floatType>() && yarray.IsType<floatType>() && zarray.IsType<floatType>())
    {
      auto xarrayF = xarray.AsArrayHandle<floatType>();
      auto yarrayF = yarray.AsArrayHandle<floatType>();
      auto zarrayF = zarray.AsArrayHandle<floatType>();
      this->CachedCoords =
        viskores::cont::make_ArrayHandleSOA<viskores::Vec3f_32>({ xarrayF, yarrayF, zarrayF });
    }
    else if (xarray.IsType<doubleType>() && yarray.IsType<doubleType>() &&
             zarray.IsType<doubleType>())
    {
      auto xarrayD = xarray.AsArrayHandle<doubleType>();
      auto yarrayD = yarray.AsArrayHandle<doubleType>();
      auto zarrayD = zarray.AsArrayHandle<doubleType>();
      this->CachedCoords =
        viskores::cont::make_ArrayHandleSOA<viskores::Vec3f_64>({ xarrayD, yarrayD, zarrayD });
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
                                              DataSourcesType&,
                                              const std::string& /*=""*/)
{
  return 1;
}

std::set<std::string> ArrayGTCCoordinates::GetGroupNames(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources)
{
  return this->XArray->GetGroupNames(paths, sources);
}

class ArrayGTCCoordinates::PlaneInserter : public viskores::worklet::WorkletMapField
{
public:
  PlaneInserter(viskores::Id nPlanes, viskores::Id nPtsPerPlane, viskores::Id numInsert)
    : NumPlanes(nPlanes)
    , PtsPerPlane(nPtsPerPlane)
    , NumInsert(numInsert)
  {
    this->dT = 1 / static_cast<viskores::Float64>(this->NumInsert + 1);
    this->dPhi = 1 / static_cast<viskores::Float64>(this->NumPlanes * (1 + this->NumInsert));
  }

  using ControlSignature = void(WholeArrayIn inCoords, WholeArrayOut outCoords);
  using ExecutionSignature = void(InputIndex, _1, _2);
  using InputDomain = _1;

  //Need to template this later.
  using PointType = viskores::Vec<viskores::Float32, 3>;

  template <typename InCoordsType, typename OutCoordsType>
  VISKORES_EXEC void operator()(const viskores::Id& inIdx,
                                const InCoordsType& inCoords,
                                OutCoordsType& outCoords) const
  {
    viskores::Id plane0PtIdx = inIdx;
    viskores::Id inPlaneIdx = plane0PtIdx / this->PtsPerPlane;
    viskores::Id pt0Offset = plane0PtIdx % this->PtsPerPlane;
    // This is correct:
    viskores::Id plane1PtIdx = plane0PtIdx + this->PtsPerPlane;
    // Unless we're in the last plane:
    if (inPlaneIdx == this->NumPlanes - 1)
    {
      plane1PtIdx = plane0PtIdx % this->PtsPerPlane;
    }

    viskores::Id firstOutputPlaneIndex = inPlaneIdx * (1 + this->NumInsert);

    const auto& plane0Pt = inCoords.Get(plane0PtIdx);
    const auto& plane1Pt = inCoords.Get(plane1PtIdx);
    outCoords.Set(firstOutputPlaneIndex * this->PtsPerPlane + pt0Offset, plane0Pt);

    //Now add NumInsert interpolated points.
    const auto rad = viskores::Sqrt((plane0Pt[0] * plane0Pt[0] + plane0Pt[1] * plane0Pt[1]));
    const auto Z = plane0Pt[2];

    //optimize this later...
    auto phi0 = viskores::ATan2(plane0Pt[1], plane0Pt[0]);
    auto phi1 = viskores::ATan2(plane1Pt[1], plane1Pt[0]);

    if (phi0 < phi1)
    {
      phi0 += viskores::TwoPi();
    }

    viskores::Float64 t = dT;
    viskores::Id numOutCoords = outCoords.GetNumberOfValues();

    for (viskores::Id i = 0; i < this->NumInsert; i++)
    {
      //calculate the index for the inbetween plane points.
      viskores::Id outIdx = (firstOutputPlaneIndex + i + 1) * this->PtsPerPlane + pt0Offset;
      if (outIdx > numOutCoords)
      {
#if defined(VISKORES_ENABLE_CUDA) || defined(VISKORES_ENABLE_KOKKOS)
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
      PointType outPt(rad * viskores::Cos(phi), rad * viskores::Sin(phi), Z);
      outCoords.Set(outIdx, outPt);
      t += dT;
    }
  }

private:
  viskores::Id NumPlanes;
  viskores::Id PtsPerPlane;
  viskores::Id NumInsert;
  viskores::Float64 dT;
  viskores::Float64 dPhi;
};

void ArrayGTCCoordinates::PostRead(std::vector<viskores::cont::DataSet>& dataSets,
                                   const fides::metadata::MetaData& metaData)
{
  if (dataSets.size() != 1)
  {
    throw std::runtime_error("Wrong number of partitions for GTC DataSets.");
  }

  auto& dataSet = dataSets[0];
  auto& cs = dataSet.GetField(dataSet.GetCoordinateSystemName());

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

    using intType = viskores::cont::ArrayHandle<int>;
    auto numPlanes = dataSet.GetField("num_planes").GetData().AsArrayHandle<intType>();
    auto numPtsPerPlane = dataSet.GetField("num_pts_per_plane").GetData().AsArrayHandle<intType>();

    viskores::Id numberOfPlanes = static_cast<viskores::Id>(numPlanes.ReadPortal().Get(0));
    viskores::Id numberOfPointsPerPlane =
      static_cast<viskores::Id>(numPtsPerPlane.ReadPortal().Get(0));

    ArrayGTCCoordinates::PlaneInserter plnIns(
      numberOfPlanes, numberOfPointsPerPlane, numInsertPlanes);
    if (cs.GetData().IsType<GTCCoordsType32>())
    {
      auto inCoords = cs.GetData().AsArrayHandle<GTCCoordsType32>();
      GTCCoordsType32 newCoords;
      viskores::Id numTotalPlanes = numberOfPlanes * (1 + numInsertPlanes);
      viskores::cont::Invoker invoke;
      newCoords.Allocate(numTotalPlanes * numberOfPointsPerPlane);
      invoke(plnIns, inCoords, newCoords);

      this->CachedCoords = newCoords;
    }
    else if (cs.GetData().IsType<GTCCoordsType64>())
    {
      auto inCoords = cs.GetData().AsArrayHandle<GTCCoordsType64>();
      GTCCoordsType64 newCoords;
      viskores::Id numTotalPlanes = numberOfPlanes * (1 + numInsertPlanes);
      viskores::cont::Invoker invoke;
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

  if (this->IsCached)
  {
    //Set coords to cached coordinates.
    cs = viskores::cont::CoordinateSystem("coords",
                                          make_ArrayHandleWithoutDataOwnership(this->CachedCoords));
  }
  else
  {
    throw std::runtime_error("No coordinates were cached!");
  }
}

/// Reads and returns array handles.
std::vector<viskores::cont::UnknownArrayHandle> ArrayGTCField::Read(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources,
  const fides::metadata::MetaData& selections)
{
  auto newSelections = selections;
  fides::metadata::Bool flag(true);
  newSelections.Set(fides::keys::READ_AS_MULTIBLOCK(), flag);
  // Removing because for XGC Fides blocks are not the same as ADIOS blocks
  newSelections.Remove(fides::keys::BLOCK_SELECTION());

  return this->ReadSelf(paths, sources, newSelections);
}

std::set<std::string> ArrayGTCField::GetGroupNames(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources)
{
  const auto& ds = sources[this->DataSourceName];
  ds->OpenSource(paths, this->DataSourceName);
  return ds->GetGroupNames(this->VariableName);
}


void ArrayGTCField::PostRead(std::vector<viskores::cont::DataSet>& dataSets,
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
    using intType = viskores::cont::ArrayHandle<int>;
    auto numPlanes = dataSet.GetField("num_planes").GetData().AsArrayHandle<intType>();
    auto numPtsPerPlane = dataSet.GetField("num_pts_per_plane").GetData().AsArrayHandle<intType>();

    this->NumberOfPointsPerPlane = static_cast<viskores::Id>(numPtsPerPlane.ReadPortal().Get(0));
    this->NumberOfPlanes = static_cast<viskores::Id>(numPlanes.ReadPortal().Get(0));

    this->IsCached = true;
  }

  if (dataSet.HasPointField(this->VariableName))
  {
    const auto& field = dataSet.GetField(this->VariableName);
    const auto& arr = field.GetData();

    viskores::Id numTotalPlanes = this->NumberOfPlanes * (1 + numInsertPlanes);
    fusionutil::PlaneInserterField planeInserter(
      this->NumberOfPlanes, this->NumberOfPointsPerPlane, numInsertPlanes);
    viskores::cont::Invoker invoke;

    if (arr.IsType<viskores::cont::ArrayHandle<viskores::Float32>>())
    {
      auto inArr = arr.AsArrayHandle<viskores::cont::ArrayHandle<viskores::Float32>>();
      viskores::cont::ArrayHandle<viskores::Float32> outArr;
      outArr.Allocate(numTotalPlanes * this->NumberOfPointsPerPlane);
      invoke(planeInserter, inArr, outArr);

      dataSet.AddPointField(this->VariableName, outArr);
    }
    else if (arr.IsType<viskores::cont::ArrayHandle<viskores::Float64>>())
    {
      auto inArr = arr.AsArrayHandle<viskores::cont::ArrayHandle<viskores::Float64>>();
      viskores::cont::ArrayHandle<viskores::Float64> outArr;
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

static void PrintJSONValue(const rapidjson::Value& value, int indent = 0)
{
  // Add indentation for nested structures
  std::string indentStr(indent, ' ');

  if (value.IsObject())
  {
    std::cout << indentStr << "{\n";
    for (rapidjson::Value::ConstMemberIterator itr = value.MemberBegin(); itr != value.MemberEnd();
         ++itr)
    {
      std::cout << indentStr << "  \"" << itr->name.GetString() << "\": ";
      PrintJSONValue(itr->value, indent + 4);
    }
    std::cout << indentStr << "}\n";
  }
  else if (value.IsArray())
  {
    std::cout << indentStr << "[\n";
    for (rapidjson::SizeType i = 0; i < value.Size(); i++)
    {
      PrintJSONValue(value[i], indent + 4);
    }
    std::cout << indentStr << "]\n";
  }
  else if (value.IsString())
  {
    std::cout << "\"" << value.GetString() << "\"\n";
  }
  else if (value.IsBool())
  {
    std::cout << (value.GetBool() ? "true" : "false") << "\n";
  }
  else if (value.IsInt())
  {
    std::cout << value.GetInt() << "\n";
  }
  else if (value.IsUint())
  {
    std::cout << value.GetUint() << "\n";
  }
  else if (value.IsInt64())
  {
    std::cout << value.GetInt64() << "\n";
  }
  else if (value.IsUint64())
  {
    std::cout << value.GetUint64() << "\n";
  }
  else if (value.IsDouble())
  {
    std::cout << value.GetDouble() << "\n";
  }
  else if (value.IsNull())
  {
    std::cout << "null\n";
  }
}

void ArrayGXCoordinates::ProcessJSONHelper(const rapidjson::Value& json,
                                           DataSourcesType& sources,
                                           const std::string& varName,
                                           std::unique_ptr<ArrayBasic>& array)
{
  if (!json.HasMember(varName.c_str()) || !json[varName.c_str()].IsObject())
  {
    throw std::runtime_error(this->ObjectName + " must provide a " + varName + "object.");
  }
  array.reset(new ArrayBasic());
  const auto& arrayJSON = json[varName.c_str()];
  array->ProcessJSON(arrayJSON, sources);
}


/// Overridden to handle ArrayXGCCoordinates specific items.
void ArrayGXCoordinates::ProcessJSON(const rapidjson::Value& json, DataSourcesType& sources)
{
  this->ProcessJSONHelper(json, sources, "xm", this->XM);
  this->ProcessJSONHelper(json, sources, "xn", this->XN);
  this->ProcessJSONHelper(json, sources, "rmnc", this->RMNC);
  this->ProcessJSONHelper(json, sources, "zmns", this->ZMNS);
  this->ProcessJSONHelper(json, sources, "lmns", this->LMNS);
  this->ProcessJSONHelper(json, sources, "nfp", this->nfp);
  this->ProcessJSONHelper(json, sources, "phi", this->phi);

  if (json.HasMember("num_theta") && json["num_theta"].IsInt())
  {
    this->NumTheta = json["num_theta"].GetInt();
  }
  if (json.HasMember("num_zeta") && json["num_zeta"].IsInt())
  {
    this->NumZeta = json["num_zeta"].GetInt();
  }
  if (json.HasMember("surface_min_index") && json["surface_min_index"].IsInt())
  {
    this->SurfaceMinIdxSet = true;
    this->SurfaceMinIdx = json["surface_min_index"].GetInt();
  }
  if (json.HasMember("surface_max_index") && json["surface_max_index"].IsInt())
  {
    this->SurfaceMaxIdxSet = true;
    this->SurfaceMaxIdx = json["surface_max_index"].GetInt();
  }
  if (json.HasMember("full_torus") && json["full_torus"].IsBool())
  {
    this->FullTorus = json["full_torus"].GetBool();
  }
}

std::vector<viskores::cont::UnknownArrayHandle> ArrayGXCoordinates::Read(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources,
  const fides::metadata::MetaData& selections)
{
  auto xmArrays = this->XM->Read(paths, sources, selections);
  auto xnArrays = this->XN->Read(paths, sources, selections);
  auto rmncArrays = this->RMNC->Read(paths, sources, selections);
  auto zmnsArrays = this->ZMNS->Read(paths, sources, selections);
  auto lmnsArrays = this->LMNS->Read(paths, sources, selections);
  auto nfpArrays = this->nfp->Read(paths, sources, selections);
  auto phiArrays = this->phi->Read(paths, sources, selections);

  this->XMArrayHandle = xmArrays[0];
  this->NFPArrayHandle = nfpArrays[0];
  this->RMNCArrayHandle = rmncArrays[0];
  this->ZMNSArrayHandle = zmnsArrays[0];
  this->LMNSArrayHandle = lmnsArrays[0];
  this->XNArrayHandle = xnArrays[0];
  this->PhiArrayHandle = phiArrays[0];

  std::vector<viskores::cont::UnknownArrayHandle> retVal;
  return retVal;
}

void ArrayGXCoordinates::PostRead(std::vector<viskores::cont::DataSet>& dataSets,
                                  const fides::metadata::MetaData& fidesNotUsed(metaData))
{
  if (!this->FullTorus)
  {
    throw std::runtime_error("Error: Only full torus case supported.");
  }

  if (dataSets.size() != 1)
  {
    throw std::runtime_error("Error: ArrayGXCoordinates must have 1 dataset.");
  }

  auto& dataSet = dataSets[0];

  auto xm = this->XMArrayHandle.AsArrayHandle<viskores::cont::ArrayHandle<double>>();
  auto xn = this->XNArrayHandle.AsArrayHandle<viskores::cont::ArrayHandle<double>>();
  if (xm.GetNumberOfValues() != xn.GetNumberOfValues())
    throw std::runtime_error("Error: Xm and Xn must be the same size.");

  auto rmnc = this->RMNCArrayHandle.AsArrayHandle<viskores::cont::ArrayHandleRuntimeVec<double>>();
  auto zmns = this->ZMNSArrayHandle.AsArrayHandle<viskores::cont::ArrayHandleRuntimeVec<double>>();
  auto lmns = this->LMNSArrayHandle.AsArrayHandle<viskores::cont::ArrayHandleRuntimeVec<double>>();
  if (rmnc.GetNumberOfValues() != zmns.GetNumberOfValues() ||
      rmnc.GetNumberOfValues() != lmns.GetNumberOfValues() ||
      rmnc.GetNumberOfComponents() != zmns.GetNumberOfComponents() ||
      rmnc.GetNumberOfComponents() != lmns.GetNumberOfComponents())
  {
    throw std::runtime_error("Error: rmnc, zmns and lmns must be the same size.");
  }

  //auto phi = this->PhiArrayHandle.AsArrayHandle<viskores::cont::ArrayHandle<double>>();
  viskores::Id numSurfaces = this->RMNCArrayHandle.GetNumberOfValues();

  viskores::Id srfIdxMin = 0;
  viskores::Id srfIdxMax = srfIdxMin + numSurfaces;
  if (this->SurfaceMinIdxSet)
  {
    srfIdxMin = this->SurfaceMinIdx;
  }
  if (this->SurfaceMaxIdxSet)
  {
    srfIdxMax = this->SurfaceMaxIdx;
  }

  if (srfIdxMax - srfIdxMin > numSurfaces)
  {
    throw std::runtime_error("Error: Number of surfaces exceeds the number in the file: " +
                             std::to_string(numSurfaces));
  }

  numSurfaces = srfIdxMax - srfIdxMin;

  this->NFP = static_cast<viskores::Id>(
    this->NFPArrayHandle.AsArrayHandle<viskores::cont::ArrayHandle<int>>().ReadPortal().Get(0));
  auto z0 = viskores::Pi() / this->NFP;

  //Add num_theta, num_zeta and NFP to the dataset.
  dataSet.AddField(viskores::cont::make_Field("num_theta",
                                              viskores::cont::Field::Association::WholeDataSet,
                                              &this->NumTheta,
                                              1,
                                              viskores::CopyFlag::On));
  dataSet.AddField(viskores::cont::make_Field("num_zeta",
                                              viskores::cont::Field::Association::WholeDataSet,
                                              &this->NumZeta,
                                              1,
                                              viskores::CopyFlag::On));
  dataSet.AddField(viskores::cont::make_Field("nfp",
                                              viskores::cont::Field::Association::WholeDataSet,
                                              &this->NFP,
                                              1,
                                              viskores::CopyFlag::On));
  dataSet.AddField(viskores::cont::make_Field("num_surfaces",
                                              viskores::cont::Field::Association::WholeDataSet,
                                              &numSurfaces,
                                              1,
                                              viskores::CopyFlag::On));
  dataSet.AddField(viskores::cont::make_Field("surface_min_index",
                                              viskores::cont::Field::Association::WholeDataSet,
                                              &srfIdxMin,
                                              1,
                                              viskores::CopyFlag::On));

  viskores::cont::ArrayHandleCounting<viskores::Float64> tax, zax;
  if (this->ThetaZeroMid)
  {
    tax = viskores::cont::make_ArrayHandleCounting(
      -viskores::Pi(), viskores::Pi() / static_cast<double>(this->NumTheta - 1), this->NumTheta);
  }
  else
  {
    tax = viskores::cont::make_ArrayHandleCounting(
      0.0, viskores::TwoPi() / static_cast<double>(this->NumTheta - 1), this->NumTheta);
  }

  if (this->ZetaZeroMid)
  {
    zax = viskores::cont::make_ArrayHandleCounting(
      -z0, z0 / static_cast<double>(this->NumZeta - 1), this->NumZeta);
  }
  else
  {
    zax = viskores::cont::make_ArrayHandleCounting(
      0.0, 2 * z0 / static_cast<double>(this->NumZeta - 1), this->NumZeta);
  }

  viskores::Id numZeta = zax.GetNumberOfValues();
  viskores::Id numTheta = tax.GetNumberOfValues();
  viskores::Id numZetaTheta = this->NumZeta * this->NumTheta;
  viskores::Id numAmplitudes = xm.GetNumberOfValues();

  //Calculate Cos/Sin values.
  viskores::cont::ArrayHandle<viskores::Float64> cosValuesBase, sinValuesBase, xValuesBase;
  cosValuesBase.Allocate(numZeta * numTheta * numAmplitudes);
  sinValuesBase.Allocate(numZeta * numTheta * numAmplitudes);
  xValuesBase.Allocate(numZeta * numTheta * numAmplitudes);
  auto cosValues = viskores::cont::make_ArrayHandleRuntimeVec(numAmplitudes, cosValuesBase);
  auto sinValues = viskores::cont::make_ArrayHandleRuntimeVec(numAmplitudes, sinValuesBase);
  auto xValues = viskores::cont::make_ArrayHandleRuntimeVec(numAmplitudes, xValuesBase);
  fides::datamodel::fusionutil::CalcCosSin calcCosSinWorklet(numZeta, numTheta, numAmplitudes);
  viskores::cont::Invoker invoke;
  invoke(calcCosSinWorklet, zax, tax, xm, xn, cosValues, sinValues, xValues);

  viskores::cont::ArrayHandle<viskores::Vec3f_64> RZLArrayBase;
  viskores::cont::ArrayHandle<viskores::Float64> LambdaBase;
  RZLArrayBase.Allocate(numZetaTheta * numSurfaces);
  LambdaBase.Allocate(numZetaTheta * numSurfaces);

  viskores::cont::ArrayHandle<viskores::Vec3f_64> RZLArray, XYZArrayGlobal;
  viskores::cont::ArrayHandle<viskores::Float64> LambdaArrayGlobal;
  RZLArray.Allocate(numZetaTheta);
  XYZArrayGlobal.Allocate(numSurfaces * numZetaTheta * this->NFP);
  LambdaArrayGlobal.Allocate(numSurfaces * numZetaTheta * this->NFP);

  viskores::cont::Invoker invoker;

  viskores::Id numPtsPerSrf = this->NumZeta * this->NumTheta * this->NFP;
  viskores::Id offset = 0;
  auto zn = viskores::cont::make_ArrayHandleCounting(
    0.0, viskores::TwoPi() / static_cast<double>(this->NFP), this->NFP);

  for (viskores::Id si = srfIdxMin; si < srfIdxMax; si++, offset += numPtsPerSrf)
  {
    //calc RZL for this surface.
    fides::datamodel::fusionutil::CalcRZL calcRZLWorklet(numAmplitudes, si);
    invoker(calcRZLWorklet, RZLArray, rmnc, zmns, lmns, cosValues, sinValues);

    auto XYZArray = viskores::cont::make_ArrayHandleView(XYZArrayGlobal, offset, numPtsPerSrf);
    auto LambdaArray =
      viskores::cont::make_ArrayHandleView(LambdaArrayGlobal, offset, numPtsPerSrf);

    if (this->FullTorus)
    {
      auto Phi_n = viskores::cont::ArrayHandle<viskores::Float64>();
      auto RZL_n = viskores::cont::ArrayHandle<viskores::Vec3f_64>();
      Phi_n.Allocate(this->NFP * this->NumZeta * this->NumTheta);
      RZL_n.Allocate(this->NFP * this->NumZeta * this->NumTheta);

      fides::datamodel::fusionutil::CalcNFP calcNFPWorklet(
        this->NFP, this->NumZeta, this->NumTheta);
      invoker(calcNFPWorklet, RZLArray, zn, zax, Phi_n, RZL_n);
      //Convert RZPhi to XYZ
      fides::datamodel::fusionutil::ConvertRZPhiToXYZ convertToXYZWorklet(
        this->NFP, this->NumZeta, this->NumTheta);
      invoker(convertToXYZWorklet, XYZArray, LambdaArray, Phi_n, RZL_n);
    }
    else
    {
      throw std::runtime_error("Error: Only full torus case supported.");
    }
  }

  dataSet.AddCoordinateSystem(viskores::cont::CoordinateSystem("coordinates", XYZArrayGlobal));

  //Add lambda field.
  dataSet.AddPointField("Lambda", LambdaArrayGlobal);
}

}
}
