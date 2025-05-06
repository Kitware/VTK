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

#include <viskores/rendering/MapperQuad.h>

#include <viskores/cont/Timer.h>
#include <viskores/cont/TryExecute.h>

#include <viskores/rendering/CanvasRayTracer.h>
#include <viskores/rendering/Cylinderizer.h>
#include <viskores/rendering/raytracing/Camera.h>
#include <viskores/rendering/raytracing/Logger.h>
#include <viskores/rendering/raytracing/QuadExtractor.h>
#include <viskores/rendering/raytracing/QuadIntersector.h>
#include <viskores/rendering/raytracing/RayOperations.h>
#include <viskores/rendering/raytracing/RayTracer.h>

namespace viskores
{
namespace rendering
{

struct MapperQuad::InternalsType
{
  viskores::rendering::CanvasRayTracer* Canvas;
  viskores::rendering::raytracing::RayTracer Tracer;
  viskores::rendering::raytracing::Camera RayCamera;
  viskores::rendering::raytracing::Ray<viskores::Float32> Rays;
  bool CompositeBackground;
  VISKORES_CONT
  InternalsType()
    : Canvas(nullptr)
    , CompositeBackground(true)
  {
  }
};

MapperQuad::MapperQuad()
  : Internals(new InternalsType)
{
}

MapperQuad::~MapperQuad() {}

void MapperQuad::SetCanvas(viskores::rendering::Canvas* canvas)
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

viskores::rendering::Canvas* MapperQuad::GetCanvas() const
{
  return this->Internals->Canvas;
}

void MapperQuad::RenderCellsImpl(const viskores::cont::UnknownCellSet& cellset,
                                 const viskores::cont::CoordinateSystem& coords,
                                 const viskores::cont::Field& scalarField,
                                 const viskores::cont::ColorTable& viskoresNotUsed(colorTable),
                                 const viskores::rendering::Camera& camera,
                                 const viskores::Range& scalarRange,
                                 const viskores::cont::Field& viskoresNotUsed(ghostField))
{
  raytracing::Logger* logger = raytracing::Logger::GetInstance();
  logger->OpenLogEntry("mapper_ray_tracer");
  viskores::cont::Timer tot_timer;
  tot_timer.Start();
  viskores::cont::Timer timer;


  //
  // Add supported shapes
  //
  viskores::Bounds shapeBounds;
  raytracing::QuadExtractor quadExtractor;
  quadExtractor.ExtractCells(cellset);
  if (quadExtractor.GetNumberOfQuads() > 0)
  {
    auto quadIntersector = std::make_shared<raytracing::QuadIntersector>();
    quadIntersector->SetData(coords, quadExtractor.GetQuadIds());
    this->Internals->Tracer.AddShapeIntersector(quadIntersector);
    shapeBounds.Include(quadIntersector->GetShapeBounds());
  }

  //
  // Create rays
  //
  viskores::Int32 width = (viskores::Int32)this->Internals->Canvas->GetWidth();
  viskores::Int32 height = (viskores::Int32)this->Internals->Canvas->GetHeight();

  this->Internals->RayCamera.SetParameters(camera, width, height);

  this->Internals->RayCamera.CreateRays(this->Internals->Rays, shapeBounds);
  this->Internals->Rays.Buffers.at(0).InitConst(0.f);
  raytracing::RayOperations::MapCanvasToRays(
    this->Internals->Rays, camera, *this->Internals->Canvas);

  this->Internals->Tracer.GetCamera() = this->Internals->RayCamera;
  this->Internals->Tracer.SetField(scalarField, scalarRange);
  this->Internals->Tracer.SetColorMap(this->ColorMap);
  this->Internals->Tracer.Render(this->Internals->Rays);

  timer.Start();
  this->Internals->Canvas->WriteToCanvas(
    this->Internals->Rays, this->Internals->Rays.Buffers.at(0).Buffer, camera);

  if (this->Internals->CompositeBackground)
  {
    this->Internals->Canvas->BlendBackground();
  }

  viskores::Float64 time = timer.GetElapsedTime();
  logger->AddLogData("write_to_canvas", time);
  time = tot_timer.GetElapsedTime();
  logger->CloseLogEntry(time);
}

void MapperQuad::SetCompositeBackground(bool on)
{
  this->Internals->CompositeBackground = on;
}

viskores::rendering::Mapper* MapperQuad::NewCopy() const
{
  return new viskores::rendering::MapperQuad(*this);
}
}
} // viskores::rendering
