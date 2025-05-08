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

#include <viskores/rendering/MapperVolume.h>

#include <viskores/cont/Timer.h>
#include <viskores/cont/TryExecute.h>

#include <viskores/rendering/CanvasRayTracer.h>
#include <viskores/rendering/raytracing/Camera.h>
#include <viskores/rendering/raytracing/Logger.h>
#include <viskores/rendering/raytracing/RayOperations.h>
#include <viskores/rendering/raytracing/VolumeRendererStructured.h>

#include <sstream>

#define DEFAULT_SAMPLE_DISTANCE -1.f

namespace viskores
{
namespace rendering
{

struct MapperVolume::InternalsType
{
  viskores::rendering::CanvasRayTracer* Canvas;
  viskores::Float32 SampleDistance;
  bool CompositeBackground;

  VISKORES_CONT
  InternalsType()
    : Canvas(nullptr)
    , SampleDistance(DEFAULT_SAMPLE_DISTANCE)
    , CompositeBackground(true)
  {
  }
};

MapperVolume::MapperVolume()
  : Internals(new InternalsType)
{
}

MapperVolume::~MapperVolume() {}

void MapperVolume::SetCanvas(viskores::rendering::Canvas* canvas)
{
  if (canvas != nullptr)
  {
    this->Internals->Canvas = dynamic_cast<CanvasRayTracer*>(canvas);

    if (this->Internals->Canvas == nullptr)
    {
      throw viskores::cont::ErrorBadValue("Ray Tracer: bad canvas type. Must be CanvasRayTracer");
    }
  }
  else
  {
    this->Internals->Canvas = nullptr;
  }
}

viskores::rendering::Canvas* MapperVolume::GetCanvas() const
{
  return this->Internals->Canvas;
}

void MapperVolume::RenderCellsImpl(const viskores::cont::UnknownCellSet& cellset,
                                   const viskores::cont::CoordinateSystem& coords,
                                   const viskores::cont::Field& scalarField,
                                   const viskores::cont::ColorTable& viskoresNotUsed(colorTable),
                                   const viskores::rendering::Camera& camera,
                                   const viskores::Range& scalarRange,
                                   const viskores::cont::Field& viskoresNotUsed(ghostField))
{
  if (!cellset.CanConvert<viskores::cont::CellSetStructured<3>>())
  {
    std::stringstream msg;
    std::string theType = typeid(cellset).name();
    msg << "Mapper volume: cell set type not currently supported\n";
    msg << "Type : " << theType << std::endl;
    throw viskores::cont::ErrorBadValue(msg.str());
  }
  else
  {
    raytracing::Logger* logger = raytracing::Logger::GetInstance();
    logger->OpenLogEntry("mapper_volume");
    viskores::cont::Timer tot_timer;
    tot_timer.Start();
    viskores::cont::Timer timer;

    viskores::rendering::raytracing::VolumeRendererStructured tracer;

    viskores::rendering::raytracing::Camera rayCamera;
    viskores::Int32 width = (viskores::Int32)this->Internals->Canvas->GetWidth();
    viskores::Int32 height = (viskores::Int32)this->Internals->Canvas->GetHeight();
    rayCamera.SetParameters(camera, width, height);

    viskores::rendering::raytracing::Ray<viskores::Float32> rays;
    rayCamera.CreateRays(rays, coords.GetBounds());
    rays.Buffers.at(0).InitConst(0.f);
    raytracing::RayOperations::MapCanvasToRays(rays, camera, *this->Internals->Canvas);


    if (this->Internals->SampleDistance != DEFAULT_SAMPLE_DISTANCE)
    {
      tracer.SetSampleDistance(this->Internals->SampleDistance);
    }

    tracer.SetData(
      coords, scalarField, cellset.AsCellSet<viskores::cont::CellSetStructured<3>>(), scalarRange);
    tracer.SetColorMap(this->ColorMap);

    tracer.Render(rays);

    timer.Start();
    this->Internals->Canvas->WriteToCanvas(rays, rays.Buffers.at(0).Buffer, camera);

    if (this->Internals->CompositeBackground)
    {
      this->Internals->Canvas->BlendBackground();
    }
    viskores::Float64 time = timer.GetElapsedTime();
    logger->AddLogData("write_to_canvas", time);
    time = tot_timer.GetElapsedTime();
    logger->CloseLogEntry(time);
  }
}

viskores::rendering::Mapper* MapperVolume::NewCopy() const
{
  return new viskores::rendering::MapperVolume(*this);
}

void MapperVolume::SetSampleDistance(const viskores::Float32 sampleDistance)
{
  this->Internals->SampleDistance = sampleDistance;
}

void MapperVolume::SetCompositeBackground(const bool compositeBackground)
{
  this->Internals->CompositeBackground = compositeBackground;
}
}
} // namespace viskores::rendering
