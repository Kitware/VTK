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
#ifndef viskores_rendering_MapperGlyphScalar_h
#define viskores_rendering_MapperGlyphScalar_h

#include <viskores/rendering/GlyphType.h>
#include <viskores/rendering/MapperGlyphBase.h>

namespace viskores
{
namespace rendering
{

/// @brief A mapper that produces unoriented glyphs.
///
/// This mapper is meant to be used with scalar fields. The glyphs can be optionally
/// sized based on the field.
class VISKORES_RENDERING_EXPORT MapperGlyphScalar : public viskores::rendering::MapperGlyphBase
{
public:
  MapperGlyphScalar();

  ~MapperGlyphScalar();

  /// @brief Specify the shape of the glyphs.
  viskores::rendering::GlyphType GetGlyphType() const;
  /// @copydoc GetGlyphType
  void SetGlyphType(viskores::rendering::GlyphType glyphType);

  viskores::rendering::Mapper* NewCopy() const override;

protected:
  viskores::rendering::GlyphType GlyphType;

  void RenderCellsImpl(const viskores::cont::UnknownCellSet& cellset,
                       const viskores::cont::CoordinateSystem& coords,
                       const viskores::cont::Field& scalarField,
                       const viskores::cont::ColorTable& colorTable,
                       const viskores::rendering::Camera& camera,
                       const viskores::Range& scalarRange,
                       const viskores::cont::Field& ghostField) override;
};
}
} //namespace viskores::rendering

#endif //viskores_rendering_MapperGlyphScalar_h
