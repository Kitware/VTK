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
#include <viskores/filter/vector_analysis/SurfaceNormals.h>
#include <viskores/interop/anari/ANARIMapperTriangles.h>
#include <viskores/rendering/raytracing/TriangleExtractor.h>
#include <viskores/worklet/WorkletMapField.h>
// std
#include <numeric>
// anari
#include <anari/anari_cpp/ext/linalg.h>

namespace viskores
{
namespace interop
{
namespace anari
{

// Worklets ///////////////////////////////////////////////////////////////////

class ExtractTriangleFields : public viskores::worklet::WorkletMapField
{
public:
  bool PopulateField1{ false };
  bool PopulateField2{ false };
  bool PopulateField3{ false };
  bool PopulateField4{ false };
  viskores::Range Field1Range;
  viskores::Range Field2Range;
  viskores::Range Field3Range;
  viskores::Range Field4Range;

  VISKORES_CONT
  ExtractTriangleFields(bool emptyField1,
                        bool emptyField2,
                        bool emptyField3,
                        bool emptyField4,
                        viskores::Range field1Range,
                        viskores::Range field2Range,
                        viskores::Range field3Range,
                        viskores::Range field4Range)
    : PopulateField1(!emptyField1)
    , PopulateField2(!emptyField2)
    , PopulateField3(!emptyField3)
    , PopulateField4(!emptyField4)
    , Field1Range(field1Range)
    , Field2Range(field2Range)
    , Field3Range(field3Range)
    , Field4Range(field4Range)
  {
  }

  using ControlSignature = void(FieldIn,
                                WholeArrayIn,  // [in] field1
                                WholeArrayIn,  // [in] field2
                                WholeArrayIn,  // [in] field3
                                WholeArrayIn,  // [in] field4
                                WholeArrayOut, // [out] field1
                                WholeArrayOut, // [out] field2
                                WholeArrayOut, // [out] field3
                                WholeArrayOut  // [out] field4
  );
  using ExecutionSignature = void(InputIndex,
                                  _1, // [in] indices
                                  _2, // [in] field1
                                  _3, // [in] field2
                                  _4, // [in] field3
                                  _5, // [in] field4
                                  _6, // [out] field1
                                  _7, // [out] field2
                                  _8, // [out] field3
                                  _9  // [out] field4
  );

  template <typename FieldPortalType, typename OutFieldPortalType>
  VISKORES_EXEC void operator()(const viskores::Id idx,
                                const viskores::Id4 indices,
                                const FieldPortalType& field1,
                                const FieldPortalType& field2,
                                const FieldPortalType& field3,
                                const FieldPortalType& field4,
                                OutFieldPortalType& outF1,
                                OutFieldPortalType& outF2,
                                OutFieldPortalType& outF3,
                                OutFieldPortalType& outF4) const
  {
    const auto i0 = indices[1];
    const auto i1 = indices[2];
    const auto i2 = indices[3];
    if (this->PopulateField1)
    {
      outF1.Set(3 * idx + 0, static_cast<viskores::Float32>(field1.Get(i0)));
      outF1.Set(3 * idx + 1, static_cast<viskores::Float32>(field1.Get(i1)));
      outF1.Set(3 * idx + 2, static_cast<viskores::Float32>(field1.Get(i2)));
    }
    if (this->PopulateField2)
    {
      outF2.Set(3 * idx + 0, static_cast<viskores::Float32>(field2.Get(i0)));
      outF2.Set(3 * idx + 1, static_cast<viskores::Float32>(field2.Get(i1)));
      outF2.Set(3 * idx + 2, static_cast<viskores::Float32>(field2.Get(i2)));
    }
    if (this->PopulateField3)
    {
      outF3.Set(3 * idx + 0, static_cast<viskores::Float32>(field3.Get(i0)));
      outF3.Set(3 * idx + 1, static_cast<viskores::Float32>(field3.Get(i1)));
      outF3.Set(3 * idx + 2, static_cast<viskores::Float32>(field3.Get(i2)));
    }
    if (this->PopulateField4)
    {
      outF4.Set(3 * idx + 0, static_cast<viskores::Float32>(field4.Get(i0)));
      outF4.Set(3 * idx + 1, static_cast<viskores::Float32>(field4.Get(i1)));
      outF4.Set(3 * idx + 2, static_cast<viskores::Float32>(field4.Get(i2)));
    }
  }
};

class ExtractTriangleVerticesAndNormals : public viskores::worklet::WorkletMapField
{
public:
  bool ExtractNormals{ false };

