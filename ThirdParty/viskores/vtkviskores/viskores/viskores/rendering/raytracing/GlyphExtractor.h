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
#ifndef viskores_rendering_raytracing_Glyph_Extractor_h
#define viskores_rendering_raytracing_Glyph_Extractor_h

#include <viskores/cont/DataSet.h>

namespace viskores
{
namespace rendering
{
namespace raytracing
{

class GlyphExtractor
{
public:
  GlyphExtractor();

  //
  // Extract all nodes using a constant size
  //
  void ExtractCoordinates(const viskores::cont::CoordinateSystem& coords,
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
  void ExtractCells(const viskores::cont::UnknownCellSet& cells, viskores::Float32 size);

  //
  // Extract all vertex elements with size based on scalar values
  //
  void ExtractCells(const viskores::cont::UnknownCellSet& cells,
                    const viskores::cont::Field& field,
                    const viskores::Float32 minSize,
                    const viskores::Float32 maxSize);


  viskores::cont::ArrayHandle<viskores::Id> GetPointIds();
  viskores::cont::ArrayHandle<viskores::Float32> GetSizes();

  viskores::Id GetNumberOfGlyphs() const;

protected:
  void SetUniformSize(const viskores::Float32 size);
  void SetVaryingSize(const viskores::Float32 minSize,
                      const viskores::Float32 maxSize,
                      const viskores::cont::Field& field);

  void SetPointIdsFromCoords(const viskores::cont::CoordinateSystem& coords);
  void SetPointIdsFromCells(const viskores::cont::UnknownCellSet& cells);

  viskores::cont::ArrayHandle<viskores::Id> PointIds;
  viskores::cont::ArrayHandle<viskores::Float32> Sizes;
}; // class GlyphExtractor
}
}
} //namespace viskores::rendering::raytracing
#endif //viskores_rendering_raytracing_Glyph_Extractor_h
