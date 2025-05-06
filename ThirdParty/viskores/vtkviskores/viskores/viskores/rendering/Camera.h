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
#ifndef viskores_rendering_Camera_h
#define viskores_rendering_Camera_h

#include <viskores/rendering/viskores_rendering_export.h>

#include <viskores/Bounds.h>
#include <viskores/Math.h>
#include <viskores/Matrix.h>
#include <viskores/Range.h>
#include <viskores/Transform3D.h>
#include <viskores/VectorAnalysis.h>
#include <viskores/rendering/MatrixHelpers.h>

namespace viskores
{
namespace rendering
{

/// @brief Specifies the viewport for a rendering.
///
/// The `viskores::rendering::View` object holds a `Camera` object to specify from
/// what perspective the rendering should take place.
///
/// A `Camera` operates in one of two major modes: 2D mode
/// or 3D mode. 2D mode is designed for looking at flat geometry (or close to flat
/// geometry) that is parallel to the x-y plane. 3D mode provides the freedom to
/// place the camera anywhere in 3D space.
class VISKORES_RENDERING_EXPORT Camera
{
  struct Camera3DStruct
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

  struct VISKORES_RENDERING_EXPORT Camera2DStruct
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

    viskores::Matrix<viskores::Float32, 4, 4> CreateProjectionMatrix(
      viskores::Float32 size,
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

public:
  enum struct Mode
  {
    TwoD,
    ThreeD,
  };

  VISKORES_CONT
  Camera(Mode vtype = Camera::Mode::ThreeD)
    : ModeType(vtype)
    , NearPlane(0.01f)
    , FarPlane(1000.0f)
    , ViewportLeft(-1.0f)
    , ViewportRight(1.0f)
    , ViewportBottom(-1.0f)
    , ViewportTop(1.0f)
  {
  }

  viskores::Matrix<viskores::Float32, 4, 4> CreateViewMatrix() const;

  viskores::Matrix<viskores::Float32, 4, 4> CreateProjectionMatrix(viskores::Id screenWidth,
                                                                   viskores::Id screenHeight) const;

  void GetRealViewport(viskores::Id screenWidth,
                       viskores::Id screenHeight,
                       viskores::Float32& left,
                       viskores::Float32& right,
                       viskores::Float32& bottom,
                       viskores::Float32& top) const;

  /// @brief The mode of the camera (2D or 3D).
  ///
  /// `viskores::rendering::Camera` can be set to a 2D or 3D mode. 2D mode is used for
  /// looking at data in the x-y plane. 3D mode allows the camera to be
  /// positioned anywhere and pointing at any place in 3D.
  ///
  VISKORES_CONT
  viskores::rendering::Camera::Mode GetMode() const { return this->ModeType; }
  /// @copydoc GetMode
  VISKORES_CONT
  void SetMode(viskores::rendering::Camera::Mode mode) { this->ModeType = mode; }
  /// @copydoc GetMode
  VISKORES_CONT
  void SetModeTo3D() { this->SetMode(viskores::rendering::Camera::Mode::ThreeD); }
  /// @copydoc GetMode
  VISKORES_CONT
  void SetModeTo2D() { this->SetMode(viskores::rendering::Camera::Mode::TwoD); }

  /// @brief The clipping range of the camera.
  ///
  /// The clipping range establishes the near and far clipping planes. These
  /// clipping planes are parallel to the viewing plane. The planes are defined
  /// by simply specifying the distance from the viewpoint. Renderers can (and
  /// usually do) remove any geometry closer than the near plane and further
  /// than the far plane.
  ///
  /// For precision purposes, it is best to place the near plane as far away as
  /// possible (while still being in front of any geometry). The far plane
  /// usually has less effect on the depth precision, so can be placed well far
  /// behind the geometry.
  ///
  VISKORES_CONT
  viskores::Range GetClippingRange() const
  {
    return viskores::Range(this->NearPlane, this->FarPlane);
  }
  /// @copydoc GetClippingRange
  VISKORES_CONT
  void SetClippingRange(viskores::Float32 nearPlane, viskores::Float32 farPlane)
  {
    this->NearPlane = nearPlane;
    this->FarPlane = farPlane;
  }
  /// @copydoc GetClippingRange
  VISKORES_CONT
  void SetClippingRange(viskores::Float64 nearPlane, viskores::Float64 farPlane)
  {
    this->SetClippingRange(static_cast<viskores::Float32>(nearPlane),
                           static_cast<viskores::Float32>(farPlane));
  }
  /// @copydoc GetClippingRange
  VISKORES_CONT
  void SetClippingRange(const viskores::Range& nearFarRange)
  {
    this->SetClippingRange(nearFarRange.Min, nearFarRange.Max);
  }

  /// @brief The viewport of the projection
  ///
  /// The projection of the camera can be offset to be centered around a subset
  /// of the rendered image. This is established with a "viewport," which is
  /// defined by the left/right and bottom/top of this viewport. The values of
  /// the viewport are relative to the rendered image's bounds. The left and
  /// bottom of the image are at -1 and the right and top are at 1.
  ///
  VISKORES_CONT
  void GetViewport(viskores::Float32& left,
                   viskores::Float32& right,
                   viskores::Float32& bottom,
                   viskores::Float32& top) const
  {
    left = this->ViewportLeft;
    right = this->ViewportRight;
    bottom = this->ViewportBottom;
    top = this->ViewportTop;
  }
  /// @copydoc GetViewport
  VISKORES_CONT
  void GetViewport(viskores::Float64& left,
                   viskores::Float64& right,
                   viskores::Float64& bottom,
                   viskores::Float64& top) const
  {
    left = this->ViewportLeft;
    right = this->ViewportRight;
    bottom = this->ViewportBottom;
    top = this->ViewportTop;
  }
  /// @copydoc GetViewport
  VISKORES_CONT
  viskores::Bounds GetViewport() const
  {
    return viskores::Bounds(
      this->ViewportLeft, this->ViewportRight, this->ViewportBottom, this->ViewportTop, 0.0, 0.0);
  }
  /// @copydoc GetViewport
  VISKORES_CONT
  void SetViewport(viskores::Float32 left,
                   viskores::Float32 right,
                   viskores::Float32 bottom,
                   viskores::Float32 top)
  {
    this->ViewportLeft = left;
    this->ViewportRight = right;
    this->ViewportBottom = bottom;
    this->ViewportTop = top;
  }
  /// @copydoc GetViewport
  VISKORES_CONT
  void SetViewport(viskores::Float64 left,
                   viskores::Float64 right,
                   viskores::Float64 bottom,
                   viskores::Float64 top)
  {
    this->SetViewport(static_cast<viskores::Float32>(left),
                      static_cast<viskores::Float32>(right),
                      static_cast<viskores::Float32>(bottom),
                      static_cast<viskores::Float32>(top));
  }
  /// @copydoc GetViewport
  VISKORES_CONT
  void SetViewport(const viskores::Bounds& viewportBounds)
  {
    this->SetViewport(
      viewportBounds.X.Min, viewportBounds.X.Max, viewportBounds.Y.Min, viewportBounds.Y.Max);
  }

  /// @brief The focal point the camera is looking at in 3D mode
  ///
  /// When in 3D mode, the camera is set up to be facing the \c LookAt
  /// position. If \c LookAt is set, the mode is changed to 3D mode.
  ///
  VISKORES_CONT
  const viskores::Vec3f_32& GetLookAt() const { return this->Camera3D.LookAt; }
  /// @copydoc GetLookAt
  VISKORES_CONT
  void SetLookAt(const viskores::Vec3f_32& lookAt)
  {
    this->SetModeTo3D();
    this->Camera3D.LookAt = lookAt;
  }
  /// @copydoc GetLookAt
  VISKORES_CONT
  void SetLookAt(const viskores::Vec<Float64, 3>& lookAt)
  {
    this->SetLookAt(viskores::Vec<Float32, 3>(lookAt));
  }

  /// @brief The spatial position of the camera in 3D mode
  ///
  /// When in 3D mode, the camera is modeled to be at a particular location. If
  /// `Position` is set, the mode is changed to 3D mode.
  ///
  VISKORES_CONT
  const viskores::Vec3f_32& GetPosition() const { return this->Camera3D.Position; }
  /// @copydoc GetPosition
  VISKORES_CONT
  void SetPosition(const viskores::Vec3f_32& position)
  {
    this->SetModeTo3D();
    this->Camera3D.Position = position;
  }
  /// @copydoc GetPosition
  VISKORES_CONT
  void SetPosition(const viskores::Vec3f_64& position)
  {
    this->SetPosition(viskores::Vec3f_32(position));
  }

  /// @brief The up orientation of the camera in 3D mode
  ///
  /// When in 3D mode, the camera is modeled to be at a particular location and
  /// looking at a particular spot. The view up vector orients the rotation of
  /// the image so that the top of the image is in the direction pointed to by
  /// view up. If `ViewUp` is set, the mode is changed to 3D mode.
  ///
  VISKORES_CONT
  const viskores::Vec3f_32& GetViewUp() const { return this->Camera3D.ViewUp; }
  /// @copydoc GetViewUp
  VISKORES_CONT
  void SetViewUp(const viskores::Vec3f_32& viewUp)
  {
    this->SetModeTo3D();
    this->Camera3D.ViewUp = viewUp;
  }
  /// @copydoc GetViewUp
  VISKORES_CONT
  void SetViewUp(const viskores::Vec3f_64& viewUp) { this->SetViewUp(viskores::Vec3f_32(viewUp)); }

  /// @brief The xscale of the camera
  ///
  /// The xscale forces the 2D curves to be full-frame
  ///
  /// Setting the xscale changes the mode to 2D.
  ///
  VISKORES_CONT
  viskores::Float32 GetXScale() const { return this->Camera2D.XScale; }
  /// @copydoc GetXScale
  VISKORES_CONT
  void SetXScale(viskores::Float32 xscale)
  {
    this->SetModeTo2D();
    this->Camera2D.XScale = xscale;
  }
  /// @copydoc GetXScale
  VISKORES_CONT
  void SetXScale(viskores::Float64 xscale)
  {
    this->SetXScale(static_cast<viskores::Float32>(xscale));
  }

  /// @brief The field of view angle
  ///
  /// The field of view defines the angle (in degrees) that are visible from
  /// the camera position.
  ///
  /// Setting the field of view changes the mode to 3D.
  ///
  VISKORES_CONT
  viskores::Float32 GetFieldOfView() const { return this->Camera3D.FieldOfView; }
  /// @copydoc GetFieldOfView
  VISKORES_CONT
  void SetFieldOfView(viskores::Float32 fov)
  {
    this->SetModeTo3D();
    this->Camera3D.FieldOfView = fov;
  }
  /// @copydoc GetFieldOfView
  VISKORES_CONT
  void SetFieldOfView(viskores::Float64 fov)
  {
    this->SetFieldOfView(static_cast<viskores::Float32>(fov));
  }

  /// @brief Pans the camera
  ///
  /// Panning the camera shifts the view horizontially and/or vertically with
  /// respect to the image plane.
  ///
  /// Panning works in either 2D or 3D mode.
  void Pan(viskores::Float32 dx, viskores::Float32 dy);
  /// @copydoc Pan
  VISKORES_CONT
  void Pan(viskores::Float64 dx, viskores::Float64 dy)
  {
    this->Pan(static_cast<viskores::Float32>(dx), static_cast<viskores::Float32>(dy));
  }
  /// @copydoc Pan
  VISKORES_CONT
  void Pan(viskores::Vec2f_32 direction) { this->Pan(direction[0], direction[1]); }
  /// @copydoc Pan
  VISKORES_CONT
  void Pan(viskores::Vec2f_64 direction) { this->Pan(direction[0], direction[1]); }
  /// @copydoc Pan
  VISKORES_CONT
  viskores::Vec2f_32 GetPan() const
  {
    viskores::Vec2f_32 pan;
    // Note that the 2D and 3D pan are always set the same.
    pan[0] = this->Camera3D.XPan;
    pan[1] = this->Camera3D.YPan;
    return pan;
  }


  /// @brief Zooms the camera in or out
  ///
  /// Zooming the camera scales everything in the image up or down. Positive
  /// zoom makes the geometry look bigger or closer. Negative zoom has the
  /// opposite effect. A zoom of 0 has no effect.
  ///
  /// Zooming works in either 2D or 3D mode.
  void Zoom(viskores::Float32 zoom);
  /// @copydoc Zoom
  VISKORES_CONT
  void Zoom(viskores::Float64 zoom) { this->Zoom(static_cast<viskores::Float32>(zoom)); }
  /// @copydoc Zoom
  VISKORES_CONT
  viskores::Float32 GetZoom() const { return this->Camera3D.Zoom; }

  /// @brief Moves the camera as if a point was dragged along a sphere.
  ///
  /// `TrackballRotate()` takes the normalized screen coordinates (in the range
  /// -1 to 1) and rotates the camera around the `LookAt` position. The rotation
  /// first projects the points to a sphere around the `LookAt` position. The
  /// camera is then rotated as if the start point was dragged to the end point
  /// along with the world.
  ///
  /// `TrackballRotate()` changes the mode to 3D.
  ///
  void TrackballRotate(viskores::Float32 startX,
                       viskores::Float32 startY,
                       viskores::Float32 endX,
                       viskores::Float32 endY);
  /// @copydoc TrackballRotate
  VISKORES_CONT
  void TrackballRotate(viskores::Float64 startX,
                       viskores::Float64 startY,
                       viskores::Float64 endX,
                       viskores::Float64 endY)
  {
    this->TrackballRotate(static_cast<viskores::Float32>(startX),
                          static_cast<viskores::Float32>(startY),
                          static_cast<viskores::Float32>(endX),
                          static_cast<viskores::Float32>(endY));
  }

  /// @brief Set up the camera to look at geometry
  ///
  /// `ResetToBounds()` takes a `viskores::Bounds` structure containing the bounds in
  /// 3D space that contain the geometry being rendered. This method sets up
  /// the camera so that it is looking at this region in space. The view
  /// direction is preserved. `ResetToBounds()` can also take optional padding
  /// that the viewpoint should preserve around the object. Padding is specified
  /// as the fraction of the bounds to add as padding.
  ///
  void ResetToBounds(const viskores::Bounds& dataBounds);
  /// @copydoc ResetToBounds
  void ResetToBounds(const viskores::Bounds& dataBounds, viskores::Float64 dataViewPadding);
  /// @copydoc ResetToBounds
  void ResetToBounds(const viskores::Bounds& dataBounds,
                     viskores::Float64 XDataViewPadding,
                     viskores::Float64 YDataViewPadding,
                     viskores::Float64 ZDataViewPadding);

  /// @brief Roll the camera
  ///
  /// Rotates the camera around the view direction by the given angle. The
  /// angle is given in degrees.
  ///
  /// Roll is currently only supported for 3D cameras.
  ///
  void Roll(viskores::Float32 angleDegrees);
  /// @copydoc Roll
  VISKORES_CONT
  void Roll(viskores::Float64 angleDegrees)
  {
    this->Roll(static_cast<viskores::Float32>(angleDegrees));
  }

  /// @brief Rotate the camera about the view up vector centered at the focal point.
  ///
  /// Note that the view up vector is whatever was set via `SetViewUp()`, and is
  /// not necessarily perpendicular to the direction of projection. The angle is
  /// given in degrees.
  ///
  /// `Azimuth()` only makes sense for 3D cameras, so the camera mode will be set
  /// to 3D when this method is called.
  ///
  void Azimuth(viskores::Float32 angleDegrees);
  /// @copydoc Azimuth
  VISKORES_CONT
  void Azimuth(viskores::Float64 angleDegrees)
  {
    this->Azimuth(static_cast<viskores::Float32>(angleDegrees));
  }

  /// @brief Rotate the camera vertically around the focal point.
  ///
  /// Specifically, this rotates the camera about the cross product of the
  /// negative of the direction of projection and the view up vector, using the
  /// focal point (`LookAt`) as the center of rotation. The angle is given
  /// in degrees.
  ///
  /// `Elevation()` only makes sense for 3D cameras, so the camera mode will be set
  /// to 3D when this method is called.
  ///
  void Elevation(viskores::Float32 angleDegrees);
  /// @copydoc Elevation
  VISKORES_CONT
  void Elevation(viskores::Float64 angleDegrees)
  {
    this->Elevation(static_cast<viskores::Float32>(angleDegrees));
  }

  /// @brief Move the camera toward or away from the focal point.
  ///
  /// Specifically, this divides the camera's distance from the focal point
  /// (`LookAt`) by the given value. Use a value greater than one to dolly in
  /// toward the focal point, and use a value less than one to dolly-out away
  /// from the focal point.
  ///
  /// `Dolly()` has a similar effect as `Zoom()` since an object will appear larger
  /// when the camera is closer. However, because you are moving the camera, `Dolly()`
  /// can change the perspective relative to objects such as moving inside an object
  /// for an interior perspective whereas `Zoom()` will just change the size of the
  /// visible objects.
  ///
  /// `Dolly()` only makes sense for 3D cameras, so the camera mode will be set to
  /// 3D when this method is called.
  ///
  void Dolly(viskores::Float32 value);
  /// @copydoc Dolly
  VISKORES_CONT
  void Dolly(viskores::Float64 value) { this->Dolly(static_cast<viskores::Float32>(value)); }

  /// @brief The viewable region in the x-y plane
  ///
  /// When the camera is in 2D, it is looking at some region of the x-y plane.
  /// The region being looked at is defined by the range in x (determined by
  /// the left and right sides) and by the range in y (determined by the bottom
  /// and top sides).
  ///
  /// `SetViewRange2D()` changes the camera mode to 2D.
  ///
  VISKORES_CONT
  void GetViewRange2D(viskores::Float32& left,
                      viskores::Float32& right,
                      viskores::Float32& bottom,
                      viskores::Float32& top) const
  {
    left = this->Camera2D.Left;
    right = this->Camera2D.Right;
    bottom = this->Camera2D.Bottom;
    top = this->Camera2D.Top;
  }
  /// @copydoc GetViewRange2D
  VISKORES_CONT
  viskores::Bounds GetViewRange2D() const
  {
    return viskores::Bounds(this->Camera2D.Left,
                            this->Camera2D.Right,
                            this->Camera2D.Bottom,
                            this->Camera2D.Top,
                            0.0,
                            0.0);
  }
  /// @copydoc GetViewRange2D
  VISKORES_CONT
  void SetViewRange2D(viskores::Float32 left,
                      viskores::Float32 right,
                      viskores::Float32 bottom,
                      viskores::Float32 top)
  {
    this->SetModeTo2D();
    this->Camera2D.Left = left;
    this->Camera2D.Right = right;
    this->Camera2D.Bottom = bottom;
    this->Camera2D.Top = top;

    this->Camera2D.XPan = 0;
    this->Camera2D.YPan = 0;
    this->Camera2D.Zoom = 1;
  }
  /// @copydoc GetViewRange2D
  VISKORES_CONT
  void SetViewRange2D(viskores::Float64 left,
                      viskores::Float64 right,
                      viskores::Float64 bottom,
                      viskores::Float64 top)
  {
    this->SetViewRange2D(static_cast<viskores::Float32>(left),
                         static_cast<viskores::Float32>(right),
                         static_cast<viskores::Float32>(bottom),
                         static_cast<viskores::Float32>(top));
  }
  /// @copydoc GetViewRange2D
  VISKORES_CONT
  void SetViewRange2D(const viskores::Range& xRange, const viskores::Range& yRange)
  {
    this->SetViewRange2D(xRange.Min, xRange.Max, yRange.Min, yRange.Max);
  }
  VISKORES_CONT
  void SetViewRange2D(const viskores::Bounds& viewRange)
  {
    this->SetViewRange2D(viewRange.X, viewRange.Y);
  }

  VISKORES_CONT
  void Print() const;

private:
  Mode ModeType;
  Camera3DStruct Camera3D;
  Camera2DStruct Camera2D;

  viskores::Float32 NearPlane;
  viskores::Float32 FarPlane;

  viskores::Float32 ViewportLeft;
  viskores::Float32 ViewportRight;
  viskores::Float32 ViewportBottom;
  viskores::Float32 ViewportTop;
};
}
} // namespace viskores::rendering

#endif // viskores_rendering_Camera_h
