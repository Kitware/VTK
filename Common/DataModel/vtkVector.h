// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkVector
 * @brief   templated base type for storage of vectors.
 *
 *
 * This class is a templated data type for storing and manipulating fixed size
 * vectors, which can be used to represent two and three dimensional points. The
 * memory layout is a contiguous array of the specified type, such that a
 * float[2] can be cast to a vtkVector2f and manipulated. Also a float[6] could
 * be cast and used as a vtkVector2f[3].
 */

#ifndef vtkVector_h
#define vtkVector_h

#include "vtkObject.h" // for legacy macros
#include "vtkTuple.h"

#include <cmath> // For math functions

VTK_ABI_NAMESPACE_BEGIN
template <typename T, int Size>
class vtkVector : public vtkTuple<T, Size>
{
public:
  vtkVector() = default;

  /**
   * Initialize all of the vector's elements with the supplied scalar.
   */
  explicit vtkVector(const T& scalar)
    : vtkTuple<T, Size>(scalar)
  {
  }

  /**
   * Initialize the vector's elements with the elements of the supplied array.
   * Note that the supplied pointer must contain at least as many elements as
   * the vector, or it will result in access to out of bounds memory.
   */
  explicit vtkVector(const T* init)
    : vtkTuple<T, Size>(init)
  {
  }

  ///@{
  /**
   * Get the squared norm of the vector.
   */
  T SquaredNorm() const
  {
    T result = 0;
    for (int i = 0; i < Size; ++i)
    {
      result += this->Data[i] * this->Data[i];
    }
    return result;
  }
  ///@}

  /**
   * Get the norm of the vector, i.e. its length.
   */
  double Norm() const { return sqrt(static_cast<double>(this->SquaredNorm())); }

  ///@{
  /**
   * Normalize the vector in place.
   * \return The length of the vector.
   */
  double Normalize()
  {
    const double norm(this->Norm());
    if (norm == 0.0)
    {
      return 0.0;
    }
    const double inv(1.0 / norm);
    for (int i = 0; i < Size; ++i)
    {
      this->Data[i] = static_cast<T>(this->Data[i] * inv);
    }
    return norm;
  }
  ///@}

  ///@{
  /**
   * Return the normalized form of this vector.
   * \return The normalized form of this vector.
   */
  vtkVector<T, Size> Normalized() const
  {
    vtkVector<T, Size> temp(*this);
    temp.Normalize();
    return temp;
  }
  ///@}

  ///@{
  /**
   * The dot product of this and the supplied vector.
   */
  T Dot(const vtkVector<T, Size>& other) const
  {
    T result(0);
    for (int i = 0; i < Size; ++i)
    {
      result += this->Data[i] * other[i];
    }
    return result;
  }
  ///@}

  ///@{
  /**
   * Cast the vector to the specified type, returning the result.
   */
  template <typename TR>
  vtkVector<TR, Size> Cast() const
  {
    vtkVector<TR, Size> result;
    for (int i = 0; i < Size; ++i)
    {
      result[i] = static_cast<TR>(this->Data[i]);
    }
    return result;
  }
  ///@}
};

// .NAME vtkVector2 - templated base type for storage of 2D vectors.
//
template <typename T>
class vtkVector2 : public vtkVector<T, 2>
{
public:
  vtkVector2() = default;

  explicit vtkVector2(const T& scalar)
    : vtkVector<T, 2>(scalar)
  {
  }

  explicit vtkVector2(const T* init)
    : vtkVector<T, 2>(init)
  {
  }

  vtkVector2(const T& x, const T& y)
  {
    this->Data[0] = x;
    this->Data[1] = y;
  }

  ///@{
  /**
   * Set the x and y components of the vector.
   */
  void Set(const T& x, const T& y)
  {
    this->Data[0] = x;
    this->Data[1] = y;
  }
  ///@}

  /**
   * Set the x component of the vector, i.e. element 0.
   */
  void SetX(const T& x) { this->Data[0] = x; }

  /**
   * Get the x component of the vector, i.e. element 0.
   */
  const T& GetX() const { return this->Data[0]; }

