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

#ifndef viskores_filter_field_transform_PointTransform_h
#define viskores_filter_field_transform_PointTransform_h

#include <viskores/Matrix.h>
#include <viskores/Transform3D.h>

#include <viskores/filter/Filter.h>
#include <viskores/filter/field_transform/viskores_filter_field_transform_export.h>

namespace viskores
{
namespace filter
{
namespace field_transform
{
/// \brief Perform affine transforms to point coordinates or vector fields.
///
/// This filter will take a data set and a field of 3 dimensional vectors and perform
/// the specified point transform operation. Several methods are provided to apply
/// many common affine transformations (e.g., translation, rotation, and scale).
/// You can also provide a general 4x4 transformation matrix with `SetTransform()`.
///
/// The main use case for `PointTransform` is to perform transformations of
/// objects in 3D space, which is done by applying these transforms to the
/// coordinate system. This filter will operate on the `viskores::cont::CoordinateSystem`
/// of the input data unless a different active field is specified. Likewise,
/// this filter will save its results as the first coordinate system in the output
/// unless `SetChangeCoordinateSystem()` is set to say otherwise.
///
/// The default name for the output field is "transform", but that can be overridden as
/// always using the `SetOutputFieldName()` method.
///
class VISKORES_FILTER_FIELD_TRANSFORM_EXPORT PointTransform : public viskores::filter::Filter
{
public:
  VISKORES_CONT
  PointTransform();

  /// @brief Translates, or moves, each point in the input field by a given direction.
  VISKORES_CONT void SetTranslation(const viskores::FloatDefault& tx,
                                    const viskores::FloatDefault& ty,
                                    const viskores::FloatDefault& tz)
  {
    matrix = viskores::Transform3DTranslate(tx, ty, tz);
  }

  /// @copydoc SetTranslation
  VISKORES_CONT void SetTranslation(const viskores::Vec3f& v) { SetTranslation(v[0], v[1], v[2]); }

  /// @brief Rotate the input field about a given axis.
  ///
  /// @param[in] angleDegrees The amount of rotation to perform, given in degrees.
  /// @param[in] axis The rotation is made around a line that goes through the origin
  ///   and pointing in this direction in the counterclockwise direction.
  VISKORES_CONT void SetRotation(const viskores::FloatDefault& angleDegrees,
                                 const viskores::Vec3f& axis)
  {
    matrix = viskores::Transform3DRotate(angleDegrees, axis);
  }

  /// @brief Rotate the input field about a given axis.
  ///
  /// The rotation is made around a line that goes through the origin
  /// and pointing in the direction specified by @p axisX, @p axisY,
  /// and @p axisZ in the counterclockwise direction.
  ///
  /// @param[in] angleDegrees The amount of rotation to perform, given in degrees.
  /// @param[in] axisX The X value of the rotation axis.
  /// @param[in] axisY The Y value of the rotation axis.
  /// @param[in] axisZ The Z value of the rotation axis.
  VISKORES_CONT void SetRotation(const viskores::FloatDefault& angleDegrees,
                                 const viskores::FloatDefault& axisX,
                                 const viskores::FloatDefault& axisY,
                                 const viskores::FloatDefault& axisZ)
  {
    SetRotation(angleDegrees, { axisX, axisY, axisZ });
  }

  /// @brief Rotate the input field around the X axis by the given degrees.
  VISKORES_CONT void SetRotationX(const viskores::FloatDefault& angleDegrees)
  {
    SetRotation(angleDegrees, 1, 0, 0);
  }

  /// @brief Rotate the input field around the Y axis by the given degrees.
  VISKORES_CONT void SetRotationY(const viskores::FloatDefault& angleDegrees)
  {
    SetRotation(angleDegrees, 0, 1, 0);
  }

  /// @brief Rotate the input field around the Z axis by the given degrees.
  VISKORES_CONT void SetRotationZ(const viskores::FloatDefault& angleDegrees)
  {
    SetRotation(angleDegrees, 0, 0, 1);
  }

  /// @brief Scale the input field.
  ///
  /// Each coordinate is multiplied by tghe associated scale factor.
  VISKORES_CONT void SetScale(const viskores::FloatDefault& s)
  {
    matrix = viskores::Transform3DScale(s, s, s);
  }

  /// @copydoc SetScale
  VISKORES_CONT void SetScale(const viskores::FloatDefault& sx,
                              const viskores::FloatDefault& sy,
                              const viskores::FloatDefault& sz)
  {
    matrix = viskores::Transform3DScale(sx, sy, sz);
  }

  /// @copydoc SetScale
  VISKORES_CONT void SetScale(const viskores::Vec3f& v)
  {
    matrix = viskores::Transform3DScale(v[0], v[1], v[2]);
  }

  /// @brief Set a general transformation matrix.
  ///
  /// Each field value is multiplied by this 4x4 as a homogeneous coordinate. That is
  /// a 1 component is added to the end of each 3D vector to put it in the form [x, y, z, 1].
  /// The matrix is then premultiplied to this as a column vector.
  ///
  /// The functions in viskores/Transform3D.h can be used to help build these transform
  /// matrices.
  VISKORES_CONT
  void SetTransform(const viskores::Matrix<viskores::FloatDefault, 4, 4>& mtx) { matrix = mtx; }

  /// @brief Specify whether the result should become the coordinate system of the output.
  ///
  /// When this flag is on (the default) the first coordinate system in the output
  /// `viskores::cont::DataSet` is set to the transformed point coordinates.
  void SetChangeCoordinateSystem(bool flag);
  bool GetChangeCoordinateSystem() const;

private:
  VISKORES_CONT viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& input) override;

  viskores::Matrix<viskores::FloatDefault, 4, 4> matrix;
  bool ChangeCoordinateSystem = true;
};
} // namespace field_transform
} // namespace filter
} // namespace viskores

#endif // viskores_filter_field_transform_PointTransform_h
