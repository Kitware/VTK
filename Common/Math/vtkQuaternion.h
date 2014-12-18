/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuaternion.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkQuaternion - templated base type for storage of quaternions.
// .SECTION Description
// This class is a templated data type for storing and manipulating
// quaternions. The quaternions have the form [w, x, y, z].
// Given a rotation of angle theta and axis v, the corresponding
// quaternion is [w, x, y, z] = [cos(theta/2), v*sin(theta/2)]
//
// This class implements the Spherical Linear interpolation (SLERP)
// and the Spherical Spline Quaternion interpolation (SQUAD).
// It is advised to use the vtkQuaternionInterpolator when dealing
// with multiple quaternions and or interpolations.
//
// .SECTION See also
// vtkQuaternionInterpolator

#ifndef vtkQuaternion_h
#define vtkQuaternion_h

#include "vtkTuple.h"

template<typename T> class vtkQuaternion : public vtkTuple<T, 4>
{
public:
  // Description:
  // Default constructor. Creates an identity quaternion.
  vtkQuaternion();

  // Description:
  // Initialize all of the quaternion's elements with the supplied scalar.
  explicit vtkQuaternion(const T& scalar) : vtkTuple<T, 4>(scalar) {}

  // Description:
  // Initalize the quaternion's elements with the elements of the supplied array.
  // Note that the supplied pointer must contain at least as many elements as
  // the quaternion, or it will result in access to out of bounds memory.
  explicit vtkQuaternion(const T* init) : vtkTuple<T, 4>(init) {}

  // Description:
  // Initialize the quaternion element explicitly.
  vtkQuaternion(const T& w, const T& x, const T& y, const T& z);

  // Description:
  // Get the squared norm of the quaternion.
  T SquaredNorm() const;

  // Description:
  // Get the norm of the quaternion, i.e. its length.
  T Norm() const;

  // Description:
  // Set the quaternion to identity in place.
  void ToIdentity();

  // Description:
  // Return the identity quaternion.
  // Note that the default constructor also creates an identity quaternion.
  static vtkQuaternion<T> Identity();

  // Description:
  // Normalize the quaternion in place.
  // Return the norm of the quaternion.
  T Normalize();

  // Description:
  // Return the normalized form of this quaternion.
  vtkQuaternion<T> Normalized() const;

  // Description:
  // Conjugate the quaternion in place.
  void Conjugate();

  // Description:
  // Return the conjugate form of this quaternion.
  vtkQuaternion<T> Conjugated() const;

  // Description:
  // Invert the quaternion in place.
  // This is equivalent to conjugate the quaternion and then divide
  // it by its squared norm.
  void Invert();

  // Description:
  // Return the inverted form of this quaternion.
  vtkQuaternion<T> Inverse() const;

  // Description:
  // Convert this quaternion to a unit log quaternion.
  // The unit log quaternion is defined by:
  // [w, x, y, z] =  [0.0, v*sin(theta)].
  void ToUnitLog();

  // Description:
  // Return the unit log version of this quaternion.
  // The unit log quaternion is defined by:
  // [w, x, y, z] =  [0.0, v*sin(theta)].
  vtkQuaternion<T> UnitLog() const;

  // Description:
  // Convert this quaternion to a unit exponential quaternion.
  // The unit exponential quaternion is defined by:
  // [w, x, y, z] =  [cos(theta), v*sin(theta)].
  void ToUnitExp();

  // Description:
  // Return the unit exponential version of this quaternion.
  // The unit exponential quaternion is defined by:
  // [w, x, y, z] =  [cos(theta), v*sin(theta)].
  vtkQuaternion<T> UnitExp() const;

  // Description:
  // Normalize a quaternion in place and transform it to
  // so its angle is in degrees and its axis normalized.
  void NormalizeWithAngleInDegrees();

  // Description:
  // Returns a quaternion normalized and transformed
  // so its angle is in degrees and its axis normalized.
  vtkQuaternion<T> NormalizedWithAngleInDegrees() const;

  // Description:
  // Set/Get the w, x, y and z components of the quaternion.
  void Set(const T& w, const T& x, const T& y, const T& z);
  void Set(T quat[4]);
  void Get(T quat[4]) const;

  // Description:
  // Set/Get the w component of the quaternion, i.e. element 0.
  void SetW(const T& w);
  const T& GetW() const;

  // Description:
  // Set/Get the x component of the quaternion, i.e. element 1.
  void SetX(const T& x);
  const T& GetX() const;

  // Description:
  // Set/Get the y component of the quaternion, i.e. element 2.
  void SetY(const T& y);
  const T& GetY() const;

  // Description:
  // Set/Get the y component of the quaternion, i.e. element 3.
  void SetZ(const T& z);
  const T& GetZ() const;

  // Description:
  // Set/Get the angle (in radians) and the axis corresponding to
  // the axis-angle rotation of this quaternion.
  T GetRotationAngleAndAxis(T axis[3]) const;
  void SetRotationAngleAndAxis(T angle, T axis[3]);
  void SetRotationAngleAndAxis(
    const T& angle, const T& x, const T& y, const T& z);

  // Description:
  // Cast the quaternion to the specified type and return the result.
  template<typename CastTo> vtkQuaternion<CastTo> Cast() const;

  // Description:
  // Convert a quaternion to a 3x3 rotation matrix. The quaternion
  // does not have to be normalized beforehand.
  // @sa FromMatrix3x3()
  void ToMatrix3x3(T A[3][3]) const;

  // Description:
  // Convert a 3x3 matrix into a quaternion.  This will provide the
  // best possible answer even if the matrix is not a pure rotation matrix.
  // The method used is that of B.K.P. Horn.
  // @sa ToMatrix3x3()
  void FromMatrix3x3(const T A[3][3]);

  // Description:
  // Interpolate quaternions using spherical linear interpolation between
  // this quaternion and q1 to produce the output.
  // The parametric coordinate t belongs to [0,1] and lies between (this,q1).
  // @sa vtkQuaternionInterpolator
  vtkQuaternion<T> Slerp(T t, const vtkQuaternion<T>& q) const;

  // Description:
  // Interpolates between quaternions, using spherical quadrangle
  // interpolation.
  // @sa vtkQuaternionInterpolator
  vtkQuaternion<T> InnerPoint(const vtkQuaternion<T>& q1,
    const vtkQuaternion<T>& q2) const;

  // Description:
  // Performs addition of quaternion of the same basic type.
  vtkQuaternion<T> operator+(const vtkQuaternion<T>& q) const;

  // Description:
  // Performs subtraction of quaternions of the same basic type.
  vtkQuaternion<T> operator-(const vtkQuaternion<T>& q) const;

  // Description:
  // Performs multiplication of quaternion of the same basic type.
  vtkQuaternion<T> operator*(const vtkQuaternion<T>& q) const;

  // Description:
  // Performs multiplication of the quaternions by a scalar value.
  vtkQuaternion<T> operator*(const T& scalar) const;

  // Description:
  // Performs in place multiplication of the quaternions by a scalar value.
  void operator*=(const T& scalar) const;

  // Description:
  // Performs division of quaternions of the same type.
  vtkQuaternion<T> operator/(const vtkQuaternion<T>& q) const;

  // Description:
  // Performs division of the quaternions by a scalar value.
  vtkQuaternion<T> operator/(const T& scalar) const;

  // Description:
  // Performs in place division of the quaternions by a scalar value.
  void operator/=(const T& scalar);
};

