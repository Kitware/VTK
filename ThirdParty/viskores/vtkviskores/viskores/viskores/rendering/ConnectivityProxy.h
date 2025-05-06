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
#ifndef viskores_rendering_ConnectivityProxy_h
#define viskores_rendering_ConnectivityProxy_h

#include <memory>
#include <viskores/cont/DataSet.h>
#include <viskores/rendering/CanvasRayTracer.h>
#include <viskores/rendering/Mapper.h>
#include <viskores/rendering/View.h>
#include <viskores/rendering/raytracing/Camera.h>
#include <viskores/rendering/raytracing/PartialComposite.h>
#include <viskores/rendering/raytracing/Ray.h>

namespace viskores
{
namespace rendering
{

using PartialVector64 =
  std::vector<viskores::rendering::raytracing::PartialComposite<viskores::Float64>>;
using PartialVector32 =
  std::vector<viskores::rendering::raytracing::PartialComposite<viskores::Float32>>;

class VISKORES_RENDERING_EXPORT ConnectivityProxy
{
public:
  ConnectivityProxy(const viskores::cont::DataSet& dataset, const std::string& fieldName);

  ConnectivityProxy(const viskores::cont::DataSet& dataSet,
                    const std::string& fieldName,
                    const std::string& coordinateName);

  ConnectivityProxy(const viskores::cont::UnknownCellSet& cellset,
                    const viskores::cont::CoordinateSystem& coords,
                    const viskores::cont::Field& scalarField);

  ConnectivityProxy(const ConnectivityProxy&);
  ConnectivityProxy& operator=(const ConnectivityProxy&);

  ConnectivityProxy(ConnectivityProxy&&) noexcept;
  ConnectivityProxy& operator=(ConnectivityProxy&&) noexcept;

  ~ConnectivityProxy();

  enum struct RenderMode
  {
    Volume,
    Energy,
  };

  void SetRenderMode(RenderMode mode);
  void SetSampleDistance(const viskores::Float32&);
  void SetScalarField(const std::string& fieldName);
  void SetEmissionField(const std::string& fieldName);
  void SetScalarRange(const viskores::Range& range);
  void SetColorMap(viskores::cont::ArrayHandle<viskores::Vec4f_32>& colormap);
  void SetCompositeBackground(bool on);
  void SetDebugPrints(bool on);
  void SetUnitScalar(viskores::Float32 unitScalar);
  void SetEpsilon(viskores::Float64 epsilon); // epsilon for bumping lost rays

  viskores::Bounds GetSpatialBounds();
  viskores::Range GetScalarFieldRange();
  viskores::Range GetScalarRange();

  void Trace(const viskores::rendering::Camera& camera,
             viskores::rendering::CanvasRayTracer* canvas);
  void Trace(viskores::rendering::raytracing::Ray<viskores::Float64>& rays);
  void Trace(viskores::rendering::raytracing::Ray<viskores::Float32>& rays);

  PartialVector64 PartialTrace(viskores::rendering::raytracing::Ray<viskores::Float64>& rays);
  PartialVector32 PartialTrace(viskores::rendering::raytracing::Ray<viskores::Float32>& rays);

protected:
  struct InternalsType;
  std::unique_ptr<InternalsType> Internals;
};
}
} //namespace viskores::rendering
#endif //viskores_rendering_ConnectivityProxy_h
