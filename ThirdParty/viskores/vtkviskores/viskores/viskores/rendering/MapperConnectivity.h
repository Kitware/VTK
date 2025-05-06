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
#ifndef viskores_rendering_MapperConnectivity_h
#define viskores_rendering_MapperConnectivity_h

#include <viskores/cont/ColorTable.h>
#include <viskores/rendering/CanvasRayTracer.h>
#include <viskores/rendering/Mapper.h>
#include <viskores/rendering/View.h>

namespace viskores
{
namespace rendering
{

class VISKORES_RENDERING_EXPORT MapperConnectivity : public Mapper
{
public:
  MapperConnectivity();

  ~MapperConnectivity();
  void SetSampleDistance(const viskores::Float32&);
  void SetCanvas(viskores::rendering::Canvas* canvas) override;
  virtual viskores::rendering::Canvas* GetCanvas() const override;

  viskores::rendering::Mapper* NewCopy() const override;
  void CreateDefaultView();

protected:
  viskores::Float32 SampleDistance;
  CanvasRayTracer* CanvasRT;
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
#endif //viskores_rendering_SceneRendererVolume_h