  VISKORES_CONT
  ExtractTriangleVerticesAndNormals(bool withNormals)
    : ExtractNormals(withNormals)
  {
  }

  using ControlSignature = void(FieldIn,
                                WholeArrayIn,  // [in] points
                                WholeArrayIn,  // [in] normals
                                WholeArrayOut, // [out] points
                                WholeArrayOut  // [out] normals
  );
  using ExecutionSignature = void(InputIndex,
                                  _1, // [in] indices
                                  _2, // [in] points
                                  _3, // [in] normals
                                  _4, // [out] points
                                  _5  // [out] normals
  );

  template <typename PointPortalType,
            typename NormalPortalType,
            typename OutPointsPortalType,
            typename OutNormalsPortalType>
  VISKORES_EXEC void operator()(const viskores::Id idx,
                                const viskores::Id4 indices,
                                const PointPortalType& points,
                                const NormalPortalType& normals,
                                OutPointsPortalType& outP,
                                OutNormalsPortalType& outN) const
  {
    const auto i0 = indices[1];
    const auto i1 = indices[2];
    const auto i2 = indices[3];
    outP.Set(3 * idx + 0, static_cast<viskores::Vec3f_32>(points.Get(i0)));
    outP.Set(3 * idx + 1, static_cast<viskores::Vec3f_32>(points.Get(i1)));
    outP.Set(3 * idx + 2, static_cast<viskores::Vec3f_32>(points.Get(i2)));
    if (this->ExtractNormals)
    {
      outN.Set(3 * idx + 0, static_cast<viskores::Vec3f_32>(normals.Get(i0)));
      outN.Set(3 * idx + 1, static_cast<viskores::Vec3f_32>(normals.Get(i1)));
      outN.Set(3 * idx + 2, static_cast<viskores::Vec3f_32>(normals.Get(i2)));
    }
  }
};

// Helper functions ///////////////////////////////////////////////////////////

static TriangleFieldArrays UnpackFields(viskores::cont::ArrayHandle<viskores::Id4> tris,
                                        FieldSet fields,
                                        viskores::Range range)
{
  TriangleFieldArrays retval;

  const auto numTris = tris.GetNumberOfValues();

  auto isFieldEmpty = [](const viskores::cont::Field& f) -> bool
  { return f.GetNumberOfValues() == 0 || f.GetData().GetNumberOfComponentsFlat() != 1; };

  const bool emptyField1 = isFieldEmpty(fields[0]);
  const bool emptyField2 = isFieldEmpty(fields[1]);
  const bool emptyField3 = isFieldEmpty(fields[2]);
  const bool emptyField4 = isFieldEmpty(fields[3]);

  viskores::cont::ArrayHandle<viskores::Float32> floatField1;
  viskores::cont::ArrayHandle<viskores::Float32> floatField2;
  viskores::cont::ArrayHandle<viskores::Float32> floatField3;
  viskores::cont::ArrayHandle<viskores::Float32> floatField4;

  if (!emptyField1)
    viskores::cont::ArrayCopyShallowIfPossible(fields[0].GetData(), floatField1);
  if (!emptyField2)
    viskores::cont::ArrayCopyShallowIfPossible(fields[1].GetData(), floatField2);
  if (!emptyField3)
    viskores::cont::ArrayCopyShallowIfPossible(fields[2].GetData(), floatField3);
  if (!emptyField4)
    viskores::cont::ArrayCopyShallowIfPossible(fields[3].GetData(), floatField4);

  viskores::Range field1Range = range;
  viskores::Range field2Range = range;
  viskores::Range field3Range = range;
  viskores::Range field4Range = range;

  if (!emptyField1)
  {
    retval.Field1.Allocate(numTris * 3);
    retval.Field1Name = fields[0].GetName();
  }
  if (!emptyField2)
  {
    retval.Field2.Allocate(numTris * 3);
    retval.Field2Name = fields[1].GetName();
  }
  if (!emptyField3)
  {
    retval.Field3.Allocate(numTris * 3);
    retval.Field3Name = fields[2].GetName();
  }
  if (!emptyField4)
  {
    retval.Field4.Allocate(numTris * 3);
    retval.Field4Name = fields[3].GetName();
  }

  ExtractTriangleFields fieldsWorklet(emptyField1,
                                      emptyField2,
                                      emptyField3,
                                      emptyField4,
                                      field1Range,
                                      field2Range,
                                      field3Range,
                                      field4Range);
  viskores::worklet::DispatcherMapField<ExtractTriangleFields>(fieldsWorklet)
    .Invoke(tris,
            floatField1,
            floatField2,
            floatField3,
            floatField4,
            retval.Field1,
            retval.Field2,
            retval.Field3,
            retval.Field4);

  return retval;
}

static TriangleArrays UnpackTriangles(viskores::cont::ArrayHandle<viskores::Id4> tris,
                                      viskores::cont::CoordinateSystem coords,
                                      viskores::cont::ArrayHandle<viskores::Vec3f_32> normals)
{
  TriangleArrays retval;

  const auto numTris = tris.GetNumberOfValues();

  bool extractNormals = normals.GetNumberOfValues() != 0;

  retval.Vertices.Allocate(numTris * 3);
  if (extractNormals)
    retval.Normals.Allocate(numTris * 3);

  ExtractTriangleVerticesAndNormals worklet(extractNormals);
  viskores::worklet::DispatcherMapField<ExtractTriangleVerticesAndNormals>(worklet).Invoke(
    tris, coords, normals, retval.Vertices, retval.Normals);

  return retval;
}

// ANARIMapperTriangles definitions ///////////////////////////////////////////

ANARIMapperTriangles::ANARIMapperTriangles(anari_cpp::Device device,
                                           const ANARIActor& actor,
                                           const std::string& name,
                                           const viskores::cont::ColorTable& colorTable)
  : ANARIMapper(device, actor, name, colorTable)
{
  this->Handles = std::make_shared<ANARIMapperTriangles::ANARIHandles>();
  this->Handles->Device = device;
  auto& attributes = this->Handles->Parameters.Vertex.Attribute;
  std::fill(attributes.begin(), attributes.end(), nullptr);
  anari_cpp::retain(device, device);
}

ANARIMapperTriangles::~ANARIMapperTriangles()
{
  // ensure ANARI handles are released before host memory goes away
  this->Handles.reset();
}

void ANARIMapperTriangles::SetActor(const ANARIActor& actor)
{
  this->ANARIMapper::SetActor(actor);
  this->ConstructArrays(true);
  this->UpdateMaterial();
}

void ANARIMapperTriangles::SetMapFieldAsAttribute(bool enabled)
{
  this->ANARIMapper::SetMapFieldAsAttribute(enabled);
  this->UpdateGeometry();
  this->UpdateMaterial();
}

void ANARIMapperTriangles::SetANARIColorMap(anari_cpp::Array1D color,
                                            anari_cpp::Array1D opacity,
                                            bool releaseArrays)
{
  this->GetANARISurface();
  auto d = this->GetDevice();
  auto s = this->Handles->Sampler;
  anari_cpp::setParameter(d, s, "image", color);
  anari_cpp::commitParameters(d, s);
  this->ANARIMapper::SetANARIColorMap(color, opacity, releaseArrays);
}

void ANARIMapperTriangles::SetANARIColorMapValueRange(const viskores::Vec2f_32& valueRange)
{
  this->GetANARISurface();
  auto s = this->Handles->Sampler;
  auto d = this->GetDevice();
  auto scale = anari_cpp::scaling_matrix(anari_cpp::float3(1.f / (valueRange[1] - valueRange[0])));
  auto translation = anari_cpp::translation_matrix(anari_cpp::float3(-valueRange[0], 0, 0));
  anari_cpp::setParameter(d, s, "inTransform", anari_cpp::mul(scale, translation));
  anari_cpp::setParameter(d, s, "outTransform", anari_cpp::math::mat4(anari_cpp::identity));
  anari_cpp::setParameter(d, s, "inOffset", viskores::Vec4f_32(0.f, 0.f, 0.f, 0.f));
  anari_cpp::setParameter(d, s, "outOffset", viskores::Vec4f_32(0.f, 0.f, 0.f, 0.f));
  anari_cpp::commitParameters(d, s);
}

void ANARIMapperTriangles::SetCalculateNormals(bool enabled)
{
  this->CalculateNormals = enabled;
}

anari_cpp::Geometry ANARIMapperTriangles::GetANARIGeometry()
{
  if (this->Handles->Geometry)
    return this->Handles->Geometry;

  auto d = this->GetDevice();
  this->Handles->Geometry = anari_cpp::newObject<anari_cpp::Geometry>(d, "triangle");
  this->ConstructArrays();
  this->UpdateGeometry();
  return this->Handles->Geometry;
}

anari_cpp::Surface ANARIMapperTriangles::GetANARISurface()
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
  anari_cpp::setParameter(d, s, "filter", "nearest");
  anari_cpp::setParameter(d, s, "wrapMode", "clampToEdge");
  anari_cpp::setParameter(d, s, "name", this->MakeObjectName("colormap"));
  anari_cpp::setParameter(d, s, "inTransform", anari_cpp::mat4(anari_cpp::identity));
  anari_cpp::setParameter(d, s, "outTransform", anari_cpp::math::mat4(anari_cpp::identity));
  anari_cpp::setParameter(d, s, "inOffset", viskores::Vec4f_32(0.f, 0.f, 0.f, 0.f));
  anari_cpp::setParameter(d, s, "outOffset", viskores::Vec4f_32(0.f, 0.f, 0.f, 0.f));
  anari_cpp::commitParameters(d, s);

