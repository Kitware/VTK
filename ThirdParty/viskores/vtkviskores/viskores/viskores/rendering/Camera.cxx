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

#include <viskores/rendering/Camera.h>

namespace viskores
{
namespace rendering
{

viskores::Matrix<viskores::Float32, 4, 4> Camera::Camera3DStruct::CreateViewMatrix() const
{
  return MatrixHelpers::ViewMatrix(this->Position, this->LookAt, this->ViewUp);
}

viskores::Matrix<viskores::Float32, 4, 4> Camera::Camera3DStruct::CreateProjectionMatrix(
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

viskores::Matrix<viskores::Float32, 4, 4> Camera::Camera2DStruct::CreateViewMatrix() const
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

viskores::Matrix<viskores::Float32, 4, 4> Camera::Camera2DStruct::CreateProjectionMatrix(
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

//---------------------------------------------------------------------------

viskores::Matrix<viskores::Float32, 4, 4> Camera::CreateViewMatrix() const
{
  if (this->ModeType == Camera::Mode::ThreeD)
  {
    return this->Camera3D.CreateViewMatrix();
  }
  else
  {
    return this->Camera2D.CreateViewMatrix();
  }
}

viskores::Matrix<viskores::Float32, 4, 4> Camera::CreateProjectionMatrix(
  viskores::Id screenWidth,
  viskores::Id screenHeight) const
{
  if (this->ModeType == Camera::Mode::ThreeD)
  {
    return this->Camera3D.CreateProjectionMatrix(
      screenWidth, screenHeight, this->NearPlane, this->FarPlane);
  }
  else
  {
    viskores::Float32 size = viskores::Abs(this->Camera2D.Top - this->Camera2D.Bottom);
    viskores::Float32 left, right, bottom, top;
    this->GetRealViewport(screenWidth, screenHeight, left, right, bottom, top);
    viskores::Float32 aspect = (static_cast<viskores::Float32>(screenWidth) * (right - left)) /
      (static_cast<viskores::Float32>(screenHeight) * (top - bottom));

    return this->Camera2D.CreateProjectionMatrix(size, this->NearPlane, this->FarPlane, aspect);
  }
}

void Camera::GetRealViewport(viskores::Id screenWidth,
                             viskores::Id screenHeight,
                             viskores::Float32& left,
                             viskores::Float32& right,
                             viskores::Float32& bottom,
                             viskores::Float32& top) const
{
  if (this->ModeType == Camera::Mode::ThreeD)
  {
    left = this->ViewportLeft;
    right = this->ViewportRight;
    bottom = this->ViewportBottom;
    top = this->ViewportTop;
  }
  else
  {
    viskores::Float32 maxvw =
      (this->ViewportRight - this->ViewportLeft) * static_cast<viskores::Float32>(screenWidth);
    viskores::Float32 maxvh =
      (this->ViewportTop - this->ViewportBottom) * static_cast<viskores::Float32>(screenHeight);
    viskores::Float32 waspect = maxvw / maxvh;
    viskores::Float32 daspect =
      (this->Camera2D.Right - this->Camera2D.Left) / (this->Camera2D.Top - this->Camera2D.Bottom);
    daspect *= this->Camera2D.XScale;

//needed as center is a constant value
#if defined(VISKORES_MSVC)
#pragma warning(push)
#pragma warning(disable : 4127) // conditional expression is constant
#endif

    const bool center = true; // if false, anchor to bottom-left
    if (waspect > daspect)
    {
      viskores::Float32 new_w = (this->ViewportRight - this->ViewportLeft) * daspect / waspect;
      if (center)
      {
        left = (this->ViewportLeft + this->ViewportRight) / 2.f - new_w / 2.f;
        right = (this->ViewportLeft + this->ViewportRight) / 2.f + new_w / 2.f;
      }
      else
      {
        left = this->ViewportLeft;
        right = this->ViewportLeft + new_w;
      }
      bottom = this->ViewportBottom;
      top = this->ViewportTop;
    }
    else
    {
      viskores::Float32 new_h = (this->ViewportTop - this->ViewportBottom) * waspect / daspect;
      if (center)
      {
        bottom = (this->ViewportBottom + this->ViewportTop) / 2.f - new_h / 2.f;
        top = (this->ViewportBottom + this->ViewportTop) / 2.f + new_h / 2.f;
      }
      else
      {
        bottom = this->ViewportBottom;
        top = this->ViewportBottom + new_h;
      }
      left = this->ViewportLeft;
      right = this->ViewportRight;
    }
#if defined(VISKORES_MSVC)
#pragma warning(pop)
#endif
  }
}

void Camera::Pan(viskores::Float32 dx, viskores::Float32 dy)
{
  this->Camera3D.XPan += dx;
  this->Camera3D.YPan += dy;
  this->Camera2D.XPan += dx;
  this->Camera2D.YPan += dy;
}

void Camera::Zoom(viskores::Float32 zoom)
{
  viskores::Float32 factor = viskores::Pow(4.0f, zoom);
  this->Camera3D.Zoom *= factor;
  this->Camera3D.XPan *= factor;
  this->Camera3D.YPan *= factor;
  this->Camera2D.Zoom *= factor;
  this->Camera2D.XPan *= factor;
  this->Camera2D.YPan *= factor;
}

void Camera::TrackballRotate(viskores::Float32 startX,
                             viskores::Float32 startY,
                             viskores::Float32 endX,
                             viskores::Float32 endY)
{
  viskores::Matrix<viskores::Float32, 4, 4> rotate =
    MatrixHelpers::TrackballMatrix(startX, startY, endX, endY);

  //Translate matrix
  viskores::Matrix<viskores::Float32, 4, 4> translate =
    viskores::Transform3DTranslate(-this->Camera3D.LookAt);

  //Translate matrix
  viskores::Matrix<viskores::Float32, 4, 4> inverseTranslate =
    viskores::Transform3DTranslate(this->Camera3D.LookAt);

  viskores::Matrix<viskores::Float32, 4, 4> view = this->CreateViewMatrix();
  view(0, 3) = 0;
  view(1, 3) = 0;
  view(2, 3) = 0;

  viskores::Matrix<viskores::Float32, 4, 4> inverseView = viskores::MatrixTranspose(view);

  //fullTransform = inverseTranslate * inverseView * rotate * view * translate
  viskores::Matrix<viskores::Float32, 4, 4> fullTransform;
  fullTransform = viskores::MatrixMultiply(
    inverseTranslate,
    viskores::MatrixMultiply(
      inverseView, viskores::MatrixMultiply(rotate, viskores::MatrixMultiply(view, translate))));
  this->Camera3D.Position = viskores::Transform3DPoint(fullTransform, this->Camera3D.Position);
  this->Camera3D.LookAt = viskores::Transform3DPoint(fullTransform, this->Camera3D.LookAt);
  this->Camera3D.ViewUp = viskores::Transform3DVector(fullTransform, this->Camera3D.ViewUp);
}

namespace
{

void PadRange(viskores::Range& range, viskores::Float64 padding)
{
  if (range.IsNonEmpty())
  {
    viskores::Float64 padAmount = padding * range.Length();
    range.Min -= padAmount;
    range.Max += padAmount;
  }
  else
  {
    range.Include(0);
  }
}

} // anonymous namespace

void Camera::ResetToBounds(const viskores::Bounds& dataBounds,
                           const viskores::Float64 XDataViewPadding,
                           const viskores::Float64 YDataViewPadding,
                           const viskores::Float64 ZDataViewPadding)
{
  // Save camera mode
  Mode saveMode = this->GetMode();

  // Pad view around data extents
  viskores::Bounds db = dataBounds;
  PadRange(db.X, XDataViewPadding);
  PadRange(db.Y, YDataViewPadding);
  PadRange(db.Z, ZDataViewPadding);

  // Reset for 3D camera
  viskores::Vec3f_32 directionOfProjection = this->GetPosition() - this->GetLookAt();
  viskores::Normalize(directionOfProjection);

  viskores::Vec3f_32 center = db.Center();
  this->SetLookAt(center);

  viskores::Vec3f_32 totalExtent;
  totalExtent[0] = viskores::Float32(db.X.Length());
  totalExtent[1] = viskores::Float32(db.Y.Length());
  totalExtent[2] = viskores::Float32(db.Z.Length());
  viskores::Float32 diagonalLength = viskores::Magnitude(totalExtent);
  if (diagonalLength < XDataViewPadding)
  {
    diagonalLength = static_cast<viskores::Float32>(XDataViewPadding);
  }
  this->SetPosition(center + directionOfProjection * diagonalLength * 1.0f);
  this->SetFieldOfView(60.0f);
  this->SetClippingRange(0.1f * diagonalLength, diagonalLength * 10.0f);

  // Reset for 2D camera
  this->SetViewRange2D(db);

  // Reset pan and zoom
  this->Camera3D.XPan = 0;
  this->Camera3D.YPan = 0;
  this->Camera3D.Zoom = 1;
  this->Camera2D.XPan = 0;
  this->Camera2D.YPan = 0;
  this->Camera2D.Zoom = 1;

  // Restore camera mode
  this->SetMode(saveMode);
}

// Enable the ability to pad the data extents in the final view
void Camera::ResetToBounds(const viskores::Bounds& dataBounds,
                           const viskores::Float64 dataViewPadding)
{
  Camera::ResetToBounds(dataBounds, dataViewPadding, dataViewPadding, dataViewPadding);
}

void Camera::ResetToBounds(const viskores::Bounds& dataBounds)
{
  Camera::ResetToBounds(dataBounds, 0);
}

void Camera::Roll(viskores::Float32 angleDegrees)
{
  viskores::Vec3f_32 directionOfProjection = this->GetLookAt() - this->GetPosition();
  viskores::Matrix<viskores::Float32, 4, 4> rotateTransform =
    viskores::Transform3DRotate(angleDegrees, directionOfProjection);

  this->SetViewUp(viskores::Transform3DVector(rotateTransform, this->GetViewUp()));
}

void Camera::Azimuth(viskores::Float32 angleDegrees)
{
  // Translate to the focal point (LookAt), rotate about view up, and
  // translate back again.
  viskores::Matrix<viskores::Float32, 4, 4> transform =
    viskores::Transform3DTranslate(this->GetLookAt());
  transform = viskores::MatrixMultiply(
    transform, viskores::Transform3DRotate(angleDegrees, this->GetViewUp()));
  transform =
    viskores::MatrixMultiply(transform, viskores::Transform3DTranslate(-this->GetLookAt()));

  this->SetPosition(viskores::Transform3DPoint(transform, this->GetPosition()));
}

void Camera::Elevation(viskores::Float32 angleDegrees)
{
  viskores::Vec3f_32 axisOfRotation =
    viskores::Cross(this->GetPosition() - this->GetLookAt(), this->GetViewUp());

  // Translate to the focal point (LookAt), rotate about the defined axis,
  // and translate back again.
  viskores::Matrix<viskores::Float32, 4, 4> transform =
    viskores::Transform3DTranslate(this->GetLookAt());
  transform =
    viskores::MatrixMultiply(transform, viskores::Transform3DRotate(angleDegrees, axisOfRotation));
  transform =
    viskores::MatrixMultiply(transform, viskores::Transform3DTranslate(-this->GetLookAt()));

  this->SetPosition(viskores::Transform3DPoint(transform, this->GetPosition()));
}

void Camera::Dolly(viskores::Float32 value)
{
  if (value <= viskores::Epsilon32())
  {
    return;
  }

  viskores::Vec3f_32 lookAtToPos = this->GetPosition() - this->GetLookAt();

  this->SetPosition(this->GetLookAt() + (1.0f / value) * lookAtToPos);
}

void Camera::Print() const
{
  if (this->ModeType == Camera::Mode::ThreeD)
  {
    std::cout << "Camera: 3D" << std::endl;
    std::cout << "  LookAt: " << Camera3D.LookAt << std::endl;
    std::cout << "  Pos   : " << Camera3D.Position << std::endl;
    std::cout << "  Up    : " << Camera3D.ViewUp << std::endl;
    std::cout << "  FOV   : " << GetFieldOfView() << std::endl;
    std::cout << "  Clip  : " << GetClippingRange() << std::endl;
    std::cout << "  XyZ   : " << Camera3D.XPan << " " << Camera3D.YPan << " " << Camera3D.Zoom
              << std::endl;
    viskores::Matrix<viskores::Float32, 4, 4> pm, vm;
    pm = CreateProjectionMatrix(512, 512);
    vm = CreateViewMatrix();
    std::cout << " PM: " << std::endl;
    std::cout << pm[0][0] << " " << pm[0][1] << " " << pm[0][2] << " " << pm[0][3] << std::endl;
    std::cout << pm[1][0] << " " << pm[1][1] << " " << pm[1][2] << " " << pm[1][3] << std::endl;
    std::cout << pm[2][0] << " " << pm[2][1] << " " << pm[2][2] << " " << pm[2][3] << std::endl;
    std::cout << pm[3][0] << " " << pm[3][1] << " " << pm[3][2] << " " << pm[3][3] << std::endl;
    std::cout << " VM: " << std::endl;
    std::cout << vm[0][0] << " " << vm[0][1] << " " << vm[0][2] << " " << vm[0][3] << std::endl;
    std::cout << vm[1][0] << " " << vm[1][1] << " " << vm[1][2] << " " << vm[1][3] << std::endl;
    std::cout << vm[2][0] << " " << vm[2][1] << " " << vm[2][2] << " " << vm[2][3] << std::endl;
    std::cout << vm[3][0] << " " << vm[3][1] << " " << vm[3][2] << " " << vm[3][3] << std::endl;
  }
  else if (this->ModeType == Camera::Mode::TwoD)
  {
    std::cout << "Camera: 2D" << std::endl;
    std::cout << "  LRBT: " << Camera2D.Left << " " << Camera2D.Right << " " << Camera2D.Bottom
              << " " << Camera2D.Top << std::endl;
    std::cout << "  XY  : " << Camera2D.XPan << " " << Camera2D.YPan << std::endl;
    std::cout << "  SZ  : " << Camera2D.XScale << " " << Camera2D.Zoom << std::endl;
    viskores::Matrix<viskores::Float32, 4, 4> pm, vm;
    pm = CreateProjectionMatrix(512, 512);
    vm = CreateViewMatrix();
    std::cout << " PM: " << std::endl;
    std::cout << pm[0][0] << " " << pm[0][1] << " " << pm[0][2] << " " << pm[0][3] << std::endl;
    std::cout << pm[1][0] << " " << pm[1][1] << " " << pm[1][2] << " " << pm[1][3] << std::endl;
    std::cout << pm[2][0] << " " << pm[2][1] << " " << pm[2][2] << " " << pm[2][3] << std::endl;
    std::cout << pm[3][0] << " " << pm[3][1] << " " << pm[3][2] << " " << pm[3][3] << std::endl;
    std::cout << " VM: " << std::endl;
    std::cout << vm[0][0] << " " << vm[0][1] << " " << vm[0][2] << " " << vm[0][3] << std::endl;
    std::cout << vm[1][0] << " " << vm[1][1] << " " << vm[1][2] << " " << vm[1][3] << std::endl;
    std::cout << vm[2][0] << " " << vm[2][1] << " " << vm[2][2] << " " << vm[2][3] << std::endl;
    std::cout << vm[3][0] << " " << vm[3][1] << " " << vm[3][2] << " " << vm[3][3] << std::endl;
  }
}
}
} // namespace viskores::rendering
