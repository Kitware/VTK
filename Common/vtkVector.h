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

#ifndef __vtkVector_h
#define __vtkVector_h

template<typename T, int Size>
class vtkVector
{
public:
  vtkVector()
  {
    for (int i = 0; i < Size; ++i)
      {
      Data[i] = 0;
      }
  }

  explicit vtkVector(const T* init)
  {
    for (int i = 0; i < Size; ++i)
      {
      Data[i] = init[i];
      }
  }

  // Description:
  // Get the size of the vtkVector.
  int GetSize() const { return Size; }

  // Description:
  // Get a pointer to the underlying data of the vtkVector.
  T* GetData() { return this->Data; }
  const T* GetData() const { return this->Data; }

  // Description:
  // Get a reference to the underlying data element of the vtkVector. Can be
  // used in much the same way as vector[i] is used.
  T& operator[](int i) { return this->Data[i]; }
  const T& operator[](int i) const { return this->Data[i]; }

  // Description:
  // Get the value of the vector at the index speciifed. Does bounds checking,
  // used in much the same way as vector.at(i) is used.
  T operator()(int i) const { return this->Data[i]; }

  // Description:
  // Cast the vector to the specified type, returning the result.
  template<typename TR>
  vtkVector<TR, Size> Cast() const
  {
    vtkVector<TR, Size> result;
    for (int i = 0; i < Size; ++i)
      {
      result[i] = static_cast<TR>(Data[i]);
      }
    return result;
  }

protected:
  // Description:
  // The only thing stored in memory!
  T Data[Size];
};

// .NAME vtkVector2 - templated base type for storage of 2D vectors.
//
template<typename T>
class vtkVector2 : public vtkVector<T, 2>
{
public:
  vtkVector2(const T& x = 0, const T& y = 0)
  {
    this->Data[0] = x;
    this->Data[1] = y;
  }

  explicit vtkVector2(const T* init) : vtkVector<T, 2>(init)
  {
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
  const T& X() const { return this->Data[0]; }

  // Description:
  // Set the y component of the vector, i.e. element 1.
  void SetY(const T& y) { this->Data[1] = y; }

  // Description:
  // Get the y component of the vector, i.e. element 1.
  const T& GetY() const { return this->Data[1]; }
  const T& Y() const { return this->Data[1]; }
};

// .NAME vtkVector3 - templated base type for storage of 3D vectors.
//
template<typename T>
class vtkVector3 : public vtkVector<T, 3>
{
public:
  vtkVector3(const T& x = 0, const T& y = 0, const T& z = 0)
  {
    this->Data[0] = x;
    this->Data[1] = y;
    this->Data[2] = z;
  }

  explicit vtkVector3(const T* init) : vtkVector<T, 3>(init) { }

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
  const T& X() const { return this->Data[0]; }

  // Description:
  // Set the y component of the vector, i.e. element 1.
  void SetY(const T& y) { this->Data[1] = y; }

  // Description:
  // Get the y component of the vector, i.e. element 1.
  const T& GetY() const { return this->Data[1]; }
  const T& Y() const { return this->Data[1]; }

  // Description:
  // Set the z component of the vector, i.e. element 2.
  void SetZ(const T& z) { this->Data[2] = z; }

  // Description:
  // Get the z component of the vector, i.e. element 2.
  const T& GetZ() const { return this->Data[2]; }
  const T& Z() const { return this->Data[2]; }

};

// Description:
// Some derived classes for the different vectors commonly used.
class vtkVector2i : public vtkVector2<int>
{
public:
  vtkVector2i(int x = 0, int y = 0) : vtkVector2<int>(x, y) {}
  explicit vtkVector2i(const int *init) : vtkVector2<int>(init) {}
};

class vtkVector2f : public vtkVector2<float>
{
public:
  vtkVector2f(float x = 0.0, float y = 0.0) : vtkVector2<float>(x, y) {}
  vtkVector2f(const float* i) : vtkVector2<float>(i) {}
};

class vtkVector2d : public vtkVector2<double>
{
public:
  vtkVector2d(double x = 0.0, double y = 0.0) : vtkVector2<double>(x, y) {}
  explicit vtkVector2d(const double *init) : vtkVector2<double>(init) {}
};

class vtkVector3i : public vtkVector3<int>
{
public:
  vtkVector3i(int x = 0, int y = 0, int z = 0) : vtkVector3<int>(x, y, z) {}
  explicit vtkVector3i(const int *init) : vtkVector3<int>(init) {}
};

class vtkVector3f : public vtkVector3<float>
{
public:
  vtkVector3f(float x = 0.0, float y = 0.0, float z = 0.0)
    : vtkVector3<float>(x, y, z) {}
  explicit vtkVector3f(const float *init) : vtkVector3<float>(init) {}
};

class vtkVector3d : public vtkVector3<double>
{
public:
  vtkVector3d(double x = 0.0, double y = 0.0, double z = 0.0)
    : vtkVector3<double>(x, y, z) {}
  explicit vtkVector3d(const double *init) : vtkVector3<double>(init) {}
};

// Some operators for easy addition etc
inline const vtkVector2f operator+(const vtkVector2f &lhs, const vtkVector2f &rhs)
{
  return vtkVector2f(lhs[0] + rhs[0], lhs[1] + rhs[1]);
}
inline const vtkVector2f operator-(const vtkVector2f &lhs, const vtkVector2f &rhs)
{
  return vtkVector2f(lhs[0] - rhs[0], lhs[1] - rhs[1]);
}

#endif // __vtkVector_h
