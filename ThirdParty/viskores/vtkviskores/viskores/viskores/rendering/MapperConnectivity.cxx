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
#include <viskores/rendering/CanvasRayTracer.h>

#include <viskores/rendering/ConnectivityProxy.h>
#include <viskores/rendering/Mapper.h>
#include <viskores/rendering/MapperConnectivity.h>
#include <viskores/rendering/View.h>

#include <cstdlib>
#include <viskores/rendering/raytracing/Camera.h>

namespace viskores
{
namespace rendering
{

VISKORES_CONT
MapperConnectivity::MapperConnectivity()
{
  CanvasRT = nullptr;
  SampleDistance = -1;
}

VISKORES_CONT
MapperConnectivity::~MapperConnectivity() {}

VISKORES_CONT
void MapperConnectivity::SetSampleDistance(const viskores::Float32& distance)
{
  SampleDistance = distance;
}

VISKORES_CONT
void MapperConnectivity::SetCanvas(Canvas* canvas)
{
  if (canvas != nullptr)
  {

    CanvasRT = dynamic_cast<CanvasRayTracer*>(canvas);
    if (CanvasRT == nullptr)
    {
      throw viskores::cont::ErrorBadValue(
        "Volume Render: bad canvas type. Must be CanvasRayTracer");
    }
  }
}

viskores::rendering::Canvas* MapperConnectivity::GetCanvas() const
{
  return CanvasRT;
}


VISKORES_CONT
void MapperConnectivity::RenderCellsImpl(
  const viskores::cont::UnknownCellSet& cellset,
  const viskores::cont::CoordinateSystem& coords,
  const viskores::cont::Field& scalarField,
  const viskores::cont::ColorTable& viskoresNotUsed(colorTable),
  const viskores::rendering::Camera& camera,
  const viskores::Range& scalarRange,
  const viskores::cont::Field& ghostField)
{
  viskores::cont::DataSet dataset;

  dataset.SetCellSet(cellset);
  dataset.AddCoordinateSystem(coords);
  dataset.AddField(scalarField);
  dataset.AddField(ghostField);

  viskores::rendering::ConnectivityProxy tracerProxy(dataset, scalarField.GetName());

  if (SampleDistance == -1.f)
  {
    // set a default distance
    viskores::Bounds bounds = coords.GetBounds();
    viskores::Float64 x2 = bounds.X.Length() * bounds.X.Length();
    viskores::Float64 y2 = bounds.Y.Length() * bounds.Y.Length();
    viskores::Float64 z2 = bounds.Z.Length() * bounds.Z.Length();
    viskores::Float64 length = viskores::Sqrt(x2 + y2 + z2);
    constexpr viskores::Float64 defaultSamples = 200.;
    SampleDistance = static_cast<viskores::Float32>(length / defaultSamples);
  }
  tracerProxy.SetScalarRange(scalarRange);
  tracerProxy.SetSampleDistance(SampleDistance);
  tracerProxy.SetColorMap(ColorMap);
  tracerProxy.Trace(camera, CanvasRT);
}

viskores::rendering::Mapper* MapperConnectivity::NewCopy() const
{
  return new viskores::rendering::MapperConnectivity(*this);
}
}
} // namespace viskores::rendering
