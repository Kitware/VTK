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

#include <viskores/Matrix.h>
#include <viskores/cont/CoordinateSystem.h>
#include <viskores/rendering/raytracing/Ray.h>

namespace viskores
{
namespace rendering
{
namespace raytracing
{

struct VISKORES_RENDERING_RAYTRACING_EXPORT Camera3DStruct
{
public:
  VISKORES_CONT
  Camera3DStruct()
    : LookAt(0.0f, 0.0f, 0.0f)
    , Position(0.0f, 0.0f, 1.0f)
    , ViewUp(0.0f, 1.0f, 0.0f)
    , FieldOfView(60.0f)
    , XPan(0.0f)
    , YPan(0.0f)
    , Zoom(1.0f)
  {
  }

  viskores::Matrix<viskores::Float32, 4, 4> CreateViewMatrix() const;

  viskores::Matrix<viskores::Float32, 4, 4> CreateProjectionMatrix(
    viskores::Id width,
    viskores::Id height,
    viskores::Float32 nearPlane,
    viskores::Float32 farPlane) const;

  viskores::Vec3f_32 LookAt;
  viskores::Vec3f_32 Position;
  viskores::Vec3f_32 ViewUp;
  viskores::Float32 FieldOfView;
  viskores::Float32 XPan;
  viskores::Float32 YPan;
  viskores::Float32 Zoom;
};

struct VISKORES_RENDERING_RAYTRACING_EXPORT Camera2DStruct
{
public:
  VISKORES_CONT
  Camera2DStruct()
    : Left(-1.0f)
    , Right(1.0f)
    , Bottom(-1.0f)
    , Top(1.0f)
    , XScale(1.0f)
    , XPan(0.0f)
    , YPan(0.0f)
    , Zoom(1.0f)
  {
  }

  viskores::Matrix<viskores::Float32, 4, 4> CreateViewMatrix() const;

  viskores::Matrix<viskores::Float32, 4, 4> CreateProjectionMatrix(viskores::Float32 size,
                                                                   viskores::Float32 znear,
                                                                   viskores::Float32 zfar,
                                                                   viskores::Float32 aspect) const;

  viskores::Float32 Left;
  viskores::Float32 Right;
  viskores::Float32 Bottom;
  viskores::Float32 Top;
  viskores::Float32 XScale;
  viskores::Float32 XPan;
  viskores::Float32 YPan;
  viskores::Float32 Zoom;
};

class VISKORES_RENDERING_RAYTRACING_EXPORT Camera
{
private:
  viskores::Int32 Height = 500;
  viskores::Int32 Width = 500;
  viskores::Int32 SubsetWidth = 500;
  viskores::Int32 SubsetHeight = 500;
  viskores::Int32 SubsetMinX = 0;
  viskores::Int32 SubsetMinY = 0;
  bool IsViewDirty = true;

  bool IsOrthogonalProjection = false;
  Camera3DStruct Camera3D;
  Camera2DStruct Camera2D;
  viskores::Float32 ViewportLeft = -1.0f;
  viskores::Float32 ViewportRight = 1.0f;
  viskores::Float32 ViewportBottom = -1.0f;
  viskores::Float32 ViewportTop = 1.0f;

  viskores::Vec3f_32 LookDirection{ 0.f, 0.f, -1.f };
  viskores::Float32 NearPlane = 0.01f;
  viskores::Float32 FarPlane = 1000.0f;
  mutable viskores::Matrix<viskores::Float32, 4, 4> ViewProjectionMat;

public:
  VISKORES_CONT
  std::string ToString();

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

  VISKORES_CONT viskores::Int32 GetSubsetMinX() const;
  VISKORES_CONT viskores::Int32 GetSubsetMinY() const;

  VISKORES_CONT
  void SetPan(const viskores::Float32& xpan, const viskores::Float32& ypan)
  {
    this->Camera3D.XPan = xpan;
    this->Camera3D.YPan = ypan;
    this->Camera2D.XPan = xpan;
    this->Camera2D.YPan = ypan;
  }

  VISKORES_CONT
  void SetPan(viskores::Vec2f_32 pan) { this->SetPan(pan[0], pan[1]); }

  VISKORES_CONT
  viskores::Vec2f_32 GetPan() const
  {
    viskores::Vec2f_32 pan;
    // 2D and 3D values should be the same.
    pan[0] = this->Camera3D.XPan;
    pan[1] = this->Camera3D.YPan;
    return pan;
  }

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

  VISKORES_CONT void SetClippingRange(viskores::Float32 nearPlane, viskores::Float32 farPlane)
  {
    this->NearPlane = nearPlane;
    this->FarPlane = farPlane;
  }

  VISKORES_CONT void GetClippingRange(viskores::Float32& nearPlane,
                                      viskores::Float32& farPlane) const
  {
    nearPlane = this->NearPlane;
    farPlane = this->FarPlane;
  }

  VISKORES_CONT void SetIsOrthogonalProjection(bool isOrtho)
  {
    this->IsOrthogonalProjection = isOrtho;
  }

  VISKORES_CONT bool GetIsOrthogonalProjection() const { return this->IsOrthogonalProjection; }

  VISKORES_CONT void SetCamera3D(const viskores::rendering::raytracing::Camera3DStruct& camera3d)
  {
    this->Camera3D = camera3d;
  }

  VISKORES_CONT const viskores::rendering::raytracing::Camera3DStruct& GetCamera3D() const
  {
    return this->Camera3D;
  }

  VISKORES_CONT void SetCamera2D(const viskores::rendering::raytracing::Camera2DStruct& camera2d)
  {
    this->Camera2D = camera2d;
  }

  VISKORES_CONT const viskores::rendering::raytracing::Camera2DStruct& GetCamera2D() const
  {
    return this->Camera2D;
  }

  VISKORES_CONT void SetViewport(viskores::Float32 left,
                                 viskores::Float32 right,
                                 viskores::Float32 bottom,
                                 viskores::Float32 top)
  {
    this->ViewportLeft = left;
    this->ViewportRight = right;
    this->ViewportBottom = bottom;
    this->ViewportTop = top;
  }

  VISKORES_CONT void GetViewport(viskores::Float32& left,
                                 viskores::Float32& right,
                                 viskores::Float32& bottom,
                                 viskores::Float32& top) const
  {
    left = this->ViewportLeft;
    right = this->ViewportRight;
    bottom = this->ViewportBottom;
    top = this->ViewportTop;
  }

  VISKORES_CONT viskores::Matrix<viskores::Float32, 4, 4>& GetViewProjectionMatrix() const;

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

private:
  template <typename Precision>
  VISKORES_CONT void CreateDebugRayImp(viskores::Vec2i_32 pixel, Ray<Precision>& rays);

  VISKORES_CONT
  void FindSubset(const viskores::Bounds& bounds);

  VISKORES_CONT
  void WriteSettingsToLog();

  template <typename Precision>
  VISKORES_CONT void UpdateDimensions(Ray<Precision>& rays, const viskores::Bounds& boundingBox);

  VISKORES_CONT void UpdateViewProjectionMatrix() const;

}; // class camera
}
}
} //namespace viskores::rendering::raytracing
#endif //viskores_rendering_raytracing_Camera_h
