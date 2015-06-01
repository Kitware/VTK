/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVector.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkVector - templated base type for storage of vectors.
//
// .SECTION Description
// This class is a templated data type for storing and manipulating fixed size
// vectors, which can be used to represent two and three dimensional points. The
// memory layout is a contiguous array of the specified type, such that a
// float[2] can be cast to a vtkVector2f and manipulated. Also a float[6] could
// be cast and used as a vtkVector2f[3].

#ifndef vtkVector_h
#define vtkVector_h

#include "vtkTuple.h"
#include "vtkObject.h" // for legacy macros

#include <cmath>   // For math functions

template<typename T, int Size>
class vtkVector : public vtkTuple<T, Size>
{
public:
  vtkVector()
  {
  }

  // Description:
  // Initialize all of the vector's elements with the supplied scalar.
  explicit vtkVector(const T& scalar) : vtkTuple<T, Size>(scalar)
  {
  }

  // Description:
  // Initalize the vector's elements with the elements of the supplied array.
  // Note that the supplied pointer must contain at least as many elements as
  // the vector, or it will result in access to out of bounds memory.
  explicit vtkVector(const T* init) : vtkTuple<T, Size>(init)
  {
  }

  // Description:
  // Get the squared norm of the vector.
  T SquaredNorm() const
  {
    T result = 0;
    for (int i = 0; i < Size; ++i)
      {
      result += this->Data[i] * this->Data[i];
      }
    return result;
  }

  // Description:
  // Get the norm of the vector, i.e. its length.
  double Norm() const
  {
    return sqrt(static_cast<double>(this->SquaredNorm()));
  }

  // Description:
  // Normalize the vector in place.
  // \return The length of the vector.
  double Normalize()
  {
    const double norm(this->Norm());
    const double inv(1.0 / norm);
    for (int i = 0; i < Size; ++i)
      {
      this->Data[i] = static_cast<T>(this->Data[i] * inv);
      }
    return norm;
  }

  // Description:
  // Return the normalized form of this vector.
  // \return The normalized form of this vector.
  vtkVector<T, Size> Normalized() const
  {
    vtkVector<T, Size> temp(*this);
    temp.Normalize();
    return temp;
  }

  // Description:
  // The dot product of this and the supplied vector.
  T Dot(const vtkVector<T, Size>& other) const
  {
    T result(0);
    for (int i = 0; i < Size; ++i)
      {
      result += this->Data[i] * other[i];
      }
    return result;
  }

  // Description:
  // Cast the vector to the specified type, returning the result.
  template<typename TR>
  vtkVector<TR, Size> Cast() const
  {
    vtkVector<TR, Size> result;
    for (int i = 0; i < Size; ++i)
      {
      result[i] = static_cast<TR>(this->Data[i]);
      }
    return result;
  }
};

// .NAME vtkVector2 - templated base type for storage of 2D vectors.
//
template<typename T>
class vtkVector2 : public vtkVector<T, 2>
{
public:
  vtkVector2()
  {
  }

  explicit vtkVector2(const T& scalar) : vtkVector<T, 2>(scalar)
  {
  }

  explicit vtkVector2(const T* init) : vtkVector<T, 2>(init)
  {
  }

  vtkVector2(const T& x, const T& y)
  {
    this->Data[0] = x;
    this->Data[1] = y;
  }

  // Description:
  // Set the x and y components of the vector.
  void Set(const T& x, const T& y)
  {
    this->Data[0] = x;
    this->Data[1] = y;
  }

  // Description:
  // Set the x component of the vector, i.e. element 0.
  void SetX(const T& x) { this->Data[0] = x; }

  // Description:
  // Get the x component of the vector, i.e. element 0.
  const T& GetX() const { return this->Data[0]; }

  // Description:
  // Set the y component of the vector, i.e. element 1.
  void SetY(const T& y) { this->Data[1] = y; }

  // Description:
  // Get the y component of the vector, i.e. element 1.
  const T& GetY() const { return this->Data[1]; }
};

// .NAME vtkVector3 - templated base type for storage of 3D vectors.
//
template<typename T>
class vtkVector3 : public vtkVector<T, 3>
{
public:
  vtkVector3()
  {
  }

  explicit vtkVector3(const T& scalar) : vtkVector<T, 3>(scalar)
  {
  }

  explicit vtkVector3(const T* init) : vtkVector<T, 3>(init)
  {
  }

  vtkVector3(const T& x, const T& y, const T& z)
  {
    this->Data[0] = x;
    this->Data[1] = y;
    this->Data[2] = z;
  }

  // Description:
  // Set the x, y and z components of the vector.
  void Set(const T& x, const T& y, const T& z)
  {
    this->Data[0] = x;
    this->Data[1] = y;
    this->Data[2] = z;
  }

