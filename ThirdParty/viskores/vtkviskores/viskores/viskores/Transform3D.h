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
#ifndef viskores_Transform3D_h
#define viskores_Transform3D_h

// This header file contains a collection of math functions useful in the
// linear transformation of homogeneous points for rendering in 3D.

#include <viskores/Matrix.h>
#include <viskores/VectorAnalysis.h>

namespace viskores
{

/// \brief Transform a 3D point by a transformation matrix.
///
/// Given a 4x4 transformation matrix and a 3D point, returns the point
/// transformed by the given matrix in homogeneous coordinates.
///
/// This method ignores any change in the fourth component of the transformed
/// homogeneous coordinate, assuming that it is always 1 (that is, the last row
/// of the matrix is 0, 0, 0, 1). This will be true for affine transformations
/// (such as translate, scale, and rotate), but not for perspective
/// transformations.
///
template <typename T>
VISKORES_EXEC_CONT viskores::Vec<T, 3> Transform3DPoint(const viskores::Matrix<T, 4, 4>& matrix,
                                                        const viskores::Vec<T, 3>& point)
{
  viskores::Vec<T, 4> homogeneousPoint(point[0], point[1], point[2], T(1));
  return viskores::Vec<T, 3>(viskores::Dot(viskores::MatrixGetRow(matrix, 0), homogeneousPoint),
                             viskores::Dot(viskores::MatrixGetRow(matrix, 1), homogeneousPoint),
                             viskores::Dot(viskores::MatrixGetRow(matrix, 2), homogeneousPoint));
}

/// \brief Transform a 3D point by a transformation matrix with perspective.
///
/// Given a 4x4 transformation matrix and a 3D point, returns the point
/// transformed by the given matrix in homogeneous coordinates.
///
/// Unlike Transform3DPoint, this method honors the fourth component of the
/// transformed homogeneous coordinate. This makes it applicable for perspective
/// transformations, but requires some more computations.
///
template <typename T>
VISKORES_EXEC_CONT viskores::Vec<T, 3> Transform3DPointPerspective(
  const viskores::Matrix<T, 4, 4>& matrix,
  const viskores::Vec<T, 3>& point)
{
  viskores::Vec<T, 4> homogeneousPoint(point[0], point[1], point[2], T(1));
  T inverseW = 1 / viskores::Dot(viskores::MatrixGetRow(matrix, 3), homogeneousPoint);
  return viskores::Vec<T, 3>(
    viskores::Dot(viskores::MatrixGetRow(matrix, 0), homogeneousPoint) * inverseW,
    viskores::Dot(viskores::MatrixGetRow(matrix, 1), homogeneousPoint) * inverseW,
    viskores::Dot(viskores::MatrixGetRow(matrix, 2), homogeneousPoint) * inverseW);
}

/// \brief Transform a 3D vector by a transformation matrix.
///
/// Given a 4x4 transformation matrix and a 3D vector, returns the vector
/// transformed by the given matrix in homogeneous coordinates. Unlike points,
/// vectors do not get translated.
///
template <typename T>
VISKORES_EXEC_CONT viskores::Vec<T, 3> Transform3DVector(const viskores::Matrix<T, 4, 4>& matrix,
                                                         const viskores::Vec<T, 3>& vector)
{
  viskores::Vec<T, 4> homogeneousVector(vector[0], vector[1], vector[2], T(0));
  homogeneousVector = viskores::MatrixMultiply(matrix, homogeneousVector);
  return viskores::Vec<T, 3>(homogeneousVector[0], homogeneousVector[1], homogeneousVector[2]);
}

/// \brief Returns a scale matrix.
///
/// Given a scale factor for the x, y, and z directions, returns a
/// transformation matrix for those scales.
///
template <typename T>
VISKORES_EXEC_CONT viskores::Matrix<T, 4, 4> Transform3DScale(const T& scaleX,
                                                              const T& scaleY,
                                                              const T& scaleZ)
{
  viskores::Matrix<T, 4, 4> scaleMatrix(T(0));
  scaleMatrix(0, 0) = scaleX;
  scaleMatrix(1, 1) = scaleY;
  scaleMatrix(2, 2) = scaleZ;
  scaleMatrix(3, 3) = T(1);
  return scaleMatrix;
}

/// \brief Returns a scale matrix.
///
/// Given a scale factor for the x, y, and z directions (defined in a Vec),
/// returns a transformation matrix for those scales.
///
template <typename T>
VISKORES_EXEC_CONT viskores::Matrix<T, 4, 4> Transform3DScale(const viskores::Vec<T, 3>& scaleVec)
{
  return viskores::Transform3DScale(scaleVec[0], scaleVec[1], scaleVec[2]);
}

/// \brief Returns a scale matrix.
///
/// Given a uniform scale factor, returns a transformation matrix for those
/// scales.
///
template <typename T>
VISKORES_EXEC_CONT viskores::Matrix<T, 4, 4> Transform3DScale(const T& scale)
{
  return viskores::Transform3DScale(scale, scale, scale);
}

/// \brief Returns a translation matrix.
///
template <typename T>
VISKORES_EXEC_CONT viskores::Matrix<T, 4, 4> Transform3DTranslate(const T& x,
                                                                  const T& y,
                                                                  const T& z)
{
  viskores::Matrix<T, 4, 4> translateMatrix;
  viskores::MatrixIdentity(translateMatrix);
  translateMatrix(0, 3) = x;
  translateMatrix(1, 3) = y;
  translateMatrix(2, 3) = z;
  return translateMatrix;
}
template <typename T>
VISKORES_EXEC_CONT viskores::Matrix<T, 4, 4> Transform3DTranslate(const viskores::Vec<T, 3>& v)
{
  return viskores::Transform3DTranslate(v[0], v[1], v[2]);
}

/// \brief Returns a rotation matrix.
///
/// Given an angle (in degrees) and an axis of rotation, returns a
/// transformation matrix that rotates around the given axis. The rotation
/// follows the right-hand rule, so if the vector points toward the user, the
/// rotation will be counterclockwise.
///
template <typename T>
VISKORES_EXEC_CONT viskores::Matrix<T, 4, 4> Transform3DRotate(
  T angleDegrees,
  const viskores::Vec<T, 3>& axisOfRotation)
{
  T angleRadians = viskores::Pi_180<T>() * angleDegrees;
  const viskores::Vec<T, 3> normAxis = viskores::Normal(axisOfRotation);
  T sinAngle = viskores::Sin(angleRadians);
  T cosAngle = viskores::Cos(angleRadians);

  viskores::Matrix<T, 4, 4> matrix;

  matrix(0, 0) = normAxis[0] * normAxis[0] * (1 - cosAngle) + cosAngle;
  matrix(0, 1) = normAxis[0] * normAxis[1] * (1 - cosAngle) - normAxis[2] * sinAngle;
  matrix(0, 2) = normAxis[0] * normAxis[2] * (1 - cosAngle) + normAxis[1] * sinAngle;
  matrix(0, 3) = T(0);

  matrix(1, 0) = normAxis[1] * normAxis[0] * (1 - cosAngle) + normAxis[2] * sinAngle;
  matrix(1, 1) = normAxis[1] * normAxis[1] * (1 - cosAngle) + cosAngle;
  matrix(1, 2) = normAxis[1] * normAxis[2] * (1 - cosAngle) - normAxis[0] * sinAngle;
  matrix(1, 3) = T(0);

  matrix(2, 0) = normAxis[2] * normAxis[0] * (1 - cosAngle) - normAxis[1] * sinAngle;
  matrix(2, 1) = normAxis[2] * normAxis[1] * (1 - cosAngle) + normAxis[0] * sinAngle;
  matrix(2, 2) = normAxis[2] * normAxis[2] * (1 - cosAngle) + cosAngle;
  matrix(2, 3) = T(0);

  matrix(3, 0) = T(0);
  matrix(3, 1) = T(0);
  matrix(3, 2) = T(0);
  matrix(3, 3) = T(1);

  return matrix;
}
template <typename T>
VISKORES_EXEC_CONT viskores::Matrix<T, 4, 4> Transform3DRotate(T angleDegrees, T x, T y, T z)
{
  return viskores::Transform3DRotate(angleDegrees, viskores::Vec<T, 3>(x, y, z));
}

/// \brief Returns a rotation matrix.
///
/// Returns a transformation matrix that rotates around the x axis.
///
template <typename T>
VISKORES_EXEC_CONT viskores::Matrix<T, 4, 4> Transform3DRotateX(T angleDegrees)
{
  return viskores::Transform3DRotate(angleDegrees, T(1), T(0), T(0));
}

/// \brief Returns a rotation matrix.
///
/// Returns a transformation matrix that rotates around the y axis.
///
template <typename T>
VISKORES_EXEC_CONT viskores::Matrix<T, 4, 4> Transform3DRotateY(T angleDegrees)
{
  return viskores::Transform3DRotate(angleDegrees, T(0), T(1), T(0));
}

/// \brief Returns a rotation matrix.
///
/// Returns a transformation matrix that rotates around the z axis.
///
template <typename T>
VISKORES_EXEC_CONT viskores::Matrix<T, 4, 4> Transform3DRotateZ(T angleDegrees)
{
  return viskores::Transform3DRotate(angleDegrees, T(0), T(0), T(1));
}

} // namespace viskores

#endif //viskores_Transform3D_h
