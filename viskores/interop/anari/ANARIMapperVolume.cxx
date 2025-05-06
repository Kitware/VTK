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

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayCopyDevice.h>
#include <viskores/interop/anari/ANARIMapperVolume.h>

namespace viskores
{
namespace interop
{
namespace anari
{

ANARIMapperVolume::ANARIMapperVolume(anari_cpp::Device device,
                                     const ANARIActor& actor,
                                     const std::string& name,
                                     const viskores::cont::ColorTable& colorTable)
  : ANARIMapper(device, actor, name, colorTable)
{
  this->Handles = std::make_shared<ANARIMapperVolume::ANARIHandles>();
  this->Handles->Device = device;
  anari_cpp::retain(device, device);
}

ANARIMapperVolume::~ANARIMapperVolume()
{
  // ensure ANARI handles are released before host memory goes away
  this->Handles.reset();
}

void ANARIMapperVolume::SetActor(const ANARIActor& actor)
{
  this->ANARIMapper::SetActor(actor);
  this->ConstructArrays(true);
}

void ANARIMapperVolume::SetANARIColorMap(anari_cpp::Array1D color,
                                         anari_cpp::Array1D opacity,
                                         bool releaseArrays)
{
  auto d = this->GetDevice();
  auto v = this->GetANARIVolume();
  anari_cpp::setParameter(d, v, "color", color);
  anari_cpp::setParameter(d, v, "opacity", opacity);
  anari_cpp::commitParameters(d, v);
  this->ANARIMapper::SetANARIColorMap(color, opacity, releaseArrays);
}

void ANARIMapperVolume::SetANARIColorMapValueRange(const viskores::Vec2f_32& valueRange)
{
  auto d = this->GetDevice();
  auto v = this->GetANARIVolume();
  anari_cpp::setParameter(d, v, "valueRange", ANARI_FLOAT32_BOX1, &valueRange);
  anari_cpp::commitParameters(d, v);
}

void ANARIMapperVolume::SetANARIColorMapOpacityScale(viskores::Float32 opacityScale)
{
  auto d = this->GetDevice();
  auto v = this->GetANARIVolume();
  anari_cpp::setParameter(d, v, "densityScale", opacityScale);
  anari_cpp::commitParameters(d, v);
}

anari_cpp::SpatialField ANARIMapperVolume::GetANARISpatialField()
{
  if (this->Handles->SpatialField)
    return this->Handles->SpatialField;

  this->ConstructArrays();
  this->UpdateSpatialField();
  return this->Handles->SpatialField;
}

anari_cpp::Volume ANARIMapperVolume::GetANARIVolume()
{
  if (this->Handles->Volume)
    return this->Handles->Volume;

  auto d = this->GetDevice();

  this->Handles->Volume = anari_cpp::newObject<anari_cpp::Volume>(d, "transferFunction1D");

  auto colorArray = anari_cpp::newArray1D(d, ANARI_FLOAT32_VEC3, 3);
  auto* colors = anari_cpp::map<viskores::Vec3f_32>(d, colorArray);
  colors[0] = viskores::Vec3f_32(1.f, 0.f, 0.f);
  colors[1] = viskores::Vec3f_32(0.f, 1.f, 0.f);
  colors[2] = viskores::Vec3f_32(0.f, 0.f, 1.f);
  anari_cpp::unmap(d, colorArray);

  auto opacityArray = anari_cpp::newArray1D(d, ANARI_FLOAT32, 2);
  auto* opacities = anari_cpp::map<float>(d, opacityArray);
  opacities[0] = 0.f;
  opacities[1] = 1.f;
  anari_cpp::unmap(d, opacityArray);

  anari_cpp::setAndReleaseParameter(d, this->Handles->Volume, "color", colorArray);
  anari_cpp::setAndReleaseParameter(d, this->Handles->Volume, "opacity", opacityArray);
  // Keep old name as a parameter for bug-backwards compatibility: 'field' isn't the right name
  anari_cpp::setParameter(d, this->Handles->Volume, "field", this->GetANARISpatialField());
  anari_cpp::setParameter(d, this->Handles->Volume, "value", this->GetANARISpatialField());
  anari_cpp::setParameter(d, this->Handles->Volume, "name", this->MakeObjectName("volume"));
  anari_cpp::commitParameters(d, this->Handles->Volume);

  SetANARIColorMapValueRange(viskores::Vec2f_32(0.f, 10.f));

  return this->Handles->Volume;
}

// For the moment, we use ospray conventions
//    uint8_t VKL_TETRAHEDRON = 10;
//    uint8_t VKL_HEXAHEDRON = 12;
//    uint8_t VKL_WEDGE = 13;
//    uint8_t VKL_PYRAMID = 14;
//
struct ToAnariCellType
{
  VISKORES_EXEC_CONT viskores::UInt8 operator()(viskores::UInt8 shape) const
  {
    if (shape == viskores::CELL_SHAPE_TETRA)
    {
      return 10;
    }
    else if (shape == viskores::CELL_SHAPE_HEXAHEDRON)
    {
      return 14;
    }
    else if (shape == viskores::CELL_SHAPE_WEDGE)
    {
      return 13;
    }
    else if (shape == viskores::CELL_SHAPE_PYRAMID)
    {
      return 12;
    }
    return uint8_t(-1);
  }
};

void ANARIMapperVolume::ConstructArrays(bool regenerate)
{
  if (regenerate)
    this->Current = false;

  if (this->Current)
    return;

  this->Current = true;
  this->Valid = false;

  auto d = this->GetDevice();

  const auto& actor = this->GetActor();
  const auto& coords = actor.GetCoordinateSystem();
  const auto& cells = actor.GetCellSet();
  const auto& fieldArray = actor.GetField().GetData();

  const bool isPointBased =
    actor.GetField().GetAssociation() == viskores::cont::Field::Association::Points;
  const bool isStructured = cells.CanConvert<viskores::cont::CellSetStructured<3>>();
  const bool isScalar = fieldArray.GetNumberOfComponentsFlat() == 1;

  this->Handles->ReleaseArrays();
  anari_cpp::release(d, this->Handles->SpatialField);
  this->Handles->SpatialField = nullptr;

  // Structured regular volume data
  if (isStructured && isScalar)
  {
    this->Handles->SpatialField =
      anari_cpp::newObject<anari_cpp::SpatialField>(this->GetDevice(), "structuredRegular");

    auto structuredCells = cells.AsCellSet<viskores::cont::CellSetStructured<3>>();
    auto pdims =
      isPointBased ? structuredCells.GetPointDimensions() : structuredCells.GetCellDimensions();

    StructuredVolumeArrays arrays;

    viskores::cont::ArrayCopyShallowIfPossible(fieldArray, arrays.Data);
    auto* ptr = (float*)arrays.Data.GetBuffers()[0].ReadPointerHost(*arrays.Token);

    auto bounds = coords.GetBounds();
    viskores::Vec3f_32 bLower(bounds.X.Min, bounds.Y.Min, bounds.Z.Min);
    viskores::Vec3f_32 bUpper(bounds.X.Max, bounds.Y.Max, bounds.Z.Max);
    viskores::Vec3f_32 size = bUpper - bLower;

    viskores::Vec3ui_32 dims(pdims[0], pdims[1], pdims[2]);
    auto spacing = size / (viskores::Vec3f_32(dims) - 1.f);

    std::memcpy(this->Handles->StructuredParameters.Dims, &dims, sizeof(dims));
    std::memcpy(this->Handles->StructuredParameters.Origin, &bLower, sizeof(bLower));
    std::memcpy(this->Handles->StructuredParameters.Spacing, &spacing, sizeof(spacing));
    this->Handles->StructuredParameters.Data =
      anari_cpp::newArray3D(d, ptr, NoopANARIDeleter, nullptr, dims[0], dims[1], dims[2]);

    this->StructuredArrays = arrays;
    this->Valid = true;
  }
  // Unstructured volume data
  else if (isPointBased)
  {
    this->Handles->SpatialField =
      anari_cpp::newObject<anari_cpp::SpatialField>(this->GetDevice(), "unstructured");

    UntructuredVolumeArrays arrays;

    // Cell Data
    if (cells.IsType<viskores::cont::CellSetSingleType<>>())
    {
      // 1. Cell Type
      viskores::cont::CellSetSingleType<> sgl =
        cells.AsCellSet<viskores::cont::CellSetSingleType<>>();
      auto shapes =
        sgl.GetShapesArray(viskores::TopologyElementTagCell(), viskores::TopologyElementTagPoint());
      viskores::cont::ArrayCopyDevice(
        viskores::cont::make_ArrayHandleTransform(shapes, ToAnariCellType{}), arrays.CellType);

      // 2. Cell Connectivity
      auto conn = sgl.GetConnectivityArray(viskores::TopologyElementTagCell(),
                                           viskores::TopologyElementTagPoint());
      viskores::cont::ArrayCopyDevice(conn, arrays.Index);

      // 3. Cell Index
      auto offsets = sgl.GetOffsetsArray(viskores::TopologyElementTagCell(),
                                         viskores::TopologyElementTagPoint());
      viskores::cont::ArrayCopyDevice(offsets, arrays.CellIndex);
    }

    else if (cells.IsType<viskores::cont::CellSetExplicit<>>())
    {
      // 1. Cell Type
      viskores::cont::CellSetExplicit<> exp = cells.AsCellSet<viskores::cont::CellSetExplicit<>>();
      auto shapes =
        exp.GetShapesArray(viskores::TopologyElementTagCell(), viskores::TopologyElementTagPoint());
      viskores::cont::ArrayCopyDevice(
        viskores::cont::make_ArrayHandleTransform(shapes, ToAnariCellType{}), arrays.CellType);

      // 2. Cell Connectivity
      auto conn = exp.GetConnectivityArray(viskores::TopologyElementTagCell(),
                                           viskores::TopologyElementTagPoint());
      viskores::cont::ArrayCopyDevice(conn, arrays.Index);

      // 3. Cell Index
      auto offsets = exp.GetOffsetsArray(viskores::TopologyElementTagCell(),
                                         viskores::TopologyElementTagPoint());
      viskores::cont::ArrayCopyDevice(offsets, arrays.CellIndex);
    }

    // Vetrex Coordinates
    viskores::cont::ArrayCopyShallowIfPossible(coords.GetData(), arrays.VertexPosition);

    // Vetrex Data
    viskores::cont::ArrayCopyShallowIfPossible(fieldArray, arrays.VertexData);

    // "indexPrefixed"
    this->Handles->UnstructuredParameters.IndexPrefixed = false;

    // "vertex.position"
    {
      auto* ptr =
        (viskores::Vec3f_32*)arrays.VertexPosition.GetBuffers()[0].ReadPointerHost(*arrays.Token);
      this->Handles->UnstructuredParameters.VertexPosition = anari_cpp::newArray1D(
        d, ptr, NoopANARIDeleter, nullptr, arrays.VertexPosition.GetNumberOfValues());
    }

    // "vertex.data"
    {
      auto* ptr = (float*)arrays.VertexData.GetBuffers()[0].ReadPointerHost(*arrays.Token);
      this->Handles->UnstructuredParameters.VertexData = anari_cpp::newArray1D(
        d, ptr, NoopANARIDeleter, nullptr, arrays.VertexData.GetNumberOfValues());
    }

    // "index"
    {
      auto* ptr = (uint64_t*)arrays.Index.GetBuffers()[0].ReadPointerHost(*arrays.Token);
      this->Handles->UnstructuredParameters.Index =
        anari_cpp::newArray1D(d, ptr, NoopANARIDeleter, nullptr, arrays.Index.GetNumberOfValues());
    }

    // "cell.index"
    {
      auto* ptr = (uint64_t*)arrays.CellIndex.GetBuffers()[0].ReadPointerHost(*arrays.Token);
      this->Handles->UnstructuredParameters.CellIndex = anari_cpp::newArray1D(
        d, ptr, NoopANARIDeleter, nullptr, arrays.CellIndex.GetNumberOfValues() - 1);
    }

    // TODO "cell.data" (NOT SUPPORED YET)
    // {
    //   auto* ptr = (float*)arrays.CellData.GetBuffers()[0].ReadPointerHost(*arrays.Token);
    //   this->Handles->UnstructuredParameters.CellData =
    //     anari_cpp::newArray1D(d, ptr, NoopANARIDeleter, nullptr, arrays.CellData.GetNumberOfValues());
    // }

    // "cell.type"
    {
      uint8_t* ptr = (uint8_t*)arrays.CellType.GetBuffers()[0].ReadPointerHost(*arrays.Token);
      this->Handles->UnstructuredParameters.CellType = anari_cpp::newArray1D(
        d, ptr, NoopANARIDeleter, nullptr, arrays.CellType.GetNumberOfValues());
    }

    this->UnstructuredArrays = arrays;
    this->Valid = true;
  }

  this->UpdateSpatialField();
  this->RefreshGroup();
}

void ANARIMapperVolume::UpdateSpatialField()
{
  if (!this->Handles->SpatialField)
    return;

  auto d = this->GetDevice();

  anari_cpp::unsetParameter(d, this->Handles->SpatialField, "origin");
  anari_cpp::unsetParameter(d, this->Handles->SpatialField, "spacing");
  anari_cpp::unsetParameter(d, this->Handles->SpatialField, "data");

  anari_cpp::unsetParameter(d, this->Handles->SpatialField, "vertex.position");
  anari_cpp::unsetParameter(d, this->Handles->SpatialField, "vertex.data");
  anari_cpp::unsetParameter(d, this->Handles->SpatialField, "index");
  anari_cpp::unsetParameter(d, this->Handles->SpatialField, "indexPrefixed");
  anari_cpp::unsetParameter(d, this->Handles->SpatialField, "cell.index");
  anari_cpp::unsetParameter(d, this->Handles->SpatialField, "cell.data");
  anari_cpp::unsetParameter(d, this->Handles->SpatialField, "cell.type");

  anari_cpp::setParameter(
    d, this->Handles->SpatialField, "name", this->MakeObjectName("spatialField"));

  if (this->Handles->StructuredParameters.Data)
  {
    anari_cpp::setParameter(
      d, this->Handles->SpatialField, "origin", this->Handles->StructuredParameters.Origin);
    anari_cpp::setParameter(
      d, this->Handles->SpatialField, "spacing", this->Handles->StructuredParameters.Spacing);
    anari_cpp::setParameter(
      d, this->Handles->SpatialField, "data", this->Handles->StructuredParameters.Data);
  }

  if (this->Handles->UnstructuredParameters.VertexPosition)
  {
    anari_cpp::setParameter(d,
                            this->Handles->SpatialField,
                            "vertex.position",
                            this->Handles->UnstructuredParameters.VertexPosition);
  }
  if (this->Handles->UnstructuredParameters.VertexData)
  {
    anari_cpp::setParameter(d,
                            this->Handles->SpatialField,
                            "vertex.data",
                            this->Handles->UnstructuredParameters.VertexData);
  }
  if (this->Handles->UnstructuredParameters.Index)
  {
    anari_cpp::setParameter(
      d, this->Handles->SpatialField, "index", this->Handles->UnstructuredParameters.Index);
  }
  if (this->Handles->UnstructuredParameters.CellIndex)
  {
    anari_cpp::setParameter(d,
                            this->Handles->SpatialField,
                            "indexPrefixed",
                            this->Handles->UnstructuredParameters.IndexPrefixed);
  }
  if (this->Handles->UnstructuredParameters.CellIndex)
  {
    anari_cpp::setParameter(d,
                            this->Handles->SpatialField,
                            "cell.index",
                            this->Handles->UnstructuredParameters.CellIndex);
  }
  // if (this->Handles->UnstructuredParameters.CellData)
  // {
  //   anari_cpp::setParameter(
  //     d, this->Handles->SpatialField, "cell.data", this->Handles->UnstructuredParameters.CellData);
  // }
  if (this->Handles->UnstructuredParameters.CellType)
  {
    anari_cpp::setParameter(
      d, this->Handles->SpatialField, "cell.type", this->Handles->UnstructuredParameters.CellType);
  }

  anari_cpp::commitParameters(d, this->Handles->SpatialField);

  if (this->Handles->Volume)
  {
    anari_cpp::setParameter(d, this->Handles->Volume, "field", this->GetANARISpatialField());
    anari_cpp::setParameter(d, this->Handles->Volume, "value", this->GetANARISpatialField());
    anari_cpp::commitParameters(d, this->Handles->Volume);
  }
}

ANARIMapperVolume::ANARIHandles::~ANARIHandles()
{
  this->ReleaseArrays();
  anari_cpp::release(this->Device, this->Volume);
  anari_cpp::release(this->Device, this->SpatialField);
  anari_cpp::release(this->Device, this->Device);
}

void ANARIMapperVolume::ANARIHandles::ReleaseArrays()
{
  anari_cpp::release(this->Device, this->StructuredParameters.Data);
  this->StructuredParameters.Data = nullptr;

  anari_cpp::release(this->Device, this->UnstructuredParameters.VertexPosition);
  this->UnstructuredParameters.VertexPosition = nullptr;
}

} // namespace anari
} // namespace interop
} // namespace viskores
