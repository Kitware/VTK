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
#include <viskores/rendering/raytracing/ScalarRenderer.h>

#include <iostream>
#include <viskores/cont/ArrayHandleUniformPointCoordinates.h>
#include <viskores/cont/Timer.h>

#include <viskores/rendering/raytracing/Logger.h>
#include <viskores/rendering/raytracing/RayTracingTypeDefs.h>
#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace rendering
{
namespace raytracing
{

namespace detail
{

template <typename Precision>
class SurfaceShade
{
public:
  class Shade : public viskores::worklet::WorkletMapField
  {
  private:
    viskores::Vec3f_32 LightPosition;
    viskores::Vec3f_32 LightAmbient{ .5f, .5f, .5f };
    viskores::Vec3f_32 LightDiffuse{ .7f, .7f, .7f };
    viskores::Vec3f_32 LightSpecular{ .7f, .7f, .7f };
    viskores::Float32 SpecularExponent = 20.f;
    viskores::Vec3f_32 CameraPosition;
    viskores::Vec3f_32 LookAt;
    Precision MissScalar;

  public:
    VISKORES_CONT
    Shade(const viskores::Vec3f_32& lightPosition,
          const viskores::Vec3f_32& cameraPosition,
          const viskores::Vec3f_32& lookAt,
          const Precision missScalar)
      : LightPosition(lightPosition)
      , CameraPosition(cameraPosition)
      , LookAt(lookAt)
      , MissScalar(missScalar)
    {
    }

    using ControlSignature = void(FieldIn, FieldIn, FieldIn, FieldOut);
    using ExecutionSignature = void(_1, _2, _3, _4);

    VISKORES_EXEC void operator()(const viskores::Id& hitIdx,
                                  const viskores::Vec<Precision, 3>& normal,
                                  const viskores::Vec<Precision, 3>& intersection,
                                  Precision& output) const
    {
      if (hitIdx < 0)
      {
        output = MissScalar;
      }

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

      Precision shade = viskores::Min(
        LightAmbient[0] + LightDiffuse[0] * cosTheta + LightSpecular[0] * specularConstant, one);
      output = shade;
    }

    viskores::Vec3f_32 GetDiffuse() { return LightDiffuse; }

    viskores::Vec3f_32 GetAmbient() { return LightAmbient; }

    viskores::Vec3f_32 GetSpecular() { return LightSpecular; }

    viskores::Float32 GetSpecularExponent() { return SpecularExponent; }


    void SetDiffuse(viskores::Vec3f_32 newDiffuse)
    {
      LightDiffuse[0] = newDiffuse[0];
      LightDiffuse[1] = newDiffuse[1];
      LightDiffuse[2] = newDiffuse[2];
    }

    void SetAmbient(viskores::Vec3f_32 newAmbient)
    {
      LightAmbient[0] = newAmbient[0];
      LightAmbient[1] = newAmbient[1];
      LightAmbient[2] = newAmbient[2];
    }

    void SetSpecular(viskores::Vec3f_32 newSpecular)
    {
      LightSpecular[0] = newSpecular[0];
      LightSpecular[1] = newSpecular[1];
      LightSpecular[2] = newSpecular[2];
    }

    void SetSpecularExponent(viskores::Float32 newExponent) { SpecularExponent = newExponent; }


  }; //class Shade

  VISKORES_CONT void run(Ray<Precision>& rays,
                         const viskores::rendering::raytracing::Camera& camera,
                         const Precision missScalar,
                         viskores::cont::ArrayHandle<Precision> shadings,
                         bool shade)
  {
    if (shade)
    {
      // TODO: support light positions
      viskores::Vec3f_32 scale(2, 2, 2);
      viskores::Vec3f_32 lightPosition = camera.GetPosition() + scale * camera.GetUp();
      viskores::worklet::DispatcherMapField<Shade>(
        Shade(lightPosition, camera.GetPosition(), camera.GetLookAt(), missScalar))
        .Invoke(rays.HitIdx, rays.Normal, rays.Intersection, shadings);
    }
  }
}; // class SurfaceShade


template <typename Precision>
class FilterDepth : public viskores::worklet::WorkletMapField
{
private:
  Precision MissScalar;

public:
  VISKORES_CONT
  explicit FilterDepth(const Precision missScalar)
    : MissScalar(missScalar)
  {
  }

  typedef void ControlSignature(FieldIn, FieldInOut);

  typedef void ExecutionSignature(_1, _2);
  VISKORES_EXEC void operator()(const viskores::Id& hitIndex, Precision& scalar) const
  {
    Precision value = scalar;
    if (hitIndex < 0)
    {
      value = MissScalar;
    }

    scalar = value;
  }
}; //class FilterDepth

template <typename Precision>
class WriteBuffer : public viskores::worklet::WorkletMapField
{
private:
  Precision MissScalar;

public:
  VISKORES_CONT
  explicit WriteBuffer(const Precision missScalar)
    : MissScalar(missScalar)
  {
  }

  typedef void ControlSignature(FieldIn, FieldIn, FieldOut);

  typedef void ExecutionSignature(_1, _2, _3);
  VISKORES_EXEC void operator()(const viskores::Id& hitIndex,
                                const Precision& scalar,
                                Precision& output) const
  {
    Precision value = scalar;
    if (hitIndex < 0)
    {
      value = MissScalar;
    }

    output = value;
  }
}; //class WriteBuffer

template <typename Precision>
class WriteDepthBuffer : public viskores::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn, FieldOut);

  typedef void ExecutionSignature(_1, _2);
  VISKORES_EXEC void operator()(const Precision& depth, Precision& output) const { output = depth; }
}; //class WriteDepthBuffer
} // namespace detail

