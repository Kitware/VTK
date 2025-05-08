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

// viskores
#include <viskores/cont/ArrayCopy.h>
#include <viskores/interop/anari/ANARIMapperPoints.h>
#include <viskores/rendering/raytracing/SphereExtractor.h>
#include <viskores/worklet/WorkletMapField.h>
// anari
#include <anari/anari_cpp/ext/linalg.h>

namespace viskores
{
namespace interop
{
namespace anari
{

// Worklets ///////////////////////////////////////////////////////////////////

class ExtractPointPositions : public viskores::worklet::WorkletMapField
{
public:
  VISKORES_CONT
  ExtractPointPositions() = default;

  using ControlSignature = void(FieldIn,      // [in] index
                                WholeArrayIn, // [in] point
                                WholeArrayOut // [out] point
  );
  using ExecutionSignature = void(InputIndex,
                                  _1, // [in] index
                                  _2, // [in] point
                                  _3  // [out] points
  );

  template <typename InPointPortalType, typename OutPointPortalType>
  VISKORES_EXEC void operator()(const viskores::Id out_idx,
                                const viskores::Id in_idx,
                                const InPointPortalType& points,
                                OutPointPortalType& outP) const
  {
    outP.Set(out_idx, static_cast<viskores::Vec3f_32>(points.Get(in_idx)));
  }
};

// Helper functions ///////////////////////////////////////////////////////////

static PointsFieldArrays UnpackFields(FieldSet fields)
{
  PointsFieldArrays retval;

  using AttributeHandleT = decltype(retval.Field1);

  auto makeFieldArray = [](auto field, auto& numComps) -> AttributeHandleT
  {
    if (field.GetNumberOfValues() == 0)
      return {};

    auto fieldData = field.GetData();
    numComps = fieldData.GetNumberOfComponentsFlat();
    if (numComps >= 1 && numComps <= 4)
    {
      viskores::cont::ArrayHandleRuntimeVec<viskores::Float32> outData(numComps);
      viskores::cont::ArrayCopyShallowIfPossible(fieldData, outData);
      return outData;
    }

    return {};
  };

  retval.Field1 = makeFieldArray(fields[0], retval.NumberOfField1Components);
  retval.Field2 = makeFieldArray(fields[1], retval.NumberOfField2Components);
  retval.Field3 = makeFieldArray(fields[2], retval.NumberOfField3Components);
  retval.Field4 = makeFieldArray(fields[3], retval.NumberOfField4Components);

  auto isFieldEmpty = [](const viskores::cont::Field& f) -> bool
  { return f.GetNumberOfValues() == 0 || f.GetData().GetNumberOfComponentsFlat() != 1; };

  if (!isFieldEmpty(fields[0]))
    retval.Field1Name = fields[0].GetName();
  if (!isFieldEmpty(fields[1]))
    retval.Field2Name = fields[1].GetName();
  if (!isFieldEmpty(fields[2]))
    retval.Field3Name = fields[2].GetName();
  if (!isFieldEmpty(fields[3]))
    retval.Field4Name = fields[3].GetName();

  return retval;
}

static PointsArrays UnpackPoints(viskores::cont::ArrayHandle<viskores::Id> points,
                                 viskores::cont::CoordinateSystem coords)
{
  PointsArrays retval;

  const auto numPoints = points.GetNumberOfValues();
  retval.Vertices.Allocate(numPoints);
  viskores::worklet::DispatcherMapField<ExtractPointPositions>().Invoke(
    points, coords, retval.Vertices);

  return retval;
}

// ANARIMapperPoints definitions //////////////////////////////////////////////

ANARIMapperPoints::ANARIMapperPoints(anari_cpp::Device device,
                                     const ANARIActor& actor,
                                     const std::string& name,
                                     const viskores::cont::ColorTable& colorTable)
  : ANARIMapper(device, actor, name, colorTable)
{
  this->Handles = std::make_shared<ANARIMapperPoints::ANARIHandles>();
  this->Handles->Device = device;
  auto& attributes = this->Handles->Parameters.Vertex.Attribute;
  std::fill(attributes.begin(), attributes.end(), nullptr);
  anari_cpp::retain(device, device);
}

ANARIMapperPoints::~ANARIMapperPoints()
{
  // ensure ANARI handles are released before host memory goes away
  this->Handles.reset();
}

void ANARIMapperPoints::SetActor(const ANARIActor& actor)
{
  this->ANARIMapper::SetActor(actor);
  this->ConstructArrays(true);
  this->UpdateMaterial();
}

void ANARIMapperPoints::SetMapFieldAsAttribute(bool enabled)
{
  this->ANARIMapper::SetMapFieldAsAttribute(enabled);
  this->UpdateGeometry();
  this->UpdateMaterial();
}

void ANARIMapperPoints::SetANARIColorMap(anari_cpp::Array1D color,
                                         anari_cpp::Array1D opacity,
                                         bool releaseArrays)
{
  this->GetANARISurface();
  auto s = this->Handles->Sampler;
  if (s)
  {
    auto d = this->GetDevice();
    anari_cpp::setParameter(d, s, "image", color);
    anari_cpp::commitParameters(d, s);
  }
  this->ANARIMapper::SetANARIColorMap(color, opacity, releaseArrays);
}

void ANARIMapperPoints::SetANARIColorMapValueRange(const viskores::Vec2f_32& valueRange)
{
  this->GetANARISurface();
  auto s = this->Handles->Sampler;
  if (s)
  {
    auto d = this->GetDevice();
    auto scale =
      anari_cpp::scaling_matrix(anari_cpp::float3(1.f / (valueRange[1] - valueRange[0])));
    auto translation = anari_cpp::translation_matrix(anari_cpp::float3(-valueRange[0], 0, 0));
    anari_cpp::setParameter(d, s, "inTransform", anari_cpp::mul(scale, translation));
    anari_cpp::commitParameters(d, s);
  }
}

anari_cpp::Geometry ANARIMapperPoints::GetANARIGeometry()
{
  if (this->Handles->Geometry)
    return this->Handles->Geometry;

  auto d = this->GetDevice();
  this->Handles->Geometry = anari_cpp::newObject<anari_cpp::Geometry>(d, "sphere");
  this->ConstructArrays();
  this->UpdateGeometry();

  return this->Handles->Geometry;
}

anari_cpp::Surface ANARIMapperPoints::GetANARISurface()
{
  if (this->Handles->Surface)
    return this->Handles->Surface;

  auto d = this->GetDevice();

  this->Handles->Surface = anari_cpp::newObject<anari_cpp::Surface>(d);

  if (!this->Handles->Material)
  {
    this->Handles->Material = anari_cpp::newObject<anari_cpp::Material>(d, "matte");
    anari_cpp::setParameter(d, this->Handles->Material, "name", this->MakeObjectName("material"));
  }

  auto s = anari_cpp::newObject<anari_cpp::Sampler>(d, "image1D");
  this->Handles->Sampler = s;
  auto colorArray = anari_cpp::newArray1D(d, ANARI_FLOAT32_VEC4, 3);
  auto* colors = anari_cpp::map<viskores::Vec4f_32>(d, colorArray);
  colors[0] = viskores::Vec4f_32(1.f, 0.f, 0.f, 0.f);
  colors[1] = viskores::Vec4f_32(0.f, 1.f, 0.f, 0.5f);
  colors[2] = viskores::Vec4f_32(0.f, 0.f, 1.f, 1.f);
  anari_cpp::unmap(d, colorArray);
  anari_cpp::setAndReleaseParameter(d, s, "image", colorArray);
  anari_cpp::setParameter(d, s, "filter", "linear");
  anari_cpp::setParameter(d, s, "wrapMode", "clampToEdge");
  anari_cpp::setParameter(d, s, "name", this->MakeObjectName("colormap"));
  anari_cpp::commitParameters(d, s);

  this->SetANARIColorMapValueRange(viskores::Vec2f_32(0.f, 10.f));

  this->UpdateMaterial();

  anari_cpp::setParameter(d, this->Handles->Surface, "name", this->MakeObjectName("surface"));
  anari_cpp::setParameter(d, this->Handles->Surface, "geometry", this->GetANARIGeometry());
  anari_cpp::setParameter(d, this->Handles->Surface, "material", this->Handles->Material);
  anari_cpp::commitParameters(d, this->Handles->Surface);

  return this->Handles->Surface;
}

void ANARIMapperPoints::ConstructArrays(bool regenerate)
{
  if (regenerate)
    this->Current = false;

  if (this->Current)
    return;

  this->Current = true;
  this->Valid = false;

  this->Handles->ReleaseArrays();

  const auto& actor = this->GetActor();
  const auto& coords = actor.GetCoordinateSystem();

  if (coords.GetNumberOfPoints() == 0)
  {
    this->RefreshGroup();
    return;
  }

  viskores::Bounds coordBounds = coords.GetBounds();
  // set a default radius
  viskores::Float64 lx = coordBounds.X.Length();
  viskores::Float64 ly = coordBounds.Y.Length();
  viskores::Float64 lz = coordBounds.Z.Length();
  viskores::Float64 mag = viskores::Sqrt(lx * lx + ly * ly + lz * lz);
  // same as used in vtk ospray
  constexpr viskores::Float64 heuristic = 500.;
  auto baseRadius = static_cast<viskores::Float32>(mag / heuristic);

  viskores::rendering::raytracing::SphereExtractor sphereExtractor;

  sphereExtractor.ExtractCoordinates(coords, baseRadius);

  auto numPoints = sphereExtractor.GetNumberOfSpheres();
  this->Handles->Parameters.NumPrimitives = static_cast<uint32_t>(numPoints);

  if (numPoints == 0)
  {
    this->RefreshGroup();
    return;
  }

  this->PrimaryField = actor.GetPrimaryFieldIndex();

  auto pts = sphereExtractor.GetPointIds();

  auto arrays = UnpackPoints(pts, coords);
  auto fieldArrays = UnpackFields(actor.GetFieldSet());

  arrays.Radii = sphereExtractor.GetRadii();
  auto* p = (viskores::Vec3f_32*)arrays.Vertices.GetBuffers()[0].ReadPointerHost(*arrays.Token);
  auto* r = (float*)arrays.Radii.GetBuffers()[0].ReadPointerHost(*arrays.Token);

  auto d = this->GetDevice();
  this->Handles->Parameters.Vertex.Position =
    anari_cpp::newArray1D(d, p, NoopANARIDeleter, nullptr, numPoints);
  this->Handles->Parameters.Vertex.Radius =
    anari_cpp::newArray1D(d, r, NoopANARIDeleter, nullptr, numPoints);

  auto createANARIArray = [](auto device, auto fieldArray, auto& token) -> anari_cpp::Array1D
  {
    const auto nv = fieldArray.GetNumberOfValues();
    if (nv == 0)
      return nullptr;

    anari_cpp::DataType type = ANARI_FLOAT32 + fieldArray.GetNumberOfComponents() - 1;
    const auto& buffers = fieldArray.GetBuffers();
    auto* a = buffers[0].ReadPointerHost(*token);
    if (!a && buffers.size() > 1)
      a = buffers[1].ReadPointerHost(*token);

    return a ? anariNewArray1D(device, a, NoopANARIDeleter, nullptr, type, nv) : nullptr;
  };

  this->Handles->Parameters.Vertex.Attribute[0] =
    createANARIArray(d, fieldArrays.Field1, fieldArrays.Token);
  this->Handles->Parameters.Vertex.Attribute[1] =
    createANARIArray(d, fieldArrays.Field2, fieldArrays.Token);
  this->Handles->Parameters.Vertex.Attribute[2] =
    createANARIArray(d, fieldArrays.Field3, fieldArrays.Token);
  this->Handles->Parameters.Vertex.Attribute[3] =
    createANARIArray(d, fieldArrays.Field4, fieldArrays.Token);

  this->Handles->Parameters.Vertex.AttributeName[0] = fieldArrays.Field1Name;
  this->Handles->Parameters.Vertex.AttributeName[1] = fieldArrays.Field2Name;
  this->Handles->Parameters.Vertex.AttributeName[2] = fieldArrays.Field3Name;
  this->Handles->Parameters.Vertex.AttributeName[3] = fieldArrays.Field4Name;

  this->UpdateGeometry();
  this->UpdateMaterial();

  this->Arrays = arrays;
  this->FieldArrays = fieldArrays;
  this->Valid = true;

  this->RefreshGroup();
}

void ANARIMapperPoints::UpdateGeometry()
{
  if (!this->Handles->Geometry)
    return;

  auto d = this->GetDevice();

  anari_cpp::unsetParameter(d, this->Handles->Geometry, "vertex.position");
  anari_cpp::unsetParameter(d, this->Handles->Geometry, "vertex.radius");
  anari_cpp::unsetParameter(d, this->Handles->Geometry, "vertex.attribute0");
  anari_cpp::unsetParameter(d, this->Handles->Geometry, "vertex.attribute1");
  anari_cpp::unsetParameter(d, this->Handles->Geometry, "vertex.attribute2");
  anari_cpp::unsetParameter(d, this->Handles->Geometry, "vertex.attribute3");
  anari_cpp::unsetParameter(d, this->Handles->Geometry, "usd::attribute0.name");
  anari_cpp::unsetParameter(d, this->Handles->Geometry, "usd::attribute1.name");
  anari_cpp::unsetParameter(d, this->Handles->Geometry, "usd::attribute2.name");
  anari_cpp::unsetParameter(d, this->Handles->Geometry, "usd::attribute3.name");

  anari_cpp::setParameter(d, this->Handles->Geometry, "name", this->MakeObjectName("geometry"));

  if (this->Handles->Parameters.Vertex.Position)
  {
    anari_cpp::setParameter(
      d, this->Handles->Geometry, "vertex.position", this->Handles->Parameters.Vertex.Position);
    anari_cpp::setParameter(
      d, this->Handles->Geometry, "vertex.radius", this->Handles->Parameters.Vertex.Radius);
    if (this->GetMapFieldAsAttribute())
    {
      // Attributes //

      anari_cpp::setParameter(d,
                              this->Handles->Geometry,
                              "vertex.attribute0",
                              this->Handles->Parameters.Vertex.Attribute[0]);
      anari_cpp::setParameter(d,
                              this->Handles->Geometry,
                              "vertex.attribute1",
                              this->Handles->Parameters.Vertex.Attribute[1]);
      anari_cpp::setParameter(d,
                              this->Handles->Geometry,
                              "vertex.attribute2",
                              this->Handles->Parameters.Vertex.Attribute[2]);
      anari_cpp::setParameter(d,
                              this->Handles->Geometry,
                              "vertex.attribute3",
                              this->Handles->Parameters.Vertex.Attribute[3]);

      // Attribute names for USD //

      if (!this->Handles->Parameters.Vertex.AttributeName[0].empty())
      {
        anari_cpp::setParameter(d,
                                this->Handles->Geometry,
                                "usd::attribute0.name",
                                this->Handles->Parameters.Vertex.AttributeName[0]);
      }
      if (!this->Handles->Parameters.Vertex.AttributeName[1].empty())
      {
        anari_cpp::setParameter(d,
                                this->Handles->Geometry,
                                "usd::attribute1.name",
                                this->Handles->Parameters.Vertex.AttributeName[1]);
      }
      if (!this->Handles->Parameters.Vertex.AttributeName[2].empty())
      {
        anari_cpp::setParameter(d,
                                this->Handles->Geometry,
                                "usd::attribute2.name",
                                this->Handles->Parameters.Vertex.AttributeName[2]);
      }
      if (!this->Handles->Parameters.Vertex.AttributeName[3].empty())
      {
        anari_cpp::setParameter(d,
                                this->Handles->Geometry,
                                "usd::attribute3.name",
                                this->Handles->Parameters.Vertex.AttributeName[3]);
      }
    }
  }

  anari_cpp::commitParameters(d, this->Handles->Geometry);
}

void ANARIMapperPoints::UpdateMaterial()
{
  if (!this->Handles->Material)
    return;

  auto d = this->GetDevice();
  auto s = this->Handles->Sampler;
  auto a = this->Handles->Parameters.Vertex.Attribute[PrimaryField];
  if (s && a && this->GetMapFieldAsAttribute())
  {
    anari_cpp::setParameter(d, s, "inAttribute", AnariMaterialInputString(PrimaryField));
    anari_cpp::commitParameters(d, s);
    anari_cpp::setParameter(d, this->Handles->Material, "color", s);
  }
  else
    anari_cpp::setParameter(d, this->Handles->Material, "color", viskores::Vec3f_32(1.f));

  anari_cpp::commitParameters(d, this->Handles->Material);
}

ANARIMapperPoints::ANARIHandles::~ANARIHandles()
{
  this->ReleaseArrays();
  anari_cpp::release(this->Device, this->Surface);
  anari_cpp::release(this->Device, this->Material);
  anari_cpp::release(this->Device, this->Sampler);
  anari_cpp::release(this->Device, this->Geometry);
  anari_cpp::release(this->Device, this->Device);
}

void ANARIMapperPoints::ANARIHandles::ReleaseArrays()
{
  anari_cpp::release(this->Device, this->Parameters.Vertex.Position);
  anari_cpp::release(this->Device, this->Parameters.Vertex.Radius);
  anari_cpp::release(this->Device, this->Parameters.Vertex.Attribute[0]);
  anari_cpp::release(this->Device, this->Parameters.Vertex.Attribute[1]);
  anari_cpp::release(this->Device, this->Parameters.Vertex.Attribute[2]);
  anari_cpp::release(this->Device, this->Parameters.Vertex.Attribute[3]);
  this->Parameters.Vertex.Position = nullptr;
  this->Parameters.Vertex.Radius = nullptr;
  this->Parameters.Vertex.Attribute[0] = nullptr;
  this->Parameters.Vertex.Attribute[1] = nullptr;
  this->Parameters.Vertex.Attribute[2] = nullptr;
  this->Parameters.Vertex.Attribute[3] = nullptr;
}

} // namespace anari
} // namespace interop
} // namespace viskores