  // Description:
  // Set the x component of the vector, i.e. element 0.
  void SetX(const T& x) { this->Data[0] = x; }

  // Description:
  // Get the x component of the vector, i.e. element 0.
  const T& GetX() const { return this->Data[0]; }

  // Description:
  // Set the y component of the vector, i.e. element 1.
  void SetY(const T& y) { this->Data[1] = y; }

  // Description:
  // Get the y component of the vector, i.e. element 1.
  const T& GetY() const { return this->Data[1]; }

  // Description:
  // Set the z component of the vector, i.e. element 2.
  void SetZ(const T& z) { this->Data[2] = z; }

  // Description:
  // Get the z component of the vector, i.e. element 2.
  const T& GetZ() const { return this->Data[2]; }

  // Description:
  // Return the cross product of this X other.
  vtkVector3<T> Cross(const vtkVector3<T>& other) const
  {
    vtkVector3<T> res;
    res[0] = this->Data[1] * other.Data[2] - this->Data[2] * other.Data[1];
    res[1] = this->Data[2] * other.Data[0] - this->Data[0] * other.Data[2];
    res[2] = this->Data[0] * other.Data[1] - this->Data[1] * other.Data[0];
    return res;
  }
};

// Description:
// Some inline functions for the derived types.
#define vtkVectorNormalized(vectorType, type, size) \
vectorType Normalized() const \
{ \
  return vectorType(vtkVector<type, size>::Normalized().GetData()); \
} \

#define vtkVectorDerivedMacro(vectorType, type, size) \
vtkVectorNormalized(vectorType, type, size) \

// Description:
// Some derived classes for the different vectors commonly used.
class vtkVector2i : public vtkVector2<int>
{
public:
  vtkVector2i() {}
  vtkVector2i(int x, int y) : vtkVector2<int>(x, y) {}
  explicit vtkVector2i(int scalar) : vtkVector2<int>(scalar) {}
  explicit vtkVector2i(const int *init) : vtkVector2<int>(init) {}
  vtkVectorDerivedMacro(vtkVector2i, int, 2)
};

class vtkVector2f : public vtkVector2<float>
{
public:
  vtkVector2f() {}
  vtkVector2f(float x, float y) : vtkVector2<float>(x, y) {}
  explicit vtkVector2f(float scalar) : vtkVector2<float>(scalar) {}
  explicit vtkVector2f(const float* i) : vtkVector2<float>(i) {}
  vtkVectorDerivedMacro(vtkVector2f, float, 2)
};

class vtkVector2d : public vtkVector2<double>
{
public:
  vtkVector2d() {}
  vtkVector2d(double x, double y) : vtkVector2<double>(x, y) {}
  explicit vtkVector2d(double scalar) : vtkVector2<double>(scalar) {}
  explicit vtkVector2d(const double *init) : vtkVector2<double>(init) {}
  vtkVectorDerivedMacro(vtkVector2d, double, 2)
};

#define vtkVector3Cross(vectorType, type) \
vectorType Cross(const vectorType& other) const \
{ \
  return vectorType(vtkVector3<type>::Cross(other).GetData()); \
} \

class vtkVector3i : public vtkVector3<int>
{
public:
  vtkVector3i() {}
  vtkVector3i(int x, int y, int z) : vtkVector3<int>(x, y, z) {}
  explicit vtkVector3i(int scalar) : vtkVector3<int>(scalar) {}
  explicit vtkVector3i(const int *init) : vtkVector3<int>(init) {}
  vtkVectorDerivedMacro(vtkVector3i, int, 3)
  vtkVector3Cross(vtkVector3i, int)
};

class vtkVector3f : public vtkVector3<float>
{
public:
  vtkVector3f() {}
  vtkVector3f(float x, float y, float z) : vtkVector3<float>(x, y, z) {}
  explicit vtkVector3f(float scalar) : vtkVector3<float>(scalar) {}
  explicit vtkVector3f(const float *init) : vtkVector3<float>(init) {}
  vtkVectorDerivedMacro(vtkVector3f, float, 3)
  vtkVector3Cross(vtkVector3f, float)
};

class vtkVector3d : public vtkVector3<double>
{
public:
  vtkVector3d() {}
  vtkVector3d(double x, double y, double z) : vtkVector3<double>(x, y, z) {}
  explicit vtkVector3d(double scalar) : vtkVector3<double>(scalar) {}
  explicit vtkVector3d(const double *init) : vtkVector3<double>(init) {}
  vtkVectorDerivedMacro(vtkVector3d, double, 3)
  vtkVector3Cross(vtkVector3d, double)
};

#endif // vtkVector_h
// VTK-HeaderTest-Exclude: vtkVector.h
