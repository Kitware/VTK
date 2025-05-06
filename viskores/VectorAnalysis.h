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
#ifndef viskores_VectorAnalysis_h
#define viskores_VectorAnalysis_h

// This header file defines math functions that deal with linear albegra functions

#include <viskores/Math.h>
#include <viskores/TypeTraits.h>
#include <viskores/Types.h>
#include <viskores/VecTraits.h>

namespace viskores
{

// ----------------------------------------------------------------------------
/// @brief Returns the linear interpolation of two values based on weight
///
/// `Lerp` returns the linear interpolation of two values based on a weight. If
/// `weight` is outside [0,1] then `Lerp`
/// extrapolates. If `weight`=0 then `value0` is returned. If `weight`=1 then
/// `value1` is returned.
///
template <typename ValueType, typename WeightType>
inline VISKORES_EXEC_CONT ValueType Lerp(const ValueType& value0,
                                         const ValueType& value1,
                                         const WeightType& weight)
{
  using ScalarType = typename detail::FloatingPointReturnType<ValueType>::Type;
  return static_cast<ValueType>((WeightType(1) - weight) * static_cast<ScalarType>(value0) +
                                weight * static_cast<ScalarType>(value1));
}
template <typename ValueType, viskores::IdComponent N, typename WeightType>
VISKORES_EXEC_CONT viskores::Vec<ValueType, N> Lerp(const viskores::Vec<ValueType, N>& value0,
                                                    const viskores::Vec<ValueType, N>& value1,
                                                    const WeightType& weight)
{
  return (WeightType(1) - weight) * value0 + weight * value1;
}
template <typename ValueType, viskores::IdComponent N>
VISKORES_EXEC_CONT viskores::Vec<ValueType, N> Lerp(const viskores::Vec<ValueType, N>& value0,
                                                    const viskores::Vec<ValueType, N>& value1,
                                                    const viskores::Vec<ValueType, N>& weight)
{
  const viskores::Vec<ValueType, N> One(ValueType(1));
  return (One - weight) * value0 + weight * value1;
}

// ----------------------------------------------------------------------------
/// @brief Returns the square of the magnitude of a vector.
///
/// It is usually much faster to compute the square of the magnitude than the
/// magnitude, so you should use this function in place of Magnitude or RMagnitude
/// when possible.
///
template <typename T>
VISKORES_EXEC_CONT typename detail::FloatingPointReturnType<T>::Type MagnitudeSquared(const T& x)
{
  using U = typename detail::FloatingPointReturnType<T>::Type;
  return static_cast<U>(viskores::Dot(x, x));
}

// ----------------------------------------------------------------------------
namespace detail
{
template <typename T>
VISKORES_EXEC_CONT typename detail::FloatingPointReturnType<T>::Type MagnitudeTemplate(
  T x,
  viskores::TypeTraitsScalarTag)
{
  return static_cast<typename detail::FloatingPointReturnType<T>::Type>(viskores::Abs(x));
}

template <typename T>
VISKORES_EXEC_CONT typename detail::FloatingPointReturnType<T>::Type MagnitudeTemplate(
  const T& x,
  viskores::TypeTraitsVectorTag)
{
  return viskores::Sqrt(viskores::MagnitudeSquared(x));
}

} // namespace detail

/// @brief Returns the magnitude of a vector.
///
/// It is usually much faster to compute MagnitudeSquared, so that should be
/// substituted when possible (unless you are just going to take the square
/// root, which would be besides the point). On some hardware it is also faster
/// to find the reciprocal magnitude, so RMagnitude should be used if you
/// actually plan to divide by the magnitude.
///
template <typename T>
VISKORES_EXEC_CONT typename detail::FloatingPointReturnType<T>::Type Magnitude(const T& x)
{
  return detail::MagnitudeTemplate(x, typename viskores::TypeTraits<T>::DimensionalityTag());
}

// ----------------------------------------------------------------------------
namespace detail
{
template <typename T>
VISKORES_EXEC_CONT typename detail::FloatingPointReturnType<T>::Type RMagnitudeTemplate(
  T x,
  viskores::TypeTraitsScalarTag)
{
  return T(1) / viskores::Abs(x);
}

template <typename T>
VISKORES_EXEC_CONT typename detail::FloatingPointReturnType<T>::Type RMagnitudeTemplate(
  const T& x,
  viskores::TypeTraitsVectorTag)
{
  return viskores::RSqrt(viskores::MagnitudeSquared(x));
}
} // namespace detail

/// @brief Returns the reciprocal magnitude of a vector.
///
/// On some hardware `RMagnitude` is faster than `Magnitude`, but neither is
/// as fast as `MagnitudeSquared`. This function works on scalars as well
/// as vectors, in which case it just returns the reciprocal of the scalar.
///
template <typename T>
VISKORES_EXEC_CONT typename detail::FloatingPointReturnType<T>::Type RMagnitude(const T& x)
{
  return detail::RMagnitudeTemplate(x, typename viskores::TypeTraits<T>::DimensionalityTag());
}

// ----------------------------------------------------------------------------
namespace detail
{
template <typename T>
VISKORES_EXEC_CONT T NormalTemplate(T x, viskores::TypeTraitsScalarTag)
{
  return viskores::CopySign(T(1), x);
}

template <typename T>
VISKORES_EXEC_CONT T NormalTemplate(const T& x, viskores::TypeTraitsVectorTag)
{
  return viskores::RMagnitude(x) * x;
}
} // namespace detail

/// @brief Returns a normalized version of the given vector.
///
/// The resulting vector points in the same direction but has unit length.
///
template <typename T>
VISKORES_EXEC_CONT T Normal(const T& x)
{
  return detail::NormalTemplate(x, typename viskores::TypeTraits<T>::DimensionalityTag());
}

// ----------------------------------------------------------------------------
/// @brief Changes a vector to be normal.
///
/// The given vector is scaled to be unit length.
///
template <typename T>
VISKORES_EXEC_CONT void Normalize(T& x)
{
  x = viskores::Normal(x);
}

// ----------------------------------------------------------------------------
/// @brief Find the cross product of two vectors.
///
/// If Viskores is compiled with FMA support, it uses Kahan's difference of
/// products algorithm to achieve a maximum error of 1.5 ulps in each component.
template <typename T>
VISKORES_EXEC_CONT viskores::Vec<typename detail::FloatingPointReturnType<T>::Type, 3> Cross(
  const viskores::Vec<T, 3>& x,
  const viskores::Vec<T, 3>& y)
{
  return viskores::Vec<typename detail::FloatingPointReturnType<T>::Type, 3>(
    DifferenceOfProducts(x[1], y[2], x[2], y[1]),
    DifferenceOfProducts(x[2], y[0], x[0], y[2]),
    DifferenceOfProducts(x[0], y[1], x[1], y[0]));
}

//-----------------------------------------------------------------------------
/// @brief Find the normal of a triangle.
///
/// Given three coordinates in space, which, unless degenerate, uniquely define
/// a triangle and the plane the triangle is on, returns a vector perpendicular
/// to that triangle/plane.
///
/// Note that the returned vector might not be a unit vector. In fact, the length
/// is equal to twice the area of the triangle. If you want a unit vector,
/// send the result through the `viskores::Normal()` or `viskores::Normalize()` function.
///
template <typename T>
VISKORES_EXEC_CONT viskores::Vec<typename detail::FloatingPointReturnType<T>::Type, 3>
TriangleNormal(const viskores::Vec<T, 3>& a,
               const viskores::Vec<T, 3>& b,
               const viskores::Vec<T, 3>& c)
{
  return viskores::Cross(b - a, c - a);
}

//-----------------------------------------------------------------------------
/// @brief Project a vector onto another vector.
///
/// This method computes the orthogonal projection of the vector v onto u;
/// that is, it projects its first argument onto its second.
///
/// Note that if the vector `u` has zero length, the output
/// vector will have all its entries equal to NaN.
template <typename T, int N>
VISKORES_EXEC_CONT viskores::Vec<T, N> Project(const viskores::Vec<T, N>& v,
                                               const viskores::Vec<T, N>& u)
{
  T uu = viskores::Dot(u, u);
  T uv = viskores::Dot(u, v);
  T factor = uv / uu;
  viskores::Vec<T, N> result = factor * u;
  return result;
}

//-----------------------------------------------------------------------------
/// @brief Project a vector onto another vector, returning only the projected distance.
///
/// This method computes the orthogonal projection of the vector v onto u;
/// that is, it projects its first argument onto its second.
///
/// Note that if the vector `u` has zero length, the output will be NaN.
template <typename T, int N>
VISKORES_EXEC_CONT T ProjectedDistance(const viskores::Vec<T, N>& v, const viskores::Vec<T, N>& u)
{
  T uu = viskores::Dot(u, u);
  T uv = viskores::Dot(u, v);
  T factor = uv / uu;
  return factor;
}

//-----------------------------------------------------------------------------
/// @brief Convert a set of vectors to an orthonormal basis.
///
/// This function performs Gram-Schmidt orthonormalization for 3-D vectors.
/// The first output vector will always be parallel to the first input vector.
/// The remaining output vectors will be orthogonal and unit length and have
/// the same handedness as their corresponding input vectors.
///
/// This method is geometric.
/// It does not require a matrix solver.
/// However, unlike the algebraic eigensolver techniques which do use matrix
/// inversion, this method may return zero-length output vectors if some input
/// vectors are collinear. The number of non-zero (to within the specified
/// tolerance, `tol`) output vectors is the return value.
///
/// See https://en.wikipedia.org/wiki/Gram%E2%80%93Schmidt_process for details.
template <typename T, int N>
VISKORES_EXEC_CONT int Orthonormalize(const viskores::Vec<viskores::Vec<T, N>, N>& inputs,
                                      viskores::Vec<viskores::Vec<T, N>, N>& outputs,
                                      T tol = static_cast<T>(1e-6))
{
  T tolsqr = tol * tol;
  int j = 0; // j is the number of non-zero-length, non-collinear inputs encountered.
  viskores::Vec<viskores::Vec<T, N>, N> u;
  for (int i = 0; i < N; ++i)
  {
    u[j] = inputs[i];
    for (int k = 0; k < j; ++k)
    {
      u[j] -= viskores::Project(inputs[i], u[k]);
    }
    T magsqr = viskores::MagnitudeSquared(u[j]);
    if (magsqr <= tolsqr)
    {
      // skip this vector, it is zero-length or collinear with others.
      continue;
    }
    outputs[j] = viskores::RSqrt(magsqr) * u[j];
    ++j;
  }
  for (int i = j; i < N; ++i)
  {
    outputs[j] = Vec<T, N>{ 0. };
  }
  return j;
}

} // namespace viskores

#endif //viskores_VectorAnalysis_h