  /**
   * Set the y component of the vector, i.e. element 1.
   */
  void SetY(const T& y) { this->Data[1] = y; }

  /**
   * Get the y component of the vector, i.e. element 1.
   */
  const T& GetY() const { return this->Data[1]; }

  ///@{
  /**
   * Lexicographical comparison of two vector.
   */
  bool operator<(const vtkVector2<T>& v) const
  {
    return (this->Data[0] < v.Data[0]) || (this->Data[0] == v.Data[0] && this->Data[1] < v.Data[1]);
  }
  ///@}
};

// .NAME vtkVector3 - templated base type for storage of 3D vectors.
//
template <typename T>
class vtkVector3 : public vtkVector<T, 3>
{
public:
  vtkVector3() = default;

  explicit vtkVector3(const T& scalar)
    : vtkVector<T, 3>(scalar)
  {
  }

  explicit vtkVector3(const T* init)
    : vtkVector<T, 3>(init)
  {
  }

  vtkVector3(const T& x, const T& y, const T& z)
  {
    this->Data[0] = x;
    this->Data[1] = y;
    this->Data[2] = z;
  }

  ///@{
  /**
   * Set the x, y and z components of the vector.
   */
  void Set(const T& x, const T& y, const T& z)
  {
    this->Data[0] = x;
    this->Data[1] = y;
    this->Data[2] = z;
  }
  ///@}

  /**
   * Set the x component of the vector, i.e. element 0.
   */
  void SetX(const T& x) { this->Data[0] = x; }

  /**
   * Get the x component of the vector, i.e. element 0.
   */
  const T& GetX() const { return this->Data[0]; }

  /**
   * Set the y component of the vector, i.e. element 1.
   */
  void SetY(const T& y) { this->Data[1] = y; }

  /**
   * Get the y component of the vector, i.e. element 1.
   */
  const T& GetY() const { return this->Data[1]; }

  /**
   * Set the z component of the vector, i.e. element 2.
   */
  void SetZ(const T& z) { this->Data[2] = z; }

  /**
   * Get the z component of the vector, i.e. element 2.
   */
  const T& GetZ() const { return this->Data[2]; }

  ///@{
  /**
   * Return the cross product of this X other.
   */
  vtkVector3<T> Cross(const vtkVector3<T>& other) const
  {
    vtkVector3<T> res;
    res[0] = this->Data[1] * other.Data[2] - this->Data[2] * other.Data[1];
    res[1] = this->Data[2] * other.Data[0] - this->Data[0] * other.Data[2];
    res[2] = this->Data[0] * other.Data[1] - this->Data[1] * other.Data[0];
    return res;
  }
  ///@}

  ///@{
  /**
   * Lexicographical comparison of two vector.
   */
  bool operator<(const vtkVector3<T>& v) const
  {
    return (this->Data[0] < v.Data[0]) ||
      (this->Data[0] == v.Data[0] && this->Data[1] < v.Data[1]) ||
      (this->Data[0] == v.Data[0] && this->Data[1] == v.Data[1] && this->Data[2] < v.Data[2]);
  }
  ///@}
};

// .NAME vtkVector4 - templated base type for storage of 4D vectors.
//
template <typename T>
class vtkVector4 : public vtkVector<T, 4>
{
public:
  vtkVector4() = default;

  explicit vtkVector4(const T& scalar)
    : vtkVector<T, 4>(scalar)
  {
  }

  explicit vtkVector4(const T* init)
    : vtkVector<T, 4>(init)
  {
  }

  vtkVector4(const T& x, const T& y, const T& z, const T& w)
  {
    this->Data[0] = x;
    this->Data[1] = y;
    this->Data[2] = z;
    this->Data[3] = w;
  }

  ///@{
  /**
   * Set the x, y, z and w components of a 3D vector in homogeneous coordinates.
   */
  void Set(const T& x, const T& y, const T& z, const T& w)
  {
    this->Data[0] = x;
    this->Data[1] = y;
    this->Data[2] = z;
    this->Data[3] = w;
  }
  ///@}

