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

#include <viskores/rendering/MapperCylinder.h>

#include <viskores/cont/Timer.h>
#include <viskores/cont/TryExecute.h>

#include <viskores/rendering/CanvasRayTracer.h>
#include <viskores/rendering/Cylinderizer.h>
#include <viskores/rendering/raytracing/Camera.h>
#include <viskores/rendering/raytracing/CylinderExtractor.h>
#include <viskores/rendering/raytracing/CylinderIntersector.h>
#include <viskores/rendering/raytracing/Logger.h>
#include <viskores/rendering/raytracing/RayOperations.h>
#include <viskores/rendering/raytracing/RayTracer.h>
#include <viskores/rendering/raytracing/Worklets.h>

namespace viskores
{
namespace rendering
{

class CalcDistance : public viskores::worklet::WorkletMapField
{
public:
  VISKORES_CONT
  CalcDistance(const viskores::Vec3f_32& _eye_pos)
    : eye_pos(_eye_pos)
  {
  }
  typedef void ControlSignature(FieldIn, FieldOut);
  typedef void ExecutionSignature(_1, _2);
  template <typename VecType, typename OutType>
  VISKORES_EXEC inline void operator()(const VecType& pos, OutType& out) const
  {
    VecType tmp = eye_pos - pos;
    out = static_cast<OutType>(viskores::Sqrt(viskores::dot(tmp, tmp)));
  }

  const viskores::Vec3f_32 eye_pos;
}; //class CalcDistance

struct MapperCylinder::InternalsType
{
  viskores::rendering::CanvasRayTracer* Canvas;
  viskores::rendering::raytracing::RayTracer Tracer;
  viskores::rendering::raytracing::Camera RayCamera;
  viskores::rendering::raytracing::Ray<viskores::Float32> Rays;
  bool CompositeBackground;
  viskores::Float32 Radius;
  viskores::Float32 Delta;
  bool UseVariableRadius;
  VISKORES_CONT
  InternalsType()
    : Canvas(nullptr)
    , CompositeBackground(true)
    , Radius(-1.0f)
    , Delta(0.5)
    , UseVariableRadius(false)
  {
  }
};

MapperCylinder::MapperCylinder()
  : Internals(new InternalsType)
{
}

MapperCylinder::~MapperCylinder() {}

void MapperCylinder::SetCanvas(viskores::rendering::Canvas* canvas)
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

viskores::rendering::Canvas* MapperCylinder::GetCanvas() const
{
  return this->Internals->Canvas;
}

void MapperCylinder::UseVariableRadius(bool useVariableRadius)
{
  this->Internals->UseVariableRadius = useVariableRadius;
}

void MapperCylinder::SetRadius(const viskores::Float32& radius)
{
  if (radius <= 0.f)
  {
    throw viskores::cont::ErrorBadValue("MapperCylinder: radius must be positive");
  }
  this->Internals->Radius = radius;
}
void MapperCylinder::SetRadiusDelta(const viskores::Float32& delta)
{
  this->Internals->Delta = delta;
}

void MapperCylinder::RenderCellsImpl(const viskores::cont::UnknownCellSet& cellset,
                                     const viskores::cont::CoordinateSystem& coords,
                                     const viskores::cont::Field& scalarField,
                                     const viskores::cont::ColorTable& viskoresNotUsed(colorTable),
                                     const viskores::rendering::Camera& camera,
                                     const viskores::Range& scalarRange,
                                     const viskores::cont::Field& viskoresNotUsed(ghostField))
{
  raytracing::Logger* logger = raytracing::Logger::GetInstance();
  logger->OpenLogEntry("mapper_cylinder");
  viskores::cont::Timer tot_timer;
  tot_timer.Start();
  viskores::cont::Timer timer;


  viskores::Bounds shapeBounds;
  raytracing::CylinderExtractor cylExtractor;

  viskores::Float32 baseRadius = this->Internals->Radius;
  if (baseRadius == -1.f)
  {
    // set a default radius
    viskores::cont::ArrayHandle<viskores::Float32> dist;
    viskores::worklet::DispatcherMapField<CalcDistance>(CalcDistance(camera.GetPosition()))
      .Invoke(coords, dist);


    viskores::Float32 min_dist = viskores::cont::Algorithm::Reduce(
      dist, viskores::Infinity<viskores::Float32>(), viskores::Minimum());

    baseRadius = 0.576769694f * min_dist -
      0.603522029f * viskores::Pow(viskores::Float32(min_dist), 2.f) +
      0.232171175f * viskores::Pow(viskores::Float32(min_dist), 3.f) -
      0.038697244f * viskores::Pow(viskores::Float32(min_dist), 4.f) +
      0.002366979f * viskores::Pow(viskores::Float32(min_dist), 5.f);
    baseRadius /= min_dist;
    viskores::worklet::DispatcherMapField<
      viskores::rendering::raytracing::MemSet<viskores::Float32>>(
      viskores::rendering::raytracing::MemSet<viskores::Float32>(baseRadius))
      .Invoke(cylExtractor.GetRadii());
  }

  if (this->Internals->UseVariableRadius)
  {
    viskores::Float32 minRadius = baseRadius - baseRadius * this->Internals->Delta;
    viskores::Float32 maxRadius = baseRadius + baseRadius * this->Internals->Delta;

    cylExtractor.ExtractCells(cellset, scalarField, minRadius, maxRadius);
  }
  else
  {
    cylExtractor.ExtractCells(cellset, baseRadius);
  }

  //
  // Add supported shapes
  //

  if (cylExtractor.GetNumberOfCylinders() > 0)
  {
    auto cylIntersector = std::make_shared<raytracing::CylinderIntersector>();
    cylIntersector->SetData(coords, cylExtractor.GetCylIds(), cylExtractor.GetRadii());
    this->Internals->Tracer.AddShapeIntersector(cylIntersector);
    shapeBounds.Include(cylIntersector->GetShapeBounds());
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

void MapperCylinder::SetCompositeBackground(bool on)
{
  this->Internals->CompositeBackground = on;
}

viskores::rendering::Mapper* MapperCylinder::NewCopy() const
{
  return new viskores::rendering::MapperCylinder(*this);
}
}
} // viskores::rendering
