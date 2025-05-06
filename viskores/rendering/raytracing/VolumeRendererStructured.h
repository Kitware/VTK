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
#ifndef viskores_rendering_raytracing_VolumeRendererStructured_h
#define viskores_rendering_raytracing_VolumeRendererStructured_h

#include <viskores/cont/DataSet.h>

#include <viskores/rendering/raytracing/Ray.h>
#include <viskores/rendering/viskores_rendering_export.h>

namespace viskores
{
namespace rendering
{
namespace raytracing
{

class VISKORES_RENDERING_EXPORT VolumeRendererStructured
{
public:
  VISKORES_CONT
  void SetColorMap(const viskores::cont::ArrayHandle<viskores::Vec4f_32>& colorMap);

  VISKORES_CONT
  void SetData(const viskores::cont::CoordinateSystem& coords,
               const viskores::cont::Field& scalarField,
               const viskores::cont::CellSetStructured<3>& cellset,
               const viskores::Range& scalarRange);


  VISKORES_CONT
  void Render(viskores::rendering::raytracing::Ray<viskores::Float32>& rays);
  //VISKORES_CONT
  ///void Render(viskores::rendering::raytracing::Ray<viskores::Float64>& rays);


  VISKORES_CONT
  void SetSampleDistance(const viskores::Float32& distance);

protected:
  template <typename Precision, typename Device>
  VISKORES_CONT void RenderOnDevice(viskores::rendering::raytracing::Ray<Precision>& rays, Device);

  bool IsSceneDirty = false;
  bool IsUniformDataSet = true;
  viskores::Bounds SpatialExtent;
  viskores::cont::CoordinateSystem Coordinates;
  viskores::cont::CellSetStructured<3> Cellset;
  const viskores::cont::Field* ScalarField;
  viskores::cont::ArrayHandle<viskores::Vec4f_32> ColorMap;
  viskores::Float32 SampleDistance = -1.f;
  viskores::Range ScalarRange;
};
}
}
} //namespace viskores::rendering::raytracing
#endif