  /**
   * Set the x component of the vector, i.e. element 0.
   */
  void SetX(const T& x) { this->Data[0] = x; }

  /**
   * Get the x component of the vector, i.e. element 0.
   */
  const T& GetX() const { return this->Data[0]; }

  /**
   * Set the y component of the vector, i.e. element 1.
   */
  void SetY(const T& y) { this->Data[1] = y; }

  /**
   * Get the y component of the vector, i.e. element 1.
   */
  const T& GetY() const { return this->Data[1]; }

  /**
   * Set the z component of the vector, i.e. element 2.
   */
  void SetZ(const T& z) { this->Data[2] = z; }

  /**
   * Get the z component of the vector, i.e. element 2.
   */
  const T& GetZ() const { return this->Data[2]; }

  /**
   * Set the w component of the vector, i.e. element 3.
   */
  void SetW(const T& w) { this->Data[3] = w; }

  /**
   * Get the w component of the vector, i.e. element 3.
   */
  const T& GetW() const { return this->Data[3]; }
};

/**
 * Some inline functions for the derived types.
 */
#define vtkVectorNormalized(vectorType, type, size)                                                \
  vectorType Normalized() const                                                                    \
  {                                                                                                \
    return vectorType(vtkVector<type, size>::Normalized().GetData());                              \
  }

#define vtkVectorDerivedMacro(vectorType, type, size)                                              \
  vtkVectorNormalized(vectorType, type, size);                                                     \
  explicit vectorType(type s)                                                                      \
    : Superclass(s)                                                                                \
  {                                                                                                \
  }                                                                                                \
  explicit vectorType(const type* i)                                                               \
    : Superclass(i)                                                                                \
  {                                                                                                \
  }                                                                                                \
  explicit vectorType(const vtkTuple<type, size>& o)                                               \
    : Superclass(o.GetData())                                                                      \
  {                                                                                                \
  }                                                                                                \
  vectorType(const vtkVector<type, size>& o)                                                       \
    : Superclass(o.GetData())                                                                      \
  {                                                                                                \
  }

///@{
/**
 * Some derived classes for the different vectors commonly used.
 */
class vtkVector2i : public vtkVector2<int>
{
public:
  typedef vtkVector2<int> Superclass;
  vtkVector2i() = default;
  vtkVector2i(int x, int y)
    : vtkVector2<int>(x, y)
  {
  }
  vtkVectorDerivedMacro(vtkVector2i, int, 2);
};
///@}

class vtkVector2f : public vtkVector2<float>
{
public:
  typedef vtkVector2<float> Superclass;
  vtkVector2f() = default;
  vtkVector2f(float x, float y)
    : vtkVector2<float>(x, y)
  {
  }
  vtkVectorDerivedMacro(vtkVector2f, float, 2);
};

class vtkVector2d : public vtkVector2<double>
{
public:
  typedef vtkVector2<double> Superclass;
  vtkVector2d() = default;
  vtkVector2d(double x, double y)
    : vtkVector2<double>(x, y)
  {
  }
  vtkVectorDerivedMacro(vtkVector2d, double, 2);
};

#define vtkVector3Cross(vectorType, type)                                                          \
  vectorType Cross(const vectorType& other) const                                                  \
  {                                                                                                \
    return vectorType(vtkVector3<type>::Cross(other).GetData());                                   \
  }

class vtkVector3i : public vtkVector3<int>
{
public:
  typedef vtkVector3<int> Superclass;
  vtkVector3i() = default;
  vtkVector3i(int x, int y, int z)
    : vtkVector3<int>(x, y, z)
  {
  }
  vtkVectorDerivedMacro(vtkVector3i, int, 3);
  vtkVector3Cross(vtkVector3i, int);
};

class vtkVector3f : public vtkVector3<float>
{
public:
  typedef vtkVector3<float> Superclass;
  vtkVector3f() = default;
  vtkVector3f(float x, float y, float z)
    : vtkVector3<float>(x, y, z)
  {
  }
  vtkVectorDerivedMacro(vtkVector3f, float, 3);
  vtkVector3Cross(vtkVector3f, float);
};

