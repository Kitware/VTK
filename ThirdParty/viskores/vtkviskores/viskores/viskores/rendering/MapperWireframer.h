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
#ifndef viskores_rendering_MapperWireframer_h
#define viskores_rendering_MapperWireframer_h

#include <memory>

#include <viskores/cont/ColorTable.h>
#include <viskores/cont/CoordinateSystem.h>
#include <viskores/cont/Field.h>
#include <viskores/cont/UnknownCellSet.h>
#include <viskores/rendering/Camera.h>
#include <viskores/rendering/Canvas.h>
#include <viskores/rendering/Mapper.h>

namespace viskores
{
namespace rendering
{

/// @brief Mapper that renders the edges of a mesh.
///
/// Each edge in the mesh is rendered as a line, which provides a wireframe
/// representation of the data.
class VISKORES_RENDERING_EXPORT MapperWireframer : public Mapper
{
public:
  VISKORES_CONT
  MapperWireframer();
  virtual ~MapperWireframer();

  virtual viskores::rendering::Canvas* GetCanvas() const override;
  virtual void SetCanvas(viskores::rendering::Canvas* canvas) override;

  /// @brief Specify whether to show interior edges.
  ///
  /// When rendering a 3D volume of data, the `MapperWireframer` can show
  /// either the wireframe of the external surface of the data (the default)
  /// or render the entire wireframe including the internal edges.
  bool GetShowInternalZones() const;
  /// @copydoc GetShowInternalZones
  void SetShowInternalZones(bool showInternalZones);
  void SetCompositeBackground(bool on);

  bool GetIsOverlay() const;
  void SetIsOverlay(bool isOverlay);

  virtual viskores::rendering::Mapper* NewCopy() const override;

private:
  struct InternalsType;
  std::shared_ptr<InternalsType> Internals;

  virtual void RenderCellsImpl(const viskores::cont::UnknownCellSet& cellset,
                               const viskores::cont::CoordinateSystem& coords,
                               const viskores::cont::Field& scalarField,
                               const viskores::cont::ColorTable& colorTable,
                               const viskores::rendering::Camera& camera,
                               const viskores::Range& scalarRange,
                               const viskores::cont::Field& ghostField) override;
}; // class MapperWireframer
}
} // namespace viskores::rendering
#endif // viskores_rendering_MapperWireframer_h
