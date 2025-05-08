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
#ifndef viskores_rendering_MapperGlyphBase_h
#define viskores_rendering_MapperGlyphBase_h

#include <viskores/Deprecated.h>
#include <viskores/cont/ColorTable.h>
#include <viskores/rendering/Camera.h>
#include <viskores/rendering/Mapper.h>

#include <memory>

namespace viskores
{
namespace rendering
{

class CanvasRayTracer;

/// @brief Base class for glyph mappers.
///
/// Glyph mappers place 3D icons at various places in the mesh. The icons are
/// placed based on the location of points or cells in the mesh.
class VISKORES_RENDERING_EXPORT MapperGlyphBase : public Mapper
{
public:
  MapperGlyphBase();

  virtual ~MapperGlyphBase();

  void SetCanvas(viskores::rendering::Canvas* canvas) override;
  virtual viskores::rendering::Canvas* GetCanvas() const override;

  /// @brief Specify the elements the glyphs will be associated with.
  ///
  /// The glyph mapper will place glyphs over locations specified by either the points
  /// or the cells of a mesh. The glyph may also be oriented by a scalar field with the
  /// same association.
  virtual viskores::cont::Field::Association GetAssociation() const;
  /// @copydoc GetAssociation
  virtual void SetAssociation(viskores::cont::Field::Association association);
  /// @copydoc GetAssociation
  virtual bool GetUseCells() const;
  /// @copydoc GetAssociation
  virtual void SetUseCells();
  /// @copydoc GetAssociation
  virtual bool GetUsePoints() const;
  /// @copydoc GetAssociation
  virtual void SetUsePoints();
  VISKORES_DEPRECATED(2.2, "Use GetUsePoints() or GetAssociation().")
  virtual bool GetUseNodes() const;
  VISKORES_DEPRECATED(2.2, "Use SetUsePoints() or SetAssociation().")
  virtual void SetUseNodes();

  // These options do not seem to be supported yet.
  // I'm not sure why you would need UseStride. Just use Stride = 1.
  virtual bool GetUseStride() const;
  virtual void SetUseStride(bool on);
  virtual viskores::Id GetStride() const;
  virtual void SetStride(viskores::Id stride);

  /// @brief Specify the size of each glyph (before scaling).
  ///
  /// If the base size is not set to a positive value, it is automatically sized with a heuristic
  /// based off the bounds of the geometry.
  virtual viskores::Float32 GetBaseSize() const;
  /// @copydoc GetBaseSize
  virtual void SetBaseSize(viskores::Float32 size);

  /// @brief Specify whether to scale the glyphs by a field.
  virtual bool GetScaleByValue() const;
  /// @copydoc GetScaleByValue
  virtual void SetScaleByValue(bool on);

  /// @brief Specify the range of values to scale the glyphs.
  ///
  /// When `ScaleByValue` is on, the glyphs will be scaled proportionally to the field
  /// magnitude. The `ScaleDelta` determines how big and small they get. For a `ScaleDelta`
  /// of one, the smallest field values will have glyphs of zero size and the maximum field
  /// values will be twice the base size. A `ScaleDelta` of 0.5 will result in glyphs sized
  /// in the range of 0.5 times the base size to 1.5 times the base size. `ScaleDelta` outside
  /// the range [0, 1] is undefined.
  virtual viskores::Float32 GetScaleDelta() const;
  /// @copydoc GetScaleDelta
  virtual void SetScaleDelta(viskores::Float32 delta);

  virtual void SetCompositeBackground(bool on);

protected:
  virtual viskores::cont::DataSet FilterPoints(const viskores::cont::UnknownCellSet& cellSet,
                                               const viskores::cont::CoordinateSystem& coords,
                                               const viskores::cont::Field& scalarField) const;


  viskores::rendering::CanvasRayTracer* Canvas = nullptr;
  bool CompositeBackground = true;

  viskores::cont::Field::Association Association = viskores::cont::Field::Association::Points;

  bool UseStride = false;
  viskores::Id Stride = 1;

  bool ScaleByValue = false;
  viskores::Float32 BaseSize = -1.f;
  viskores::Float32 ScaleDelta = 0.5f;
};
}
} //namespace viskores::rendering

#endif //viskores_rendering_MapperGlyphBase_h