class vtkVector3d : public vtkVector3<double>
{
public:
  typedef vtkVector3<double> Superclass;
  vtkVector3d() = default;
  vtkVector3d(double x, double y, double z)
    : vtkVector3<double>(x, y, z)
  {
  }
  vtkVectorDerivedMacro(vtkVector3d, double, 3);
  vtkVector3Cross(vtkVector3d, double);
};

class vtkVector4i : public vtkVector4<int>
{
public:
  typedef vtkVector4<int> Superclass;
  vtkVector4i() = default;
  vtkVector4i(int x, int y, int z, int w)
    : vtkVector4<int>(x, y, z, w)
  {
  }
  vtkVectorDerivedMacro(vtkVector4i, int, 4);
};

class vtkVector4d : public vtkVector4<double>
{
public:
  using Superclass = vtkVector4<double>;
  vtkVector4d() = default;
  vtkVector4d(double x, double y, double z, double w)
    : vtkVector4<double>(x, y, z, w)
  {
  }
  vtkVectorDerivedMacro(vtkVector4d, double, 4);
};

/**
 * This following operators enhance the vtkVector classes, allowing various
 * operator overloads one might expect.
 */

/**
 * Unary minus / negation of vector.
 */
template <typename A, int Size>
vtkVector<A, Size> operator-(const vtkVector<A, Size>& v)
{
  vtkVector<A, Size> ret;
  for (int i = 0; i < Size; ++i)
  {
    ret[i] = -v[i];
  }
  return ret;
}

/**
 * Performs addition of vectors of the same basic type.
 */
template <typename A, int Size>
vtkVector<A, Size> operator+(const vtkVector<A, Size>& v1, const vtkVector<A, Size>& v2)
{
  vtkVector<A, Size> ret;
  for (int i = 0; i < Size; ++i)
  {
    ret[i] = v1[i] + v2[i];
  }
  return ret;
}

/**
 * Add the vector b to the vector a of the same basic type.
 */
template <typename T, int Size>
vtkVector<T, Size>& operator+=(vtkVector<T, Size>& a, const vtkVector<T, Size>& b)
{
  for (int dim = 0; dim < Size; ++dim)
  {
    a[dim] += b[dim];
  }

  return a;
}

/**
 * Performs subtraction of vectors of the same basic type.
 */
template <typename A, int Size>
vtkVector<A, Size> operator-(const vtkVector<A, Size>& v1, const vtkVector<A, Size>& v2)
{
  vtkVector<A, Size> ret;
  for (int i = 0; i < Size; ++i)
  {
    ret[i] = v1[i] - v2[i];
  }
  return ret;
}

/**
 * Subtract the vector b to the vector a of the same basic type.
 */
template <typename T, int Size>
vtkVector<T, Size>& operator-=(vtkVector<T, Size>& a, const vtkVector<T, Size>& b)
{
  for (int dim = 0; dim < Size; ++dim)
  {
    a[dim] -= b[dim];
  }

  return a;
}

/**
 * Performs multiplication of vectors of the same basic type.
 */
template <typename A, int Size>
vtkVector<A, Size> operator*(const vtkVector<A, Size>& v1, const vtkVector<A, Size>& v2)
{
  vtkVector<A, Size> ret;
  for (int i = 0; i < Size; ++i)
  {
    ret[i] = v1[i] * v2[i];
  }
  return ret;
}

/**
 * Performs multiplication of vectors by a scalar value.
 */
template <typename A, typename B, int Size>
vtkVector<A, Size> operator*(const vtkVector<A, Size>& v1, const B& scalar)
{
  vtkVector<A, Size> ret;
  for (int i = 0; i < Size; ++i)
  {
    ret[i] = v1[i] * scalar;
  }
  return ret;
}

/**
 * Performs division of vectors of the same type.
 */
template <typename A, int Size>
vtkVector<A, Size> operator/(const vtkVector<A, Size>& v1, const vtkVector<A, Size>& v2)
{
  vtkVector<A, Size> ret;
  for (int i = 0; i < Size; ++i)
  {
    ret[i] = v1[i] / v2[i];
  }
  return ret;
}

