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
#include <viskores/rendering/raytracing/RayTracer.h>

#include <iostream>
#include <math.h>
#include <stdio.h>
#include <viskores/cont/ArrayHandleUniformPointCoordinates.h>
#include <viskores/cont/ColorTable.h>
#include <viskores/cont/Timer.h>

#include <viskores/rendering/raytracing/Camera.h>
#include <viskores/rendering/raytracing/Logger.h>
#include <viskores/rendering/raytracing/RayTracingTypeDefs.h>
#include <viskores/worklet/DispatcherMapField.h>
#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace rendering
{
namespace raytracing
{

namespace detail
{

class SurfaceColor
{
public:
  class Shade : public viskores::worklet::WorkletMapField
  {
  private:
    viskores::Vec3f_32 LightPosition;
    viskores::Vec3f_32 LightAbmient;
    viskores::Vec3f_32 LightDiffuse;
    viskores::Vec3f_32 LightSpecular;
    viskores::Float32 SpecularExponent;
    viskores::Vec3f_32 CameraPosition;
    viskores::Vec3f_32 LookAt;

  public:
    VISKORES_CONT
    Shade(const viskores::Vec3f_32& lightPosition,
          const viskores::Vec3f_32& cameraPosition,
          const viskores::Vec3f_32& lookAt)
      : LightPosition(lightPosition)
      , CameraPosition(cameraPosition)
      , LookAt(lookAt)
    {
      //Set up some default lighting parameters for now
      LightAbmient[0] = .5f;
      LightAbmient[1] = .5f;
      LightAbmient[2] = .5f;
      LightDiffuse[0] = .7f;
      LightDiffuse[1] = .7f;
      LightDiffuse[2] = .7f;
      LightSpecular[0] = .7f;
      LightSpecular[1] = .7f;
      LightSpecular[2] = .7f;
      SpecularExponent = 20.f;
    }

    using ControlSignature =
      void(FieldIn, FieldIn, FieldIn, FieldIn, WholeArrayInOut, WholeArrayIn);
    using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, WorkIndex);

    template <typename ColorPortalType, typename Precision, typename ColorMapPortalType>
    VISKORES_EXEC void operator()(const viskores::Id& hitIdx,
                                  const Precision& scalar,
                                  const viskores::Vec<Precision, 3>& normal,
                                  const viskores::Vec<Precision, 3>& intersection,
                                  ColorPortalType& colors,
                                  ColorMapPortalType colorMap,
                                  const viskores::Id& idx) const
    {
      viskores::Vec<Precision, 4> color;
      viskores::Id offset = idx * 4;

      if (hitIdx < 0)
      {
        return;
      }

      color[0] = colors.Get(offset + 0);
      color[1] = colors.Get(offset + 1);
      color[2] = colors.Get(offset + 2);
      color[3] = colors.Get(offset + 3);

      viskores::Vec<Precision, 3> lightDir = LightPosition - intersection;
      viskores::Vec<Precision, 3> viewDir = CameraPosition - LookAt;
      viskores::Normalize(lightDir);
      viskores::Normalize(viewDir);
      //Diffuse lighting
      Precision cosTheta = viskores::dot(normal, lightDir);
      //clamp tp [0,1]
      const Precision zero = 0.f;
      const Precision one = 1.f;
      cosTheta = viskores::Min(viskores::Max(cosTheta, zero), one);
      //Specular lighting
      viskores::Vec<Precision, 3> reflect =
        2.f * viskores::dot(lightDir, normal) * normal - lightDir;
      viskores::Normalize(reflect);
      Precision cosPhi = viskores::dot(reflect, viewDir);
      Precision specularConstant =
        viskores::Pow(viskores::Max(cosPhi, zero), static_cast<Precision>(SpecularExponent));
      viskores::Int32 colorMapSize = static_cast<viskores::Int32>(colorMap.GetNumberOfValues());
      viskores::Int32 colorIdx = viskores::Int32(scalar * Precision(colorMapSize - 1));

      // clamp color index
      colorIdx = viskores::Max(0, colorIdx);
      colorIdx = viskores::Min(colorMapSize - 1, colorIdx);
      color = colorMap.Get(colorIdx);

      color[0] *= viskores::Min(
        LightAbmient[0] + LightDiffuse[0] * cosTheta + LightSpecular[0] * specularConstant, one);
      color[1] *= viskores::Min(
        LightAbmient[1] + LightDiffuse[1] * cosTheta + LightSpecular[1] * specularConstant, one);
      color[2] *= viskores::Min(
        LightAbmient[2] + LightDiffuse[2] * cosTheta + LightSpecular[2] * specularConstant, one);

      colors.Set(offset + 0, color[0]);
      colors.Set(offset + 1, color[1]);
      colors.Set(offset + 2, color[2]);
      colors.Set(offset + 3, color[3]);
    }

  }; //class Shade

  class MapScalarToColor : public viskores::worklet::WorkletMapField
  {
  public:
    VISKORES_CONT
    MapScalarToColor() {}

    using ControlSignature = void(FieldIn, FieldIn, WholeArrayInOut, WholeArrayIn);
    using ExecutionSignature = void(_1, _2, _3, _4, WorkIndex);

    template <typename ColorPortalType, typename Precision, typename ColorMapPortalType>
    VISKORES_EXEC void operator()(const viskores::Id& hitIdx,
                                  const Precision& scalar,
                                  ColorPortalType& colors,
                                  ColorMapPortalType colorMap,
                                  const viskores::Id& idx) const
    {

      if (hitIdx < 0)
      {
        return;
      }

      viskores::Vec<Precision, 4> color;
      viskores::Id offset = idx * 4;

      viskores::Int32 colorMapSize = static_cast<viskores::Int32>(colorMap.GetNumberOfValues());
      viskores::Int32 colorIdx = viskores::Int32(scalar * Precision(colorMapSize - 1));

      // clamp color index
      colorIdx = viskores::Max(0, colorIdx);
      colorIdx = viskores::Min(colorMapSize - 1, colorIdx);
      color = colorMap.Get(colorIdx);

      colors.Set(offset + 0, color[0]);
      colors.Set(offset + 1, color[1]);
      colors.Set(offset + 2, color[2]);
      colors.Set(offset + 3, color[3]);
    }

  }; //class MapScalarToColor

  template <typename Precision>
  VISKORES_CONT void run(Ray<Precision>& rays,
                         viskores::cont::ArrayHandle<viskores::Vec4f_32>& colorMap,
                         const viskores::rendering::raytracing::Camera& camera,
                         bool shade)
  {
    if (shade)
    {
      // TODO: support light positions
      viskores::Vec3f_32 scale(2, 2, 2);
      viskores::Vec3f_32 lightPosition = camera.GetPosition() + scale * camera.GetUp();
      viskores::worklet::DispatcherMapField<Shade>(
        Shade(lightPosition, camera.GetPosition(), camera.GetLookAt()))
        .Invoke(rays.HitIdx,
                rays.Scalar,
                rays.Normal,
                rays.Intersection,
                rays.Buffers.at(0).Buffer,
                colorMap);
    }
    else
    {
      viskores::worklet::DispatcherMapField<MapScalarToColor>(MapScalarToColor())
        .Invoke(rays.HitIdx, rays.Scalar, rays.Buffers.at(0).Buffer, colorMap);
    }
  }
}; // class SurfaceColor

} // namespace detail

RayTracer::RayTracer()
  : NumberOfShapes(0)
  , Shade(true)
{
}

RayTracer::~RayTracer()
{
  Clear();
}

Camera& RayTracer::GetCamera()
{
  return camera;
}


void RayTracer::AddShapeIntersector(std::shared_ptr<ShapeIntersector> intersector)
{
  NumberOfShapes += intersector->GetNumberOfShapes();
  Intersectors.push_back(intersector);
}

void RayTracer::SetField(const viskores::cont::Field& scalarField,
                         const viskores::Range& scalarRange)
{
  ScalarField = scalarField;
  ScalarRange = scalarRange;
}

void RayTracer::SetColorMap(const viskores::cont::ArrayHandle<viskores::Vec4f_32>& colorMap)
{
  ColorMap = colorMap;
}

void RayTracer::Render(Ray<viskores::Float32>& rays)
{
  RenderOnDevice(rays);
}

void RayTracer::Render(Ray<viskores::Float64>& rays)
{
  RenderOnDevice(rays);
}

void RayTracer::SetShadingOn(bool on)
{
  Shade = on;
}

viskores::Id RayTracer::GetNumberOfShapes() const
{
  return NumberOfShapes;
}

void RayTracer::Clear()
{
  Intersectors.clear();
}

template <typename Precision>
void RayTracer::RenderOnDevice(Ray<Precision>& rays)
{
  using Timer = viskores::cont::Timer;

  Logger* logger = Logger::GetInstance();
  Timer renderTimer;
  renderTimer.Start();
  viskores::Float64 time = 0.;
  logger->OpenLogEntry("ray_tracer");
  logger->AddLogData("device", GetDeviceString());

  logger->AddLogData("shapes", NumberOfShapes);
  logger->AddLogData("num_rays", rays.NumRays);

  size_t numShapes = Intersectors.size();
  if (NumberOfShapes > 0)
  {
    Timer timer;
    timer.Start();

    for (size_t i = 0; i < numShapes; ++i)
    {
      Intersectors[i]->IntersectRays(rays);
      time = timer.GetElapsedTime();
      logger->AddLogData("intersect", time);

      timer.Start();
      Intersectors[i]->IntersectionData(rays, ScalarField, ScalarRange);
      time = timer.GetElapsedTime();
      logger->AddLogData("intersection_data", time);
      timer.Start();

      // Calculate the color at the intersection  point
      detail::SurfaceColor surfaceColor;
      surfaceColor.run(rays, ColorMap, camera, this->Shade);

      time = timer.GetElapsedTime();
      logger->AddLogData("shade", time);
    }
  }

  time = renderTimer.GetElapsedTime();
  logger->CloseLogEntry(time);
} // RenderOnDevice
}
}
} // namespace viskores::rendering::raytracing