// Description:
// Several macros to define the various operator overloads for the quaternions.
// These are necessary for the derived classes that are commonly used.
#define vtkQuaternionIdentity(quaternionType, type) \
quaternionType Identity() const \
{ \
  return quaternionType(vtkQuaternion<type>::Identity().GetData()); \
}
#define vtkQuaternionNormalized(quaternionType, type) \
quaternionType Normalized() const \
{ \
  return quaternionType(vtkQuaternion<type>::Normalized().GetData()); \
}
#define vtkQuaternionConjugated(quaternionType, type) \
quaternionType Conjugated() const \
{ \
  return quaternionType(vtkQuaternion<type>::Conjugated().GetData()); \
}
#define vtkQuaternionInverse(quaternionType, type) \
quaternionType Inverse() const \
{ \
  return quaternionType(vtkQuaternion<type>::Inverse().GetData()); \
}
#define vtkQuaternionUnitLog(quaternionType, type) \
quaternionType UnitLog() const \
{ \
  return quaternionType( \
    vtkQuaternion<type>::UnitLog().GetData()); \
}
#define vtkQuaternionUnitExp(quaternionType, type) \
quaternionType UnitExp() const \
{ \
  return quaternionType( \
    vtkQuaternion<type>::UnitExp().GetData()); \
}
#define vtkQuaternionNormalizedWithAngleInDegrees(quaternionType, type) \
quaternionType NormalizedWithAngleInDegrees() const \
{ \
  return quaternionType( \
    vtkQuaternion<type>::NormalizedWithAngleInDegrees().GetData()); \
}
#define vtkQuaternionSlerp(quaternionType, type) \
quaternionType Slerp(type t, const quaternionType& q) const \
{ \
  return quaternionType( \
    vtkQuaternion<type>::Slerp(t, q).GetData()); \
}
#define vtkQuaternionInnerPoint(quaternionType, type) \
quaternionType InnerPoint(const quaternionType& q1, \
                          const quaternionType& q2) const \
{ \
  return quaternionType( \
    vtkQuaternion<type>::InnerPoint(q1, q2).GetData()); \
}
#define vtkQuaternionOperatorPlus(quaternionType, type) \
inline quaternionType operator+(const quaternionType& q) const \
{ \
  return quaternionType( ( \
    static_cast< vtkQuaternion<type> > (*this) + \
    static_cast< vtkQuaternion<type> > (q)).GetData()); \
}
#define vtkQuaternionOperatorMinus(quaternionType, type) \
inline quaternionType operator-(const quaternionType& q) const \
{ \
  return quaternionType( ( \
    static_cast< vtkQuaternion<type> > (*this) - \
    static_cast< vtkQuaternion<type> > (q)).GetData()); \
}
#define vtkQuaternionOperatorMultiply(quaternionType, type) \
inline quaternionType operator*(const quaternionType& q) const \
{ \
  return quaternionType( ( \
    static_cast< vtkQuaternion<type> > (*this) * \
    static_cast< vtkQuaternion<type> > (q)).GetData()); \
}
#define vtkQuaternionOperatorMultiplyScalar(quaternionType, type) \
inline quaternionType operator*(const type& scalar) const \
{ \
  return quaternionType( ( \
    static_cast< vtkQuaternion<type> > (*this) * \
    scalar).GetData()); \
}
#define vtkQuaternionOperatorDivide(quaternionType, type) \
inline quaternionType operator/(const quaternionType& q) const \
{ \
  return quaternionType( ( \
    static_cast< vtkQuaternion<type> > (*this) / \
    static_cast< vtkQuaternion<type> > (q)).GetData()); \
}
#define vtkQuaternionOperatorDivideScalar(quaternionType, type) \
inline quaternionType operator/(const type& scalar) const \
{ \
  return quaternionType( ( \
    static_cast< vtkQuaternion<type> > (*this) / \
    scalar).GetData()); \
}