  this->SetANARIColorMapValueRange(viskores::Vec2f_32(0.f, 10.f));

  this->UpdateMaterial();

  anari_cpp::setParameter(d, this->Handles->Surface, "name", this->MakeObjectName("surface"));
  anari_cpp::setParameter(d, this->Handles->Surface, "geometry", this->GetANARIGeometry());
  anari_cpp::setParameter(d, this->Handles->Surface, "material", this->Handles->Material);
  anari_cpp::commitParameters(d, this->Handles->Surface);

  return this->Handles->Surface;
}

bool ANARIMapperTriangles::NeedToGenerateData() const
{
  const bool haveNormals = this->Handles->Parameters.Vertex.Normal != nullptr;
  const bool needNormals = this->CalculateNormals && !haveNormals;
  return !this->Current || needNormals;
}

void ANARIMapperTriangles::ConstructArrays(bool regenerate)
{
  if (regenerate)
    this->Current = false;

  if (!regenerate && !this->NeedToGenerateData())
    return;

  this->Current = true;
  this->Valid = false;

  this->Handles->ReleaseArrays();

  const auto& actor = this->GetActor();
  const auto& cells = actor.GetCellSet();

  if (cells.GetNumberOfCells() == 0)
  {
    this->RefreshGroup();
    return;
  }

  viskores::rendering::raytracing::TriangleExtractor triExtractor;
  triExtractor.ExtractCells(cells);

  if (triExtractor.GetNumberOfTriangles() == 0)
  {
    this->RefreshGroup();
    return;
  }

  viskores::cont::ArrayHandle<viskores::Vec3f_32> inNormals;

  if (this->CalculateNormals)
  {
    viskores::filter::vector_analysis::SurfaceNormals normalsFilter;
    normalsFilter.SetOutputFieldName("Normals");
    auto dataset = normalsFilter.Execute(actor.MakeDataSet());
    auto field = dataset.GetField("Normals");
    auto fieldArray = field.GetData();
    viskores::cont::ArrayCopyShallowIfPossible(fieldArray, inNormals);
  }

  auto tris = triExtractor.GetTriangles();

  auto arrays = UnpackTriangles(tris, actor.GetCoordinateSystem(), inNormals);
  auto fieldArrays = UnpackFields(tris, actor.GetFieldSet(), this->GetColorTable().GetRange());

  this->PrimaryField = actor.GetPrimaryFieldIndex();

  auto numVerts = arrays.Vertices.GetNumberOfValues();

  auto* v = (viskores::Vec3f_32*)arrays.Vertices.GetBuffers()[0].ReadPointerHost(*arrays.Token);
  auto* n = (viskores::Vec3f_32*)arrays.Normals.GetBuffers()[0].ReadPointerHost(*arrays.Token);

  auto d = this->GetDevice();
  this->Handles->Parameters.NumPrimitives = numVerts / 3;
  this->Handles->Parameters.Vertex.Position =
    anari_cpp::newArray1D(d, v, NoopANARIDeleter, nullptr, numVerts);

  if (fieldArrays.Field1.GetNumberOfValues() != 0)
  {
    auto* a = (float*)fieldArrays.Field1.GetBuffers()[0].ReadPointerHost(*fieldArrays.Token);
    this->Handles->Parameters.Vertex.Attribute[0] =
      anari_cpp::newArray1D(d, a, NoopANARIDeleter, nullptr, numVerts);
    this->Handles->Parameters.Vertex.AttributeName[0] = fieldArrays.Field1Name;
  }
  if (fieldArrays.Field2.GetNumberOfValues() != 0)
  {
    auto* a = (float*)fieldArrays.Field2.GetBuffers()[0].ReadPointerHost(*fieldArrays.Token);
    this->Handles->Parameters.Vertex.Attribute[1] =
      anari_cpp::newArray1D(d, a, NoopANARIDeleter, nullptr, numVerts);
    this->Handles->Parameters.Vertex.AttributeName[1] = fieldArrays.Field2Name;
  }
  if (fieldArrays.Field3.GetNumberOfValues() != 0)
  {
    auto* a = (float*)fieldArrays.Field3.GetBuffers()[0].ReadPointerHost(*fieldArrays.Token);
    this->Handles->Parameters.Vertex.Attribute[2] =
      anari_cpp::newArray1D(d, a, NoopANARIDeleter, nullptr, numVerts);
    this->Handles->Parameters.Vertex.AttributeName[2] = fieldArrays.Field3Name;
  }
  if (fieldArrays.Field4.GetNumberOfValues() != 0)
  {
    auto* a = (float*)fieldArrays.Field4.GetBuffers()[0].ReadPointerHost(*fieldArrays.Token);
    this->Handles->Parameters.Vertex.Attribute[3] =
      anari_cpp::newArray1D(d, a, NoopANARIDeleter, nullptr, numVerts);
    this->Handles->Parameters.Vertex.AttributeName[3] = fieldArrays.Field4Name;
  }
  if (this->CalculateNormals)
  {
    this->Handles->Parameters.Vertex.Normal =
      anari_cpp::newArray1D(d, n, NoopANARIDeleter, nullptr, arrays.Normals.GetNumberOfValues());
  }

  // NOTE: usd device requires indices, but shouldn't
  {
    auto indexArray =
      anari_cpp::newArray1D(d, ANARI_UINT32_VEC3, this->Handles->Parameters.NumPrimitives);
    auto* begin = anari_cpp::map<unsigned int>(d, indexArray);
    auto* end = begin + numVerts;
    std::iota(begin, end, 0);
    anari_cpp::unmap(d, indexArray);
    this->Handles->Parameters.Primitive.Index = indexArray;
  }

  this->UpdateGeometry();
  this->UpdateMaterial();

  this->Arrays = arrays;
  this->FieldArrays = fieldArrays;
  this->Valid = true;

  this->RefreshGroup();
}

void ANARIMapperTriangles::UpdateGeometry()
{
  if (!this->Handles->Geometry)
    return;

  auto d = this->GetDevice();

  anari_cpp::unsetParameter(d, this->Handles->Geometry, "vertex.position");
  anari_cpp::unsetParameter(d, this->Handles->Geometry, "vertex.attribute0");
  anari_cpp::unsetParameter(d, this->Handles->Geometry, "vertex.attribute1");
  anari_cpp::unsetParameter(d, this->Handles->Geometry, "vertex.attribute2");
  anari_cpp::unsetParameter(d, this->Handles->Geometry, "vertex.attribute3");
  anari_cpp::unsetParameter(d, this->Handles->Geometry, "vertex.normal");
  anari_cpp::unsetParameter(d, this->Handles->Geometry, "primitive.index");
  anari_cpp::unsetParameter(d, this->Handles->Geometry, "usd::attribute0.name");
  anari_cpp::unsetParameter(d, this->Handles->Geometry, "usd::attribute1.name");
  anari_cpp::unsetParameter(d, this->Handles->Geometry, "usd::attribute2.name");
  anari_cpp::unsetParameter(d, this->Handles->Geometry, "usd::attribute3.name");

  anari_cpp::setParameter(d, this->Handles->Geometry, "name", this->MakeObjectName("geometry"));

  if (this->Handles->Parameters.Vertex.Position)
  {
    anari_cpp::setParameter(
      d, this->Handles->Geometry, "vertex.position", this->Handles->Parameters.Vertex.Position);
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

    if (CalculateNormals)
    {
      anari_cpp::setParameter(
        d, this->Handles->Geometry, "vertex.normal", this->Handles->Parameters.Vertex.Normal);
    }
    anari_cpp::setParameter(
      d, this->Handles->Geometry, "primitive.index", this->Handles->Parameters.Primitive.Index);
  }

  anari_cpp::commitParameters(d, this->Handles->Geometry);
}

void ANARIMapperTriangles::UpdateMaterial()
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

ANARIMapperTriangles::ANARIHandles::~ANARIHandles()
{
  this->ReleaseArrays();
  anari_cpp::release(this->Device, this->Surface);
  anari_cpp::release(this->Device, this->Material);
  anari_cpp::release(this->Device, this->Sampler);
  anari_cpp::release(this->Device, this->Geometry);
  anari_cpp::release(this->Device, this->Device);
}

void ANARIMapperTriangles::ANARIHandles::ReleaseArrays()
{
  anari_cpp::release(this->Device, this->Parameters.Vertex.Position);
  anari_cpp::release(this->Device, this->Parameters.Vertex.Normal);
  anari_cpp::release(this->Device, this->Parameters.Vertex.Attribute[0]);
  anari_cpp::release(this->Device, this->Parameters.Vertex.Attribute[1]);
  anari_cpp::release(this->Device, this->Parameters.Vertex.Attribute[2]);
  anari_cpp::release(this->Device, this->Parameters.Vertex.Attribute[3]);
  anari_cpp::release(this->Device, this->Parameters.Primitive.Index);
  this->Parameters.Vertex.Position = nullptr;
  this->Parameters.Vertex.Normal = nullptr;
  this->Parameters.Vertex.Attribute[0] = nullptr;
  this->Parameters.Vertex.Attribute[1] = nullptr;
  this->Parameters.Vertex.Attribute[2] = nullptr;
  this->Parameters.Vertex.Attribute[3] = nullptr;
  this->Parameters.Primitive.Index = nullptr;
}

} // namespace anari
} // namespace interop
} // namespace viskores
