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

#include <viskores/rendering/MapperPoint.h>

#include <viskores/cont/Timer.h>

#include <viskores/rendering/CanvasRayTracer.h>
#include <viskores/rendering/internal/RunTriangulator.h>
#include <viskores/rendering/raytracing/Camera.h>
#include <viskores/rendering/raytracing/Logger.h>
#include <viskores/rendering/raytracing/RayOperations.h>
#include <viskores/rendering/raytracing/RayTracer.h>
#include <viskores/rendering/raytracing/SphereExtractor.h>
#include <viskores/rendering/raytracing/SphereIntersector.h>

namespace viskores
{
namespace rendering
{

struct MapperPoint::InternalsType
{
  viskores::rendering::CanvasRayTracer* Canvas = nullptr;
  viskores::rendering::raytracing::RayTracer Tracer;
  viskores::rendering::raytracing::Camera RayCamera;
  viskores::rendering::raytracing::Ray<viskores::Float32> Rays;
  bool CompositeBackground = true;
  viskores::Float32 PointRadius = -1.f;
  viskores::cont::Field::Association Association = viskores::cont::Field::Association::Points;
  viskores::Float32 PointDelta = 0.5f;
  bool UseVariableRadius = false;

  VISKORES_CONT
  InternalsType() = default;
};

MapperPoint::MapperPoint()
  : Internals(new InternalsType)
{
}

MapperPoint::~MapperPoint() {}

void MapperPoint::SetCanvas(viskores::rendering::Canvas* canvas)
{
  if (canvas != nullptr)
  {
    this->Internals->Canvas = dynamic_cast<CanvasRayTracer*>(canvas);
    if (this->Internals->Canvas == nullptr)
    {
      throw viskores::cont::ErrorBadValue("MapperPoint: bad canvas type. Must be CanvasRayTracer");
    }
  }
  else
  {
    this->Internals->Canvas = nullptr;
  }
}

viskores::rendering::Canvas* MapperPoint::GetCanvas() const
{
  return this->Internals->Canvas;
}

viskores::cont::Field::Association MapperPoint::GetAssociation() const
{
  return this->Internals->Association;
}

void MapperPoint::SetAssociation(cont::Field::Association association)
{
  switch (association)
  {
    case viskores::cont::Field::Association::Cells:
    case viskores::cont::Field::Association::Points:
      this->Internals->Association = association;
      break;
    default:
      throw viskores::cont::ErrorBadValue("Invalid point mapper association.");
  }
}

bool MapperPoint::GetUseCells() const
{
  return this->GetAssociation() == viskores::cont::Field::Association::Cells;
}

void MapperPoint::SetUseCells()
{
  this->SetAssociation(viskores::cont::Field::Association::Cells);
}

bool MapperPoint::GetUsePoints() const
{
  return this->GetAssociation() == viskores::cont::Field::Association::Points;
}

void MapperPoint::SetUsePoints()
{
  this->SetAssociation(viskores::cont::Field::Association::Points);
}

void MapperPoint::UseCells()
{
  this->SetUseCells();
}
void MapperPoint::UseNodes()
{
  this->SetUsePoints();
}

void MapperPoint::SetRadius(const viskores::Float32& radius)
{
  if (radius <= 0.f)
  {
    throw viskores::cont::ErrorBadValue("MapperPoint: point radius must be positive");
  }
  this->Internals->PointRadius = radius;
}

void MapperPoint::SetRadiusDelta(const viskores::Float32& delta)
{
  this->Internals->PointDelta = delta;
}

void MapperPoint::UseVariableRadius(bool useVariableRadius)
{
  this->Internals->UseVariableRadius = useVariableRadius;
}

void MapperPoint::RenderCellsImpl(const viskores::cont::UnknownCellSet& cellset,
                                  const viskores::cont::CoordinateSystem& coords,
                                  const viskores::cont::Field& scalarField,
                                  const viskores::cont::ColorTable& viskoresNotUsed(colorTable),
                                  const viskores::rendering::Camera& camera,
                                  const viskores::Range& scalarRange,
                                  const viskores::cont::Field& viskoresNotUsed(ghostField))
{
  raytracing::Logger* logger = raytracing::Logger::GetInstance();

  // make sure we start fresh
  this->Internals->Tracer.Clear();

  logger->OpenLogEntry("mapper_ray_tracer");
  viskores::cont::Timer tot_timer;
  tot_timer.Start();
  viskores::cont::Timer timer;

  viskores::Bounds coordBounds = coords.GetBounds();
  viskores::Float32 baseRadius = this->Internals->PointRadius;
  if (baseRadius == -1.f)
  {
    // set a default radius
    viskores::Float64 lx = coordBounds.X.Length();
    viskores::Float64 ly = coordBounds.Y.Length();
    viskores::Float64 lz = coordBounds.Z.Length();
    viskores::Float64 mag = viskores::Sqrt(lx * lx + ly * ly + lz * lz);
    // same as used in vtk ospray
    constexpr viskores::Float64 heuristic = 500.;
    baseRadius = static_cast<viskores::Float32>(mag / heuristic);
  }

  viskores::Bounds shapeBounds;

  raytracing::SphereExtractor sphereExtractor;

  if (this->Internals->UseVariableRadius)
  {
    viskores::Float32 minRadius = baseRadius - baseRadius * this->Internals->PointDelta;
    viskores::Float32 maxRadius = baseRadius + baseRadius * this->Internals->PointDelta;
    switch (this->Internals->Association)
    {
      case viskores::cont::Field::Association::Points:
        sphereExtractor.ExtractCoordinates(coords, scalarField, minRadius, maxRadius);
        break;
      case viskores::cont::Field::Association::Cells:
        sphereExtractor.ExtractCells(cellset, scalarField, minRadius, maxRadius);
        break;
      default:
        throw viskores::cont::ErrorInternal("Bad association.");
    }
  }
  else
  {
    switch (this->Internals->Association)
    {
      case viskores::cont::Field::Association::Points:
        sphereExtractor.ExtractCoordinates(coords, baseRadius);
        break;
      case viskores::cont::Field::Association::Cells:
        sphereExtractor.ExtractCells(cellset, baseRadius);
        break;
      default:
        throw viskores::cont::ErrorInternal("Bad association.");
    }
  }

  if (sphereExtractor.GetNumberOfSpheres() > 0)
  {
    auto sphereIntersector = std::make_shared<raytracing::SphereIntersector>();
    sphereIntersector->SetData(coords, sphereExtractor.GetPointIds(), sphereExtractor.GetRadii());
    this->Internals->Tracer.AddShapeIntersector(sphereIntersector);
    shapeBounds.Include(sphereIntersector->GetShapeBounds());
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

  this->Internals->Tracer.SetField(scalarField, scalarRange);
  this->Internals->Tracer.GetCamera() = this->Internals->RayCamera;
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

void MapperPoint::SetCompositeBackground(bool on)
{
  this->Internals->CompositeBackground = on;
}

viskores::rendering::Mapper* MapperPoint::NewCopy() const
{
  return new viskores::rendering::MapperPoint(*this);
}
}
} // viskores::rendering
