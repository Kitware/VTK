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
#ifndef viskores_rendering_MapperRayTracer_h
#define viskores_rendering_MapperRayTracer_h

#include <viskores/cont/ColorTable.h>
#include <viskores/rendering/Camera.h>
#include <viskores/rendering/Mapper.h>

#include <memory>

namespace viskores
{
namespace rendering
{

/// @brief Mapper to render surfaces using ray tracing.
///
/// Provides a "standard" data mapper that uses ray tracing to render the surfaces
/// of `DataSet` objects.
class VISKORES_RENDERING_EXPORT MapperRayTracer : public Mapper
{
public:
  MapperRayTracer();

  ~MapperRayTracer();

  void SetCanvas(viskores::rendering::Canvas* canvas) override;
  virtual viskores::rendering::Canvas* GetCanvas() const override;

  void SetCompositeBackground(bool on);
  viskores::rendering::Mapper* NewCopy() const override;
  void SetShadingOn(bool on);

private:
  struct InternalsType;
  std::shared_ptr<InternalsType> Internals;
  struct CompareIndices;

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

#endif //viskores_rendering_MapperRayTracer_h
