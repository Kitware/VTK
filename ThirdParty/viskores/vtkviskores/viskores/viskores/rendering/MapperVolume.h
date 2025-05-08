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
#ifndef viskores_rendering_MapperVolume_h
#define viskores_rendering_MapperVolume_h

#include <viskores/rendering/Mapper.h>

#include <memory>

namespace viskores
{
namespace rendering
{

/// @brief Mapper that renders a volume as a translucent cloud.
class VISKORES_RENDERING_EXPORT MapperVolume : public Mapper
{
public:
  MapperVolume();

  ~MapperVolume();

  void SetCanvas(viskores::rendering::Canvas* canvas) override;
  virtual viskores::rendering::Canvas* GetCanvas() const override;

  viskores::rendering::Mapper* NewCopy() const override;
  /// @brief Specify how much space is between samples of rays that traverse the volume.
  ///
  /// The volume rendering ray caster finds the entry point of the ray through the volume
  /// and then samples the volume along the direction of the ray at regular intervals.
  /// This parameter specifies how far these samples occur.
  void SetSampleDistance(const viskores::Float32 distance);
  void SetCompositeBackground(const bool compositeBackground);

private:
  struct InternalsType;
  std::shared_ptr<InternalsType> Internals;

  virtual void RenderCellsImpl(const viskores::cont::UnknownCellSet& cellset,
                               const viskores::cont::CoordinateSystem& coords,
                               const viskores::cont::Field& scalarField,
                               const viskores::cont::ColorTable&, //colorTable
                               const viskores::rendering::Camera& camera,
                               const viskores::Range& scalarRange,
                               const viskores::cont::Field& ghostField) override;
};
}
} //namespace viskores::rendering

#endif //viskores_rendering_MapperVolume_h