/**
 * Several macros to define the various operator overloads for the vectors.
 *
 * These macros are necessary to define operator overloads for common vector types
 * (e.g vtkVector3d...), without them, there could be ambiguous overloads.
 * XXX(c++20): might use constraints instead
 */
#define vtkVectorOperatorNegate(vectorType, type, size)                                            \
  inline vectorType operator-(const vectorType& v)                                                 \
  {                                                                                                \
    return vectorType((-static_cast<vtkVector<type, size>>(v)).GetData());                         \
  }
#define vtkVectorOperatorPlus(vectorType, type, size)                                              \
  inline vectorType operator+(const vectorType& v1, const vectorType& v2)                          \
  {                                                                                                \
    return vectorType(                                                                             \
      (static_cast<vtkVector<type, size>>(v1) + static_cast<vtkVector<type, size>>(v2))            \
        .GetData());                                                                               \
  }
#define vtkVectorOperatorMinus(vectorType, type, size)                                             \
  inline vectorType operator-(const vectorType& v1, const vectorType& v2)                          \
  {                                                                                                \
    return vectorType(                                                                             \
      (static_cast<vtkVector<type, size>>(v1) - static_cast<vtkVector<type, size>>(v2))            \
        .GetData());                                                                               \
  }
#define vtkVectorOperatorMultiply(vectorType, type, size)                                          \
  inline vectorType operator*(const vectorType& v1, const vectorType& v2)                          \
  {                                                                                                \
    return vectorType(                                                                             \
      (static_cast<vtkVector<type, size>>(v1) * static_cast<vtkVector<type, size>>(v2))            \
        .GetData());                                                                               \
  }
#define vtkVectorOperatorMultiplyScalar(vectorType, type, size)                                    \
  template <typename B>                                                                            \
  inline vectorType operator*(const vectorType& v1, const B& scalar)                               \
  {                                                                                                \
    return vectorType((static_cast<vtkVector<type, size>>(v1) * scalar).GetData());                \
  }
#define vtkVectorOperatorMultiplyScalarPre(vectorType, type, size)                                 \
  template <typename B>                                                                            \
  inline vectorType operator*(const B& scalar, const vectorType& v1)                               \
  {                                                                                                \
    return vectorType((static_cast<vtkVector<type, size>>(v1) * scalar).GetData());                \
  }
#define vtkVectorOperatorDivide(vectorType, type, size)                                            \
  inline vectorType operator/(const vectorType& v1, const vectorType& v2)                          \
  {                                                                                                \
    return vectorType(                                                                             \
      (static_cast<vtkVector<type, size>>(v1) / static_cast<vtkVector<type, size>>(v2))            \
        .GetData());                                                                               \
  }

#define vtkVectorOperatorMacro(vectorType, type, size)                                             \
  vtkVectorOperatorNegate(vectorType, type, size);                                                 \
  vtkVectorOperatorPlus(vectorType, type, size);                                                   \
  vtkVectorOperatorMinus(vectorType, type, size);                                                  \
  vtkVectorOperatorMultiply(vectorType, type, size);                                               \
  vtkVectorOperatorMultiplyScalar(vectorType, type, size);                                         \
  vtkVectorOperatorMultiplyScalarPre(vectorType, type, size);                                      \
  vtkVectorOperatorDivide(vectorType, type, size)

/**
 * Overload the operators for the common types.
 */
vtkVectorOperatorMacro(vtkVector2i, int, 2);
vtkVectorOperatorMacro(vtkVector2f, float, 2);
vtkVectorOperatorMacro(vtkVector2d, double, 2);
vtkVectorOperatorMacro(vtkVector3i, int, 3);
vtkVectorOperatorMacro(vtkVector3f, float, 3);
vtkVectorOperatorMacro(vtkVector3d, double, 3);

VTK_ABI_NAMESPACE_END
#endif // vtkVector_h
// VTK-HeaderTest-Exclude: vtkVector.h