void ScalarRenderer::SetShapeIntersector(std::unique_ptr<ShapeIntersector>&& intersector)
{
  Intersector = std::move(intersector);
}

void ScalarRenderer::AddField(const viskores::cont::Field& scalarField)
{
  const auto& ranges = scalarField.GetRange();
  if (ranges.GetNumberOfValues() != 1)
  {
    throw viskores::cont::ErrorBadValue("ScalarRenderer(AddField): field must be a scalar");
  }
  Fields.push_back(scalarField);
}

void ScalarRenderer::Render(Ray<viskores::Float32>& rays,
                            viskores::Float32 missScalar,
                            viskores::rendering::raytracing::Camera& cam)
{
  RenderOnDevice(rays, missScalar, cam);
}

void ScalarRenderer::Render(Ray<viskores::Float64>& rays,
                            viskores::Float64 missScalar,
                            viskores::rendering::raytracing::Camera& cam)
{
  RenderOnDevice(rays, missScalar, cam);
}

template <typename Precision>
void ScalarRenderer::RenderOnDevice(Ray<Precision>& rays,
                                    Precision missScalar,
                                    viskores::rendering::raytracing::Camera& cam)
{
  using Timer = viskores::cont::Timer;

  Logger* logger = Logger::GetInstance();
  Timer renderTimer;
  renderTimer.Start();
  viskores::Float64 time = 0.;
  logger->OpenLogEntry("scalar_renderer");
  logger->AddLogData("device", GetDeviceString());

  logger->AddLogData("num_rays", rays.NumRays);
  const size_t numFields = Fields.size();

  if (numFields == 0)
  {
    throw viskores::cont::ErrorBadValue("ScalarRenderer: no fields added");
  }
  if (!Intersector)
  {
    throw viskores::cont::ErrorBadValue("ScalarRenderer: intersector never set");
  }

  Timer timer;
  timer.Start();

  Intersector->IntersectRays(rays);
  time = timer.GetElapsedTime();
  logger->AddLogData("intersect", time);

  for (size_t f = 0; f < numFields; ++f)
  {
    timer.Start();
    Intersector->IntersectionData(rays, Fields[f]);
    time = timer.GetElapsedTime();
    logger->AddLogData("intersection_data", time);
    AddBuffer(rays, missScalar, Fields[f].GetName());
  }

  const viskores::Int32 numChannels = 1;
  ChannelBuffer<Precision> buffer(numChannels, rays.NumRays);
  detail::SurfaceShade<Precision> surfaceShade;
  surfaceShade.run(rays, cam, missScalar, buffer.Buffer, true);
  buffer.SetName("shading");
  rays.Buffers.push_back(buffer);

  this->Invoke(detail::FilterDepth<Precision>{ missScalar }, rays.HitIdx, rays.Distance);

  time = renderTimer.GetElapsedTime();
  logger->CloseLogEntry(time);
} // RenderOnDevice

template <typename Precision>
void ScalarRenderer::AddBuffer(Ray<Precision>& rays, Precision missScalar, const std::string& name)
{
  const viskores::Int32 numChannels = 1;
  ChannelBuffer<Precision> buffer(numChannels, rays.NumRays);

  this->Invoke(
    detail::WriteBuffer<Precision>{ missScalar }, rays.HitIdx, rays.Scalar, buffer.Buffer);

  buffer.SetName(name);
  rays.Buffers.push_back(buffer);
}

template <typename Precision>
void ScalarRenderer::AddDepthBuffer(Ray<Precision>& rays)
{
  const viskores::Int32 numChannels = 1;
  ChannelBuffer<Precision> buffer(numChannels, rays.NumRays);

  this->Invoke(detail::WriteDepthBuffer<Precision>{}, rays.Depth, buffer.Buffer);

  buffer.SetName("depth");
  rays.Buffers.push_back(buffer);
}
}
}
} // namespace viskores::rendering::raytracing
