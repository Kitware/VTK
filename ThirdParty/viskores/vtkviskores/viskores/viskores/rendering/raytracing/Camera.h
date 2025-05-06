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
#ifndef viskores_rendering_raytracing_Camera_h
#define viskores_rendering_raytracing_Camera_h

#include <viskores/cont/CoordinateSystem.h>
#include <viskores/rendering/Camera.h>
#include <viskores/rendering/raytracing/Ray.h>

namespace viskores
{
namespace rendering
{
namespace raytracing
{

class VISKORES_RENDERING_EXPORT Camera
{
private:
  viskores::Int32 Height = 500;
  viskores::Int32 Width = 500;
  viskores::Int32 SubsetWidth = 500;
  viskores::Int32 SubsetHeight = 500;
  viskores::Int32 SubsetMinX = 0;
  viskores::Int32 SubsetMinY = 0;
  viskores::Float32 FovX = 30.f;
  viskores::Float32 FovY = 30.f;
  viskores::Float32 Zoom = 1.f;
  bool IsViewDirty = true;

  viskores::Vec3f_32 Look{ 0.f, 0.f, -1.f };
  viskores::Vec3f_32 Up{ 0.f, 1.f, 0.f };
  viskores::Vec3f_32 LookAt{ 0.f, 0.f, -1.f };
  viskores::Vec3f_32 Position{ 0.f, 0.f, 0.f };
  viskores::rendering::Camera CameraView;
  viskores::Matrix<viskores::Float32, 4, 4> ViewProjectionMat;

public:
  VISKORES_CONT
  std::string ToString();

  VISKORES_CONT
  void SetParameters(const viskores::rendering::Camera& camera,
                     viskores::Int32 width,
                     viskores::Int32 height);

  VISKORES_CONT
  void SetHeight(const viskores::Int32& height);

  VISKORES_CONT
  viskores::Int32 GetHeight() const;

  VISKORES_CONT
  void SetWidth(const viskores::Int32& width);

  VISKORES_CONT
  viskores::Int32 GetWidth() const;

  VISKORES_CONT
  viskores::Int32 GetSubsetWidth() const;

  VISKORES_CONT
  viskores::Int32 GetSubsetHeight() const;

  VISKORES_CONT
  void SetZoom(const viskores::Float32& zoom);

  VISKORES_CONT
  viskores::Float32 GetZoom() const;

  VISKORES_CONT
  void SetFieldOfView(const viskores::Float32& degrees);

  VISKORES_CONT
  viskores::Float32 GetFieldOfView() const;

  VISKORES_CONT
  void SetUp(const viskores::Vec3f_32& up);

  VISKORES_CONT
  void SetPosition(const viskores::Vec3f_32& position);

  VISKORES_CONT
  viskores::Vec3f_32 GetPosition() const;

  VISKORES_CONT
  viskores::Vec3f_32 GetUp() const;

  VISKORES_CONT
  void SetLookAt(const viskores::Vec3f_32& lookAt);

  VISKORES_CONT
  viskores::Vec3f_32 GetLookAt() const;

  VISKORES_CONT
  void ResetIsViewDirty();

  VISKORES_CONT
  bool GetIsViewDirty() const;

  VISKORES_CONT
  void CreateRays(Ray<viskores::Float32>& rays, const viskores::Bounds& bounds);

  VISKORES_CONT
  void CreateRays(Ray<viskores::Float64>& rays, const viskores::Bounds& bounds);

  VISKORES_CONT
  void GetPixelData(const viskores::cont::CoordinateSystem& coords,
                    viskores::Int32& activePixels,
                    viskores::Float32& aveRayDistance);

  template <typename Precision>
  VISKORES_CONT void CreateRaysImpl(Ray<Precision>& rays, const viskores::Bounds& boundingBox);

  void CreateDebugRay(viskores::Vec2i_32 pixel, Ray<viskores::Float32>& rays);

  void CreateDebugRay(viskores::Vec2i_32 pixel, Ray<viskores::Float64>& rays);

  bool operator==(const Camera& other) const;

private:
  template <typename Precision>
  VISKORES_CONT void CreateDebugRayImp(viskores::Vec2i_32 pixel, Ray<Precision>& rays);

  VISKORES_CONT
  void FindSubset(const viskores::Bounds& bounds);

  VISKORES_CONT
  void WriteSettingsToLog();

  template <typename Precision>
  VISKORES_CONT void UpdateDimensions(Ray<Precision>& rays,
                                      const viskores::Bounds& boundingBox,
                                      bool ortho2D);

}; // class camera
}
}
} //namespace viskores::rendering::raytracing
#endif //viskores_rendering_raytracing_Camera_h
