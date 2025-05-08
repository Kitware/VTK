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

#ifndef viskores_interop_anari_ANARIMapperTriangles_h
#define viskores_interop_anari_ANARIMapperTriangles_h

#include <viskores/interop/anari/ANARIMapper.h>

namespace viskores
{
namespace interop
{
namespace anari
{

/// @brief Raw ANARI arrays and parameter values set on the `ANARIGeometry`.
///
struct TrianglesParameters
{
  struct VertexData
  {
    anari_cpp::Array1D Position{ nullptr };
    anari_cpp::Array1D Normal{ nullptr };
    std::array<anari_cpp::Array1D, 4> Attribute;
    std::array<std::string, 4> AttributeName;
  } Vertex{};

  struct PrimitiveData
  {
    anari_cpp::Array1D Index{ nullptr };
  } Primitive{};

  unsigned int NumPrimitives{ 0 };
};

/// @brief Viskores data arrays underlying the `ANARIArray` handles created by the mapper.
///
struct TriangleArrays
{
  viskores::cont::ArrayHandle<viskores::Vec3f_32> Vertices;
  viskores::cont::ArrayHandle<viskores::Vec3f_32> Normals;
  std::shared_ptr<viskores::cont::Token> Token{ new viskores::cont::Token };
};

/// @brief Viskores data arrays underlying the `ANARIArray` handles created by the mapper for field attributes.
///
struct TriangleFieldArrays
{
  viskores::cont::ArrayHandle<viskores::Float32> Field1;
  std::string Field1Name;
  viskores::cont::ArrayHandle<viskores::Float32> Field2;
  std::string Field2Name;
  viskores::cont::ArrayHandle<viskores::Float32> Field3;
  std::string Field3Name;
  viskores::cont::ArrayHandle<viskores::Float32> Field4;
  std::string Field4Name;
  std::shared_ptr<viskores::cont::Token> Token{ new viskores::cont::Token };
};

/// @brief Mapper which triangulates cells into ANARI `triangle` geometry.
///
/// NOTE: This mapper will only color map values that are 1-component Float32
/// fields, otherwise they will be ignored. This restriction will allow 2-4
/// component fields in the future.
struct VISKORES_ANARI_EXPORT ANARIMapperTriangles : public ANARIMapper
{
  /// @brief Constructor
  ///
  ANARIMapperTriangles(
    anari_cpp::Device device,
    const ANARIActor& actor = {},
    const std::string& name = "<triangles>",
    const viskores::cont::ColorTable& colorTable = viskores::cont::ColorTable::Preset::Default);

  /// @brief Destructor
  ///
  ~ANARIMapperTriangles() override;

  /// @brief Set the current actor on this mapper.
  ///
  /// See `ANARIMapper` for more detail.
  void SetActor(const ANARIActor& actor) override;

  /// @brief Set whether fields from `ANARIActor` should end up as geometry attributes.
  ///
  /// See `ANARIMapper` for more detail.
  void SetMapFieldAsAttribute(bool enabled) override;

  /// @brief Set color map arrays using raw ANARI array handles.
  ///
  /// See `ANARIMapper` for more detail.
  void SetANARIColorMap(anari_cpp::Array1D color,
                        anari_cpp::Array1D opacity,
                        bool releaseArrays = true) override;

  /// @brief Set the value range (domain) for the color map.
  ///
  void SetANARIColorMapValueRange(const viskores::Vec2f_32& valueRange) override;

  /// @brief Set whether `vertex.normal` data should also be calculated when triangulating geometry.
  ///
  void SetCalculateNormals(bool enabled);

  anari_cpp::Geometry GetANARIGeometry() override;
  anari_cpp::Surface GetANARISurface() override;

private:
  bool NeedToGenerateData() const;
  /// @brief Do the work to construct the basic ANARI arrays for the ANARIGeometry.
  /// @param regenerate Force the position/radius arrays are regenerated.
  ///
  void ConstructArrays(bool regenerate = false);
  /// @brief Update ANARIGeometry object with the latest data from the actor.
  void UpdateGeometry();
  /// @brief Update ANARIMaterial object with the latest data from the actor.
  void UpdateMaterial();

  /// @brief Container of all relevant ANARI scene object handles.
  struct ANARIHandles
  {
    anari_cpp::Device Device{ nullptr };
    anari_cpp::Geometry Geometry{ nullptr };
    anari_cpp::Sampler Sampler{ nullptr };
    anari_cpp::Material Material{ nullptr };
    anari_cpp::Surface Surface{ nullptr };
    TrianglesParameters Parameters;
    ~ANARIHandles();
    void ReleaseArrays();
  };

  std::shared_ptr<ANARIHandles> Handles;

  bool CalculateNormals{ false };
  viskores::IdComponent PrimaryField{ 0 };
  TriangleArrays Arrays;
  TriangleFieldArrays FieldArrays;
};

} // namespace anari
} // namespace interop
} // namespace viskores

#endif
