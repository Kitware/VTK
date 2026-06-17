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

#include <viskores/Transform3D.h>
#include <viskores/VectorAnalysis.h>

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ErrorBadValue.h>
#include <viskores/cont/Timer.h>

#include <viskores/rendering/MatrixHelpers.h>
#include <viskores/rendering/raytracing/Camera.h>
#include <viskores/rendering/raytracing/Logger.h>
#include <viskores/rendering/raytracing/RayOperations.h>
#include <viskores/rendering/raytracing/RayTracingTypeDefs.h>
#include <viskores/rendering/raytracing/Sampler.h>

#include <viskores/worklet/DispatcherMapField.h>
#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace rendering
{
namespace raytracing
{

class PixelData : public viskores::worklet::WorkletMapField
{
public:
  viskores::Int32 w;
  viskores::Int32 h;
  viskores::Int32 Minx;
  viskores::Int32 Miny;
  viskores::Int32 SubsetWidth;
  viskores::Vec3f_32 nlook; // normalized look
  viskores::Vec3f_32 delta_x;
  viskores::Vec3f_32 delta_y;
  viskores::Vec3f_32 Origin;
  viskores::Bounds BoundingBox;
  VISKORES_CONT
  PixelData(viskores::Int32 width,
            viskores::Int32 height,
            viskores::Float32 fovY,
            viskores::Vec3f_32 look,
            viskores::Vec3f_32 up,
            viskores::Float32 _zoom,
            viskores::Int32 subsetWidth,
            viskores::Int32 minx,
            viskores::Int32 miny,
            viskores::Vec3f_32 origin,
            viskores::Bounds boundingBox)
    : w(width)
    , h(height)
    , Minx(minx)
    , Miny(miny)
    , SubsetWidth(subsetWidth)
    , Origin(origin)
    , BoundingBox(boundingBox)
  {
    viskores::Float32 thy = tanf((fovY * viskores::Pi_180f()) * .5f);
    viskores::Float32 thx = (thy * width) / height;
    viskores::Vec3f_32 ru = viskores::Cross(look, up);
    viskores::Normalize(ru);

    viskores::Vec3f_32 rv = viskores::Cross(ru, look);
    viskores::Normalize(rv);
    delta_x = ru * (2 * thx / (float)w);
    delta_y = rv * (2 * thy / (float)h);

    if (_zoom > 0)
    {
      delta_x[0] = delta_x[0] / _zoom;
      delta_x[1] = delta_x[1] / _zoom;
      delta_x[2] = delta_x[2] / _zoom;
      delta_y[0] = delta_y[0] / _zoom;
      delta_y[1] = delta_y[1] / _zoom;
      delta_y[2] = delta_y[2] / _zoom;
    }
    nlook = look;
    viskores::Normalize(nlook);
  }

  VISKORES_EXEC static inline viskores::Float32 rcp(viskores::Float32 f) { return 1.0f / f; }

  VISKORES_EXEC static inline viskores::Float32 rcp_safe(viskores::Float32 f)
  {
    return rcp((fabs(f) < 1e-8f) ? 1e-8f : f);
  }

  using ControlSignature = void(FieldOut, FieldOut);

  using ExecutionSignature = void(WorkIndex, _1, _2);
  VISKORES_EXEC
  void operator()(const viskores::Id idx, viskores::Int32& hit, viskores::Float32& distance) const
  {
    viskores::Vec3f_32 ray_dir;
    int i = viskores::Int32(idx) % SubsetWidth;
    int j = viskores::Int32(idx) / SubsetWidth;
    i += Minx;
    j += Miny;
    // Write out the global pixelId
    ray_dir = nlook + delta_x * ((2.f * viskores::Float32(i) - viskores::Float32(w)) / 2.0f) +
      delta_y * ((2.f * viskores::Float32(j) - viskores::Float32(h)) / 2.0f);

    viskores::Float32 dot = viskores::Dot(ray_dir, ray_dir);
    viskores::Float32 sq_mag = viskores::Sqrt(dot);

    ray_dir[0] = ray_dir[0] / sq_mag;
    ray_dir[1] = ray_dir[1] / sq_mag;
    ray_dir[2] = ray_dir[2] / sq_mag;

    viskores::Float32 invDirx = rcp_safe(ray_dir[0]);
    viskores::Float32 invDiry = rcp_safe(ray_dir[1]);
    viskores::Float32 invDirz = rcp_safe(ray_dir[2]);

    viskores::Float32 odirx = Origin[0] * invDirx;
    viskores::Float32 odiry = Origin[1] * invDiry;
    viskores::Float32 odirz = Origin[2] * invDirz;

    viskores::Float32 xmin = viskores::Float32(BoundingBox.X.Min) * invDirx - odirx;
    viskores::Float32 ymin = viskores::Float32(BoundingBox.Y.Min) * invDiry - odiry;
    viskores::Float32 zmin = viskores::Float32(BoundingBox.Z.Min) * invDirz - odirz;
    viskores::Float32 xmax = viskores::Float32(BoundingBox.X.Max) * invDirx - odirx;
    viskores::Float32 ymax = viskores::Float32(BoundingBox.Y.Max) * invDiry - odiry;
    viskores::Float32 zmax = viskores::Float32(BoundingBox.Z.Max) * invDirz - odirz;
    viskores::Float32 mind = viskores::Max(
      viskores::Max(viskores::Max(viskores::Min(ymin, ymax), viskores::Min(xmin, xmax)),
                    viskores::Min(zmin, zmax)),
      0.f);
    viskores::Float32 maxd =
      viskores::Min(viskores::Min(viskores::Max(ymin, ymax), viskores::Max(xmin, xmax)),
                    viskores::Max(zmin, zmax));
    if (maxd < mind)
    {
      hit = 0;
      distance = 0;
    }
    else
    {
      distance = maxd - mind;
      hit = 1;
    }
  }

}; // class pixelData

class PerspectiveRayGenJitter : public viskores::worklet::WorkletMapField
{
public:
  viskores::Int32 w;
  viskores::Int32 h;
  viskores::Vec3f_32 nlook; // normalized look
  viskores::Vec3f_32 delta_x;
  viskores::Vec3f_32 delta_y;
  viskores::Int32 CurrentSample;
  VISKORES_CONT_EXPORT
  PerspectiveRayGenJitter(viskores::Int32 width,
                          viskores::Int32 height,
                          viskores::Float32 fovY,
                          viskores::Vec3f_32 look,
                          viskores::Vec3f_32 up,
                          viskores::Float32 _zoom,
                          viskores::Int32 currentSample)
    : w(width)
    , h(height)
  {
    viskores::Float32 thy = tanf((fovY * 3.1415926f / 180.f) * .5f);
    viskores::Float32 thx = (thy * width) / height;
    viskores::Vec3f_32 ru = viskores::Cross(up, look);
    viskores::Normalize(ru);

    viskores::Vec3f_32 rv = viskores::Cross(ru, look);
    viskores::Normalize(rv);

    delta_x = ru * (2 * thx / (float)w);
    delta_y = rv * (2 * thy / (float)h);

    if (_zoom > 0)
    {
      delta_x[0] = delta_x[0] / _zoom;
      delta_x[1] = delta_x[1] / _zoom;
      delta_x[2] = delta_x[2] / _zoom;
      delta_y[0] = delta_y[0] / _zoom;
      delta_y[1] = delta_y[1] / _zoom;
      delta_y[2] = delta_y[2] / _zoom;
    }
    nlook = look;
    viskores::Normalize(nlook);
    CurrentSample = currentSample;
  }

  typedef void ControlSignature(FieldOut, FieldOut, FieldOut, FieldIn);

  typedef void ExecutionSignature(WorkIndex, _1, _2, _3, _4);
  VISKORES_EXEC
  void operator()(viskores::Id idx,
                  viskores::Float32& rayDirX,
                  viskores::Float32& rayDirY,
                  viskores::Float32& rayDirZ,
                  const viskores::Int32& seed) const
  {
    viskores::Vec2f_32 xy;
    Halton2D<3>(CurrentSample + seed, xy);
    xy[0] -= .5f;
    xy[1] -= .5f;

    viskores::Vec3f_32 ray_dir(rayDirX, rayDirY, rayDirZ);
    viskores::Float32 i = static_cast<viskores::Float32>(viskores::Int32(idx) % w);
    viskores::Float32 j = static_cast<viskores::Float32>(viskores::Int32(idx) / w);
    i += xy[0];
    j += xy[1];
    ray_dir = nlook + delta_x * ((2.f * i - viskores::Float32(w)) / 2.0f) +
      delta_y * ((2.f * j - viskores::Float32(h)) / 2.0f);
    viskores::Normalize(ray_dir);
    rayDirX = ray_dir[0];
    rayDirY = ray_dir[1];
    rayDirZ = ray_dir[2];
  }

}; // class perspective ray gen jitter

class Ortho2DRayGen : public viskores::worklet::WorkletMapField
{
public:
  viskores::Int32 w;
  viskores::Int32 h;
  viskores::Int32 Minx;
  viskores::Int32 Miny;
  viskores::Int32 SubsetWidth;
  viskores::Vec3f_32 PixelDelta;
  viskores::Vec3f_32 StartOffset;

  VISKORES_CONT
  Ortho2DRayGen(const viskores::rendering::raytracing::Camera& camera)
    : w(camera.GetWidth())
    , h(camera.GetHeight())
    , Minx(camera.GetSubsetMinX())
    , Miny(camera.GetSubsetMinY())
    , SubsetWidth(camera.GetSubsetWidth())
  {
    auto& camera2d = camera.GetCamera2D();
    viskores::Float32 left = camera2d.Left;
    viskores::Float32 right = camera2d.Right;
    viskores::Float32 bottom = camera2d.Bottom;
    viskores::Float32 top = camera2d.Top;

    viskores::Float32 vl, vr, vb, vt;
    camera.GetViewport(vl, vr, vb, vt);
    viskores::Float32 _w = static_cast<viskores::Float32>(this->w) * (vr - vl) / 2.f;
    viskores::Float32 _h = static_cast<viskores::Float32>(this->h) * (vt - vb) / 2.f;
    viskores::Vec2f_32 minPoint(left, bottom);
    viskores::Vec2f_32 maxPoint(right, top);

    // pixel size in world coordinate
    viskores::Vec2f_32 delta = maxPoint - minPoint;
    delta[0] /= viskores::Float32(_w);
    delta[1] /= viskores::Float32(_h);

    PixelDelta[0] = delta[0];
    PixelDelta[1] = delta[1];
    PixelDelta[2] = 0.f;

    // "first" ray starts at the bottom-lower corner, with half pixel offset. All other
    // pixels will be one pixel size (i.e. PixelData) apart.
    viskores::Vec2f_32 startOffset = minPoint + delta / 2.f;
    StartOffset[0] = startOffset[0];
    StartOffset[1] = startOffset[1];
    // always push the rays back from the origin
    StartOffset[2] = -1.f;
  }

  using ControlSignature =
    void(FieldOut, FieldOut, FieldOut, FieldOut, FieldOut, FieldOut, FieldOut);

  using ExecutionSignature = void(WorkIndex, _1, _2, _3, _4, _5, _6, _7);
  template <typename Precision>
  VISKORES_EXEC void operator()(viskores::Id idx,
                                Precision& rayDirX,
                                Precision& rayDirY,
                                Precision& rayDirZ,
                                Precision& rayOriginX,
                                Precision& rayOriginY,
                                Precision& rayOriginZ,
                                viskores::Id& pixelIndex) const
  {
    // this is 2d so always look down z
    rayDirX = 0.f;
    rayDirY = 0.f;
    rayDirZ = 1.f;

    //
    // Pixel subset is the pixels in the 2d viewport
    // not where the rays might intersect data like
    // the perspective ray gen
    //
    int i = viskores::Int32(idx) % SubsetWidth;
    int j = viskores::Int32(idx) / SubsetWidth;

    viskores::Vec3f_32 pos{ viskores::Float32(i), viskores::Float32(j), 0.f };

    viskores::Vec3f_32 origin = StartOffset + pos * PixelDelta;
    rayOriginX = origin[0];
    rayOriginY = origin[1];
    rayOriginZ = origin[2];

    i += Minx;
    j += Miny;
    pixelIndex = static_cast<viskores::Id>(j * w + i);
  }

}; // class perspective ray gen

class PerspectiveRayGen : public viskores::worklet::WorkletMapField
{
public:
  viskores::Int32 w;
  viskores::Int32 h;
  viskores::Float32 panX;
  viskores::Float32 panY;
  viskores::Int32 Minx;
  viskores::Int32 Miny;
  viskores::Int32 SubsetWidth;
  viskores::Vec3f_32 nlook; // normalized look
  viskores::Vec3f_32 delta_x;
  viskores::Vec3f_32 delta_y;
  VISKORES_CONT
  PerspectiveRayGen(viskores::Int32 width,
                    viskores::Int32 height,
                    viskores::Float32 fovY,
                    viskores::Vec3f_32 look,
                    viskores::Vec3f_32 up,
                    viskores::Float32 panx,
                    viskores::Float32 pany,
                    viskores::Float32 _zoom,
                    viskores::Int32 subsetWidth,
                    viskores::Int32 minx,
                    viskores::Int32 miny)
    : w(width)
    , h(height)
    , panX(panx)
    , panY(pany)
    , Minx(minx)
    , Miny(miny)
    , SubsetWidth(subsetWidth)
  {
    viskores::Float32 thy = tanf((fovY * viskores::Pi_180f()) * .5f);
    viskores::Float32 thx = (thy * width) / height;

    viskores::Vec3f_32 ru = viskores::Cross(look, up);
    viskores::Normalize(ru);

    viskores::Vec3f_32 rv = viskores::Cross(ru, look);
    viskores::Normalize(rv);

    delta_x = ru * (2 * thx / (float)w);
    delta_y = rv * (2 * thy / (float)h);

    if (_zoom > 0)
    {
      delta_x[0] = delta_x[0] / _zoom;
      delta_x[1] = delta_x[1] / _zoom;
      delta_x[2] = delta_x[2] / _zoom;
      delta_y[0] = delta_y[0] / _zoom;
      delta_y[1] = delta_y[1] / _zoom;
      delta_y[2] = delta_y[2] / _zoom;
      panX *= _zoom;
      panY *= _zoom;
    }

    nlook = look;
    viskores::Normalize(nlook);
  }

  using ControlSignature = void(FieldOut, FieldOut, FieldOut, FieldOut);

  using ExecutionSignature = void(WorkIndex, _1, _2, _3, _4);
  template <typename Precision>
  VISKORES_EXEC void operator()(viskores::Id idx,
                                Precision& rayDirX,
                                Precision& rayDirY,
                                Precision& rayDirZ,
                                viskores::Id& pixelIndex) const
  {
    auto i = viskores::Int32(idx) % SubsetWidth;
    auto j = viskores::Int32(idx) / SubsetWidth;
    i += Minx;
    j += Miny;
    // Write out the global pixelId
    pixelIndex = static_cast<viskores::Id>(j * w + i);

    viskores::Vec<Precision, 3> ray_dir = nlook +
      delta_x * ((2.f * Precision(i) - panX * Precision(w) - Precision(w)) / 2.0f) +
      delta_y * ((2.f * Precision(j) - panY * Precision(h) - Precision(h)) / 2.0f);
    // avoid some numerical issues
    for (viskores::Int32 d = 0; d < 3; ++d)
    {
      if (ray_dir[d] == 0.f)
        ray_dir[d] += 0.0000001f;
    }
    viskores::Normalize(ray_dir);
    rayDirX = ray_dir[0];
    rayDirY = ray_dir[1];
    rayDirZ = ray_dir[2];
  }

}; // class perspective ray gen

//---------------------------------------------------------------------------

viskores::Matrix<viskores::Float32, 4, 4> Camera3DStruct::CreateViewMatrix() const
{
  return MatrixHelpers::ViewMatrix(this->Position, this->LookAt, this->ViewUp);
}

viskores::Matrix<viskores::Float32, 4, 4> Camera3DStruct::CreateProjectionMatrix(
  viskores::Id width,
  viskores::Id height,
  viskores::Float32 nearPlane,
  viskores::Float32 farPlane) const
{
  viskores::Matrix<viskores::Float32, 4, 4> matrix;
  viskores::MatrixIdentity(matrix);

  viskores::Float32 AspectRatio = viskores::Float32(width) / viskores::Float32(height);
  viskores::Float32 fovRad = this->FieldOfView * viskores::Pi_180f();
  fovRad = viskores::Tan(fovRad * 0.5f);
  viskores::Float32 size = nearPlane * fovRad;
  viskores::Float32 left = -size * AspectRatio;
  viskores::Float32 right = size * AspectRatio;
  viskores::Float32 bottom = -size;
  viskores::Float32 top = size;

  matrix(0, 0) = 2.f * nearPlane / (right - left);
  matrix(1, 1) = 2.f * nearPlane / (top - bottom);
  matrix(0, 2) = (right + left) / (right - left);
  matrix(1, 2) = (top + bottom) / (top - bottom);
  matrix(2, 2) = -(farPlane + nearPlane) / (farPlane - nearPlane);
  matrix(3, 2) = -1.f;
  matrix(2, 3) = -(2.f * farPlane * nearPlane) / (farPlane - nearPlane);
  matrix(3, 3) = 0.f;

  viskores::Matrix<viskores::Float32, 4, 4> T, Z;
  T = viskores::Transform3DTranslate(this->XPan, this->YPan, 0.f);
  Z = viskores::Transform3DScale(this->Zoom, this->Zoom, 1.f);
  matrix = viskores::MatrixMultiply(Z, viskores::MatrixMultiply(T, matrix));
  return matrix;
}

//---------------------------------------------------------------------------

viskores::Matrix<viskores::Float32, 4, 4> Camera2DStruct::CreateViewMatrix() const
{
  viskores::Vec3f_32 lookAt(
    (this->Left + this->Right) / 2.f, (this->Top + this->Bottom) / 2.f, 0.f);
  viskores::Vec3f_32 position = lookAt;
  position[2] = 1.f;
  viskores::Vec3f_32 up(0, 1, 0);
  viskores::Matrix<viskores::Float32, 4, 4> V = MatrixHelpers::ViewMatrix(position, lookAt, up);
  viskores::Matrix<viskores::Float32, 4, 4> scaleMatrix =
    MatrixHelpers::CreateScale(this->XScale, 1, 1);
  V = viskores::MatrixMultiply(scaleMatrix, V);
  return V;
}

viskores::Matrix<viskores::Float32, 4, 4> Camera2DStruct::CreateProjectionMatrix(
  viskores::Float32 size,
  viskores::Float32 znear,
  viskores::Float32 zfar,
  viskores::Float32 aspect) const
{
  viskores::Matrix<viskores::Float32, 4, 4> matrix(0.f);
  viskores::Float32 left = -size / 2.f * aspect;
  viskores::Float32 right = size / 2.f * aspect;
  viskores::Float32 bottom = -size / 2.f;
  viskores::Float32 top = size / 2.f;

  matrix(0, 0) = 2.f / (right - left);
  matrix(1, 1) = 2.f / (top - bottom);
  matrix(2, 2) = -2.f / (zfar - znear);
  matrix(0, 3) = -(right + left) / (right - left);
  matrix(1, 3) = -(top + bottom) / (top - bottom);
  matrix(2, 3) = -(zfar + znear) / (zfar - znear);
  matrix(3, 3) = 1.f;

  viskores::Matrix<viskores::Float32, 4, 4> T, Z;
  T = viskores::Transform3DTranslate(this->XPan, this->YPan, 0.f);
  Z = viskores::Transform3DScale(this->Zoom, this->Zoom, 1.f);
  matrix = viskores::MatrixMultiply(Z, viskores::MatrixMultiply(T, matrix));
  return matrix;
}

VISKORES_CONT
void Camera::SetHeight(const viskores::Int32& height)
{
  if (height <= 0)
  {
    throw viskores::cont::ErrorBadValue("Camera height must be greater than zero.");
  }
  if (Height != height)
  {
    this->Height = height;
    this->IsViewDirty = true;
  }
}

VISKORES_CONT
viskores::Int32 Camera::GetHeight() const
{
  return this->Height;
}

VISKORES_CONT
void Camera::SetWidth(const viskores::Int32& width)
{
  if (width <= 0)
  {
    throw viskores::cont::ErrorBadValue("Camera width must be greater than zero.");
  }
  if (this->Width != width)
  {
    this->Width = width;
    this->IsViewDirty = true;
  }
}

VISKORES_CONT
viskores::Int32 Camera::GetWidth() const
{
  return this->Width;
}

VISKORES_CONT
viskores::Int32 Camera::GetSubsetWidth() const
{
  return this->SubsetWidth;
}

VISKORES_CONT
viskores::Int32 Camera::GetSubsetHeight() const
{
  return this->SubsetHeight;
}

VISKORES_CONT viskores::Int32 Camera::GetSubsetMinX() const
{
  return this->SubsetMinX;
}

VISKORES_CONT viskores::Int32 Camera::GetSubsetMinY() const
{
  return this->SubsetMinY;
}

VISKORES_CONT
void Camera::SetZoom(const viskores::Float32& zoom)
{
  if (zoom <= 0)
  {
    throw viskores::cont::ErrorBadValue("Camera zoom must be greater than zero.");
  }
  if ((this->Camera3D.Zoom != zoom) || (this->Camera3D.Zoom != zoom))
  {
    this->IsViewDirty = true;
    this->Camera3D.Zoom = zoom;
    this->Camera2D.Zoom = zoom;
  }
}

VISKORES_CONT
viskores::Float32 Camera::GetZoom() const
{
  if (this->IsOrthogonalProjection)
  {
    return this->Camera2D.Zoom;
  }
  else
  {
    return this->Camera2D.Zoom;
  }
}

VISKORES_CONT
void Camera::SetFieldOfView(const viskores::Float32& degrees)
{
  if (degrees <= 0)
  {
    throw viskores::cont::ErrorBadValue("Camera field of view must be greater than zero.");
  }
  if (degrees > 180)
  {
    throw viskores::cont::ErrorBadValue("Camera field of view must be less than 180.");
  }

  if (this->Camera3D.FieldOfView != degrees)
  {
    this->Camera3D.FieldOfView = degrees;
    this->IsViewDirty = true;
  }
}

VISKORES_CONT
viskores::Float32 Camera::GetFieldOfView() const
{
  return this->Camera3D.FieldOfView;
}

VISKORES_CONT
void Camera::SetUp(const viskores::Vec3f_32& up)
{
  if (this->Camera3D.ViewUp != up)
  {
    this->Camera3D.ViewUp = up;
    viskores::Normalize(this->Camera3D.ViewUp);
    this->IsViewDirty = true;
  }
}

VISKORES_CONT
viskores::Vec3f_32 Camera::GetUp() const
{
  return this->Camera3D.ViewUp;
}

VISKORES_CONT
void Camera::SetLookAt(const viskores::Vec3f_32& lookAt)
{
  if (this->Camera3D.LookAt != lookAt)
  {
    this->Camera3D.LookAt = lookAt;
    this->IsViewDirty = true;
  }
}

VISKORES_CONT
viskores::Vec3f_32 Camera::GetLookAt() const
{
  return this->Camera3D.LookAt;
}

VISKORES_CONT
void Camera::SetPosition(const viskores::Vec3f_32& position)
{
  if (this->Camera3D.Position != position)
  {
    this->Camera3D.Position = position;
    this->IsViewDirty = true;
  }
}

VISKORES_CONT
viskores::Vec3f_32 Camera::GetPosition() const
{
  return this->Camera3D.Position;
}

VISKORES_CONT viskores::Matrix<viskores::Float32, 4, 4>& Camera::GetViewProjectionMatrix() const
{
  this->UpdateViewProjectionMatrix();
  return this->ViewProjectionMat;
}

VISKORES_CONT
void Camera::ResetIsViewDirty()
{
  this->IsViewDirty = false;
}

VISKORES_CONT
bool Camera::GetIsViewDirty() const
{
  return this->IsViewDirty;
}

void Camera::GetPixelData(const viskores::cont::CoordinateSystem& coords,
                          viskores::Int32& activePixels,
                          viskores::Float32& aveRayDistance)
{
  viskores::Bounds boundingBox = coords.GetBounds();
  this->FindSubset(boundingBox);
  //Reset the camera look vector
  this->LookDirection = this->Camera3D.LookAt - this->Camera3D.Position;
  viskores::Normalize(this->LookDirection);
  const int size = this->SubsetWidth * this->SubsetHeight;
  viskores::cont::ArrayHandle<viskores::Float32> dists;
  viskores::cont::ArrayHandle<viskores::Int32> hits;
  dists.Allocate(size);
  hits.Allocate(size);

  //Create the ray direction
  viskores::worklet::DispatcherMapField<PixelData>(PixelData(this->Width,
                                                             this->Height,
                                                             this->Camera3D.FieldOfView,
                                                             this->LookDirection,
                                                             this->Camera3D.ViewUp,
                                                             this->Camera3D.Zoom,
                                                             this->SubsetWidth,
                                                             this->SubsetMinX,
                                                             this->SubsetMinY,
                                                             this->Camera3D.Position,
                                                             boundingBox))
    .Invoke(hits, dists); //X Y Z
  activePixels = viskores::cont::Algorithm::Reduce(hits, viskores::Int32(0));
  aveRayDistance = viskores::cont::Algorithm::Reduce(dists, viskores::Float32(0)) /
    viskores::Float32(activePixels);
}

VISKORES_CONT
void Camera::CreateRays(Ray<viskores::Float32>& rays, const viskores::Bounds& bounds)
{
  CreateRaysImpl(rays, bounds);
}

VISKORES_CONT
void Camera::CreateRays(Ray<viskores::Float64>& rays, const viskores::Bounds& bounds)
{
  CreateRaysImpl(rays, bounds);
}

template <typename Precision>
VISKORES_CONT void Camera::CreateRaysImpl(Ray<Precision>& rays, const viskores::Bounds& boundingBox)
{
  Logger* logger = Logger::GetInstance();
  viskores::cont::Timer createTimer;
  createTimer.Start();
  logger->OpenLogEntry("ray_camera");

  this->UpdateDimensions(rays, boundingBox);
  this->WriteSettingsToLog();

  viskores::cont::Timer timer;
  timer.Start();

  Precision infinity;
  GetInfinity(infinity);

  viskores::cont::ArrayHandleConstant<Precision> inf(infinity, rays.NumRays);
  viskores::cont::Algorithm::Copy(inf, rays.MaxDistance);

  viskores::cont::ArrayHandleConstant<Precision> zero(0, rays.NumRays);
  viskores::cont::Algorithm::Copy(zero, rays.MinDistance);
  viskores::cont::Algorithm::Copy(zero, rays.Distance);

  viskores::cont::ArrayHandleConstant<viskores::Id> initHit(-2, rays.NumRays);
  viskores::cont::Algorithm::Copy(initHit, rays.HitIdx);

  viskores::Float64 time = timer.GetElapsedTime();
  logger->AddLogData("camera_memset", time);
  timer.Start();

  //Reset the camera look vector
  this->LookDirection = this->Camera3D.LookAt - this->Camera3D.Position;
  viskores::Normalize(this->LookDirection);

  viskores::cont::Invoker invoke;
  if (this->IsOrthogonalProjection)
  {
    invoke(Ortho2DRayGen{ *this },
           rays.DirX,
           rays.DirY,
           rays.DirZ,
           rays.OriginX,
           rays.OriginY,
           rays.OriginZ,
           rays.PixelIdx);
  }
  else
  {
    //Create the ray direction
    invoke(PerspectiveRayGen{ this->Width,
                              this->Height,
                              this->Camera3D.FieldOfView,
                              this->LookDirection,
                              this->Camera3D.ViewUp,
                              this->Camera3D.XPan,
                              this->Camera3D.YPan,
                              this->Camera3D.Zoom,
                              this->SubsetWidth,
                              this->SubsetMinX,
                              this->SubsetMinY },
           rays.DirX,
           rays.DirY,
           rays.DirZ,
           rays.PixelIdx);

    //Set the origin of the ray back to the camera position
    viskores::cont::ArrayHandleConstant<Precision> posX(this->Camera3D.Position[0], rays.NumRays);
    viskores::cont::Algorithm::Copy(posX, rays.OriginX);

    viskores::cont::ArrayHandleConstant<Precision> posY(this->Camera3D.Position[1], rays.NumRays);
    viskores::cont::Algorithm::Copy(posY, rays.OriginY);

    viskores::cont::ArrayHandleConstant<Precision> posZ(this->Camera3D.Position[2], rays.NumRays);
    viskores::cont::Algorithm::Copy(posZ, rays.OriginZ);
  }

  time = timer.GetElapsedTime();
  logger->AddLogData("ray_gen", time);
  time = createTimer.GetElapsedTime();
  logger->CloseLogEntry(time);
} //create rays

VISKORES_CONT
void Camera::FindSubset(const viskores::Bounds& bounds)
{
  this->UpdateViewProjectionMatrix();
  viskores::Float32 x[2], y[2], z[2];
  x[0] = static_cast<viskores::Float32>(bounds.X.Min);
  x[1] = static_cast<viskores::Float32>(bounds.X.Max);
  y[0] = static_cast<viskores::Float32>(bounds.Y.Min);
  y[1] = static_cast<viskores::Float32>(bounds.Y.Max);
  z[0] = static_cast<viskores::Float32>(bounds.Z.Min);
  z[1] = static_cast<viskores::Float32>(bounds.Z.Max);
  //Inise the data bounds
  if (this->Camera3D.Position[0] >= x[0] && this->Camera3D.Position[0] <= x[1] &&
      this->Camera3D.Position[1] >= y[0] && this->Camera3D.Position[1] <= y[1] &&
      this->Camera3D.Position[2] >= z[0] && this->Camera3D.Position[2] <= z[1])
  {
    this->SubsetWidth = this->Width;
    this->SubsetHeight = this->Height;
    this->SubsetMinY = 0;
    this->SubsetMinX = 0;
    return;
  }

  viskores::Float32 xmin, ymin, xmax, ymax, zmin, zmax;
  xmin = viskores::Infinity32();
  ymin = viskores::Infinity32();
  zmin = viskores::Infinity32();
  xmax = viskores::NegativeInfinity32();
  ymax = viskores::NegativeInfinity32();
  zmax = viskores::NegativeInfinity32();
  viskores::Vec4f_32 extentPoint;
  for (viskores::Int32 i = 0; i < 2; ++i)
    for (viskores::Int32 j = 0; j < 2; ++j)
      for (viskores::Int32 k = 0; k < 2; ++k)
      {
        extentPoint[0] = x[i];
        extentPoint[1] = y[j];
        extentPoint[2] = z[k];
        extentPoint[3] = 1.f;
        viskores::Vec4f_32 transformed =
          viskores::MatrixMultiply(this->ViewProjectionMat, extentPoint);
        // perform the perspective divide
        for (viskores::Int32 a = 0; a < 3; ++a)
        {
          transformed[a] = transformed[a] / transformed[3];
        }

        transformed[0] = (transformed[0] * 0.5f + 0.5f) * static_cast<viskores::Float32>(Width);
        transformed[1] = (transformed[1] * 0.5f + 0.5f) * static_cast<viskores::Float32>(Height);
        transformed[2] = (transformed[2] * 0.5f + 0.5f);
        zmin = viskores::Min(zmin, transformed[2]);
        zmax = viskores::Max(zmax, transformed[2]);
        xmin = viskores::Min(xmin, transformed[0]);
        ymin = viskores::Min(ymin, transformed[1]);
        xmax = viskores::Max(xmax, transformed[0]);
        ymax = viskores::Max(ymax, transformed[1]);
      }

  xmin -= .001f;
  xmax += .001f;
  ymin -= .001f;
  ymax += .001f;
  xmin = viskores::Floor(viskores::Min(viskores::Max(0.f, xmin), viskores::Float32(Width)));
  xmax = viskores::Ceil(viskores::Min(viskores::Max(0.f, xmax), viskores::Float32(Width)));
  ymin = viskores::Floor(viskores::Min(viskores::Max(0.f, ymin), viskores::Float32(Height)));
  ymax = viskores::Ceil(viskores::Min(viskores::Max(0.f, ymax), viskores::Float32(Height)));

  Logger* logger = Logger::GetInstance();
  std::stringstream ss;
  ss << "(" << xmin << "," << ymin << "," << zmin << ")-";
  ss << "(" << xmax << "," << ymax << "," << zmax << ")";
  logger->AddLogData("pixel_range", ss.str());

  viskores::Int32 dx = viskores::Int32(xmax) - viskores::Int32(xmin);
  viskores::Int32 dy = viskores::Int32(ymax) - viskores::Int32(ymin);
  //
  //  scene is behind the camera
  //
  if (xmin >= xmax || ymin >= ymax)
  {
    this->SubsetWidth = 1;
    this->SubsetHeight = 1;
    this->SubsetMinX = 0;
    this->SubsetMinY = 0;
  }
  else
  {
    this->SubsetWidth = dx;
    this->SubsetHeight = dy;
    this->SubsetMinX = viskores::Int32(xmin);
    this->SubsetMinY = viskores::Int32(ymin);
  }
  logger->AddLogData("subset_width", dx);
  logger->AddLogData("subset_height", dy);
}

template <typename Precision>
VISKORES_CONT void Camera::UpdateDimensions(Ray<Precision>& rays,
                                            const viskores::Bounds& boundingBox)
{
  // If bounds have been provided, only cast rays that could hit the data
  bool imageSubsetModeOn = boundingBox.IsNonEmpty();

  //Find the pixel footprint
  if (imageSubsetModeOn && !this->IsOrthogonalProjection)
  {
    // Note:
    // Use clipping range provided, the subsetting does take into consideration
    // the near and far clipping planes.
    this->UpdateViewProjectionMatrix();
    this->FindSubset(boundingBox);
  }
  else if (this->IsOrthogonalProjection)
  {
    // 2D rendering has a viewport that represents the area of the canvas where the image
    // is drawn. Thus, we have to create rays corresponding to that region of the
    // canvas, so annotations are correctly rendered
    viskores::Float32 vl, vr, vb, vt;
    this->GetViewport(vl, vr, vb, vt);
    viskores::Float32 _x = static_cast<viskores::Float32>(this->GetWidth()) * (1.f + vl) / 2.f;
    viskores::Float32 _y = static_cast<viskores::Float32>(this->GetHeight()) * (1.f + vb) / 2.f;
    viskores::Float32 _w = static_cast<viskores::Float32>(this->GetWidth()) * (vr - vl) / 2.f;
    viskores::Float32 _h = static_cast<viskores::Float32>(this->GetHeight()) * (vt - vb) / 2.f;

    this->SubsetWidth = static_cast<viskores::Int32>(_w);
    this->SubsetHeight = static_cast<viskores::Int32>(_h);
    this->SubsetMinY = static_cast<viskores::Int32>(_y);
    this->SubsetMinX = static_cast<viskores::Int32>(_x);
  }
  else
  {
    //Update the image dimensions
    this->SubsetWidth = this->Width;
    this->SubsetHeight = this->Height;
    this->SubsetMinY = 0;
    this->SubsetMinX = 0;
  }

  // resize rays and buffers
  if (rays.NumRays != SubsetWidth * SubsetHeight)
  {
    RayOperations::Resize(rays, this->SubsetHeight * this->SubsetWidth);
  }
}

void Camera::UpdateViewProjectionMatrix() const
{
  viskores::Matrix<viskores::Float32, 4, 4> projection;
  viskores::Matrix<viskores::Float32, 4, 4> modelview;
  if (this->IsOrthogonalProjection)
  {
    viskores::Float32 size = viskores::Abs(this->Camera2D.Top - this->Camera2D.Bottom);
    viskores::Float32 aspect =
      (static_cast<viskores::Float32>(this->Width) * (this->ViewportRight - this->ViewportLeft)) /
      (static_cast<viskores::Float32>(this->Height) * (this->ViewportTop - this->ViewportBottom));
    projection =
      this->Camera2D.CreateProjectionMatrix(size, this->NearPlane, this->FarPlane, aspect);

    modelview = this->Camera2D.CreateViewMatrix();
  }
  else
  {
    projection = this->Camera3D.CreateProjectionMatrix(
      this->Width, this->Height, this->NearPlane, this->FarPlane);
    modelview = this->Camera3D.CreateViewMatrix();
  }
  this->ViewProjectionMat = viskores::MatrixMultiply(projection, modelview);
}

void Camera::CreateDebugRay(viskores::Vec2i_32 pixel, Ray<viskores::Float64>& rays)
{
  CreateDebugRayImp(pixel, rays);
}

void Camera::CreateDebugRay(viskores::Vec2i_32 pixel, Ray<viskores::Float32>& rays)
{
  CreateDebugRayImp(pixel, rays);
}

template <typename Precision>
void Camera::CreateDebugRayImp(viskores::Vec2i_32 pixel, Ray<Precision>& rays)
{
  RayOperations::Resize(rays, 1);
  viskores::Int32 pixelIndex = this->Width * (this->Height - pixel[1]) + pixel[0];
  rays.PixelIdx.WritePortal().Set(0, pixelIndex);
  rays.OriginX.WritePortal().Set(0, this->Camera3D.Position[0]);
  rays.OriginY.WritePortal().Set(0, this->Camera3D.Position[1]);
  rays.OriginZ.WritePortal().Set(0, this->Camera3D.Position[2]);


  viskores::Float32 infinity;
  GetInfinity(infinity);

  rays.MaxDistance.WritePortal().Set(0, infinity);
  rays.MinDistance.WritePortal().Set(0, 0.f);
  rays.HitIdx.WritePortal().Set(0, -2);


  viskores::Float32 thy = tanf((this->Camera3D.FieldOfView * viskores::Pi_180f()) * .5f);
  viskores::Float32 thx = (thy * this->Width) / this->Height;
  viskores::Vec3f_32 ru = viskores::Cross(this->LookDirection, this->Camera3D.ViewUp);
  viskores::Normalize(ru);

  viskores::Vec3f_32 rv = viskores::Cross(ru, this->LookDirection);
  viskores::Vec3f_32 delta_x, delta_y;
  viskores::Normalize(rv);
  delta_x = ru * (2 * thx / (float)this->Width);
  delta_y = rv * (2 * thy / (float)this->Height);

  if (this->Camera3D.Zoom > 0)
  {
    viskores::Float32 _zoom = this->Camera3D.Zoom;
    delta_x[0] = delta_x[0] / _zoom;
    delta_x[1] = delta_x[1] / _zoom;
    delta_x[2] = delta_x[2] / _zoom;
    delta_y[0] = delta_y[0] / _zoom;
    delta_y[1] = delta_y[1] / _zoom;
    delta_y[2] = delta_y[2] / _zoom;
  }
  viskores::Vec3f_32 nlook = this->LookDirection;
  viskores::Normalize(nlook);

  viskores::Vec<Precision, 3> ray_dir;
  int i = viskores::Int32(pixelIndex) % this->Width;
  int j = viskores::Int32(pixelIndex) / this->Height;
  ray_dir = nlook + delta_x * ((2.f * Precision(i) - Precision(this->Width)) / 2.0f) +
    delta_y * ((2.f * Precision(j) - Precision(this->Height)) / 2.0f);

  Precision dot = viskores::Dot(ray_dir, ray_dir);
  Precision sq_mag = viskores::Sqrt(dot);

  ray_dir[0] = ray_dir[0] / sq_mag;
  ray_dir[1] = ray_dir[1] / sq_mag;
  ray_dir[2] = ray_dir[2] / sq_mag;
  rays.DirX.WritePortal().Set(0, ray_dir[0]);
  rays.DirY.WritePortal().Set(0, ray_dir[1]);
  rays.DirZ.WritePortal().Set(0, ray_dir[2]);
}

void Camera::WriteSettingsToLog()
{
  Logger* logger = Logger::GetInstance();
  logger->AddLogData("position_x", this->Camera3D.Position[0]);
  logger->AddLogData("position_y", this->Camera3D.Position[1]);
  logger->AddLogData("position_z", this->Camera3D.Position[2]);

  logger->AddLogData("lookat_x", this->Camera3D.LookAt[0]);
  logger->AddLogData("lookat_y", this->Camera3D.LookAt[1]);
  logger->AddLogData("lookat_z", this->Camera3D.LookAt[2]);

  logger->AddLogData("up_x", this->Camera3D.ViewUp[0]);
  logger->AddLogData("up_y", this->Camera3D.ViewUp[1]);
  logger->AddLogData("up_z", this->Camera3D.ViewUp[2]);

  logger->AddLogData("fov_y", this->Camera3D.FieldOfView);
  logger->AddLogData("width", Width);
  logger->AddLogData("height", Height);
  logger->AddLogData("subset_height", SubsetHeight);
  logger->AddLogData("subset_width", SubsetWidth);
  logger->AddLogData("num_rays", SubsetWidth * SubsetHeight);
}

std::string Camera::ToString()
{
  std::stringstream sstream;
  sstream << "------------------------------------------------------------\n";
  sstream << "Position : [" << this->Camera3D.Position[0] << ",";
  sstream << this->Camera3D.Position[1] << ",";
  sstream << this->Camera3D.Position[2] << "]\n";
  sstream << "LookAt   : [" << this->Camera3D.LookAt[0] << ",";
  sstream << this->Camera3D.LookAt[1] << ",";
  sstream << this->Camera3D.LookAt[2] << "]\n";
  sstream << "FOV_X    : " << this->Camera3D.FieldOfView << "\n";
  sstream << "Up       : [" << this->Camera3D.ViewUp[0] << ",";
  sstream << this->Camera3D.ViewUp[1] << ",";
  sstream << this->Camera3D.ViewUp[2] << "]\n";
  sstream << "Width    : " << this->Width << "\n";
  sstream << "Height   : " << this->Height << "\n";
  sstream << "------------------------------------------------------------\n";
  return sstream.str();
}
}
}
} //namespace viskores::rendering::raytracing