#define vtkQuaternionOperatorMacro(quaternionType, type) \
vtkQuaternionIdentity(quaternionType, type) \
vtkQuaternionNormalized(quaternionType, type) \
vtkQuaternionConjugated(quaternionType, type) \
vtkQuaternionInverse(quaternionType, type) \
vtkQuaternionUnitLog(quaternionType, type) \
vtkQuaternionUnitExp(quaternionType, type) \
vtkQuaternionNormalizedWithAngleInDegrees(quaternionType, type) \
vtkQuaternionSlerp(quaternionType, type) \
vtkQuaternionInnerPoint(quaternionType, type) \
vtkQuaternionOperatorPlus(quaternionType, type) \
vtkQuaternionOperatorMinus(quaternionType, type) \
vtkQuaternionOperatorMultiply(quaternionType, type) \
vtkQuaternionOperatorMultiplyScalar(quaternionType, type) \
vtkQuaternionOperatorDivide(quaternionType, type) \
vtkQuaternionOperatorDivideScalar(quaternionType, type)

// .NAME vtkQuaternionf - Float quaternion type.
//
// .SECTION Description
// This class is uses vtkQuaternion with float type data.
// For futher description, see the templated class vtkQuaternion.
// @sa vtkQuaterniond vtkQuaternion
class vtkQuaternionf : public vtkQuaternion<float>
{
public:
  vtkQuaternionf() {}
  explicit vtkQuaternionf(float w, float x, float y, float z)
    : vtkQuaternion<float>(w, x, y, z) {}
  explicit vtkQuaternionf(float scalar) : vtkQuaternion<float>(scalar) {}
  explicit vtkQuaternionf(const float *init) : vtkQuaternion<float>(init) {}
  vtkQuaternionOperatorMacro(vtkQuaternionf, float)
};

// .NAME vtkQuaterniond - Double quaternion type.
//
// .SECTION Description
// This class is uses vtkQuaternion with double type data.
// For futher description, seethe templated class vtkQuaternion.
// @sa vtkQuaternionf vtkQuaternion
class vtkQuaterniond : public vtkQuaternion<double>
{
public:
  vtkQuaterniond() {}
  explicit vtkQuaterniond(double w, double x, double y, double z)
    : vtkQuaternion<double>(w, x, y, z) {}
  explicit vtkQuaterniond(double scalar) : vtkQuaternion<double>(scalar) {}
  explicit vtkQuaterniond(const double *init) : vtkQuaternion<double>(init) {}
  vtkQuaternionOperatorMacro(vtkQuaterniond, double);
};

#include "vtkQuaternion.txx"

#endif // vtkQuaternion_h
// VTK-HeaderTest-Exclude: vtkQuaternion.h
