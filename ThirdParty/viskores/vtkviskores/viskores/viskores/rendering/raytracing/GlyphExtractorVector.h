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
#ifndef viskores_rendering_raytracing_Glyph_Extractor_Vector_h
#define viskores_rendering_raytracing_Glyph_Extractor_Vector_h

#include <viskores/cont/DataSet.h>

namespace viskores
{
namespace rendering
{
namespace raytracing
{

class GlyphExtractorVector
{
public:
  GlyphExtractorVector();

  //
  // Extract all nodes using a constant size
  //
  void ExtractCoordinates(const viskores::cont::CoordinateSystem& coords,
                          const viskores::cont::Field& field,
                          const viskores::Float32 size);

  //
  // Set size based on scalar field values. Each is interpolated from min to max
  //
  void ExtractCoordinates(const viskores::cont::CoordinateSystem& coords,
                          const viskores::cont::Field& field,
                          const viskores::Float32 minSize,
                          const viskores::Float32 maxSize);

  //
  // Extract all vertex shapes with constant size
  //
  void ExtractCells(const viskores::cont::UnknownCellSet& cells,
                    const viskores::cont::Field& field,
                    viskores::Float32 size);

  //
  // Extract all vertex elements with size based on scalar values
  //
  void ExtractCells(const viskores::cont::UnknownCellSet& cells,
                    const viskores::cont::Field& field,
                    const viskores::Float32 minSize,
                    const viskores::Float32 maxSize);


  viskores::cont::ArrayHandle<viskores::Id> GetPointIds();
  viskores::cont::ArrayHandle<viskores::Vec<viskores::Float32, 3>> GetSizes();
  viskores::cont::Field GetMagnitudeField();

  viskores::Id GetNumberOfGlyphs() const;

protected:
  void SetUniformSize(const viskores::Float32 size, const viskores::cont::Field& field);
  void SetVaryingSize(const viskores::Float32 minSize,
                      const viskores::Float32 maxSize,
                      const viskores::cont::Field& field);

  void SetPointIdsFromCoords(const viskores::cont::CoordinateSystem& coords);
  void SetPointIdsFromCells(const viskores::cont::UnknownCellSet& cells);

  void ExtractMagnitudeField(const viskores::cont::Field& field);

  viskores::cont::ArrayHandle<viskores::Id> PointIds;
  viskores::cont::ArrayHandle<viskores::Vec<viskores::Float32, 3>> Sizes;
  viskores::cont::Field MagnitudeField;

}; // class GlyphExtractorVector
}
}
} //namespace viskores::rendering::raytracing
#endif //viskores_rendering_raytracing_Glyph_Extractor_Vector_h
